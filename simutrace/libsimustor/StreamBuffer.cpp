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

    void StreamBuffer::touch()
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

    size_t StreamBuffer::computeBufferSize(size_t segmentSize,
                                           uint32_t numSegments)
    {
        return numSegments * _computeLineSize(segmentSize);
    }

}