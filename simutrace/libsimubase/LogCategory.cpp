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

#include "LogCategory.h"

#include "Exceptions.h"
#include "Utils.h"

#include "ReaderWriterLock.h"
#include "LockRegion.h"
#include "ReaderWriterLockRegion.h"

#include "LogAppender.h"
#include "LogPriority.h"
#include "LogEvent.h"

namespace SimuTrace
{

    LogCategory::LogCategory(const std::string& name) :
        _lock(),
        _name(name),
        _parent(nullptr),
        _additivity(false),
        _priorityThreshold(LogPriority::NotSet)
    {

    }

    LogCategory::LogCategory(const std::string& name,
                             LogCategory* parent) :
        LogCategory(name)
    {
        // Inherit properties of parent.
        _parent = parent;

        for (auto i = 0; i < _parent->_appenders.size(); ++i) {
            auto appender = _parent->_appenders[i];

            if (appender->_inherit) {
                _appenders.push_back(appender);

                assert(i < _parent->_enabled.size());
                _enabled.push_back(_parent->_enabled[i]);
            }
        }

        _priorityThreshold = _parent->_priorityThreshold;
    }

    LogCategory::~LogCategory()
    {
        _appenders.clear();
        _enabled.clear();
    }

    void LogCategory::_log(const LogEvent& event)
    {
        if (event.getPriority() > _priorityThreshold) {
            return;
        }

        _logUncoditionally(event);
    }

    void LogCategory::_logUncoditionally(const LogEvent& event)
    {
        LockShared(_lock); {
            for (auto i = 0; i < _appenders.size(); ++i) {
                assert(i < _enabled.size());
                if (!_enabled[i]) {
                    continue;
                }

                assert(_appenders[i] != nullptr);
                _appenders[i]->append(event);
            }
        } Unlock();

        if ((_additivity) && (_parent != nullptr)) {
            _parent->_log(event);
        }
    }

    void LogCategory::registerAppender(std::shared_ptr<LogAppender>& appender)
    {
        LockScopeExclusive(_lock);
        ThrowOnNull(appender, ArgumentNullException, "appender");

        _appenders.push_back(appender);
        _enabled.push_back(true);
    }

    void LogCategory::enableAppender(const std::string& name, bool enable)
    {
        LockScopeShared(_lock);
        ThrowOn(name.empty(), ArgumentNullException, "name");

        for (auto i = 0; i < _appenders.size(); ++i) {
            if (_appenders[i]->getName().compare(name) == 0) {
                assert(i < _enabled.size());

                _enabled[i] = enable;
                return;
            }
        }

        Throw(NotFoundException);
    }

    void LogCategory::log(LogPriority::Value priority,
                          const std::string format, ...)
    {
        if (priority > _priorityThreshold) {
            return;
        }

        va_list va;
        std::string message;

        va_start(va, format);
        try {
            message = stringFormatVa(format, va);
            va_end(va);
        } catch (...) {
            va_end(va);

            throw;
        }

        LogEvent event(*this, message, priority);
        _logUncoditionally(event);
    }

    const std::string& LogCategory::getName() const
    {
        return _name;
    }

    bool LogCategory::getAdditivity() const
    {
        return _additivity;
    }

    void LogCategory::setAdditivity(bool additivity)
    {
        _additivity = additivity;
    }

    void LogCategory::enumerateAppenders(
        std::vector<std::shared_ptr<LogAppender>>& out) const
    {
        LockScopeShared(_lock);

        out.assign(_appenders.begin(), _appenders.end());
    }

    LogPriority::Value LogCategory::getPriorityThreshold() const
    {
        return _priorityThreshold;
    }

    void LogCategory::setPriorityThreshold(LogPriority::Value priority)
    {
        _priorityThreshold = priority;
    }

    LogCategory* LogCategory::getParent() const
    {
        return _parent;
    }

}
