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

#include "Interlocked.h"

namespace SimuTrace {
namespace Interlocked
{

    uint32_t interlockedAdd(volatile uint32_t* dst, uint32_t value)
    {
    #ifdef WIN32
        return ::InterlockedExchangeAdd(
            reinterpret_cast<volatile LONG*>(dst),
            static_cast<LONG>(value));
    #else
        return __sync_fetch_and_add(dst, value);
    #endif
    }

    uint64_t interlockedAdd(volatile uint64_t* dst, uint64_t value)
    {
    #ifdef WIN32
        return ::InterlockedExchangeAdd64(
            reinterpret_cast<volatile LONGLONG*>(dst),
            static_cast<LONGLONG>(value));
    #else
        return __sync_fetch_and_add(dst, value);
    #endif
    }

    uint32_t interlockedSub(volatile uint32_t* dst, uint32_t value)
    {
    #ifdef WIN32
        return ::InterlockedExchangeSubtract(
            reinterpret_cast<volatile unsigned long*>(dst),
            static_cast<unsigned long>(value));
    #else
        return __sync_fetch_and_sub(dst, value);
    #endif
    }

    uint64_t interlockedSub(volatile uint64_t* dst, uint64_t value)
    {
    #ifdef WIN32
        return ::InterlockedExchangeSubtract(
            reinterpret_cast<volatile unsigned long*>(dst),
            static_cast<unsigned long>(value));
    #else
        return __sync_fetch_and_sub(dst, value);
    #endif
    }

    uint32_t interlockedCompareExchange(volatile uint32_t* dst,
                                        uint32_t exchange,
                                        uint32_t comperand)
    {
    #ifdef WIN32
        return ::InterlockedCompareExchange(
            reinterpret_cast<volatile LONG*>(dst),
            static_cast<LONG>(exchange),
            static_cast<LONG>(comperand));
    #else
        return __sync_val_compare_and_swap(dst, exchange, comperand);
    #endif
    }

    uint64_t interlockedCompareExchange(volatile uint64_t* dst,
                                        uint64_t exchange,
                                        uint64_t comperand)
    {
    #ifdef WIN32
        return ::InterlockedCompareExchange64(
            reinterpret_cast<volatile LONGLONG*>(dst),
            static_cast<LONGLONG>(exchange),
            static_cast<LONGLONG>(comperand));
    #else
        return __sync_val_compare_and_swap(dst, exchange, comperand);
    #endif
    }

    uint32_t interlockedAnd(volatile uint32_t* dst, uint32_t value)
    {
    #ifdef WIN32
        return ::InterlockedAnd(
            reinterpret_cast<volatile LONG*>(dst),
            static_cast<LONG>(value));
    #else
        return __sync_fetch_and_and(dst, value);
    #endif
    }

    uint64_t interlockedAnd(volatile uint64_t* dst, uint64_t value)
    {
    #ifdef WIN32
        return ::InterlockedAnd64(
            reinterpret_cast<volatile LONGLONG*>(dst),
            static_cast<LONGLONG>(value));
    #else
        return __sync_fetch_and_and(dst, value);
    #endif
    }

    uint32_t interlockedOr(volatile uint32_t* dst, uint32_t value)
    {
    #ifdef WIN32
        return ::InterlockedOr(
            reinterpret_cast<volatile LONG*>(dst),
            static_cast<LONG>(value));
    #else
        return __sync_fetch_and_or(dst, value);
    #endif
    }

    uint64_t interlockedOr(volatile uint64_t* dst, uint64_t value)
    {
    #ifdef WIN32
        return ::InterlockedOr64(
            reinterpret_cast<volatile LONGLONG*>(dst),
            static_cast<LONGLONG>(value));
    #else
        return __sync_fetch_and_or(dst, value);
    #endif
    }

}
}