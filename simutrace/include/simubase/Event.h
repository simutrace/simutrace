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
#ifndef EVENT_H
#define EVENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "SafeHandle.h"

namespace SimuTrace
{

    class Event
    {
    private:
        DISABLE_COPY(Event);

        std::string _name;
    #ifdef WIN32
        SafeHandle _event;
    #else
        sem_t* _event;
        sem_t _sem;
    #endif

        void _initEvent(const char* name);
    public:
        Event();
        Event(const std::string& name);
        ~Event();

        void signal();
        void wait();
        bool tryWait();

        const std::string& getName() const;
    };

};

#endif