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

#include "ServerStream.h"

#include "StorageServer.h"
#include "ServerSession.h"
#include "ServerStore.h"
#include "ServerStreamBuffer.h"
#include "StreamEncoder.h"
#include "ServerSessionManager.h"

namespace SimuTrace
{

    struct ServerStream::SegmentLocation
    {
        StreamSegmentId sequenceNumber;

        std::unique_ptr<StorageLocation> location;
        SegmentId id;
        SegmentId sideId;

        bool cancel;
        bool prefetched;

        uint32_t referenceCount;
        std::map<SessionId, uint32_t> referenceMap;
        std::vector<StreamWait*> waitList;

        SegmentLocation(StreamSegmentId sequenceNumber, SessionId session, 
                        SegmentId buffer) :
            sequenceNumber(sequenceNumber),
            location(),
            id(buffer),
            sideId(INVALID_SEGMENT_ID),
            referenceCount(1),
            cancel(false),
            prefetched(false)
        {
            // This constructor should be used to create segment locations
            // for newly allocated writable segments.
            assert(sequenceNumber != INVALID_STREAM_SEGMENT_ID);
            assert(session != INVALID_SESSION_ID);
            assert(buffer != INVALID_SEGMENT_ID);

            referenceMap[session] = 1;
        }

        SegmentLocation(StreamSegmentId sequenceNumber,
                        std::unique_ptr<StorageLocation>& loc) :
            sequenceNumber(sequenceNumber),
            location(),
            id(INVALID_SEGMENT_ID),
            sideId(INVALID_SEGMENT_ID),
            referenceCount(0),
            cancel(false),
            prefetched(false)
        {
            location = std::move(loc);
            
            // This constructor should be used to create segment locations
            // for already existing segments (e.g., on open).
            assert(sequenceNumber != INVALID_STREAM_SEGMENT_ID);
            assert(location != nullptr);
        }

        ~SegmentLocation()
        {
            assert(waitList.empty());
        }

        bool isNew() const
        {
            return (location == nullptr);
        }
    };


    ServerStream::ServerStream(ServerStore& store, StreamId id, 
                               const StreamDescriptor& desc, 
                               StreamBuffer& buffer) :
        Stream(id, desc, buffer),
        _store(store),
        _lastSequenceNumber(INVALID_STREAM_SEGMENT_ID),
        _lastAppendSequenceNumber(INVALID_STREAM_SEGMENT_ID),
        _lastAppendIndex(0),
        _encoder(nullptr),
        _stats()
    {
        StreamEncoder::FactoryMethod encoderFactory;
        encoderFactory = store.getEncoderFactory(desc.type.id);

        ThrowOnNull(encoderFactory, Exception, stringFormat(
                        "Could not find an encoder for type %s.",
                        guidToString(desc.type.id).c_str()));

        _encoder = encoderFactory(store, this);
        assert(_encoder != nullptr);

        // Initialize read ahead
        Configuration::get("server.memmgmt.readAhead", _readAheadAmount);
        if (_readAheadAmount > 0) {
            _readAheadList = std::unique_ptr<StreamSegmentId[]>(
                new StreamSegmentId[_readAheadAmount]);
        }
    }

    ServerStream::~ServerStream()
    {
        _finalize();
    }

    void ServerStream::_finalize()
    {
        assert(_openList.empty());

        for (int i = 0; i <= QueryIndexType::QMaxTree; ++i) {
            _trees[i].clear();
        }

        for (auto i = 0; i < _segments.size(); ++i) {
            SegmentLocation* loc = _segments[i];
            if (loc == nullptr) {
                continue;
            }

            delete loc;
        }

        _segments.clear();

        if (_encoder != nullptr) {
            delete _encoder;
        }
    }

    ServerStream::SegmentLocation* ServerStream::_addSegmentLocation(
        SegmentLocation* loc)
    {
        // When calling the method, the caller must hold both: 
        // _lock and _appendLock ! _lock because we modify _segments.
        // _appendLock because we possibly change the last sequence number.

        assert(loc != nullptr);
        assert((loc->sequenceNumber >= _segments.size()) || 
               (_segments[loc->sequenceNumber] == nullptr));

        // Ensure there is a slot for the sequence number. This may add holes!
        auto seq = _segments.size();
        while (seq++ <= loc->sequenceNumber) {
            _segments.push_back(nullptr);
        }

        _segments[loc->sequenceNumber] = loc;

        // Add the segment to the open list if it is referenced
        if (loc->referenceCount > 0) {
            assert(loc->id != INVALID_SEGMENT_ID);
            assert(!loc->referenceMap.empty());

            _openList.push_back(loc);
        }

        // If we append new segments, we always want to append to the end of
        // the stream.
        if ((loc->sequenceNumber > _lastSequenceNumber) ||
            (_lastSequenceNumber == INVALID_STREAM_SEGMENT_ID)) {

            _lastSequenceNumber = loc->sequenceNumber;
        }

        return loc;
    }

