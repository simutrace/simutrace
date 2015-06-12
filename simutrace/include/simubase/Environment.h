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
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"
#include "SimuBaseLibraries.h"

namespace SimuTrace
{

    class LogCategory;

    struct Environment
    {
        LogCategory* log;
        libconfig::Config* config;

        static LogCategory* getLog();
        static void setLog(LogCategory* log);

        static libconfig::Config* getConfig();
        static void setConfig(libconfig::Config* config);

        static void get(Environment& env);
        static void set(const Environment* env);
    };

    class EnvironmentSwapper
    {
    private:
        Environment _safed;

        static __thread int _swapLevel;
    public:
        EnvironmentSwapper(const Environment* env)
        {
            Environment::get(_safed);
            Environment::set(env);

            _swapLevel++;
        }

        ~EnvironmentSwapper()
        {
            if (_swapLevel > 0) {
                Environment::set(&_safed);

                _swapLevel--;
            }
        }

        static void overrideSwap()
        {
            _swapLevel = 0;
        }
    };

#define _SwapEnvironment2(env, name) \
    EnvironmentSwapper env_swapper_##name(env)

#define _SwapEnvironment(env, name) \
    _SwapEnvironment2(env, name)

#define SwapEnvironment(env) \
    _SwapEnvironment(env, __LINE__)

}

#endif