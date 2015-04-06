/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuStor.h"

#include "ServerStreamBuffer.h"

#include "ServerStore.h"
#include "StorageServer.h"
#include "StreamEncoder.h"
#include "ServerSession.h"
#include "ServerStream.h"

namespace SimuTrace
{

    enum SegmentFlags {
        SfNone        = 0,
        SfFree        = 0,      // Segment is not in use and holds no data

        SfInUse       = 1 << 0, // Segment is in use or in the cache
        SfReadOnly    = 1 << 1, // Segment should not be written to
        SfScratch     = 1 << 2, // Segment is not associated with a stream
                                // Caching disallowed.

        SfCacheable   = 1 << 3, // Segment is eligible to caching, when freed
        SfLowPriority = 1 << 4, // Segment may be reused early. For pre-fetched
                                // or random-access data

        SfPrefetch    = 1 << 5, // Segment will be placed at the head of the
                                // standby list at first free

        SfMax
    };

    inline SegmentFlags operator|(SegmentFlags a, SegmentFlags b)
    {
        return static_cast<SegmentFlags>(
            static_cast<int>(a) | static_cast<int>(b));
    }

#ifdef _DEBUG
    inline std::string _getRequestString(ServerStream* stream,
                                         StreamSegmentId sequenceNumber,
                                         StorageLocation* location,
                                         StreamAccessFlags flags)
    {
        if (stream == nullptr) {
            assert(sequenceNumber == INVALID_STREAM_SEGMENT_ID);
            assert(location == nullptr);

            return "scratch";
        } else {
            assert((location == nullptr) ||
                   ((location->link.sequenceNumber == sequenceNumber) &&
                    (location->link.stream == stream->getId())));

            return stringFormat("stream: %d, sqn: %d%s%s%s%s", stream->getId(),
                                sequenceNumber, (location == nullptr) ?
                                    "" : ", read-only",
                                ((flags & StreamAccessFlags::SafRandomAccess) == 0) ?
                                    "" : ", random-access",
                                ((flags & StreamAccessFlags::SafSequentialScan) == 0) ?
                                    "" : ", sequential-scan",
                                ((flags & StreamAccessFlags::SafSynchronous) == 0) ?
                                    "" : ", synchronous");
        }
    }
#endif

    inline std::string _getBufferIdString(BufferId id)
    {
        return (id == SERVER_BUFFER_ID) ? "'server'" : stringFormat("%d", id);
    }

    struct Segment
    {
        CriticalSection lock;

        Segment* next; // used for free-list and standby-list
        Segment* prev; // used for standby-list only

        SegmentId id;
        SegmentFlags flags;

        bool isSubmitted;

        // We hold a copy of the owner information to validate it in the
        // control element on submit.
        ServerStream* stream;
        StreamSegmentId sequenceNumber;

        // The control segment is used for all read-only segments and for
        // written segments after they have been submitted.
        // In any case, a copy is still held in the buffer's element. However,
        // that is prone to modification by the client and may be corrupted.
        SegmentControlElement control;
    };

    ServerStreamBuffer::ServerStreamBuffer(BufferId id,
                                           size_t segmentSize,
                                           uint32_t numSegments,
                                           bool sharedMemory) :
        StreamBuffer(id, segmentSize, numSegments, sharedMemory),
        _cookie(0),
        _segments(nullptr),
        _freeHead(nullptr),
        _enableCache(false),
        _standbyHead(nullptr)
    {
        _cookie = (static_cast<uint64_t>(rand()) << 32) | rand();

        _enableCache = !Configuration::get<bool>("server.memmgmt.disableCache");

        _initializeSegments();
    }

    ServerStreamBuffer::~ServerStreamBuffer()
    {
        assert(_segments != nullptr);

        flushStandbyList();

        assert(_standbyHead == nullptr);
        assert(_standbyIndex.empty());

    #ifdef _DEBUG
        for (uint32_t i = 0; i < getNumSegments(); ++i) {
            Segment& seg = _segments[i];

            // Check that the segment is free and not in the standby list
            assert(seg.flags == SegmentFlags::SfFree);
            assert(seg.prev == nullptr);
        }
    #endif

        delete [] _segments;
    }