    ServerStream::SegmentLocation* ServerStream::_getPreviousSegment(
        StreamSegmentId sequenceNumber) const
    {
        if (sequenceNumber > _lastSequenceNumber) {
            sequenceNumber = _lastSequenceNumber;
        } else if (_lastSequenceNumber == INVALID_STREAM_SEGMENT_ID) {
            return nullptr;
        }
        assert(sequenceNumber <= _segments.size());

        do {
            if (sequenceNumber-- == 0) {
                return nullptr;
            }
        } while (_segments[sequenceNumber] == nullptr);

        return _segments[sequenceNumber];
    }

    ServerStream::SegmentLocation* ServerStream::_getNextSegment(
        StreamSegmentId sequenceNumber) const
    {
        auto size = _segments.size();
        do {
            if (++sequenceNumber >= size) {
                return nullptr;
            }
        } while (_segments[sequenceNumber] == nullptr);

        return _segments[sequenceNumber];
    }

    void ServerStream::_addSegment(StreamSegmentId sequenceNumber, 
                                   std::unique_ptr<StorageLocation>& location)
    {
        assert(location != nullptr);

        // Check the ranges specified by the StorageLocation for
        // overlap and ordering!!
        SegmentLocation* pseg = _getPreviousSegment(sequenceNumber);
        StorageLocation* ploc = (pseg != nullptr) ? pseg->location.get() : nullptr;
        SegmentLocation* nseg = _getNextSegment(sequenceNumber);
        StorageLocation* nloc = (nseg != nullptr) ? nseg->location.get() : nullptr;
        for (int i = 0; i <= QueryIndexType::QMaxTree; ++i) {
            Range* range = &location->ranges.ranges[i];

            if ((range->start != INVALID_LARGE_OBJECT_ID) &&
                (range->end   != INVALID_LARGE_OBJECT_ID)) {

                // This throw also catches the case, where we previously 
                // inserted an invalid range and then try to add a valid one
                // now. Note that, we do not test if the range is strictly 
                // monotonic increasing as it is needed for, e.g., the entry
                // index.
                // To ensure that the index tree works, we also need to
                // introduce the limit, that elements with the same index 
                // (e.g., may not span segments!).
                ThrowOn((range->start > range->end) ||
                        ((ploc != nullptr) && 
                         (ploc->ranges.ranges[i].start == ploc->ranges.ranges[i].end) &&
                         (range->start <= ploc->ranges.ranges[i].end)) ||
                        ((nloc != nullptr) &&
                         (nloc->ranges.ranges[i].start == nloc->ranges.ranges[i].end) &&
                         (range->end >= nloc->ranges.ranges[i].start)), 
                        Exception, stringFormat("The specified range for "
                            "index %d violates monotonicity.", i));
            } else {
                // We overwrite both values with invalid ones. This ensures,
                // that we do not keep a half valid location.
                location->ranges.ranges[i].start = INVALID_LARGE_OBJECT_ID;
                location->ranges.ranges[i].end   = INVALID_LARGE_OBJECT_ID;

                ThrowOn(((ploc != nullptr) && 
                         (ploc->ranges.ranges[i].start != INVALID_LARGE_OBJECT_ID)) ||
                        ((nloc != nullptr) &&
                         (nloc->ranges.ranges[i].start != INVALID_LARGE_OBJECT_ID)),
                        Exception, stringFormat("The specified range for "
                            "index %d violates monotonicity.", i));
            }
        }

        SegmentLocation* loc = nullptr;
        if (!_segmentIsAllocated(sequenceNumber)) {
            // We should get here only if we add an existing segment during
            // the load phase of the store! It is illegal to get here from
            // completeSegment. At this point, _appendLock must be held!

            loc = _addSegmentLocation(new SegmentLocation(sequenceNumber, 
                                                          location));

            assert(loc->location != nullptr);
            if (loc->location->ranges.startIndex > _lastAppendIndex) {
                _lastAppendIndex = loc->location->ranges.startIndex;
            }
        } else {
            // If we get here, we are completing a segment after encoding.

            assert(sequenceNumber < _segments.size());
            loc = _segments[sequenceNumber];

            assert(loc != nullptr);
            assert(loc->location == nullptr);

            loc->location = std::move(location);
        }

        // The server stream should now hold the ownership of the location
        assert(location == nullptr);

        // Insert each index into the corresponding tree
        for (int i = 0; i <= QueryIndexType::QMaxTree; ++i) {
            Range* range = &loc->location->ranges.ranges[i];
            if ((range->start != INVALID_LARGE_OBJECT_ID) &&
                (range->end   != INVALID_LARGE_OBJECT_ID)) {

                assert(range->start <= range->end);
                _trees[i].insert(range);

                // Update stream range statistics
                Range* statRange = &_stats.ranges.ranges[i];
                if (range->start < statRange->start) {
                    statRange->start = range->start;
                }

                if ((range->end > statRange->end) || 
                    (statRange->end == INVALID_LARGE_OBJECT_ID)) {
                    statRange->end = range->end;
                }
            }
        }

        // Update remaining stream statistics
        _stats.compressedSize += loc->location->compressedSize;

        _stats.entryCount     += loc->location->getEntryCount();
        _stats.rawEntryCount  += loc->location->rawEntryCount;

        LogMem("Added segment to stream %d <sqn: %d>.",
               getId(), sequenceNumber);
    }

