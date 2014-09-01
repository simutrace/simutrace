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
#ifndef LOCK_REGION_H
#define LOCK_REGION_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "CriticalSection.h"

namespace SimuTrace
{

#define _LockScope2(lock, name) \
    LockRegion lock_region_##name(lock)

#define _LockScope(lock, name) \
    _LockScope2(lock, name)

#define LockScope(lock) \
    _LockScope(lock, __LINE__)

#define Lock(lock) \
    { LockScope(lock)

#define Unlock() \
    }

    struct LockRegion
    {
    private:
        DISABLE_COPY(LockRegion);

        CriticalSection& _lock;

    public:
        LockRegion(CriticalSection& lock) :
            _lock(lock) { _lock.enter(); }
        ~LockRegion() { _lock.leave(); }
    };

}

#endif