    void ServerStreamBuffer::_initializeSegments()
    {
        assert(_segments == nullptr);

        // At his point, we do not want to support stream buffer segment sizes
        // other than the defined constant. That may change in the future.
        assert(getSegmentSize() == SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB);

        uint32_t segCount = getNumSegments();
        _segments = new Segment[segCount];

        // Setup a linked list of segment headers. We take elements from
        // the linked list, when they are requested and put them at the front
        // when they are freed. This way, we get a free segment in O(1) and
        // reuse segments as soon as possible, thus stabilizing the working
        // set on low to medium load.
        for (uint32_t i = 0; i < segCount; ++i) {
            Segment& seg = _segments[i];

            seg.next = (i == segCount - 1) ? nullptr : &_segments[i + 1];
            seg.prev = nullptr;

            seg.id = static_cast<SegmentId>(i);
            seg.flags = SegmentFlags::SfFree;

            seg.stream = nullptr;
            seg.sequenceNumber = INVALID_STREAM_SEGMENT_ID;

            seg.isSubmitted = false;

            memset(&seg.control, 0, sizeof(SegmentControlElement));
        }

        _freeHead = &_segments[0];
    }

    uint64_t ServerStreamBuffer::_computeControlCookie(
        SegmentControlElement& control, Segment& segment) const
    {
        uint64_t cookie = _cookie;
        cookie ^= (static_cast<uint64_t>(segment.id) << 32) | segment.id;
        cookie ^= static_cast<uint64_t>(control.link.stream) << 32;
        cookie ^= control.link.sequenceNumber;
        cookie ^= control.startTime;

        if ((segment.flags & SegmentFlags::SfReadOnly) != 0) {
            // Control elements that are read-only should not be modified at
            // all. We therefore hash the whole control element.
            uint32_t seed = cookie & 0xFFFFFFFF;

            const uint64_t* buffer = reinterpret_cast<uint64_t*>(&control);
            size_t len = sizeof(SegmentControlElement) - sizeof(uint64_t);
            buffer++; // do not include the cookie itself

            // Note, this will only overwrite the lower 32-bit of the cookie
            Hash::murmur3_32(buffer, len, &cookie, sizeof(uint32_t), seed);
        }

        return cookie;
    }

    bool ServerStreamBuffer::_testControlCookie(
        SegmentControlElement& control, Segment& segment) const
    {
        return (control.cookie == _computeControlCookie(control, segment));
    }

    void ServerStreamBuffer::_notifyEncoderCacheClosed(Segment& segment) const
    {
        assert(segment.stream != nullptr);
        assert(segment.sequenceNumber != INVALID_STREAM_SEGMENT_ID);

        StreamEncoder& encoder = segment.stream->getEncoder();
        encoder.notifySegmentCacheClosed(segment.sequenceNumber);
    }

    Segment* ServerStreamBuffer::_dequeueFromFreeList()
    {
        Segment* seg = _freeHead;

        // As long the head is not null, try to set the head to its next
        // element. If another thread has already taken the head, the compare
        // will set seg to the new head and we try again.
        do {
            if (seg == nullptr) {
                return nullptr;
            }

        } while (!_freeHead.compare_exchange_strong(seg, seg->next));

        assert(seg->flags == SegmentFlags::SfFree);
        assert(seg->prev == nullptr);
        assert(seg->stream == nullptr);
        assert(seg->sequenceNumber == INVALID_STREAM_SEGMENT_ID);

        seg->isSubmitted = false;
        seg->flags = SegmentFlags::SfInUse;
        seg->next  = nullptr;

        return seg;
    }

    void ServerStreamBuffer::_enqueueToFreeList(Segment& segment)
    {
        assert((segment.flags & SegmentFlags::SfInUse) != 0);
        assert(segment.next == nullptr);
        assert(segment.prev == nullptr);

        segment.stream = nullptr;
        segment.sequenceNumber = INVALID_STREAM_SEGMENT_ID;

        segment.flags = SegmentFlags::SfFree;
        segment.next  = _freeHead;

        // Make segment the new head. If the head is no longer what's stored
        // in segment.next (some other thread must have inserted a segment
        // just now) then update segment.next and try again.
        while (!_freeHead.compare_exchange_strong(segment.next, &segment)) { }
    }

    void ServerStreamBuffer::_prepareSegment(SegmentId segment,
                                             ServerStream* stream,
                                             StreamSegmentId sequenceNumber)
    {
        assert(segment < getNumSegments());
        Segment* seg = &_segments[segment];

        assert((seg->flags & SegmentFlags::SfReadOnly) == 0);
        assert(!seg->isSubmitted);

        // We use the base class version so we get the underlying buffer's
        // control element regardless of the current segment settings.
        SegmentControlElement* control =
            this->StreamBuffer::getControlElement(segment);
        assert(control != nullptr);

        // Clear the whole control element
        memset(control, 0, sizeof(SegmentControlElement));

        control->link.stream = (stream == nullptr) ?
            INVALID_STREAM_ID : stream->getId();
        control->link.sequenceNumber = sequenceNumber;

        control->startCycle = INVALID_CYCLE_COUNT;
        control->endCycle   = INVALID_CYCLE_COUNT;

        control->startTime  = Clock::getTimestamp();
        control->endTime    = INVALID_TIME_STAMP;

        control->cookie     = _computeControlCookie(*control, *seg);

        // Update our private copy of the control element
        seg->control = *control;

        assert((stream == nullptr) || (sequenceNumber != INVALID_STREAM_SEGMENT_ID));
        seg->stream         = stream;
        seg->sequenceNumber = sequenceNumber;
    }

