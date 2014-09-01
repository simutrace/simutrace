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
#ifndef SCRATCH_SEGMENT_H
#define SCRATCH_SEGMENT_H

#include "SimuStor.h"

namespace SimuTrace 
{

    class ServerStreamBuffer;

    class ScratchSegment
    {
    private:
        typedef ObjectReference<PrivateMemorySegment> PrivateMemoryReference;

    private:
        DISABLE_COPY(ScratchSegment);

        ServerStreamBuffer& _buffer;
        SegmentId _id;

        PrivateMemoryReference _fallbackMemory;

        void _initializeMemory();
    public:
        ScratchSegment();
        ScratchSegment(ServerStreamBuffer& buffer);
        ~ScratchSegment();

        void* getBuffer() const;
        size_t getLength() const;
    };

}

#endif