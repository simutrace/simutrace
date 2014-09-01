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

#include "OstreamLogAppender.h"

#include "Exceptions.h"

#include "LogAppender.h"
#include "LogLayout.h"

namespace SimuTrace
{

    OstreamLogAppender::OstreamLogAppender(const std::string& name,
                                           std::ostream& stream,
                                           std::unique_ptr<LogLayout>& layout,
                                           bool inherit) :
        LogAppender(name, layout, inherit),
        _stream(stream)
    {

    }

    OstreamLogAppender::~OstreamLogAppender()
    {
        // Do not close the stream. This would bring us into trouble with
        // the standard streams!
    }

    void OstreamLogAppender::_append(const LogEvent& event)
    {
        _stream << _getLayout().format(event);
        ThrowOn(!_stream.good(), Exception, 
                "Failed to log to stream. Stream state is bad.");
    }

}