    bool ServerStreamBuffer::_handleContention(uint32_t tryCount,
                                               bool isScratch)
    {
        LogWarn("Delaying segment request. Stream buffer %s "
                "exhausted <try: %d%s>.", _getBufferIdString(getId()).c_str(),
                tryCount, (isScratch) ? ", scratch" : "");

        const uint32_t maxRetryCount = Configuration::get<int>("server.memmgmt.retryCount");
        const uint32_t sleepTime = Configuration::get<int>("server.memmgmt.retrySleep");

        if (tryCount >= maxRetryCount) {
            return false;
        }

        ThreadBase::sleep(sleepTime);
        return true;
    }

    Segment* ServerStreamBuffer::_tryAllocateFreeSegment(ServerStream* stream,
        StreamSegmentId sequenceNumber, StorageLocation* location,
        StreamAccessFlags flags, bool prefetch)
    {
        Segment* seg = nullptr;
        uint32_t tryCount = 1;

        while (true) {
            LogMem("Requesting segment from buffer %s <try: %d, %s>.",
                   _getBufferIdString(getId()).c_str(), tryCount,
                   _getRequestString(stream, sequenceNumber, location, flags).c_str());

            seg = _dequeueFromFreeList();
            if (seg == nullptr) {
                // We could not get a segment from the free list. As a second
                // resort, we try to remove an element from the standby list.
                // This will remove the least recently used item, if any.
                seg = _evictFromStandbyList();
            }

            if (seg != nullptr) {
                _prepareSegment(seg->id, stream, sequenceNumber);

                LogMem("Allocated segment %d from buffer %s <try: %d, %s>",
                       seg->id, _getBufferIdString(getId()).c_str(), tryCount,
                       _getRequestString(stream, sequenceNumber, location, flags).c_str());

                break;
            }

            // We do not handle contention for prefetching, but instead return
            // as fast as possible.
            if (prefetch || !_handleContention(tryCount, (stream == nullptr))) {
                break;
            }

            tryCount++;
        }

        return seg;
    }

    void ServerStreamBuffer::_dequeueFromStandbyList(Segment& segment)
    {
        assert(_testControlCookie(segment.control, segment));
        assert((segment.flags & SegmentFlags::SfInUse) != 0);
        assert((segment.flags & SegmentFlags::SfReadOnly) != 0);
        assert((segment.flags & SegmentFlags::SfCacheable) != 0);
        assert(segment.next != nullptr);
        assert(segment.prev != nullptr);
        assert(_standbyHead != nullptr);

        if (segment.next == &segment) {
            assert(_standbyHead == &segment);
            _standbyHead = nullptr;
        } else {
            segment.prev->next = segment.next;
            segment.next->prev = segment.prev;

            if (&segment == _standbyHead) {
                _standbyHead = segment.next;
            }
        }

        segment.isSubmitted = false;
        segment.next = nullptr;
        segment.prev = nullptr;
    }

    void ServerStreamBuffer::_enqueueToStandbyList(Segment& segment)
    {
        assert(_testControlCookie(segment.control, segment));
        assert((segment.flags & SegmentFlags::SfInUse) != 0);
        assert((segment.flags & SegmentFlags::SfReadOnly) != 0);
        assert((segment.flags & SegmentFlags::SfCacheable) != 0);
        assert(segment.next == nullptr);
        assert(segment.prev == nullptr);

        if (_standbyHead == nullptr) {
            segment.next = &segment;
            segment.prev = &segment;

            _standbyHead = &segment;
        } else {
            segment.next = _standbyHead;
            segment.prev = _standbyHead->prev;

            _standbyHead->prev->next = &segment;
            _standbyHead->prev = &segment;

            if (((segment.flags & SegmentFlags::SfLowPriority) == 0) ||
                ((segment.flags & SegmentFlags::SfPrefetch) != 0)) {

                // Set the segment to be the head of the list. This way it
                // won't be chosen as victim the next time.
                _standbyHead = &segment;

                segment.flags = static_cast<SegmentFlags>(
                    segment.flags & ~SegmentFlags::SfPrefetch);
            }
        }
    }

    Segment* ServerStreamBuffer::_findStandbySegment(
        StoreStreamSegmentLink& link, bool erase)
    {
        auto it = _standbyIndex.find(link);
        if (it == _standbyIndex.end()) {
            return nullptr;
        }

        Segment* seg = it->second;

        if (erase) {
            _standbyIndex.erase(it);
        }

        return seg;
    }

