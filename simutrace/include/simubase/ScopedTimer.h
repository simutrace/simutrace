/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Bastian Eicher
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
#ifndef SCOPED_TIMER_H
#define SCOPED_TIMER_H

#include "SimuPlatform.h"

#include "Clock.h"
#include "Profiler.h"

namespace SimuTrace
{

    class ScopedTimer
    {
    private:
        DISABLE_COPY(ScopedTimer);

        ProfileContext& _profileContext;
        const char* _name;
        uint64_t _start;

    public:
        ScopedTimer(ProfileContext& profileContext, const char* name) :
            _profileContext(profileContext),
            _name(name),
            _start(Clock::getTicks()) { }

        ~ScopedTimer()
        {
            uint64_t milliseconds = (Clock::getTicks() - _start) / 1000000;

            _profileContext.add(_name, milliseconds);
        }
    };

}

#endif