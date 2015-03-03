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
#ifndef LOG_APPENDER_H
#define LOG_APPENDER_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"
#include "CriticalSection.h"

#include "LogPriority.h"

namespace SimuTrace
{

    class LogCategory;
    class LogLayout;
    class LogEvent;

    class LogAppender
    {
        friend class LogCategory;
    private:
        DISABLE_COPY(LogAppender);

        CriticalSection _lock;
        const std::string _name;

        std::unique_ptr<LogLayout> _layout;
        bool _inherit;
    protected:
        LogAppender(const std::string& name, std::unique_ptr<LogLayout>& layout,
                    bool inherit);

        virtual void _append(const LogEvent& event) = 0;

        LogLayout& _getLayout();
    public:
        virtual ~LogAppender();

        void append(const LogEvent& event);

        const LogLayout& getLayout() const;
        void setLayout(std::unique_ptr<LogLayout>& layout);

        const std::string& getName() const;
    };

}

#endif