    Segment* ServerStreamBuffer::_evictFromStandbyList()
    {
        Segment* seg = nullptr;
        if (_standbyHead != nullptr) {
            LockScope(_standbyLock);

            // Someone might drained the standby list in the meantime.
            if (_standbyHead == nullptr) {
                return nullptr;
            }

            // We have a circular doubly-linked list. The prev element points
            // to the last element in the list, i.e., the least recently used
            // element.
            seg = _standbyHead->prev;
            assert(seg != nullptr);

            _notifyEncoderCacheClosed(*seg);

            // Find the corresponding element in the hash map and erase it
            assert(seg->stream != nullptr);
            StoreId store = seg->stream->getStore().getId();
            StoreStreamSegmentLink link(store, seg->control.link);

            Segment* fseg = _findStandbySegment(link, true);
            assert(fseg == seg);
            (void)fseg; // Make compiler happy in release build

            // Remove the segment from the LRU list
            _dequeueFromStandbyList(*seg);

            // Reset segment flags
            seg->flags = SegmentFlags::SfInUse;
        }

        return seg;
    }

    Segment* ServerStreamBuffer::_removeStandbySegment(
        StoreStreamSegmentLink& link)
    {
        LockScope(_standbyLock);

        // Search the element in the hash map
        Segment* seg = _findStandbySegment(link, true);
        if (seg != nullptr) {
            _dequeueFromStandbyList(*seg);
        }

        return seg;
    }

    void ServerStreamBuffer::_addStandbySegment(Segment& segment)
    {
        LockScope(_standbyLock);

        assert(segment.stream != nullptr);
        StoreId store = segment.stream->getStore().getId();
        StoreStreamSegmentLink link(store, segment.control.link);

        assert(link.store != INVALID_STORE_ID);
        assert(link.stream != INVALID_STREAM_ID);
        assert(link.sequenceNumber != INVALID_STREAM_SEGMENT_ID);

        // If the same segment has been requested multiple times, we keep
        // only a single copy on the standby list.
        if (_findStandbySegment(link, false)) {
            _purgeSegment(segment.id);
        } else {
            _standbyIndex[link] = &segment;
            _enqueueToStandbyList(segment);
        }
    }

    void ServerStreamBuffer::_freeSegment(SegmentId segment, bool prefetch)
    {
        assert(segment < getNumSegments());
        Segment& seg = _segments[segment];

        // Mark the segment as submitted so the caller cannot resubmit it and
        // we return the saved control element in getControlElement(). For
        // writeable segments this should already be true.
        seg.isSubmitted = true;

        if (((seg.flags & SegmentFlags::SfCacheable) != 0) && _enableCache) {

            if (prefetch) {
                // If this is a prefetch free, we add the corresponding flag to
                // the segment. This will prevent the segment from being added
                // to the tail of the standby list (from which we fetch
                // segments for replacement) even if the segment is marked as
                // low priority. The flag is automatically removed later.
                // The flag is necessary to prevent a prefetched segment from
                // being recycled before it had the chance to be used at least
                // once. Otherwise, we would not be able to prefetch multiple
                // low priority segments.
                seg.flags = seg.flags | SegmentFlags::SfPrefetch;
            }

            if ((seg.flags & SegmentFlags::SfReadOnly) == 0) {
                // If this is a new segment, we change it to read-only here, so
                // we only have read-only segments in the cache. Since we use a
                // different hash for read-only segments, we have to update the
                // hash.
                seg.flags = seg.flags | SegmentFlags::SfReadOnly;

                seg.control.cookie = _computeControlCookie(seg.control, seg);

                // We overwrite the whole control element, because in the mean
                // time we have updated the timing information and the cookie
                // in the saved segment control element. We therefore have to
                // update our copy in the cache.
                SegmentControlElement* control =
                    this->StreamBuffer::getControlElement(seg.id);
                assert(control != nullptr);

                *control = seg.control;
            }

            _addStandbySegment(seg);
        } else {
            _purgeSegment(segment);
        }
    }

    void ServerStreamBuffer::_purgeSegment(SegmentId segment)
    {
        Segment& seg = _segments[segment];

        if (seg.stream != nullptr) {
            _notifyEncoderCacheClosed(seg);
        }

        _enqueueToFreeList(seg);
    }