    void ServerStream::_completeSegment(StreamSegmentId sequenceNumber,
        std::unique_ptr<StorageLocation>* location, bool success)
    {
        std::vector<StreamWait*> waitList;

        LockExclusive(_lock); {
            _completeSegment(sequenceNumber, location, &waitList, nullptr,
                             success, false);
        } Unlock();

        for (auto wait : waitList) {
            wait->decrement();
        }
    }

    void ServerStream::_completeSegment(StreamSegmentId sequenceNumber,
        std::unique_ptr<StorageLocation>* location,
        std::vector<StreamWait*>* waitList,
        OpenListIterator* openListIt,
        bool success, bool synchronous)
    {
        // To avoid checks if location points to valid memory, we assign it
        // a dummy pointer to point to in case the caller specified none. The
        // pointer itself will be a nullptr.
        std::unique_ptr<StorageLocation> dummyLocation;
        if (location == nullptr) {
            location = &dummyLocation;
        }

        bool sqnAlive = true;
        bool removeFromOpenList = false;
        ServerStreamBuffer& buffer = _getBuffer();
        assert(sequenceNumber < _segments.size());

        SegmentLocation* loc = _segments[sequenceNumber];
        assert(loc != nullptr);
        assert(loc->sequenceNumber == sequenceNumber);
        assert(loc->id == INVALID_SEGMENT_ID);
        assert(loc->sideId != INVALID_SEGMENT_ID);

        assert((waitList != nullptr) || (loc->waitList.empty()));

        // The caller is supposed to notify us of a finished de- or encoding of
        // a segment or a segment close (read-only segment).

        if (loc->location != nullptr) { // Decoding, Closing ------------------
            // Note to reference counting: It may happen that we get here, when
            // the reference count of the segment is still 0. This is the case 
            // if the open operation is completed asynchronously before we 
            // return in _open() to increment the counter. However, this is not
            // a problem for us here. Instead, _open() has to cope with a early
            // complete.

            // To signal a successful decoding, the encoder needs
            // to call us with the same storage location that is
            // already applied.
            assert((*location == nullptr) || (*location == loc->location));

            if ((*location == nullptr) || (loc->cancel)) {
                // The operation should be canceled. This may happen, if
                // the user initiated an open, but closed the segment
                // before it was completely loaded. To cancel the operation
                // we simply throw away the decoding result and reset
                // the segment location. We handle a failed decoding and
                // closing in the same way.

                assert(_encoder != nullptr);
                _encoder->notifySegmentClosed(sequenceNumber);

                if (!success) {
                    buffer.purgeSegment(loc->sideId);

                    loc->prefetched = false;
                } else if (loc->cancel) {
                    assert(success); // not needed, just to clarify

                    // Canceled segments have been successfully and fully
                    // decoded. Do not throw them away, but give the cache
                    // a chance to catch them.
                    buffer.freeSegment(loc->sideId, loc->prefetched);
                }

                loc->sideId = INVALID_SEGMENT_ID;
                loc->referenceCount = 0;
                loc->referenceMap.clear();

                removeFromOpenList = true;
            }

            // Set the current sideId as active id (if the operation has
            // been canceled or was not successful, this is also 
            // INVALID_SEGMENT_ID).
            loc->id = loc->sideId;
            loc->sideId = INVALID_SEGMENT_ID;
            loc->cancel = false;

        } else { // Encoding --------------------------------------------------
            assert(loc->referenceCount == 1);
            assert(loc->referenceMap.size() == 1);
            assert(!loc->prefetched);

            removeFromOpenList = true;

            // The ServerStreamBuffer has already freed/purged the segment if
            // the operation was synchronous and successful. Otherwise, we
            // have to free/purge it here.
            if (*location != nullptr) {
                // The encoding was successful. We have to add the storage 
                // location to our indices. After that operation the user can 
                // access the data through an open call.
                assert(success);

                _addSegment(sequenceNumber, *location);

                if (!synchronous) {
                    buffer.freeSegment(loc->sideId);
                }
            } else if (!synchronous || !success) {
                buffer.purgeSegment(loc->sideId);
            }

            // The segment location should now own the location
            assert(*location == nullptr);

            // We can update the internal state of the segment location
            // only after (potentially) adding the segment, because the index 
            // check might throw an exception and we do not want to end with a
            // half completed segment.
            if (loc->location != nullptr) {
                loc->sideId = INVALID_SEGMENT_ID;

                loc->referenceCount = 0;
                loc->referenceMap.clear();
                loc->cancel = false; // not respected here
            } else {
                // The storage location is nullptr:
                //   1) something went wrong during encoding. In that case,
                //      the encoder should, if 
                //      a) synchronous: throw an exception and we should 
                //         come only here if we force submission or
                //      b) asynchronous: report the error through a nullptr
                //         location.
                //   2) everything is ok. The encoder did not return a
                //      storage location by purpose. This might happen
                //      synchronously or asynchronously.
                // We can differentiate between 1) and 2) through the
                // success parameter. In both cases, we delete the whole
                // segment location as if no data had ever been allocated
                // and submitted for the current sequence number.

                sqnAlive = false;
            }
        }

        if (waitList != nullptr) {
            assert(!synchronous);

            waitList->swap(loc->waitList);

            if (!success) {
                LogDebug("Purging segment from stream %d after failed "
                         "operation <sqn: %d>.", getId(), sequenceNumber);

                StreamSegmentLink link(getId(), sequenceNumber);

                for (auto wait : *waitList) {
                    wait->pushError(link);
                }
            }
        }

        // Remove the segment location from the open list
        if (removeFromOpenList) {
            if (openListIt != nullptr) {
                assert((*(*openListIt)) == loc);
                *openListIt = _openList.erase(*openListIt);
            } else {
                _openList.remove(loc);
            }
        }

        if (!sqnAlive) {
            _segments[sequenceNumber] = nullptr;
            delete loc;
        }
    }

