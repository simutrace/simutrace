/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
 *                         http://simutrace.org
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
/*! \file */
#pragma once
#ifndef SIMUBASE_OPTIONS_H
#define SIMUBASE_OPTIONS_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"
#include "SimuBaseLibraries.h"

namespace SimuTrace {
namespace Options
{

    typedef std::map<std::string, libconfig::Setting::Type> SettingsTypeMap;

    //
    // ---------- Add new options for the SimuBase library here! ----------
    //
    // Important: The last option name must be the long name. Only one long
    //            name may be specified!
    //

    static inline void addSimuBaseOptions(ez::ezOptionParser& options,
                                          SettingsTypeMap& typeMap)
    {
        //
        // Local Channel
        //

        typeMap["network.local.retryCount"] = libconfig::Setting::Type::TypeInt;
        options.add("10",
                    false,
                    1,
                    0,
                    "The number of times a local channel tries to connect to "
                    "the specified server port if the port is busy."
                    "See also: network.local.retrySleep.",
                    OPT_LONG_PREFIX "network.local.retryCount");

        typeMap["network.local.retrySleep"] = libconfig::Setting::Type::TypeInt;
        options.add("500",
                    false,
                    1,
                    0,
                    "The number of milliseconds a local channel waits "
                    "between successive connection retries if the server port "
                    "is busy. See also: network.local.retryCount.",
                    OPT_LONG_PREFIX "network.local.retrySleep");
    }

}
}

#endif