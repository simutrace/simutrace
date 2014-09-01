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
#ifndef LOG_EVENT_H
#define LOG_EVENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "LogPriority.h"

namespace SimuTrace
{

    class LogCategory;

    class LogEvent 
    {
    private:
        const LogCategory& _category;
        const std::string _message;
        const LogPriority::Value _priority;
        
        const Timestamp _timestamp;
    public:
        LogEvent(const LogCategory& category, const std::string& message,
                 LogPriority::Value priority);
        ~LogEvent();

        const LogCategory& getCategory() const;
        const std::string& getMessage() const;
        
        LogPriority::Value getPriority() const;
        
        Timestamp getTimestamp() const;
    };

}

#endif