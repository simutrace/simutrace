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
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Utils.h"

namespace SimuTrace
{

    namespace System {
        int getLastErrorCode();
        int getLastSocketErrorCode();

        const std::string getErrorString(int errorCode);

        void ThrowOutOfMemoryException();
    }

#ifdef _DEBUG
    class ExceptionLocation {
    public:
        int lineOfCode;
        const char* sourceFile;

    public:
        ExceptionLocation(int line, const char* file) :
            lineOfCode(line),
            sourceFile(file) { }
    };

#define LOC_ARG0 location
#define LOC_PARAM0 ExceptionLocation LOC_ARG0
#define LOC_PARAM LOC_PARAM0,
#define LOC_ARG LOC_ARG0,
#define LOC_EMPTY ExceptionLocation(0, ""),

#ifdef _MSC_VER
#define Throw(exceptionClass, ...) \
    throw exceptionClass(ExceptionLocation(__LINE__, __FILE__), __VA_ARGS__)
#else
#define Throw(exceptionClass, ...) \
    throw exceptionClass(ExceptionLocation(__LINE__, __FILE__), ##__VA_ARGS__)
#endif // _MSC_VER
#else
#define LOC_ARG0
#define LOC_PARAM0
#define LOC_PARAM
#define LOC_ARG
#define LOC_EMPTY

#define Throw(exceptionClass, ...) \
    throw exceptionClass(__VA_ARGS__)
#endif // _DEBUG

#ifdef _MSC_VER
#define ThrowOn(expr, exceptionClass, ...) \
    if (expr) { Throw(exceptionClass, __VA_ARGS__); }
#define ThrowOnNull(var, exceptionClass, ...) \
    ThrowOn((var) == NULL, exceptionClass, __VA_ARGS__)
#else
#define ThrowOn(expr, exceptionClass, ...) \
    if (expr) { Throw(exceptionClass, ##__VA_ARGS__); }
#define ThrowOnNull(var, exceptionClass, ...) \
    ThrowOn((var) == NULL, exceptionClass, ##__VA_ARGS__)
#endif

    class Exception : public std::exception
    {
    private:
        std::string _message;

    #ifdef _DEBUG
        ExceptionLocation _location;
    #endif

        ExceptionClass _errorClass;
        int _errorCode;
    public:
        Exception(LOC_PARAM const std::string& message,
                  ExceptionClass errorClass, int errorCode);
        Exception(LOC_PARAM const std::string& message);
        Exception(LOC_PARAM ExceptionClass errorClass, int errorCode);
        Exception(LOC_PARAM0);

        ~Exception() throw() {}

        const char* what() const throw() { return _message.c_str(); }
        const size_t whatLength() const throw() { return _message.size(); }

        ExceptionClass getErrorClass() const throw() { return _errorClass; }
        int getErrorCode() const throw() { return _errorCode; }
    };

    //
    // System Exceptions
    //

    class PlatformException : public Exception
    {
    public:
        PlatformException(LOC_PARAM0) :
            Exception(LOC_ARG EcPlatform, System::getLastErrorCode()) {}
        PlatformException(LOC_PARAM int error) :
            Exception(LOC_ARG EcPlatform, error) {}

        ~PlatformException() throw() {}
    };

    class OutOfMemoryException : public Exception
    {
    public:
        OutOfMemoryException() :
            Exception(LOC_EMPTY "The process failed to allocate memory.",
    #if defined(_WIN32)
                EcPlatform, ERROR_NOT_ENOUGH_MEMORY) {}
    #else
                EcPlatform, ENOMEM) { }
    #endif

        ~OutOfMemoryException() throw() {}
    };

    //
    // Network Exceptions
    //

    class SocketException : public Exception
    {
    public:
        SocketException(LOC_PARAM0) :
            Exception(LOC_ARG EcPlatformNetwork, System::getLastSocketErrorCode()) {}
        SocketException(LOC_PARAM int error) :
            Exception(LOC_ARG EcPlatformNetwork, error) {}

        ~SocketException() throw() {}
    };

    class PeerException : public Exception
    {
    public:
        PeerException(LOC_PARAM const std::string& message,
                      ExceptionClass errorClass, int errorCode) :
            Exception(LOC_ARG message, errorClass, errorCode) {}

        ~PeerException() throw() {}
    };

    class RpcMessageMalformedException : public Exception
    {
    public:
        static const int code = NetworkException::NeRpcMessageMalformedException;

        RpcMessageMalformedException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "RPC protocol violation. The message is malformed.",
                      EcNetwork, code) {}

        ~RpcMessageMalformedException() throw() {}
    };

    //
    // Runtime Exceptions
    //

    class NotImplementedException : public Exception
    {
    public:
        static const int code = RuntimeException::RteNotImplementedException;

        NotImplementedException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The requested feature is not implemented.",
                      EcRuntime, code) {}

        ~NotImplementedException() throw() {}
    };

    class NotFoundException : public Exception
    {
    public:
        static const int code = RuntimeException::RteNotFoundException;

        NotFoundException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The requested object could not be found.",
                      EcRuntime, code) {}

        ~NotFoundException() throw() {}
    };

