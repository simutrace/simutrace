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

#include "ConditionVariable.h"

#include "Exceptions.h"

namespace SimuTrace
{

    ConditionVariable::ConditionVariable() :
        CriticalSection()
    {
    #if defined(_WIN32)
        ::InitializeConditionVariable(&_cv);
    #else
        int result = ::pthread_cond_init(&_cv, nullptr);
        if (result != 0) {
            Throw(PlatformException, result);
        }
    #endif
    }

    ConditionVariable::~ConditionVariable()
    {
    #if defined(_WIN32)
    #else
        ::pthread_cond_destroy(&_cv);
    #endif
    }

    void ConditionVariable::wait()
    {
    #if defined(_WIN32)
        if (!::SleepConditionVariableCS(&_cv, &_cs, INFINITE)) {
            Throw(PlatformException);
        }
    #else
        int result = ::pthread_cond_wait(&_cv, &_cs);
        if (result != 0) {
            Throw(PlatformException, result);
        }
    #endif
    }

    void ConditionVariable::wakeOne()
    {
    #if defined(_WIN32)
        ::WakeConditionVariable(&_cv);
    #else
        int result = ::pthread_cond_signal(&_cv);
        if (result != 0) {
            Throw(PlatformException, result);
        }
    #endif
    }

    void ConditionVariable::wakeAll()
    {
    #if defined(_WIN32)
        ::WakeAllConditionVariable(&_cv);
    #else
        int result = ::pthread_cond_broadcast(&_cv);
        if (result != 0) {
            Throw(PlatformException, result);
        }
    #endif
    }

}