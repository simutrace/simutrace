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

#include "LogEvent.h"

#include "Clock.h"

#include "LogPriority.h"

namespace SimuTrace
{

    LogEvent::LogEvent(const LogCategory& category, const std::string& message,
                       LogPriority::Value priority) :
        _category(category),
        _message(message),
        _priority(priority),
        _timestamp(Clock::timeToTimestamp(Clock::getTime()))
    {
    
    }

    LogEvent::~LogEvent()
    {

    }

    const LogCategory& LogEvent::getCategory() const
    {
        return _category;
    }

    const std::string& LogEvent::getMessage() const
    {
        return _message;
    }

    LogPriority::Value LogEvent::getPriority() const
    {
        return _priority;
    }

    Timestamp LogEvent::getTimestamp() const
    {
        return _timestamp;
    }

}