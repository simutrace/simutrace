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
#ifndef STOP_WATCH_H
#define STOP_WATCH_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    class StopWatch
    {
    private:
        uint64_t _start;
        uint64_t _end;

        bool _running;
    public:
        StopWatch();
        ~StopWatch();
        ///
        /// Starts the time measurement
        ///
        void start();

        ///
        /// Stops the time measurement
        ///
        void stop();

        ///
        /// Returns the time eleapsed for the measurement period
        ///
        const uint64_t getTicks() const;
        const uint64_t getSeconds() const;
    };

}

#endif

