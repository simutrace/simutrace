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
#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    class CriticalSection
    {
    private:
        DISABLE_COPY(CriticalSection);

    protected:
    #if defined(_WIN32)
        CRITICAL_SECTION _cs;
    #else
        pthread_mutex_t _cs;
    #endif
    public:
        CriticalSection();
        ~CriticalSection();
        ///
        /// Enters the critical section
        ///
        void enter();
        ///
        /// Tries to lock the critical section returns true, if lock is held
        /// otherwise it returns false
        ///
        bool tryToEnter();
        ///
        /// Leaves the critical section
        ///
        void leave();
    };

}

#endif