    bool ServerStream::_open(SessionId session, 
                             StreamSegmentId sequenceNumber,
                             StreamAccessFlags flags,
                             SegmentId& segmentId,
                             bool prefetch,
                             StreamWait* wait)
    {
        // This method must be called with the openLock held!

        ServerStreamBuffer& buffer = _getBuffer();
        SegmentLocation* loc = _segments[sequenceNumber];

        assert(loc != nullptr);
        assert(loc->location != nullptr);
        assert(loc->sequenceNumber == sequenceNumber);
        assert(loc->id == INVALID_SEGMENT_ID);
        assert(loc->sideId == INVALID_SEGMENT_ID);
        assert(loc->referenceCount == 0);
        assert(loc->referenceMap.empty());

        // We use the cancel flag to indicate that we are not interested in
        // keeping the segment open after read completion when prefetching. 
        // The prefetch flag will be kept alive until the next real open.
        // We use it to prevent us from prefetching the same segment multiple
        // times.
        // Note: We do not reset the flags, if openSegment() throws an
        // exception. A next attempt to prefetch the segment is thus not made
        // until the next real read attempt.

        loc->prefetched = prefetch;
        loc->cancel     = prefetch;

        // openSegment() guarantees that the sideId is set BEFORE the encoder
        // starts reading the segment. sideId is thus even valid, if the
        // encoder asynchronously completes the operation before we return from
        // our call to openSegment().

        bool complete = buffer.openSegment(loc->sideId, *this, flags, 
                                           *loc->location, prefetch);

        LockExclusive(_lock); {

            if (loc->id == INVALID_SEGMENT_ID) {
                segmentId = loc->sideId;

                // We must not increment the reference count if the operation
                // already finished and failed. This is indicated by all ids
                // being set to INVALID_SEGMENT_ID. This may also be the case 
                // if the allocation of a new segment failed in the buffer.
                if (loc->sideId == INVALID_SEGMENT_ID) {
                    assert(complete);

                    assert(loc->referenceCount == 0);
                    assert(loc->referenceMap.empty());

                    loc->cancel     = false;
                    loc->prefetched = false;

                    if (wait != nullptr) {
                        // At this point, there can only be the supplied wait
                        // which needs to be informed, because we are blocking
                        // all further open attempts with the open lock. The
                        // wait list therefore must have been empty, when
                        // complete() was called.
                        StreamSegmentLink link(getId(), sequenceNumber);

                        wait->pushError(link);
                    }

                    return true;

                } else if ((!complete) && (wait != nullptr)) {
                    // The operation is still in progress. Add the supplied
                    // wait to the wait list, so the caller is informed about
                    // the completed operation.
                    wait->increment();
                    loc->waitList.push_back(wait);
                }

            } else {
                // The operation already finished asynchronously. This may only
                // happen in the asynchronous case.
                assert(!complete);

                segmentId = loc->id;
            }

            loc->referenceCount++;
            loc->referenceMap[session]++;

            _openList.push_back(loc);

            if (complete) {
                _completeSegment(sequenceNumber, &loc->location, nullptr, 
                                 nullptr, true, true);
            }

        } Unlock();

        return complete;
    }

