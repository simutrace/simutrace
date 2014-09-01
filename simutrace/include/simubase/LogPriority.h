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
#ifndef LOG_PRIORITY_H
#define LOG_PRIORITY_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    namespace LogPriority {
   
        // Predefined Priority Levels
        typedef enum {
            Fatal        = 0,
            Error        = 1,
            Warning      = 2,
            Information  = 3,
            Debug        = 4,
            MemDebug     = 5,
            RpcDebug     = 6,

            NotSet       = 7
        } PriorityLevel;

        typedef uint32_t Value;

        const std::string& getPriorityName(LogPriority::Value priority);
    };

}

#endif