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
#pragma once
#ifndef SIMTRACE3_FRAME_H
#define SIMTRACE3_FRAME_H

#include "SimuStor.h"
#include "Simtrace3Format.h"

namespace SimuTrace {
namespace Simtrace
{

    typedef std::vector<AttributeHeaderDescription> AttributeList;

    class Simtrace3Frame
    {
    private:
        DISABLE_COPY(Simtrace3Frame);

        AttributeList _attributes;
        FrameHeader _header;

        std::unique_ptr<FileBackedMemorySegment> _mapping;
        void* _buffer;

        FileOffset _offset;
    public:
        Simtrace3Frame(Stream* stream = nullptr,
                       SegmentControlElement* control = nullptr);
        Simtrace3Frame(const FrameHeader& header);
        ~Simtrace3Frame();

        void map(const FileBackedMemorySegment& store, FileOffset offset,
                 size_t size);

        void addAttribute(Simtrace3AttributeType type,
                          uint64_t uncompressedSize,
                          void* buffer);
        void addAttribute(Simtrace3AttributeType type,
                          uint64_t uncompressedSize,
                          uint64_t size, void* buffer);
        void addAttribute(AttributeHeaderDescription& desc);

        AttributeHeaderDescription* findAttribute(Simtrace3AttributeType type);

        void updateHash();
        bool validateHash() const;

        FrameHeader& getHeader();
        AttributeList& getAttributeList();

        void* getBuffer() const;

        FileOffset getOffset() const;
        void setOffset(FileOffset offset);
    };

}
}

#endif