    void ServerStream::_close(SegmentLocation* loc, StreamWait* wait, 
                              bool ignoreErrors, OpenListIterator* openListIt)
    {
        assert(loc != nullptr);
        assert(loc->referenceCount == 1);
        assert(loc->referenceMap.size() == 1);

        //Performing a close on a segment in progress can happen if:
        // a) this is a close session call and we want to wait for all
        //    segments of this session to be processed.
        // b) the caller calls us multiple times
        // c) the segment is currently loading and we want to cancel the load
        if (loc->id == INVALID_SEGMENT_ID) {
            assert(loc->sideId != INVALID_SEGMENT_ID);

            if (wait != nullptr) {
                wait->increment();
                loc->waitList.push_back(wait);
            }

            if (openListIt != nullptr) {
                ++(*openListIt);
            }

            // if this is a load, cancel it.
            loc->cancel = true;
            return;
        }

        assert(loc->id != INVALID_SEGMENT_ID);
        assert(loc->sideId == INVALID_SEGMENT_ID);

        ServerStreamBuffer& buffer = _getBuffer();
        uint32_t entryCount = 0;

        std::unique_ptr<StorageLocation> location;
        bool complete;
        bool success = true;

        try {
            complete = buffer.submitSegment(loc->id, location);
        } catch (const std::exception& e) {
            // We always ignore errors and print a error message instead 
            // when we are closing a read-only segment.
            if (!ignoreErrors && (loc->location == nullptr)) {
                throw;
            }

            LogError("Exception during forced submit for stream %d <sqn: %d>. "
                     "Purging data. The exception is '%s'.", getId(), loc->id, 
                     e.what());

            // We are forced to purge the data.
            complete = true;
            success  = false;
            location = nullptr;
        }

        // We invalidate the segment mapping so that a further close 
        // attempt will fail. We do not perform this operation prior to
        // the submit() call, because the method might throw an exception!
        loc->sideId = loc->id;
        loc->id = INVALID_SEGMENT_ID;

        // Copy sequence number. _completeSegment might delete loc!
        StreamSegmentId sqn = loc->sequenceNumber; 

        if (!complete) {
            // The operation did not complete and is still running. Install
            // the wait context in the segment (if supplied).
            // N.B.: We do not remove the segment from the open list and
            // keep the reference alive. That way we can still find it if 
            // we should need to perform a close for the session.

            if (wait != nullptr) {
                wait->increment();
                loc->waitList.push_back(wait);
            }

            // In the asynchronous case, we have to purge the stream buffer
            // segment. Hence, the control element is guaranteed to be still
            // valid. Important: Use the ServerStreamBuffer here, so we get
            // the copied control element!
            SegmentControlElement* ctrl = buffer.getControlElement(loc->sideId);
            assert(ctrl != nullptr);

            entryCount = (ctrl->startIndex != INVALID_ENTRY_INDEX) ? 
                ctrl->entryCount : 0;

            if (openListIt != nullptr) {
                ++(*openListIt);
            }
        } else {
            // The operation finished synchronously, so we complete the
            // segment. This will remove all open references. 

            if (location != nullptr) {
                entryCount = location->getEntryCount();
            }

            // location might be nullptr (e.g., if this was a read-only segment 
            // and the submit immediately purged it or if the segment was 
            // empty). This is also the case, if the submit was not successful.
            _completeSegment(sqn, &location, nullptr, openListIt, success, true);

            // The segment should now own the storage location (if any)
            assert(location == nullptr);
        }

        // It is valid to submit a written segment by closing it. In that case,
        // we need to update our append state.
        if (sqn == _lastAppendSequenceNumber) {
            _lastAppendIndex += entryCount;
            _lastAppendSequenceNumber = INVALID_STREAM_SEGMENT_ID;
        }
    }

    StreamSegmentId ServerStream::_findSequenceNumber(QueryIndexType type, 
                                                      uint64_t value)
    {
        if (type <= QueryIndexType::QMaxTree) {
            std::set<Range*, RangeCompare>& tree = _trees[type];

            Range range;
            range.start = range.end = value;

            Range* targetRange = nullptr;

            auto it = tree.lower_bound(&range);
            if ((it != tree.end()) && (value == (*it)->start)) {
                targetRange = *it;
            } else {
                ReverseRangeIterator rit(it);
                if ((rit != tree.rend()) &&
                    (value <= ((*rit)->end))) {

                    assert(value >= (*rit)->start);
                    targetRange = *rit;
                }
            }

            if (targetRange == nullptr) {
                return INVALID_STREAM_SEGMENT_ID;
            }

            // This is a small hack. We have inserted pointer to the range 
            // structs in the storage locations into the trees. By taking the 
            // address of the range we found in the tree and subtracting its 
            // offset in the storage location, we get a pointer to the storage
            // location itself. This structure then leads us to the segment 
            // location.

        #ifdef __GNUC__
        // GCC gives a warning for the offsetof because StorageLocation 
        // defines a constructor which is not standard compliant.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Winvalid-offsetof"
        #endif

            StorageLocation* location = reinterpret_cast<StorageLocation*>(
                reinterpret_cast<size_t>(targetRange) - (type * sizeof(Range)) -
                offsetof(StorageLocation, ranges));

        #ifdef __GNUC__
        #pragma GCC diagnostic pop
        #endif

            return location->link.sequenceNumber;
        } else {

            switch (type)
            {
                case QueryIndexType::QSequenceNumber: {
                    return static_cast<StreamSegmentId>(value);
                }

                case QueryIndexType::QNextValidSequenceNumber: {
                    StreamSegmentId sqn0 = static_cast<StreamSegmentId>(value);
                    SegmentLocation* loc = _getNextSegment(sqn0);

                    if (loc != nullptr) {
                        return loc->sequenceNumber;
                    }

                    break;
                }

                default: {
                    Throw(NotSupportedException);
                    break;
                }
            }
        }

        return INVALID_STREAM_SEGMENT_ID;
    }