    bool ServerStreamBuffer::_submitSegment(SegmentId segment,
        std::unique_ptr<StorageLocation>& location)
    {
        location = nullptr;

        SegmentControlElement* control;
        control = this->StreamBuffer::getControlElement(segment);
        assert(control != nullptr);

        assert(segment < getNumSegments());
        Segment& seg = _segments[segment];

        // Scratch segments should not be submitted.
        assert((seg.flags & SegmentFlags::SfScratch) == 0);
        assert((seg.flags & SegmentFlags::SfInUse) != 0);
        assert(seg.next == nullptr);
        assert(seg.prev == nullptr);

        assert(seg.stream != nullptr);
        assert(seg.sequenceNumber != INVALID_STREAM_SEGMENT_ID);

        assert(!seg.isSubmitted);

        if ((seg.flags & SegmentFlags::SfReadOnly) == 0) {
            // Make a copy of the control element, so the client cannot change
            // any control information while we are processing the data.
            seg.control = *control;
            control = &seg.control;

            // In the debug build, we check for consistency before we force the
            // owner and sequence number to the given value. In the release
            // build, false arguments will only invalidate the cookie.
            assert(seg.control.link.stream         == seg.stream->getId());
            assert(seg.control.link.sequenceNumber == seg.sequenceNumber);

            seg.control.link.stream         = seg.stream->getId();
            seg.control.link.sequenceNumber = seg.sequenceNumber;
        }

        // Check the cookie
        ThrowOn(!_testControlCookie(*control, seg), Exception,
                stringFormat("Failed submitting segment %d to buffer %s. "
                             "The control cookie is invalid.",
                             segment, _getBufferIdString(getId()).c_str()));

        LogMem("Submitting segment %d to buffer %s "
               "<rvc: %d, vc: %d, stream: %d, sqn: %d>.",
               segment, _getBufferIdString(getId()).c_str(),
               seg.control.rawEntryCount, seg.control.entryCount,
               seg.control.link.stream, seg.control.link.sequenceNumber);

        // Mark the segment as submitted so the caller cannot resubmit it and
        // we return the saved control element in getControlElement()
        seg.isSubmitted = true;

        // If the segment does not contain any valid entries, we just drop it.
        if (seg.control.rawEntryCount == 0) {
            assert(seg.control.entryCount == 0);

            LogWarn("Dropping empty segment %d in buffer %s. Did you forget "
                    "to submit the entries <stream: %d, sqn: %d>?",
                    segment, _getBufferIdString(getId()).c_str(),
                    seg.control.link.stream, seg.control.link.sequenceNumber);

            _purgeSegment(segment);

            return true;
        }

        // We only need to process writable segments. If a segment is read-only
        // we can free it, potentially adding it to the standby list.
        if ((seg.flags & SegmentFlags::SfReadOnly) != 0) {
            _freeSegment(segment);

            return true;
        }

        // This is a newly written segment.
        bool completed = true;
        try {
            const StreamTypeDescriptor& desc = seg.stream->getType();

            // Fix entry count
            if (!isVariableEntrySize(desc.entrySize)) {
                assert(seg.control.entryCount == 0);
                seg.control.entryCount = seg.control.rawEntryCount;
            }

            size_t validBufferLength = getEntrySize(&desc) *
                seg.control.rawEntryCount;

            ThrowOn((validBufferLength > getSegmentSize()) ||
                    (!isVariableEntrySize(desc.entrySize) &&
                     (seg.control.entryCount != seg.control.rawEntryCount)) ||
                    (seg.control.entryCount > seg.control.rawEntryCount),
                    Exception, stringFormat("Invalid number of entries in "
                    "control element for stream %d <sqn: %d, bid: %d>.",
                    seg.control.link.stream, seg.control.link.sequenceNumber,
                    segment));

            assert(seg.control.entryCount > 0);
            assert(seg.control.rawEntryCount > 0);

            assert(seg.control.endTime == INVALID_TIME_STAMP);
            seg.control.endTime = Clock::getTimestamp();

            // Update end timing information. Note, the original control
            // element is NOT updated.
            if (desc.temporalOrder) {
                assert(!isVariableEntrySize(desc.entrySize));

                // The cycle count is only 48 bits wide. We therefore use a
                // mask to cut off any unrelated data.
                const CycleCount cycleMask = TEMPORAL_ORDER_CYCLE_COUNT_MASK;

                // Read the cycle count from the first entry
                const CycleCount* timestamp = reinterpret_cast<const CycleCount*>(
                    getSegment(segment));

                seg.control.startCycle = *timestamp & cycleMask;

                // Read the cycle count from the last valid entry
                timestamp = reinterpret_cast<const CycleCount*>(
                    reinterpret_cast<size_t>(timestamp) +
                    validBufferLength - desc.entrySize);

                seg.control.endCycle = *timestamp & cycleMask;

                ThrowOn((seg.control.startCycle == INVALID_CYCLE_COUNT) ||
                        (seg.control.endCycle == INVALID_CYCLE_COUNT) ||
                        (seg.control.startCycle > seg.control.endCycle),
                        Exception, stringFormat("Invalid cycle information in "
                            "temporally ordered stream %d for segment %d "
                            "<sqn: %d>.", seg.control.link.stream, segment,
                            seg.control.link.sequenceNumber));
            } else {
                seg.control.startCycle = INVALID_CYCLE_COUNT;
                seg.control.endCycle   = INVALID_CYCLE_COUNT;
            }

            seg.control.cookie = _computeControlCookie(seg.control, seg);

            LogDebug("Encoding segment %d in buffer %s "
                     "<stream: %d, sqn: %d, size: %s>.",
                     segment, _getBufferIdString(getId()).c_str(),
                     seg.control.link.stream, seg.control.link.sequenceNumber,
                     sizeToString(validBufferLength).c_str());

            // Encode the segment's data with the encoder specified for the
            // stream type. Depending on the encoder, this operation may not
            // write out any data, yet. The encoder may also perform its work
            // asynchronously. In that case we do not finish the segment here.
            // The encoder has to complete the segment at the stream!
            StreamEncoder& encoder = seg.stream->getEncoder();

            completed = encoder.write(*this, segment, location);
            if (completed) {
                if (location != nullptr) {
                    assert(location->link == seg.control.link);
                    assert(location->ranges.startIndex == seg.control.startIndex);
                    assert((location->ranges.startIndex == INVALID_ENTRY_INDEX) ||
                           (location->getEntryCount() == seg.control.entryCount));
                    assert(location->rawEntryCount == seg.control.rawEntryCount);
                    assert(location->ranges.startCycle == seg.control.startCycle);
                    assert(location->ranges.endCycle == seg.control.endCycle);
                    assert(location->ranges.startTime == seg.control.startTime);
                    assert(location->ranges.endTime == seg.control.endTime);

                    _freeSegment(segment);
                } else {
                    // The encoder did not specify a storage location, so the
                    // segment is no longer valid. Remove it from the buffer.

                    _purgeSegment(segment);
                }
            }

        } catch (const std::exception& e) {
            // In case the encoder could not process the data, re-throw the
            // exception so that the caller can react without loosing data.

            seg.isSubmitted = false;

            LogError("Failed to encode segment %d in buffer %s "
                     "<stream: %d, sqn: %d>. Exception: '%s'.",
                     segment, _getBufferIdString(getId()).c_str(),
                     seg.control.link.stream, seg.control.link.sequenceNumber,
                     e.what());

            throw;
        }

        return completed;
    }

