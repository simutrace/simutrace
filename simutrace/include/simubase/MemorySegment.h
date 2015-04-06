/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef MEMORY_SEGMENT_H
#define MEMORY_SEGMENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{
    class MemorySegment
    {
    private:
        size_t _size;
        size_t _mapStart;
        size_t _mapSize;
        bool _writeable;

        byte* _buffer;

        bool _inMappedRange(size_t start, size_t size) const;
    protected:
        MemorySegment(bool writeable);
        MemorySegment(bool writeable, size_t size);
        MemorySegment(const MemorySegment& instance);
        MemorySegment& operator=(const MemorySegment& instance) = delete;

        virtual byte* _map(size_t start, size_t size) = 0;
        virtual void _unmap(byte* buffer) = 0;

        void _finalize();

        void _setSize(size_t size);
    public:
        virtual ~MemorySegment();

        // Returns the size of the memory segment
        size_t getSize() const;

        // Returns the currently mapped size
        size_t getMappedSize() const;

        // Returns the start address of the memory segment
        byte* getBuffer() const;

        // Makes the associated memory accessible and returns the address
        byte* map();
        byte* map(size_t start, size_t size);

        // Unmaps any previously mapped memory.
        // N.B. Call unmap() before destroying the memory segment object!
        void unmap();

        // Sets the memory segment's protection
        void setProtection(bool writeable);

        // Fills the memory with zeros
        void zero();

        // Fills the memory with a user specified byte.
        void fill(uint8_t value);

        // Touches the associated memory, reading each page's first word
        void touch();

        // Copies the specified buffer to an offset in this memory segment.
        void copy(const byte* source, size_t size, size_t offset = 0);
        void copy(MemorySegment& source, size_t offset = 0);

        bool isMapped() const;
        bool isReadOnly() const;
    };
}
#endif