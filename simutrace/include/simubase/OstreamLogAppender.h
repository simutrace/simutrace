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
#ifndef O_STREAM_LOG_APPENDER_H
#define O_STREAM_LOG_APPENDER_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "LogAppender.h"

namespace SimuTrace
{

    class LogEvent;

    class OstreamLogAppender :
        public LogAppender
    {
    private:
        DISABLE_COPY(OstreamLogAppender);

        std::ostream& _stream;

        virtual void _append(const LogEvent& event);
    public:
        OstreamLogAppender(const std::string& name, std::ostream& stream, 
                           std::unique_ptr<LogLayout>& layout, bool inherit);
        virtual ~OstreamLogAppender();

    };

}

#endif