    bool ServerStreamBuffer::_requestSegment(SegmentId& segment,
                                             ServerStream* stream,
                                             StreamSegmentId sequenceNumber,
                                             StreamAccessFlags flags,
                                             StorageLocation* location,
                                             bool prefetch)
    {
        bool completed = true;
        Segment* seg = nullptr;

        segment = INVALID_SEGMENT_ID;

        // Source 1: First see if we can find the segment on the standby list
        if (stream != nullptr) {
            StoreId store = stream->getStore().getId();
            StoreStreamSegmentLink link(store, stream->getId(), sequenceNumber);

            seg = _removeStandbySegment(link);
            if (seg != nullptr) {
                segment = seg->id;
                return true;
            }
        }

        // Source 2: Allocate a new segment from the free list. This may
        //           evict segments from the cache if the free list is empty.
        if (seg == nullptr) {
            seg = _tryAllocateFreeSegment(stream, sequenceNumber,
                                          location, flags, prefetch);
        }

        // We could not get a free segment. Bail out.
        if (seg == nullptr) {
            return true;
        }

        // If the caller supplied a storage location, the segment data should
        // be loaded from the store.
        if (location != nullptr) {

            // Lock the segment to block any concurrent operations by the
            // encoder or other external classes.
            LockScope(seg->lock);

            assert(stream != nullptr);
            assert(sequenceNumber != INVALID_STREAM_SEGMENT_ID);
            assert(seg->control.link == location->link);

            seg->flags = seg->flags | SegmentFlags::SfReadOnly;

            if (_enableCache) {
                seg->flags = seg->flags | SegmentFlags::SfCacheable;

                // If the caller specified random access, we set low priority
                // for it. That will lead the cache to add the segment to the
                // tail instead of the head. On the next eviction the segment
                // will be selected. For true random access, this prevents the
                // pollution of the cache. For true sequential access, the
                // caller will not access a closed segment again. We therefore,
                // can also reuse it as soon as possible.
                if (((flags & StreamAccessFlags::SafRandomAccess) != 0) ||
                    ((flags & StreamAccessFlags::SafSequentialScan) != 0)) {
                    seg->flags = seg->flags | SegmentFlags::SfLowPriority;
                }

            }

            // If the storage location is specified, we invoke the encoder
            // to load the respective data into the fresh segment.

            try {
                StreamEncoder& encoder = stream->getEncoder();
                SegmentControlElement* control =
                    this->StreamBuffer::getControlElement(seg->id);
                assert(control != nullptr);

                // Initialize Control Segment
                control->startCycle    = location->ranges.startCycle;
                control->endCycle      = location->ranges.endCycle;

                control->startTime     = location->ranges.startTime;
                control->endTime       = location->ranges.endTime;

                control->startIndex    = location->ranges.startIndex;
                if (location->ranges.startIndex != INVALID_ENTRY_INDEX) {
                    assert(location->ranges.endIndex >= location->ranges.startIndex);
                    control->entryCount = location->getEntryCount();
                }

                control->rawEntryCount = location->rawEntryCount;

                // Update control cookie and internal control element
                control->cookie = _computeControlCookie(*control, *seg);
                seg->control = *control;

                LogDebug("Decoding segment %d in buffer %s "
                         "<stream: %d, sqn: %d>.", seg->id,
                         _getBufferIdString(getId()).c_str(),
                         stream->getId(), sequenceNumber);

                // This routine guarantees that the segment id is set BEFORE
                // the encoder read is initiated.
                segment = seg->id;

                completed = encoder.read(*this, seg->id, flags, *location,
                                         prefetch);
                assert((!completed) ||
                       ((control->startCycle == location->ranges.startCycle) &&
                        (control->endCycle == location->ranges.endCycle) &&
                        (control->startIndex == location->ranges.startIndex) &&
                        ((control->startIndex == INVALID_ENTRY_INDEX) ||
                         (control->entryCount == location->getEntryCount())) &&
                        (control->rawEntryCount == location->rawEntryCount) &&
                        (control->startTime == location->ranges.startTime) &&
                        (control->endTime == location->ranges.endTime)));

                // If the encoder should perform a synchronous read, we expect
                // the operation to be completed.
                assert(completed ||
                       ((flags & StreamAccessFlags::SafSynchronous) == 0));

            } catch (const std::exception& e) {
                segment = INVALID_SEGMENT_ID;

                _purgeSegment(seg->id);

                LogError("Failed to decode segment %d in buffer %s "
                         "<stream: %d, sqn: %d>. Exception: '%s'.", seg->id,
                         _getBufferIdString(getId()).c_str(), stream->getId(),
                         sequenceNumber, e.what());

                throw;
            }

        } else {
            if (stream == nullptr) { // This is a scratch segment
                assert(sequenceNumber == INVALID_STREAM_SEGMENT_ID);
                assert(!prefetch);

                seg->flags = seg->flags | SegmentFlags::SfScratch;
            } else if (_enableCache) { // This is a new write segment
                assert(!prefetch);

                // Although we activate caching, we mark the segment to be low
                // priority. We assume writes to be performed sequentially.
                seg->flags = seg->flags | SegmentFlags::SfCacheable |
                                          SegmentFlags::SfLowPriority;
            }

            segment = seg->id;
        }

        return completed;
    }

