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

#include "Simtrace3Frame.h"
#include "Simtrace3Format.h"

#include "../ServerStreamBuffer.h"

namespace SimuTrace {
namespace Simtrace
{

    Simtrace3Frame::Simtrace3Frame(Stream* stream,
                                   SegmentControlElement* control) :
        _attributes(),
        _mapping(nullptr),
        _buffer(nullptr),
        _offset(INVALID_FILE_OFFSET)
    {
        memset(&_header, 0, sizeof(FrameHeader));
        _header.markerValue = SIMTRACE_V3_FRAME_MARKER;

        if (stream != nullptr) {
            // The caller supplied a stream. We are either building a frame for
            // a real stream segment, or we are building the zero frame.
            const StreamTypeDescriptor& type = stream->getType();

            _header.streamId = stream->getId();
            _header.typeId   = type.id;
        } else {
            // It is not valid to supply a control element without the stream!
            ThrowOn(control != nullptr, ArgumentException);

            _header.streamId = INVALID_STREAM_ID;
        }

        if (control != nullptr) {
            assert(stream != nullptr);
            assert(control->link.stream == stream->getId());

            _header.sequenceNumber = control->link.sequenceNumber;

            _header.entryCount     = control->entryCount;
            _header.rawEntryCount  = control->rawEntryCount;

            _header.startTime      = control->startTime;
            _header.endTime        = control->endTime;
            _header.startCycle     = control->startCycle;
            _header.endCycle       = control->endCycle;

            _header.startIndex     = control->startIndex;
        } else {
            // This sequence number will be set for the frame 0, which contains
            // the meta data of a stream.
            _header.sequenceNumber = INVALID_STREAM_SEGMENT_ID;

            _header.startTime      = INVALID_TIME_STAMP;
            _header.endTime        = INVALID_TIME_STAMP;
            _header.startCycle     = INVALID_CYCLE_COUNT;
            _header.endCycle       = INVALID_CYCLE_COUNT;

            _header.startIndex     = INVALID_ENTRY_INDEX;
        }

        _header.totalSize = sizeof(FrameHeader);
    }

    Simtrace3Frame::Simtrace3Frame(const FrameHeader& header) :
        _header(header)
    {

    }

    Simtrace3Frame::~Simtrace3Frame()
    {

    }

    void Simtrace3Frame::map(const FileBackedMemorySegment& store,
                             FileOffset offset, size_t size)
    {
        std::unique_ptr<FileBackedMemorySegment> mapping(
            new FileBackedMemorySegment(store));

        mapping->updateSize();

        _buffer = mapping->map(offset, size);

        _mapping = std::move(mapping);
        _offset = offset;
    }

    void Simtrace3Frame::addAttribute(Simtrace3AttributeType type,
                                      uint64_t uncompressedSize,
                                      void* buffer)
    {
        addAttribute(type, uncompressedSize, uncompressedSize, buffer);
    }

    void Simtrace3Frame::addAttribute(Simtrace3AttributeType type,
                                      uint64_t uncompressedSize,
                                      uint64_t size,
                                      void* buffer)
    {
        AttributeHeaderDescription description = {0};
        uint8_t index = _header.attributeCount;

        ThrowOn(index >= SIMTRACE_V3_FRAME_ATTRIBUTE_TABLE_SIZE,
                InvalidOperationException);

        AttributeHeader* attrHeader  = &description.header;
        attrHeader->markerValue      = SIMTRACE_V3_ATTRIBUTE_MARKER;
        attrHeader->type             = type;
        attrHeader->size             = size;
        attrHeader->uncompressedSize = uncompressedSize;

        attrHeader->reserved1        = 0;
        memset(attrHeader->reserved0, 0, sizeof(attrHeader->reserved0));

        description.buffer = buffer;
        _attributes.push_back(description);

        // Add attribute to frame header
        _header.attributes[index].type = type;
        _header.attributes[index].relativeFileOffset = _header.totalSize;

        _header.attributeCount++;
        _header.totalSize += sizeof(AttributeHeader) + size;
    }

    void Simtrace3Frame::addAttribute(AttributeHeaderDescription& desc)
    {
        _attributes.push_back(desc);
    }

    AttributeHeaderDescription* Simtrace3Frame::findAttribute(
        Simtrace3AttributeType type)
    {
        AttributeHeaderDescription* result = nullptr;
        for (int i = 0; i < _attributes.size(); ++i) {
            AttributeHeaderDescription& desc = _attributes[i];

            if (desc.header.type == type) {
                return &desc;
            }
        }

        return nullptr;
    }

    void Simtrace3Frame::updateHash()
    {
        const int size = SIMTRACE_V3_FRAME_HEADER_CHECKSUM_DATA_SIZE;

        // Update the hash of the header.
        Hash::murmur3_32(&_header, size, &_header.checksum, sizeof(uint32_t), 0);
    }

    bool Simtrace3Frame::validateHash() const
    {
        const int size = SIMTRACE_V3_FRAME_HEADER_CHECKSUM_DATA_SIZE;
        uint32_t checksum;

        // Calculate hash for validation
        Hash::murmur3_32(&_header, size, &checksum, sizeof(uint32_t), 0);

        return (checksum == _header.checksum);
    }

    FrameHeader& Simtrace3Frame::getHeader()
    {
        return _header;
    }

    AttributeList& Simtrace3Frame::getAttributeList()
    {
        return _attributes;
    }

    void* Simtrace3Frame::getBuffer() const
    {
        assert(_buffer != nullptr);
        return _buffer;
    }

    FileOffset Simtrace3Frame::getOffset() const
    {
        assert(_offset != INVALID_FILE_OFFSET);
        return _offset;
    }

    void Simtrace3Frame::setOffset(FileOffset offset)
    {
        assert(offset != INVALID_FILE_OFFSET);
        _offset = offset;
    }
}
}