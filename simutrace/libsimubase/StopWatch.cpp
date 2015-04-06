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

#include "StopWatch.h"

#include "SimuBaseTypes.h"
#include "Exceptions.h"

#include "Clock.h"

namespace SimuTrace
{

    StopWatch::StopWatch()
    {
        _running = false;
    }

    StopWatch::~StopWatch()
    {

    }

    void StopWatch::start()
    {
        ThrowOn(_running, InvalidOperationException);

        _running = true;
        _start = Clock::getTicks();
    }

    void StopWatch::stop()
    {
        uint64_t end = Clock::getTicks();

        ThrowOn(!_running, InvalidOperationException);

        _end = end;
        _running = false;
    }

    const uint64_t StopWatch::getTicks() const
    {
        uint64_t end = (_running) ? Clock::getTicks() : _end;
        return end - _start;
    }

    const uint64_t StopWatch::getSeconds() const
    {
        return Clock::ticksToSeconds(this->getTicks());
    }

}