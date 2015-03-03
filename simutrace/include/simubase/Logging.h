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
#ifndef LOGGING_H
#define LOGGING_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Utils.h"
#include "Environment.h"
#include "LogCategory.h"
#include "LogPriority.h"

namespace SimuTrace
{

#ifdef _MSC_VER
#define Log(level, msg, ...)                                \
    do {                                                    \
        LogCategory* log = Environment::getLog();           \
        if (log != nullptr) {                               \
            log->log((level), (msg), __VA_ARGS__);          \
        } else {                                            \
            std::cout << "[PreLog] " +                      \
                stringFormat((msg), __VA_ARGS__) + "\n";    \
        }                                                   \
    } while (false)

#define LogInfo(msg, ...) \
    Log(LogPriority::Information, msg, __VA_ARGS__)
#define LogWarn(msg, ...) \
    Log(LogPriority::Warning, msg, __VA_ARGS__)
#define LogError(msg, ...) \
    Log(LogPriority::Error, msg, __VA_ARGS__)
#define LogFatal(msg, ...) \
    Log(LogPriority::Fatal, msg, __VA_ARGS__)

#ifdef _DEBUG
#define LogDebug(msg, ...) \
    Log(LogPriority::Debug, msg, __VA_ARGS__)
#define LogMem(msg, ...) \
    Log(LogPriority::MemDebug, msg, __VA_ARGS__)
#define LogRpc(msg, ...) \
    Log(LogPriority::RpcDebug, msg, __VA_ARGS__)
#else
#define LogDebug(msg, ...)
#define LogMem(msg, ...)
#define LogRpc(msg, ...)
#endif

#else
#define Log(level, msg, ...)                                \
    do {                                                    \
        LogCategory* log = Environment::getLog();           \
        if (log != nullptr) {                                  \
            log->log((level), (msg), ##__VA_ARGS__);        \
        } else {                                            \
            std::cout << "[PreLog] " +                      \
                stringFormat((msg), ##__VA_ARGS__) + "\n";  \
        }                                                   \
    } while (false)

#define LogInfo(msg, ...) \
    Log(LogPriority::Information, msg, ##__VA_ARGS__)
#define LogWarn(msg, ...) \
    Log(LogPriority::Warning, msg, ##__VA_ARGS__)
#define LogError(msg, ...) \
    Log(LogPriority::Error, msg, ##__VA_ARGS__)
#define LogFatal(msg, ...) \
    Log(LogPriority::Fatal, msg, ##__VA_ARGS__)

#ifdef _DEBUG
#define LogDebug(msg, ...) \
    Log(LogPriority::Debug, msg, ##__VA_ARGS__)
#define LogMem(msg, ...) \
    Log(LogPriority::MemDebug, msg, ##__VA_ARGS__)
#define LogRpc(msg, ...) \
    Log(LogPriority::RpcDebug, msg, ##__VA_ARGS__)
#else
#define LogDebug(msg, ...)
#define LogMem(msg, ...)
#define LogRpc(msg, ...)
#endif

#endif // _MSC_VER

}

#endif