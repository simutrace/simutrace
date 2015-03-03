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

#include "LogAppender.h"

#include "LockRegion.h"

#include "LogEvent.h"
#include "SimpleLogLayout.h"

namespace SimuTrace
{

    LogAppender::LogAppender(const std::string& name,
                             std::unique_ptr<LogLayout>& layout, bool inherit) :
        _lock(),
        _name(name),
        _layout(nullptr),
        _inherit(inherit)
    {
        setLayout(layout);
    }

    LogAppender::~LogAppender()
    {

    }

    LogLayout& LogAppender::_getLayout()
    {
        return *_layout;
    }

    void LogAppender::append(const LogEvent& event)
    {
        LockScope(_lock);
        _append(event);
    }

    const LogLayout& LogAppender::getLayout() const
    {
        return *_layout;
    }

    void LogAppender::setLayout(std::unique_ptr<LogLayout>& layout)
    {
        if (layout == nullptr) {
            _layout = std::unique_ptr<LogLayout>(new SimpleLogLayout());
        } else {
            _layout = std::move(layout);
        }
    }

    const std::string& LogAppender::getName() const
    {
        return _name;
    }

}