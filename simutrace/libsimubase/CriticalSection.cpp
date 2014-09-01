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

#include "CriticalSection.h"

#include "Exceptions.h"

namespace SimuTrace
{
    CriticalSection::CriticalSection()
    {
    #ifdef WIN32
        ::InitializeCriticalSection(&_cs);
    #else
        int result = ::pthread_mutex_init(&_cs, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    }

    CriticalSection::~CriticalSection()
    {
    #ifdef WIN32
        ::DeleteCriticalSection(&_cs);
    #else
    #endif
    }

    void CriticalSection::enter()
    {
    #ifdef WIN32
        ::EnterCriticalSection(&_cs);
    #else
        int result = ::pthread_mutex_lock(&_cs);
        if (result != 0) {
            // Replicate Windows behavior
            if (result == EDEADLK) {
                return;
            }

            Throw(PlatformException, result);
        }
    #endif
    }

    bool CriticalSection::tryToEnter()
    {
    #ifdef WIN32
        if (::TryEnterCriticalSection(&_cs) == 0) {
    #else
        if (::pthread_mutex_trylock(&_cs) != 0) {
    #endif
            return false;
        }
        return true;
    }

    void CriticalSection::leave()
    {
    #ifdef WIN32
        ::LeaveCriticalSection(&_cs);
    #else
        int result = ::pthread_mutex_unlock(&_cs);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    }

}