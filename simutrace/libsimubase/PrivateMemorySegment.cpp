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

#include "PrivateMemorySegment.h"

#include "Exceptions.h"

namespace SimuTrace
{
    PrivateMemorySegment::PrivateMemorySegment(size_t size) :
        MemorySegment(true, size)
    {

    }

    PrivateMemorySegment::PrivateMemorySegment(
        const PrivateMemorySegment& instance) :
        MemorySegment(instance)
    {

    }

    PrivateMemorySegment::~PrivateMemorySegment()
    {
        _finalize();
    }

    byte* PrivateMemorySegment::_map(size_t start, size_t size)
    {
        ThrowOn(start != 0 && size != getSize(), InvalidOperationException);

    #if defined(_WIN32)
        unsigned int prot = (isReadOnly()) ? PAGE_READONLY : PAGE_READWRITE;
        void* buffer = ::VirtualAlloc(nullptr, size, MEM_COMMIT, prot);
    #else
    #if (defined(__MACH__) && defined(__APPLE__))
    #define MAP_ANONYMOUS MAP_ANON
    #endif
        unsigned int prot = (isReadOnly()) ? PROT_READ : PROT_READ | PROT_WRITE;
        void* buffer = ::mmap(0, size, prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    #endif

        ThrowOnNull(buffer, PlatformException);

        return static_cast<byte*>(buffer);
    }

    void PrivateMemorySegment::_unmap(byte* buffer)
    {
    #if defined(_WIN32)
        if (!::VirtualFree(buffer, 0, MEM_RELEASE)) {
    #else
        if (::munmap(buffer, getMappedSize()) != 0) {
    #endif
            Throw(PlatformException);
        }
    }

}