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
#ifndef READER_WRITER_LOCK_REGION_H
#define READER_WRITER_LOCK_REGION_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "ReaderWriterLock.h"

namespace SimuTrace
{

    enum LockMethod {
        LmExclusive,
        LmShared
    };

#define _LockScopeRw2(lock, name, mode) \
    ReaderWriterLockRegion rw_lock_region_##name(lock, mode)

#define _LockScopeRw(lock, name, mode) \
    _LockScopeRw2(lock, name, mode)

#define LockScopeShared(lock) \
    _LockScopeRw(lock, __LINE__, LmShared)

#define LockScopeExclusive(lock) \
    _LockScopeRw(lock, __LINE__, LmExclusive)

#define LockShared(lock) \
    { LockScopeShared(lock)

#define LockExclusive(lock) \
    { LockScopeExclusive(lock)

    struct ReaderWriterLockRegion
    {
    private:
        DISABLE_COPY(ReaderWriterLockRegion);

        ReaderWriterLock& _lock;
        LockMethod _method;

    public:
        ReaderWriterLockRegion(ReaderWriterLock& lock, LockMethod method) :
            _lock(lock),
            _method(method)
        {
            if (_method == LmShared) {
                _lock.acquireShared();
            } else {
                _lock.acquireExclusive();
            }
        }

        ~ReaderWriterLockRegion()
        {
            if (_method == LmShared) {
                _lock.releaseShared();
            } else {
                _lock.releaseExclusive();
            }
        }
    };

}

#endif