/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "StreamBuffer.h"

#include "Store.h"
#include "Stream.h"

namespace SimuTrace
{

    StreamBuffer::StreamBuffer(BufferId id, size_t segmentSize,
                               uint32_t numSegments, bool sharedMemory) :
        _id(id),
        _master(true),
        _buffer(nullptr),
        _segmentSize(segmentSize),
        _lineSize(_computeLineSize(segmentSize)),
        _numSegments(numSegments)
    {
        ThrowOn(id == INVALID_BUFFER_ID, ArgumentException, "id");
        ThrowOn((segmentSize != SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB) ||
                (numSegments == 0) ||
                (numSegments > SIMUTRACE_MEMMGMT_MAX_NUM_SEGMENTS_PER_BUFFER),
                NotSupportedException);

        // Create the data buffer and map it
        if (sharedMemory) {
            Guid guid;

            // Generate a random GUID for the data buffer.
            generateGuid(guid);

            std::string guidStr = stringFormat("simutrace.buffer.%s",
    #if (defined(__MACH__) && defined(__APPLE__))
                                 guidToString(guid, true).c_str());
    #else
                                 guidToString(guid).c_str());
    #endif

            _buffer = std::unique_ptr<MemorySegment>(
                new SharedMemorySegment(guidStr.c_str(), true,
                                        getBufferSize()));
        } else {
            _buffer = std::unique_ptr<MemorySegment>(
                new PrivateMemorySegment(getBufferSize()));
        }

