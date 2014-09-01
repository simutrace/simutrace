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
#ifndef SHARED_MEMORY_SEGMENT_H
#define SHARED_MEMORY_SEGMENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "FileBackedMemorySegment.h"

namespace SimuTrace
{
    
    class SharedMemorySegment :
        public FileBackedMemorySegment
    {
    private:
        Handle _getSharedMemoryHandle(const std::string& name, bool writeable, 
                                      size_t size);

    public:
        SharedMemorySegment(const std::string& name, bool writeable, 
                            size_t size);
        SharedMemorySegment(Handle shmHandle, bool writeable, size_t size);
        SharedMemorySegment(const SharedMemorySegment& instance);
        virtual ~SharedMemorySegment();

        SharedMemorySegment& operator=(SharedMemorySegment instance) = delete;
    };
}

#endif