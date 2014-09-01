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
#ifndef LOG_CATEGORY_H
#define LOG_CATEGORY_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "ReaderWriterLock.h"

#include "LogPriority.h"

namespace SimuTrace
{

    class LogAppender;
    class LogEvent;

    class LogCategory
    {
    private:
        DISABLE_COPY(LogCategory);

        mutable ReaderWriterLock _lock;

        const std::string _name;
        std::vector<std::shared_ptr<LogAppender>> _appenders;
        std::vector<bool> _enabled;

        LogCategory* _parent;
        bool _additivity;

        LogPriority::Value _priorityThreshold;

        void _log(const LogEvent& event);
        void _logUncoditionally(const LogEvent& event);
    public:
        LogCategory(const std::string& name);
        LogCategory(const std::string& name, LogCategory* parent);
        ~LogCategory();

        void registerAppender(std::shared_ptr<LogAppender>& appender);
        void enableAppender(const std::string& name, bool enable);

        void log(LogPriority::Value priority, const std::string& format, ...);

        const std::string& getName() const;
        
        bool getAdditivity() const;
        void setAdditivity(bool additivity);

        void enumerateAppenders(std::vector<std::shared_ptr<LogAppender>>& out) const;

        LogPriority::Value getPriorityThreshold() const;
        void setPriorityThreshold(LogPriority::Value priority);

        LogCategory* getParent() const;
    };

}

#endif
