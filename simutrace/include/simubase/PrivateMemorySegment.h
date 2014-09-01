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
#ifndef PRIVATE_MEMORY_SEGMENT_H
#define PRIVATE_MEMORY_SEGMENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "MemorySegment.h"

namespace SimuTrace
{

    class PrivateMemorySegment :
        public MemorySegment
    {
    protected:
        virtual byte* _map(size_t start, size_t size) override;
        virtual void _unmap(byte* buffer) override;

    public:
        PrivateMemorySegment(size_t size);
        PrivateMemorySegment(const PrivateMemorySegment& instance);
        virtual ~PrivateMemorySegment();    

        PrivateMemorySegment& operator=(PrivateMemorySegment instance) = delete;
    };

}

#endif