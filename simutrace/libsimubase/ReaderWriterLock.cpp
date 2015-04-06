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

#include "ReaderWriterLock.h"

#include "CriticalSection.h"
#include "Exceptions.h"

namespace SimuTrace
{

    ReaderWriterLock::ReaderWriterLock() :
        _lock()
    {
    #ifdef _DEBUG
    #else
    #if defined(_WIN32)
        ::InitializeSRWLock(&_lock);
    #else
        int result = ::pthread_rwlock_init(&_lock, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    #endif
    }

    ReaderWriterLock::~ReaderWriterLock()
    {
    #ifdef _DEBUG
    #else
    #if defined(_WIN32)
    #else
        ::pthread_rwlock_destroy(&_lock);
    #endif
    #endif
    }

    bool ReaderWriterLock::tryAcquireExclusive()
    {
    #ifdef _DEBUG
        return _lock.tryToEnter();
    #else
    #if defined(_WIN32)
        if (::TryAcquireSRWLockExclusive(&_lock) != TRUE) {
    #else
        if (::pthread_rwlock_trywrlock(&_lock) != 0) {
    #endif
            return false;
        }
        return true;
    #endif
    }

    void ReaderWriterLock::acquireExclusive()
    {
    #ifdef _DEBUG
        _lock.enter();
    #else
    #if defined(_WIN32)
        ::AcquireSRWLockExclusive(&_lock);
    #else
        int result = ::pthread_rwlock_wrlock(&_lock);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    #endif
    }

    void ReaderWriterLock::releaseExclusive()
    {
    #ifdef _DEBUG
        _lock.leave();
    #else
    #if defined(_WIN32)
        ::ReleaseSRWLockExclusive(&_lock);
    #else
        int result = ::pthread_rwlock_unlock(&_lock);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    #endif
    }

    bool ReaderWriterLock::tryAcquireShared()
    {
    #ifdef _DEBUG
        return _lock.tryToEnter();
    #else
    #if defined(_WIN32)
        if (::TryAcquireSRWLockShared(&_lock) != TRUE) {
    #else
        if (::pthread_rwlock_tryrdlock(&_lock) != 0) {
    #endif
            return false;
        }
        return true;
    #endif
    }

    void ReaderWriterLock::acquireShared()
    {
    #ifdef _DEBUG
        _lock.enter();
    #else
    #if defined(_WIN32)
        ::AcquireSRWLockShared(&_lock);
    #else
        int result = ::pthread_rwlock_rdlock(&_lock);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    #endif
    }

    void ReaderWriterLock::releaseShared()
    {
    #ifdef _DEBUG
        _lock.leave();
    #else
    #if defined(_WIN32)
        ::ReleaseSRWLockShared(&_lock);
    #else
        int result = ::pthread_rwlock_unlock(&_lock);
        ThrowOn(result != 0, PlatformException, result);
    #endif
    #endif
    }

}