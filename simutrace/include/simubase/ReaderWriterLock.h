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
#ifndef READERWRITER_LOCK_H
#define READERWRITER_LOCK_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "CriticalSection.h"

namespace SimuTrace
{

    class ReaderWriterLock
    {
    private:
        DISABLE_COPY(ReaderWriterLock);

    #ifdef _DEBUG
        // To ease lock debugging we map reader-writer locks 
        // to critical sections in debug builds
        CriticalSection _lock;
    #else

    #ifdef _WIN32
        SRWLOCK _lock;
    #else
        pthread_rwlock_t _lock;
    #endif

    #endif        
    public:
        ReaderWriterLock();
        ~ReaderWriterLock();

        bool tryAcquireExclusive();
        void acquireExclusive();
        void releaseExclusive();

        bool tryAcquireShared();
        void acquireShared();
        void releaseShared();
    };

}

#endif // READERWRITER_LOCK_H