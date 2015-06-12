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
#ifndef WAIT_CONTEXT_H
#define WAIT_CONTEXT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "ConditionVariable.h"

#include "Exceptions.h"
#include "Interlocked.h"

namespace SimuTrace
{
    template<typename T>
    class WaitContext
    {
    private:
        DISABLE_COPY(WaitContext<T>);

        ConditionVariable _cv;

        std::vector<T> _errors;

        volatile uint32_t _count;
    public:
        WaitContext() :
            _count(0) {}
        ~WaitContext()
        {
            // Wait for any worker who uses this context to finish
            try {
                wait();
            } catch (...) {

            }
        }

        uint32_t increment()
        {
            assert(_count < std::numeric_limits<uint32_t>::max());
            return Interlocked::interlockedAdd(&_count, 1) + 1;
        }

        uint32_t decrement()
        {
            assert(_count > 0);
            uint32_t v = Interlocked::interlockedSub(&_count, 1) - 1;
            if (v == 0) {
                _cv.wakeAll();
            }

            return v;
        }

        virtual bool wait()
        {
            Lock(_cv); {
                while (_count > 0) {
                    _cv.wait();
                }
            } Unlock();

            return _errors.empty();
        }

        void pushError(T& error)
        {
            _errors.push_back(error);
        }

        bool popError(T& error)
        {
            if (!_errors.empty()) {
                error = _errors.back();
                _errors.pop_back();
                return true;
            } else {
                return false;
            }
        }

        void reset()
        {
            _count = 0;
            _errors.clear();
        }

        uint32_t getCount() const
        {
            return _count;
        }
    };

}

#endif