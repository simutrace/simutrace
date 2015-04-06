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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "MemorySegment.h"

#include "Exceptions.h"
#include "MemoryHelpers.h"

namespace SimuTrace
{

    MemorySegment::MemorySegment(bool writeable) :
        _size(0),
        _mapStart(0),
        _mapSize(0),
        _writeable(writeable),
        _buffer(nullptr)
    {

    }

    MemorySegment::MemorySegment(bool writeable, size_t size) :
        MemorySegment(writeable)
    {
        ThrowOn(size == 0, ArgumentException);

        _size = size;
    }

    MemorySegment::MemorySegment(const MemorySegment& instance) :
        _size(instance._size),
        _mapStart(0),
        _mapSize(0),
        _writeable(instance._writeable),
        _buffer(nullptr)
    {
        // Since a true copy semantic does not make sense for a memory segment
        // (we would get two owners of the same virtual address space region),
        // copying a memory segment only duplicates the general properties, but
        // does not establish a mapping (i.e., allocate a virtual address
        // range or map a file).
    }

    MemorySegment::~MemorySegment()
    {

    }

    void MemorySegment::_finalize()
    {
       try {
           unmap();
       } catch (...) {

       }
    }

    bool MemorySegment::_inMappedRange(size_t start, size_t size) const
    {
        return (start >= _mapStart) && (start + size <= _mapStart + _mapSize);
    }

    void MemorySegment::_setSize(size_t size)
    {
        ThrowOn(isMapped(), InvalidOperationException);

        _size = size;
    }

    size_t MemorySegment::getSize() const
    {
        return _size;
    }

    size_t MemorySegment::getMappedSize() const
    {
        return _mapSize;
    }

    byte* MemorySegment::getBuffer() const
    {
        return (isMapped()) ? _buffer : nullptr;
    }

    byte* MemorySegment::map()
    {
        return map(0, _size);
    }

    byte* MemorySegment::map(size_t start, size_t size)
    {
        ThrowOn(start + size > _size, ArgumentOutOfBoundsException);

        // If it is already mapped check if it is the same mapping operation
        // if not unmap and establish a new mapping

        if (isMapped()) {

            if (_inMappedRange(start, size)) {
                return _buffer;
            }

            unmap();
        }

        _buffer = _map(start, size);

        _mapStart = start;
        _mapSize = size;
        return _buffer;
    }

    void MemorySegment::unmap()
    {
        if (!isMapped()) {
            return;
        }

        assert(_buffer != nullptr);
        _unmap(_buffer);

        _buffer = nullptr;
        _mapStart = 0;
        _mapSize = 0;
    }

    void MemorySegment::setProtection(bool writeable)
    {
        ThrowOn(!isMapped(), InvalidOperationException);

        if (writeable == _writeable) {
            return;
        }

    #if defined(_WIN32)
        DWORD oldpro;
        DWORD prot = (writeable) ? PAGE_READONLY : PAGE_READWRITE;
        if (!::VirtualProtect(_buffer, _mapSize, prot, &oldpro)) {
    #else
        unsigned int prot = (writeable) ? PROT_READ : PROT_READ | PROT_WRITE;
        if (!::mprotect(_buffer, _mapSize, prot)) {
    #endif
            Throw(PlatformException);
        }

        _writeable = writeable;
    }

    void MemorySegment::zero()
    {
        fill(0);
    }

    void MemorySegment::fill(uint8_t value)
    {
        ThrowOn(!isMapped() || isReadOnly(), InvalidOperationException);

        memset(_buffer, value, _mapSize);
    }

    void MemorySegment::copy(MemorySegment& source, size_t offset)
    {
        ThrowOn(!isMapped() || isReadOnly() || !source.isMapped(),
                InvalidOperationException);

        copy(source.getBuffer(), source.getMappedSize(), offset);
    }

    void MemorySegment::copy(const byte* source, size_t size, size_t offset)
    {
        ThrowOnNull(source, ArgumentNullException);
        ThrowOn(!isMapped() || isReadOnly(), InvalidOperationException);
        ThrowOn(offset + size >= _mapSize, ArgumentOutOfBoundsException);

        byte* dest = _buffer + offset;
        memcpy(dest, source, size);
    }

    #pragma optimize("", off)
    void MemorySegment::touch()
    {
        ThrowOn(!isMapped(), InvalidOperationException);

        size_t pageSize = System::getPageSize();

        byte* tmp = _buffer;
        char test = 0x00;
        for (size_t i = 0; i < _mapSize; i += pageSize) {
            tmp[i] = test;
        }

    }
    #pragma optimize("", on)

    bool MemorySegment::isMapped() const
    {
        return (_mapSize > 0);
    }

    bool MemorySegment::isReadOnly() const
    {
        return (!_writeable);
    }
}