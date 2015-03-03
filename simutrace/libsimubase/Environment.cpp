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
#include "SimuBaseLibraries.h"

#include "Environment.h"

namespace SimuTrace
{

    // Thread-local environment.
    __thread Environment _environment = {0};
    __thread int EnvironmentSwapper::_swapLevel = 0;

    LogCategory* Environment::getLog()
    {
        return _environment.log;
    }

    void Environment::setLog(LogCategory* log)
    {
        _environment.log = log;
    }

    libconfig::Config* Environment::getConfig()
    {
        return _environment.config;
    }

    void Environment::setConfig(libconfig::Config* config)
    {
        _environment.config = config;
    }

    void Environment::get(Environment& env)
    {
        env = _environment;
    }

    void Environment::set(const Environment* env)
    {
        if (env == nullptr) {
            memset(&_environment, 0, sizeof(Environment));
        } else {
            _environment = *env;
        }
    }

}