    SegmentId ServerStreamBuffer::requestSegment(ServerStream& stream,
                                                 StreamSegmentId sequenceNumber)
    {
        ThrowOn(sequenceNumber == INVALID_STREAM_SEGMENT_ID,
                ArgumentException);

        SegmentId id;
        bool completed = _requestSegment(id, &stream, sequenceNumber);
        (void)completed; // Make compiler happy in release build
        assert(completed == true);

        return id;
    }

    SegmentId ServerStreamBuffer::requestScratchSegment()
    {
        SegmentId id;
        bool completed = _requestSegment(id);
        (void)completed; // Make compiler happy in release build
        assert(completed == true);

        return id;
    }

    void ServerStreamBuffer::freeSegment(SegmentId segment, bool prefetch)
    {
        ThrowOn(segment >= getNumSegments(), ArgumentOutOfBoundsException);
        Segment& seg = _segments[segment];

        // Lock the segment
        LockScope(seg.lock);

        // Freeing free and standby segments is forbidden. We also do not allow
        // freeing un-submitted, writable segments. For these the control
        // element is not up-to-date.
        ThrowOn(((seg.flags & SegmentFlags::SfInUse) == 0) ||
                (seg.next != nullptr) ||
                (((seg.flags & SegmentFlags::SfReadOnly) == 0) &&
                 (!seg.isSubmitted)), InvalidOperationException);

    #ifdef _DEBUG
        SegmentControlElement* ctrl = getControlElement(segment);
        assert(ctrl != nullptr);

        std::string streamStr = (ctrl->link.stream != INVALID_STREAM_ID) ?
            stringFormat("stream: %d, sqn: %d", ctrl->link.stream,
                ctrl->link.sequenceNumber) : "scratch";

        LogMem("Releasing segment %d to buffer %s <%s>.", segment,
               _getBufferIdString(getId()).c_str(), streamStr.c_str());
    #endif

        _freeSegment(segment, prefetch);
    }

