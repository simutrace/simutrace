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
#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    template<typename T>
    class IdAllocator
    {
    private:
        DISABLE_COPY(IdAllocator<T>);

        T _nextId;

    public:
        IdAllocator() :
            _nextId(0) { }
        ~IdAllocator() { }

        T getNextId()
        {
            ThrowOn(_nextId == std::numeric_limits<T>::max(), 
                    InvalidOperationException);

            return _nextId++;
        }

        void stealId(T id) { _nextId = ++id; }

        void retireId(T id) {}
    };

}

#endif