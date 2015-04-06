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
#ifndef TERMINAL_LOG_APPENDER_H
#define TERMINAL_LOG_APPENDER_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "LogAppender.h"
#include "LogPriority.h"

#include "SafeHandle.h"

namespace SimuTrace
{

    class LogEvent;

    class TerminalLogAppender :
        public LogAppender
    {
    private:
        // Define common console colors. Intensity comes from the Windows
        // console that allow to choose between 127er and 255er color
        // intensities.
        enum Color {
            Black       = 0x0000,

        #if defined(_WIN32)
            Red         = FOREGROUND_RED,
            Green       = FOREGROUND_GREEN,
            Blue        = FOREGROUND_BLUE,
            Intensity   = FOREGROUND_INTENSITY,
        #else
            Red         = 0x0001,
            Green       = 0x0002,
            Blue        = 0x0004,
            Intensity   = 0x0008,
        #endif

            Yellow      = Red | Green,
            Magenta     = Red | Blue,
            Cyan        = Green | Blue,
            White       = Blue | Green | Red | Intensity
        };

    private:
        DISABLE_COPY(TerminalLogAppender);

    #if defined(_WIN32)
        SafeHandle _consoleHandle;
    #else
    #endif

        bool _enableColor;
        std::map<LogPriority::Value, uint16_t> _colorMap;

        virtual void _append(const LogEvent& event) override;

        void _setTerminalColor(uint16_t color);
        void _setColorOff();
    public:
        TerminalLogAppender(const std::string& name,
                            std::unique_ptr<LogLayout>& layout, bool inherit);
        virtual ~TerminalLogAppender();

        void setColor(LogPriority::Value priority, uint16_t color);

        bool getEnableColor() const;
        void setEnableColor(bool enable);
    };

}

#endif