    void ServerStreamBuffer::purgeSegment(SegmentId segment)
    {
        ThrowOn(segment >= getNumSegments(), ArgumentOutOfBoundsException);
        Segment& seg = _segments[segment];

        // Lock the segment
        LockScope(seg.lock);

        // Purging free and standby segments is forbidden.
        ThrowOn(((seg.flags & SegmentFlags::SfInUse) == 0) ||
                (seg.next != nullptr), InvalidOperationException);

    #ifdef _DEBUG
        SegmentControlElement* ctrl = getControlElement(segment);
        assert(ctrl != nullptr);

        std::string streamStr = (ctrl->link.stream != INVALID_STREAM_ID) ?
            stringFormat("stream: %d, sqn: %d", ctrl->link.stream,
                ctrl->link.sequenceNumber) : "scratch";

        LogMem("Purging segment %d of buffer %s <%s>.", segment,
               _getBufferIdString(getId()).c_str(), streamStr.c_str());
    #endif

        _purgeSegment(segment);
    }

    bool ServerStreamBuffer::submitSegment(SegmentId segment,
        std::unique_ptr<StorageLocation>& locationOut)
    {
        ThrowOn(segment >= getNumSegments(), ArgumentOutOfBoundsException);

        Segment& seg = _segments[segment];
        ThrowOnNull(seg.stream, InvalidOperationException);
        assert(seg.sequenceNumber != INVALID_STREAM_SEGMENT_ID);

        // Lock the segment
        LockScope(seg.lock);

        // Submitting free and standby segments is forbidden. We also do not
        // allow submitting the same segment multiple times.
        ThrowOn(((seg.flags & SegmentFlags::SfInUse) == 0) ||
                (seg.next != nullptr) ||
                (seg.isSubmitted), InvalidOperationException);

        return _submitSegment(segment, locationOut);
    }

    bool ServerStreamBuffer::openSegment(SegmentId& segment,
                                         ServerStream& stream,
                                         StreamAccessFlags flags,
                                         StorageLocation& location,
                                         bool prefetch)
    {
        assert(location.link.stream == stream.getId());

        LogMem("%s segment into buffer %s <stream: %d, sqn: %d>.",
               (prefetch) ? "Prefetching" : "Loading",
               _getBufferIdString(getId()).c_str(), location.link.stream,
               location.link.sequenceNumber);

        return _requestSegment(segment, &stream, location.link.sequenceNumber,
                               flags, &location, prefetch);
    }

    void ServerStreamBuffer::flushStandbyList(StoreId store)
    {
        LockScope(_standbyLock);

        if (_standbyHead == nullptr) {
            return;
        }

        // We have a circular doubly-linked list. The prev element points
        // to the last element in the list
        Segment* end = _standbyHead->prev;

        Segment* seg = _standbyHead;
        do {
            assert(seg != nullptr);
            Segment* nseg = seg->next;

            assert(seg->stream != nullptr);
            StoreId streamStore = seg->stream->getStore().getId();

            if ((store == INVALID_STORE_ID) || (streamStore == store)) {

                _notifyEncoderCacheClosed(*seg);

                // Find the corresponding entry in the hash table
                StoreStreamSegmentLink link(streamStore, seg->control.link);

                Segment* fseg = _findStandbySegment(link, true);
                (void)fseg; // Make compiler happy in release build
                assert(fseg == seg);

                LogMem("Flushing cached segment %d in buffer %s "
                       "<store: %d, stream: %d, sqn: %d>.", seg->id,
                       _getBufferIdString(getId()).c_str(), store,
                       seg->stream->getId(),
                       seg->sequenceNumber);

                // Remove the segment from the LRU list and purge it
                _dequeueFromStandbyList(*seg);

                _purgeSegment(seg->id);
            }

            if (seg == end) {
                break;
            }

            // Move to the next segment
            seg = nseg;
        } while (true);

        assert((store != INVALID_STORE_ID) ||
               ((_standbyHead == nullptr) && (_standbyIndex.empty())));
    }

    SegmentControlElement* ServerStreamBuffer::getControlElement(
        SegmentId segment) const
    {
        ThrowOn(segment >= getNumSegments(), ArgumentOutOfBoundsException);
        Segment* seg = &_segments[segment];

        return ((seg->isSubmitted) ||
                ((seg->flags & SegmentFlags::SfReadOnly) != 0)) ?
            &seg->control : this->StreamBuffer::getControlElement(segment);
    }

}