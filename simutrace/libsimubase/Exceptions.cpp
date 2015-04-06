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

#include "Exceptions.h"

#include "Utils.h"

namespace SimuTrace {
namespace System
{

    int getLastErrorCode()
    {
    #if defined(_WIN32)
        return ::GetLastError();
    #else
        return errno;
    #endif
    }

    int getLastSocketErrorCode()
    {
    #if defined(_WIN32)
        return ::WSAGetLastError();
    #else
        return getLastErrorCode();
    #endif
    }

    const std::string getErrorString(const int errorCode)
    {
        std::string errorString;
        char* msg = nullptr;

    #if defined(_WIN32)
        DWORD n = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   nullptr,
                                   errorCode,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPSTR)&msg, 0, nullptr);

        if (n > 0) {
            assert(msg != nullptr);

            errorString = std::string(msg);
            ::LocalFree(msg);
        } else {
    #else
        msg = ::strerror(errorCode);

        if (msg != nullptr) {
            errorString = std::string(msg);
        } else {
    #endif
            errorString = "Unknown Error";
        }

        return errorString;
    }

    OutOfMemoryException _outofMemoryException;

    void ThrowOutOfMemoryException()
    {
        throw _outofMemoryException;
    }

}
    //
    // Exception
    //

    Exception::Exception(LOC_PARAM const std::string& message,
        ExceptionClass errorClass, int errorCode) :
    #ifdef _DEBUG
        _location(LOC_ARG0),
    #endif
        _errorClass(errorClass),
        _errorCode(errorCode)
    {
    #ifdef _DEBUG
        _message = stringFormat("%s [Line: %d, File: %s]",
            message.c_str(), _location.lineOfCode, _location.sourceFile);
    #else
        _message = stringFormat("%s", message.c_str());
    #endif
    }

    Exception::Exception(LOC_PARAM const std::string& message) :
        Exception(LOC_ARG message, ExceptionClass::EcUnknown, 0) { }

    Exception::Exception(LOC_PARAM ExceptionClass errorClass, int errorCode) :
        Exception(LOC_ARG System::getErrorString(errorCode),
                  errorClass, errorCode) { }

    Exception::Exception(LOC_PARAM0) :
        Exception(LOC_ARG "Unknown error", ExceptionClass::EcUnknown, 0) { }

}