    ServerStreamBuffer& ServerStream::_getBuffer() const
    {
        return static_cast<ServerStreamBuffer&>(getStreamBuffer());
    }

    bool ServerStream::_segmentIsAllocated(StreamSegmentId sequenceNumber) const
    {
        return (sequenceNumber < _segments.size()) && 
               (_segments[sequenceNumber] != nullptr);
    }

    void ServerStream::queryInformation(StreamQueryInformation& informationOut) const
    {
        LockScopeShared(_lock);

        informationOut.descriptor = getDescriptor();
        informationOut.stats      = _stats;

        _encoder->queryInformationStream(informationOut);
    }

    void ServerStream::addSegment(StreamSegmentId sequenceNumber, 
                                  std::unique_ptr<StorageLocation>& location)
    {
        LockScope(_appendLock);
        LockScopeExclusive(_lock);

        ThrowOnNull(location, ArgumentNullException);
        ThrowOn(sequenceNumber == INVALID_STREAM_SEGMENT_ID, 
                ArgumentOutOfBoundsException);
        ThrowOn(_segmentIsAllocated(sequenceNumber), 
                InvalidOperationException);

        _addSegment(sequenceNumber, location);
    }

    void ServerStream::addSegment(SessionId session,
                                  StreamSegmentId sequenceNumber,
                                  SegmentId* bufferSegmentOut)
    {
        LockScope(_appendLock);

        ThrowOnNull(bufferSegmentOut, ArgumentNullException);
        ThrowOn(sequenceNumber == INVALID_STREAM_SEGMENT_ID, 
                ArgumentOutOfBoundsException);
        ThrowOn(_segmentIsAllocated(sequenceNumber), 
                InvalidOperationException);

        StreamSegmentId sqn = sequenceNumber;
        ServerStreamBuffer& buffer = _getBuffer();

        SegmentId id = buffer.requestSegment(*this, sqn);
        if (id != INVALID_SEGMENT_ID) {
            LockScopeExclusive(_lock); // Get global lock !

            SegmentControlElement* ctrl = buffer.getControlElement(id);
            assert(ctrl != nullptr);

            ctrl->startIndex = INVALID_ENTRY_INDEX;

            _addSegmentLocation(new SegmentLocation(sqn, session, id));
        }

        *bufferSegmentOut = id;
    }

    void ServerStream::completeSegment(StreamSegmentId sequenceNumber)
    {
        ThrowOn(!_segmentIsAllocated(sequenceNumber),
                InvalidOperationException);

        // We simply pass a pointer to our storage location pointer. If
        // this is a for a write, then we are supposed to throw away the data. 
        // Our location pointer will and must be null in this case.
        // If this is a read, we pass the existing location pointer
        _completeSegment(sequenceNumber, &_segments[sequenceNumber]->location, 
                         true);
    }

    void ServerStream::completeSegment(StreamSegmentId sequenceNumber, 
                                       std::unique_ptr<StorageLocation>* location)
    {
        ThrowOn(!_segmentIsAllocated(sequenceNumber) ||
                (_segments[sequenceNumber]->location != nullptr),
                InvalidOperationException);

        _completeSegment(sequenceNumber, location, (location != nullptr));
    }

    StreamSegmentId ServerStream::append(SessionId session,
                                         SegmentId* bufferSegmentOut, 
                                         StreamWait* wait)
    {
        LockScope(_appendLock);

        LockExclusive(_lock); {
            // Submit the last appended segment
            if (_lastAppendSequenceNumber != INVALID_STREAM_SEGMENT_ID) {
                assert(_lastAppendSequenceNumber < _segments.size());
                SegmentLocation* loc = _segments[_lastAppendSequenceNumber];

                assert(loc != nullptr);
                assert(loc->id != INVALID_SEGMENT_ID);
                assert(loc->sideId == INVALID_SEGMENT_ID);
                assert(loc->referenceCount == 1);
                assert(loc->referenceMap.size() == 1);

                auto it = loc->referenceMap.find(session);
                assert(it != loc->referenceMap.end());
                assert(it->second == 1);

                _close(loc, wait, false, nullptr);
            }
        } Unlock();

        // If this is the first segment, _lastSequenceNumber should be set 
        // to INVALID_STREAM_SEGMENT_ID, which itself must be defined as 
        // the maximum uint32_t value. The increment will overflow and give
        // us seq number 0.
        assert(INVALID_STREAM_SEGMENT_ID ==
               std::numeric_limits<uint32_t>::max());

        StreamSegmentId sqn = _lastSequenceNumber + 1;
        ThrowOn(sqn == INVALID_STREAM_SEGMENT_ID, InvalidOperationException);

        ServerStreamBuffer& buffer = _getBuffer();
        SegmentId id = buffer.requestSegment(*this, sqn);

        if (id != INVALID_SEGMENT_ID) {
            LockScopeExclusive(_lock); // Reacquire lock!

            SegmentControlElement* ctrl = buffer.getControlElement(id);
            assert(ctrl != nullptr);

            ctrl->startIndex = _lastAppendIndex;

            _addSegmentLocation(new SegmentLocation(sqn, session, id));

            _lastAppendSequenceNumber = _lastSequenceNumber = sqn;
        }

        if (bufferSegmentOut != nullptr) {
            *bufferSegmentOut = id;
        }

        return _lastAppendSequenceNumber;
    }