    class NotSupportedException : public Exception
    {
    public:
        static const int code = RuntimeException::RteNotSupportedException;

        NotSupportedException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The requested operation or feature is not supported.",
                      EcRuntime, code) {}

        ~NotSupportedException() throw() {}
    };

    class InvalidOperationException : public Exception
    {
    public:
        static const int code = RuntimeException::RteInvalidOperationException;

        InvalidOperationException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The operation is not valid due to the current state "
                      "of the object.",
                      EcRuntime, code) {}

        ~InvalidOperationException() throw() {}
    };

    class OperationInProgressException : public Exception
    {
    public:
        static const int code = RuntimeException::RteOperationInProgressException;

        OperationInProgressException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "A blocking operation is still in progress. Please try "
                      "again later.", EcRuntime, code) {}

        ~OperationInProgressException() throw() {}
    };

    class TimeoutException : public Exception
    {
    public:
        static const int code = RuntimeException::RteTimeoutException;

        TimeoutException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The requested operation timed out.",
                      EcRuntime, code) {}

        ~TimeoutException() throw() {}
    };

    class ArgumentException : public Exception
    {
    public:
        static const int code = RuntimeException::RteArgumentException;

        ArgumentException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "One or more arguments are invalid. See the "
                      "function's documentation for valid parameter values.",
                      EcRuntime, code) {}
        ArgumentException(LOC_PARAM const std::string& argument) :
            Exception(LOC_ARG
                      stringFormat("Argument '%s' is not valid. See the "
                        "function's documentation for valid parameter values.",
                        argument.c_str()),
                      EcRuntime, code) {}

        ~ArgumentException() throw() {}
    };

    class ArgumentNullException : public Exception
    {
    public:
        static const int code = RuntimeException::RteArgumentNullException;

        ArgumentNullException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "One or more pointer arguments passed to a function "
                      "were NULL or empty, but are expected to point to valid "
                      "data or buffer space. See the function's documentation "
                      "for more information.",
                      EcRuntime, code) {}
        ArgumentNullException(LOC_PARAM const std::string& argument) :
            Exception(LOC_ARG
                      stringFormat("Argument '%s' was NULL or empty, but is "
                        "expected to point to valid data or buffer space. See the "
                        "function's documentation for valid parameter values.",
                        argument.c_str()),
                      EcRuntime, code) {}

        ~ArgumentNullException() throw() {}
    };

    class ArgumentOutOfBoundsException : public Exception
    {
    public:
        static const int code = RuntimeException::RteArgumentOutOfBoundsException;

        ArgumentOutOfBoundsException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The values for one or more arguments passed to a "
                      "function were out of bounds. See the functions "
                      "documentation for valid values.",
                      EcRuntime, code) {}
        ArgumentOutOfBoundsException(LOC_PARAM  const std::string& argument) :
            Exception(LOC_ARG
                      stringFormat("The value for argument '%s' was out of "
                        "bounds. See the function's documentation for valid "
                        "parameter values.", argument.c_str()),
                      EcRuntime, code) {}

        ~ArgumentOutOfBoundsException() throw() {}
    };

    class OptionException : public Exception
    {
    public:
        static const int code = RuntimeException::RteOptionException;

        OptionException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "One or more command line options are not valid.",
                      EcRuntime, 0x009) {}
        OptionException(LOC_PARAM const std::string& message) :
            Exception(LOC_ARG message, EcRuntime, code) {}

        ~OptionException() throw() {}
    };

    class ConfigurationException : public Exception
    {
    public:
        static const int code = RuntimeException::RteConfigurationException;

        ConfigurationException(LOC_PARAM0) :
            Exception(LOC_ARG
                      "The supplied configuration is not valid. See the "
                      "documentation of libconfig for more information on the "
                      "configuration format. See the sample configuration for "
                      "a list of all valid options and their default values.",
                      EcRuntime, code) {}
        ConfigurationException(LOC_PARAM const std::string& message) :
            Exception(LOC_ARG message, EcRuntime, code) {}

        ~ConfigurationException() throw() {}
    };

    class UserCallbackException : public Exception
    {
    public:
        UserCallbackException(LOC_PARAM int code) :
            Exception(LOC_ARG
                      stringFormat("A user-supplied callback raised return "
                        "an error (code: %i).", code),
                      EcUser, code) {}

        ~UserCallbackException() throw() {}
    };

}

#endif