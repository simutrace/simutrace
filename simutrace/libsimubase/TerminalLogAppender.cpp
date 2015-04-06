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

#include "TerminalLogAppender.h"

#include "Exceptions.h"
#include "Utils.h"

#include "LogAppender.h"
#include "LogLayout.h"
#include "LogPriority.h"
#include "LogEvent.h"

namespace SimuTrace
{

    TerminalLogAppender::TerminalLogAppender(const std::string& name,
                                             std::unique_ptr<LogLayout>& layout,
                                             bool inherit) :
        LogAppender(name, layout, inherit),
    #if defined(_WIN32)
        _consoleHandle(),
    #else
    #endif
        _enableColor(false)
    {
    #if defined(_WIN32)
        _consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);

        // If the application does not possess a console, we deactivate color
        _enableColor = _consoleHandle.isValid();
    #else
        if (::isatty(STDOUT_FILENO)) {
            _enableColor = true;
        }
    #endif

        // Create default color palette
        _colorMap[LogPriority::Fatal]       = Color::Red;
        _colorMap[LogPriority::Error]       = Color::Red    | Color::Intensity;
        _colorMap[LogPriority::Warning]     = Color::Yellow | Color::Intensity;
        _colorMap[LogPriority::Information] = Color::White;
        _colorMap[LogPriority::Debug]       = Color::Green  | Color::Intensity;
        _colorMap[LogPriority::MemDebug]    = Color::Green  | Color::Intensity;
        _colorMap[LogPriority::RpcDebug]    = Color::Green;
    }

    TerminalLogAppender::~TerminalLogAppender()
    {

    }

    void TerminalLogAppender::_append(const LogEvent& event)
    {
        if ((_enableColor) && (_colorMap.size() > 0)) {
            // Get the color for the event priority from the color map and
            // set it.
            auto it = _colorMap.find(event.getPriority());
            if (it != _colorMap.end()) {
                _setTerminalColor(it->second);
            }

            std::cout << _getLayout().format(event);

            // Reset the color to white so that any subsequently written text
            // uses default colors if not specified otherwise.
            _setColorOff();
        } else {
            std::cout << _getLayout().format(event);
        }
    }

    void TerminalLogAppender::_setTerminalColor(uint16_t color)
    {
    #if defined(_WIN32)
        ::SetConsoleTextAttribute(_consoleHandle, color);
    #else
        // We first have to convert the color value to the ANSI escape code.
        // See http://en.wikipedia.org/wiki/ANSI_escape_code for an overview
        // of codes and the respective color table.
        const uint32_t mask = (Color::Red | Color::Green | Color::Blue);
        uint32_t cvalue = (color & mask) + 30;

        std::cout << stringFormat("\x1b[%d%sm", cvalue,
                                  ((color & Color::Intensity) != 0) ?
                                  ";1" : "");
    #endif
    }

    void TerminalLogAppender::_setColorOff()
    {
    #if defined(_WIN32)
        _setTerminalColor(Color::White);
    #else
        std::cout << "\x1b[0m";
    #endif
    }

    void TerminalLogAppender::setColor(LogPriority::Value priority,
                                       uint16_t color)
    {
        ThrowOn(color > Color::White, ArgumentOutOfBoundsException);

        _colorMap[priority] = color;
    }

    bool TerminalLogAppender::getEnableColor() const
    {
        return _enableColor;
    }

    void TerminalLogAppender::setEnableColor(bool enable)
    {
        _enableColor = enable;
    }

}