    StreamSegmentId ServerStream::open(SessionId session,
                                       QueryIndexType type, uint64_t value,
                                       StreamAccessFlags flags, 
                                       SegmentId* bufferSegmentOut, 
                                       StreamWait* wait)
    {
        LockScope(_openLock);

        StreamSegmentId sqn;
        SegmentLocation* loc;
        bool completed;
        bool handled = false;
        SegmentId id;

        LockExclusive(_lock); {
            sqn = _findSequenceNumber(type, value);

            ThrowOn(sqn >= _segments.size(), NotFoundException);

            // If we test under the lock that the segment does have a storage
            // location assigned, we are guaranteed that the segment location
            // and its storage location will stay alive. We do not delete
            // segments that are ready to be read. We thus can safely release
            // the lock.
            loc = _segments[sqn];
            ThrowOnNull(loc, NotFoundException);
            ThrowOnNull(loc->location, OperationInProgressException);
            assert(loc->sequenceNumber == sqn);

            if (loc->referenceCount > 0) {
                // Only the server session is allowed to open segments which 
                // are currently written. However, it may not open segments 
                // which are currently processed by an encoder for encoding!
                if (session == SERVER_SESSION_ID) {
                    ThrowOn((loc->location == nullptr) && 
                            (loc->id == INVALID_SEGMENT_ID),
                            OperationInProgressException);
                } else {
                    ThrowOnNull(loc->location, OperationInProgressException);
                }

                // If this is a read ahead in progress, we do not increment the
                // reference count, but instead take over the reference count
                // from the read ahead operation. This is legitimate because
                // we reset the cancel flag, thus preventing the prefetched
                // segment from getting closed on completion.
                if (!loc->prefetched) {
                    loc->referenceCount++;
                } else {
                    assert(loc->referenceCount == 1);
                    assert(loc->referenceMap.size() == 1);

                    // If a different session has started the read ahead, we
                    // must fix the reference map. To ease handling we just set
                    // up a new map.
                    loc->referenceMap.clear();

                    loc->prefetched = false;
                }

                loc->referenceMap[session]++;

                // Reset the cancel flag.
                loc->cancel = false;

                // If we are still loading the segment, we add the caller to 
                // the waiters, so he gets informed as soon as the segment is 
                // available
                if (loc->id == INVALID_SEGMENT_ID) {
                    if (wait != nullptr) {
                        wait->increment();
                        loc->waitList.push_back(wait);
                    }

                    id = loc->sideId;
                    completed = false;
                } else {
                    id = loc->id;
                    completed = true;
                }

                handled = true;
            }

            // Before we open the requested segment, we check if the caller
            // specified sequential scan. In that case, we start asynchronous
            // read ahead before performing the requested open. While we are
            // holding the lock, we just find the right sequence numbers to
            // prefetch.
            if ((flags & StreamAccessFlags::SafSequentialScan) != 0) {
                uint32_t readAhead = _readAheadAmount;
                StreamSegmentId raSqn = sqn;

                while (readAhead > 0) {
                    // Find the next valid sequence number after the requested 
                    // or last prefetched one, respectively.
                    raSqn = _findSequenceNumber(
                        QueryIndexType::QNextValidSequenceNumber, raSqn);

                    _readAheadList[--readAhead] = raSqn;

                    // If there are no further segments in the stream which we
                    // can prefetch, we cancel read ahead.
                    if (raSqn == INVALID_STREAM_SEGMENT_ID) {
                        break;
                    }
                }
            }

        } Unlock();

        // Do the actual read ahead. Since we cannot delete finished segments
        // the collected sequence numbers must still be valid for read ahead!
        if ((flags & StreamAccessFlags::SafSequentialScan) != 0) {
            uint32_t readAhead = _readAheadAmount;

            StreamAccessFlags raFlags = static_cast<StreamAccessFlags>(
                    flags & ~StreamAccessFlags::SafSynchronous);

            while (readAhead > 0) {
                StreamSegmentId raSqn = _readAheadList[readAhead - 1];

                // If there are no further segments in the stream which we
                // can prefetch, we cancel read ahead.
                if (raSqn == INVALID_STREAM_SEGMENT_ID) {
                    break;
                }

                SegmentLocation* raloc = _segments[raSqn];
                assert(raloc != nullptr);

                // The segment might be still in progress
                if (raloc->location != nullptr) {

                    // Only initiate an open, if the segment is not already 
                    // open (which may also be a read ahead in progress) and 
                    // has not been prefetched since the last real open. Since
                    // the reference count can only be incremented through this
                    // method, the state is protected by the openLock we hold.
                    if ((raloc->referenceCount == 0) && (!raloc->prefetched)) {
                        SegmentId id;

                        _open(session, raSqn, raFlags, id, true);

                        // Abort read ahead if unsuccessful
                        if (id == INVALID_SEGMENT_ID) {
                            break;
                        }
                    }
                }

                readAhead--;
            }
        }

        if (!handled) {
            ThrowOnNull(loc->location, OperationInProgressException);

            // If we get here, the segment was not open (i.e., reference count 
            // was zero), when we checked the reference counter under the lock.
            // Since we are still holding the openlock, there is no way that 
            // the reference count could be increased in the mean time.
            assert(loc->referenceCount == 0);

            completed = _open(session, sqn, flags, id, false, wait);
        }

        if (bufferSegmentOut != nullptr) {
            *bufferSegmentOut = id;
        }

        // If the operation is still in progress, we will return
        // INVALID_STREAM_SEGMENT_ID. Otherwise, the valid sequence number
        // will be returned.
        return (completed && (id != INVALID_SEGMENT_ID)) ? 
            sqn : INVALID_STREAM_SEGMENT_ID;
    }

