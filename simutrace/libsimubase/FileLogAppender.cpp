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

#include "FileLogAppender.h"

#include "Exceptions.h"
#include "Utils.h"

#include "LogAppender.h"
#include "LogLayout.h"
#include "LogEvent.h"

namespace SimuTrace
{

    FileLogAppender::FileLogAppender(const std::string& path,
                                     std::unique_ptr<LogLayout>& layout,
                                     bool inherit) :
        LogAppender(path, layout, inherit),
        _file(path, File::CreateMode::OpenAlways)
    {

    }

    void FileLogAppender::_append(const LogEvent& event)
    {
        std::string line = _getLayout().format(event);

        _file.write(static_cast<const void*>(line.c_str()), line.length());
        _file.flush();
    }

}