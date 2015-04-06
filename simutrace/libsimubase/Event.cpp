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

#include "Event.h"

#include "Utils.h"
#include "Logging.h"
#include "Exceptions.h"

namespace SimuTrace
{

    void Event::_initEvent(const char* name)
    {
    #if defined(_WIN32)
        SECURITY_ATTRIBUTES security;
        security.nLength              = sizeof(SECURITY_ATTRIBUTES);
        security.bInheritHandle       = TRUE;
        security.lpSecurityDescriptor = nullptr;

        _event = ::CreateEventA(&security, FALSE, FALSE, name);
        ThrowOn(!_event.isValid(), PlatformException);
    #else
        std::string sname;
        if (name == nullptr) {
            Guid guid;

            // MacOSX does not support unnamed semaphores and plus only
            // supports names no longer than SEM_NAME_LEN (31 chars).
            // Generate a temporary random GUID name for the event.
            generateGuid(guid);

            sname = stringFormat("simutrace.event.%s",
    #if (defined(__MACH__) && defined(__APPLE__))
                                 guidToString(guid, true).c_str());
    #else
                                 guidToString(guid).c_str());
    #endif
        } else {
            sname = std::string(name);
        }

        _event = ::sem_open(sname.c_str(), O_CREAT, S_IRWXU, 0);
        ThrowOn(_event == SEM_FAILED, PlatformException);

        if (name == nullptr) {
            ::sem_unlink(sname.c_str());
        }
    #endif
    }

    Event::Event() :
        _name(),
        _event()
    {
        _initEvent(nullptr);
    }

    Event::Event(const std::string& name) :
        _name(name),
        _event()
    {
        _initEvent(name.c_str());
    }

    Event::~Event()
    {
    #if defined(_WIN32)
    #else
        if (_event != nullptr) {
            if (!_name.empty()) {
                ::sem_unlink(_name.c_str());
            }

            _event = nullptr;
        }
    #endif
    }

    void Event::signal()
    {
    #if defined(_WIN32)
        if (!::SetEvent(_event)) {
    #else
        if (::sem_post(_event) != 0) {
    #endif
            Throw(PlatformException);
        }
    }

    void Event::wait()
    {
    #if defined(_WIN32)
        DWORD result = ::WaitForSingleObject(_event, INFINITE);
        ThrowOn(result != WAIT_OBJECT_0, PlatformException, result);
    #else
        if (::sem_wait(_event) != 0) {
            Throw(PlatformException);
        }
    #endif
    }

    bool Event::tryWait()
    {
    #if defined(_WIN32)
        if (::WaitForSingleObject(_event, 0) == WAIT_TIMEOUT) {
    #else
        if (::sem_trywait(_event) != 0) {
    #endif
            return false;
        }

        return true;
    }

    const std::string& Event::getName() const
    {
        return _name;
    }

}