    void ServerStream::close(SessionId session, StreamSegmentId sequenceNumber, 
                             StreamWait* wait, bool ignoreErrors)
    {
        // We do not need to get the open lock at this point, although a read
        // on the segment might be currently in progress. In that case, the
        // operation will be only visible to us after the other thread has 
        // acquired the global lock and incremented the reference count.
        // However, the effect of getting the openlock first is the same as if
        // the other thread opens the same segment immediately after us. This
        // is the same evil we are facing when just acquiring the global lock.

        LockScopeExclusive(_lock);
        ThrowOn(!_segmentIsAllocated(sequenceNumber), InvalidOperationException);

        assert(sequenceNumber < _segments.size());
        SegmentLocation* loc = _segments[sequenceNumber];
        assert(loc != nullptr);

        // Check if the session holds any references to the segment
        auto it = loc->referenceMap.find(session);
        if ((loc->referenceCount == 0) || (it == loc->referenceMap.end())) {
            assert((loc->referenceCount > 0) || loc->referenceMap.empty());
            assert((loc->referenceCount == 0) || !loc->referenceMap.empty());

            Throw(Exception, stringFormat("Session %d does not hold any "
                    "references to stream segment <sqn: %d>.", session,
                    sequenceNumber));
        }

        assert(it->second > 0);

    #ifdef _DEBUG
        uint32_t totalRef = 0;
        for (auto pair : loc->referenceMap) {
            totalRef += pair.second;
        }
        assert(totalRef = loc->referenceCount);
    #endif

        if (loc->referenceCount == 1) {
            assert(it->second == 1);

            // This is the last reference to the segment. Close it.
            _close(loc, wait, ignoreErrors, nullptr);

        } else {
            assert(loc->referenceCount > 1);
            assert(sequenceNumber != _lastAppendSequenceNumber);

            if (it->second == 1) {
                loc->referenceMap.erase(it);
            } else {
                it->second--;
            }

            loc->referenceCount--;
        }
    }

    void ServerStream::close(SessionId session, StreamWait* wait, 
                             bool ignoreErrors)
    {
        LockScopeExclusive(_lock);
        ServerStreamBuffer& buffer = _getBuffer();

        auto lit = _openList.begin();
        while (lit != _openList.end()) {
            SegmentLocation* loc = *lit;

            auto it = loc->referenceMap.find(session);
            if (it != loc->referenceMap.end()) {
                assert(it->second > 0);

                if (loc->referenceCount == it->second) {
                    assert(loc->referenceMap.size() == 1);

                    // If we get here, all references to the segment are done 
                    // from within the specified session and we have to free 
                    // all those references.

                    _close(loc, wait, ignoreErrors, &lit);

                    continue;
                } else {
                    assert(loc->id != INVALID_SEGMENT_ID);
                    assert(loc->referenceCount > it->second);

                    loc->referenceCount -= it->second;
                    loc->referenceMap.erase(it);
                }
            }

            // Keep the element in the open list and go to the next one
            ++lit;
        }
    }

    SegmentId ServerStream::getBufferMapping(StreamSegmentId sequenceNumber) const
    {
        LockScopeShared(_lock);
        if (!_segmentIsAllocated(sequenceNumber)) {
            return INVALID_SEGMENT_ID;
        }

        SegmentLocation* loc = _segments[sequenceNumber];
        assert(loc != nullptr);

        return loc->id;
    }

    const StorageLocation& ServerStream::getStorageLocation(
        StreamSegmentId sequenceNumber) const
    {
        //TODO: Add locking !!
        ThrowOn(!_segmentIsAllocated(sequenceNumber), NotFoundException);

        SegmentLocation* loc = _segments[sequenceNumber];
        assert(loc != nullptr);

        ThrowOnNull(loc->location, NotFoundException);

        return *loc->location;
    }

    StreamSegmentId ServerStream::getCurrentSegmentId() const
    {
        return _lastAppendSequenceNumber;
    }

    ServerStore& ServerStream::getStore() const
    {
        return _store;
    }

    StreamEncoder& ServerStream::getEncoder() const
    {
        assert(_encoder != nullptr);
        return *_encoder;
    }

}