        _buffer->map();
    }

    StreamBuffer::StreamBuffer(BufferId id, size_t segmentSize,
                               uint32_t numSegments, Handle& buffer) :
        _id(id),
        _master(false),
        _buffer(nullptr),
        _segmentSize(segmentSize),
        _lineSize(_computeLineSize(segmentSize)),
        _numSegments(numSegments)
    {
        ThrowOn(id == INVALID_BUFFER_ID, ArgumentException, "id");
        ThrowOn((segmentSize != SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB) ||
                (numSegments == 0) ||
                (numSegments > SIMUTRACE_MEMMGMT_MAX_NUM_SEGMENTS_PER_BUFFER),
                NotSupportedException);

        _buffer = std::unique_ptr<SharedMemorySegment>(
            new SharedMemorySegment(buffer, true, getBufferSize()));

        // Invalidate the callers handle value, because our buffer will
        // take care of closing the handle.
        buffer = INVALID_HANDLE_VALUE;

        _buffer->map();
    }

    StreamBuffer::~StreamBuffer()
    {

    }

    byte* StreamBuffer::_getFence(SegmentId segment) const
    {
        return reinterpret_cast<byte*>(getControlElement(segment)) +
            sizeof(SegmentControlElement);
    }

    size_t StreamBuffer::_getFenceSize() const
    {
        return _lineSize - _segmentSize - sizeof(SegmentControlElement);
    }

    size_t StreamBuffer::_computeLineSize(size_t segmentSize)
    {
        // We want each segment to be page-aligned. We therefore add padding
        // after the control element. This gives us the following structure
        // of a single line in the stream buffer:
        //
        // <-----------  e.g., 64 MiB  ----------><------ 1 Page ------>
        //
        // +--------------------------------------+------+-------------+
        // |            Segment data              | Ctrl |   Padding   |
        // +--------------------------------------+------+-------------+
        //

        const size_t pageSize = System::getPageSize();
        const size_t mask = ~(pageSize - 1);

        return segmentSize + ((sizeof(SegmentControlElement) + pageSize - 1) & mask);
    }

    byte StreamBuffer::_testMemory(byte* buffer, size_t size)
    {
        if (size == 0) {
            return 0x00;
        }

        // We take the first char in the buffer and check if all bytes in the
        // buffer are the same. If not, we do not have a fully filled area and
        // return 0.
        byte chr = *buffer;
        for (size_t pos = 0; pos < size; ++pos, ++buffer) {
            if (*buffer != chr) {
                return 0x00;
            }
        }

        // We are only interested in fully filled areas with one of our special
        // characters. If the area is filled with something else, return 0.
        if ((chr != FENCE_MEMORY_FILL) && (chr != DEAD_MEMORY_FILL) &&
            (chr != CLEAR_MEMORY_FILL)) {
            return 0x00;
        }

        // The area is filled with a debug char. Return which.
        return chr;
    }

    void StreamBuffer::_touch()
    {
        _buffer->touch();
    }

    byte* StreamBuffer::getSegmentAsPayload(SegmentId segment,
                                            size_t& outSize) const
    {
        outSize = _segmentSize + sizeof(SegmentControlElement);
        return getSegment(segment);
    }

    byte* StreamBuffer::getSegment(SegmentId segment) const
    {
        ThrowOn(segment >= _numSegments, ArgumentOutOfBoundsException, "segment");

        // The segment starts with the data and ends with the control element.
        // This way, half transferred segment in a socket-based session are
        // not regarded as filled by the server because the control segment is
        // still initialized to a zero entry count. The cookie further
        // guarantees that the control element has been entirely transferred.
        size_t offset = segment * _lineSize;

        assert(offset + _lineSize <= _buffer->getSize());

        return _buffer->getBuffer() + offset;
    }

    byte* StreamBuffer::getSegmentEnd(SegmentId segment, uint32_t entrySize) const
    {
        SegmentControlElement* ctrl = getControlElement(segment);

        size_t offset = (isVariableEntrySize(entrySize) ?
            getSizeHint(entrySize) : entrySize) * ctrl->rawEntryCount;
        assert(offset <= _segmentSize);

        return getSegment(segment) + offset;
    }

    SegmentControlElement* StreamBuffer::getControlElement(
        SegmentId segment) const
    {
        ThrowOn(segment >= _numSegments, ArgumentOutOfBoundsException, "segment");

        // The segment starts with the data and ends with the control element.
        // This way, half transferred segment in a socket-based session are
        // not regarded as filled by the server because the control segment is
        // still initialized to a zero entry count. The cookie further
        // guarantees that the control element has been entirely transferred.
        size_t offset = segment * _lineSize + _segmentSize;

        assert(offset + sizeof(SegmentControlElement) <= _buffer->getSize());

        return reinterpret_cast<SegmentControlElement*>(
            _buffer->getBuffer() + offset);
    }

    void StreamBuffer::dbgSanityFill(SegmentId segment, bool dead)
    {
        // Fill the segment data area with the special debug char that
        // indicates allocated but unused or dead memory. Fill the area behind
        // the control element (the fence) with the fence char. This area must
        // not be modified.
        byte chr = (dead) ? DEAD_MEMORY_FILL : CLEAR_MEMORY_FILL;
        memset(getSegment(segment), chr, getSegmentSize());
        memset(_getFence(segment), FENCE_MEMORY_FILL, _getFenceSize());
    }

    int StreamBuffer::dbgSanityCheck(SegmentId segment, uint32_t entrySize) const
    {
        const SegmentControlElement* control = getControlElement(segment);
        int errorLevel = 0;

        if (isVariableEntrySize(entrySize) == _true) {
            entrySize = getSizeHint(entrySize);
        }

        if (entrySize > 0) {
            // In debug builds we filled the segment with 0xCD (clean memory)
            // when issued to the user. We now check if the computed valid
            // buffer length according to the number of entries matches the
            // portion of the segment that has been overwritten. Otherwise, we
            // print a warning, because this indicates that the user forgot to
            // submit entries or submitted to many.
            size_t validBufferLength = entrySize * control->rawEntryCount;
            byte* cmppos = getSegment(segment) + validBufferLength;

            // The last written entry should at least have a single byte that
            // is not 0xCD. We do not perform the check if the buffer is empty
            // or if the buffer is not indexed. In the latter case, we cannot
            // assume a simple entry-based data structure and should not
            // try to interpret anything.
            if ((validBufferLength > 0) &&
                (control->startIndex != INVALID_ENTRY_INDEX)) {
                assert(validBufferLength >= entrySize);

                byte chr = _testMemory(cmppos - entrySize, entrySize);
                assert(chr != FENCE_MEMORY_FILL);
                assert(chr != DEAD_MEMORY_FILL);

                if (chr == CLEAR_MEMORY_FILL) {
                    LogWarn("Segment sanity check failed. Segment %d of buffer %s "
                            "seems to contain less entries than specified in the "
                            "control segment <stream: %d, sqn: %d, rec: %d, ec: %d>.",
                            segment, bufferIdToString(_id).c_str(),
                            control->link.stream, control->link.sequenceNumber,
                            control->rawEntryCount, control->entryCount);

                    errorLevel = 1;
                }
            }

            // Check if the area behind the last submitted entry has been touched
            if (validBufferLength < _segmentSize) {
                byte chr = _testMemory(cmppos, _segmentSize - validBufferLength);
                assert(chr != FENCE_MEMORY_FILL);
                assert(chr != DEAD_MEMORY_FILL);

                if (chr != CLEAR_MEMORY_FILL) {
                    LogWarn("Segment sanity check failed. Segment %d of buffer %s "
                        "has been modified beyond the last submitted entry "
                        "<stream: %d, sqn: %d, rec: %d, ec: %d>.",
                        segment, bufferIdToString(_id).c_str(),
                        control->link.stream, control->link.sequenceNumber,
                        control->rawEntryCount, control->entryCount);

                    errorLevel = 1;
                }
            }
        } else {
            assert(_segmentSize != 0);

            // If the entrySize is 0, we expect the segment to be dead
            byte chr = _testMemory(getSegment(segment), _segmentSize);
            assert(chr != FENCE_MEMORY_FILL);
            assert(chr != CLEAR_MEMORY_FILL);

            if (chr != DEAD_MEMORY_FILL) {
                LogError("Segment sanity check failed. Segment %d of buffer %s "
                         "has been modified while being marked as free.",
                         segment, bufferIdToString(_id).c_str());

                errorLevel = 1;
            }
        }

        // Check if the area behind the control element has been touched
        size_t fenceSize = _getFenceSize();
        assert(fenceSize != 0);

        byte chr = _testMemory(_getFence(segment), fenceSize);
        if (chr != FENCE_MEMORY_FILL) {
            LogError("Segment sanity check failed. The control fence of "
                     "segment %d in buffer %d has been corrupted "
                     "<stream: %d, sqn: %d>.",
                     segment, bufferIdToString(_id).c_str(),
                     control->link.stream, control->link.sequenceNumber);

            errorLevel = 2;
        }

        return errorLevel;
    }

    BufferId StreamBuffer::getId() const
    {
        return _id;
    }

    bool StreamBuffer::isMaster() const
    {
        return _master;
    }

    Handle StreamBuffer::getBufferHandle() const
    {
        auto buf = dynamic_cast<SharedMemorySegment*>(_buffer.get());
        if (buf == nullptr) {
            return INVALID_HANDLE_VALUE;
        }

        return buf->getHandle();
    }

    size_t StreamBuffer::getBufferSize() const
    {
        return _numSegments * _lineSize;
    }

    size_t StreamBuffer::getSegmentSize() const
    {
        return _segmentSize;
    }

    uint32_t StreamBuffer::getNumSegments() const
    {
        return _numSegments;
    }

}