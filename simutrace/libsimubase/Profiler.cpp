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

#include "Profiler.h"
#include "ProfileEntry.h"

#include "Thread.h"
#include "File.h"
#include "Clock.h"
#include "Exceptions.h"
#include "CriticalSection.h"
#include "LockRegion.h"

namespace SimuTrace
{

    Profiler::Profiler(const std::string& csvFileName) :
        _file(csvFileName, File::CreateMode::CreateAlways),
        _writeThread(nullptr, *this),
        _writeInterval(0),
        _state(ProfileState::PsStop),
        _headerWritten(false),
        _context(nullptr),
        _contextOwner(true)
    {

    }

    Profiler::Profiler(const std::string& csvFileName, ProfileContext& context,
                       uint32_t writeInterval) :
        _file(csvFileName, File::CreateMode::CreateAlways),
        _writeThread(Profiler::_writeThreadMain, *this),
        _writeInterval(writeInterval),
        _state(ProfileState::PsStop),
        _headerWritten(false),
        _context(&context),
        _contextOwner(false)
    {
        ThrowOn(_writeInterval == 0, ArgumentException, "_writeInterval");
    }

    Profiler::~Profiler()
    {
        stop();
    }

    int Profiler::_writeThreadMain(Thread<Profiler&>& thread)
    {
        Profiler& profiler = thread.getArgument();

        while (profiler._state != ProfileState::PsStop)
        {
            thread.sleep(profiler._writeInterval);

            if (profiler._state == ProfileState::PsPause) {
                continue;
            }

            assert(profiler._context != nullptr);
            profiler.collect(*profiler._context);
        }

        return 0;
    }

    void Profiler::start()
    {
        LockScope(_lock);

        if (_writeInterval == 0) {
            return;
        }

        if (_state == ProfileState::PsStop) {
            _writeThread.start();
        }

        _state = ProfileState::PsRun;
    }

    void Profiler::pause()
    {
        LockScope(_lock);

        if (_writeInterval == 0) {
            return;
        }

        ThrowOn(_state == ProfileState::PsStop, InvalidOperationException);

        _state = ProfileState::PsPause;
    }

    void Profiler::stop()
    {
        LockScope(_lock);

        if ((_writeInterval == 0) || (_state == ProfileState::PsStop)) {
            return;
        }

        _writeThread.stop();
        _state = ProfileState::PsStop;

        _writeThread.waitForThread();
    }

    void Profiler::collect(ProfileContext& context)
    {
        LockScope(_lock);

        ThrowOn((_context != nullptr) && (&context != _context),
                InvalidOperationException);

        if (!_headerWritten) {
            std::stringstream stream;

            stream << "Time";

            for (auto i = 0; i < context.getCount(); ++i) {
                stream << "," << context.get(i).getName();
            }

            stream << "\n";

            std::string output = stream.str();
            _file.write(static_cast<const void*>(output.c_str()), output.size());

            _headerWritten = true;
        }

        std::stringstream stream;

        stream << Clock::getTimestamp();

        for (auto i = 0; i < context.getCount(); ++i) {
            stream << "," << context.get(i).collect();
        }

        stream << "\n";

        std::string output = stream.str();
        _file.write(static_cast<const void*>(output.c_str()), output.size());

        _file.flush();
    }

}