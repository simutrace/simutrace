/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef STORAGE_SERVER_OPTIONS_H
#define STORAGE_SERVER_OPTIONS_H

#include "SimuStor.h"

#include "SimuBaseOptions.h"
#include "SimuStorOptions.h"

#include "Version.h"

#include <ezOptionParser.h>
#include <libconfig.h++>

namespace SimuTrace {
namespace Options
{

    //
    // ----------- Add new options for the storage server here! -----------
    //
    // Important: The last option name must be the long name. Only one long
    //            name may be specified!
    //

    static inline void addStorageServerOptions(ez::ezOptionParser& options,
                                               SettingsTypeMap& typeMap)
    {
        options.add("",
                    false,
                    0,
                    0,
                    "Display usage information.",
                    OPT_SHORT_PREFIX "h",
                    OPT_LONG_PREFIX  "help");

        typeMap["server.configuration"] = libconfig::Setting::Type::TypeString;
        options.add("",
                    false,
                    1,
                    0,
                    "Path to configuration file.",
                    OPT_SHORT_PREFIX "c",
                    OPT_LONG_PREFIX  "server.configuration");

        //
        // Logging and Verbosity
        //

        typeMap["server.quiet"] = libconfig::Setting::Type::TypeBoolean;
        options.add("",
                    false,
                    0,
                    0,
                    "If set, the server won't print output to stdout.",
                    OPT_SHORT_PREFIX "q",
                    OPT_LONG_PREFIX "server.quiet");

        typeMap["server.loglevel"] = libconfig::Setting::Type::TypeInt;
        options.add("4", // Debug level. Will be information in release build.
                    false,
                    1,
                    0,
                    "The detail level of logging. Value in the range [0;6], "
                    "with higher values delivering more details.",
                    OPT_SHORT_PREFIX "v",
                    OPT_LONG_PREFIX "server.loglevel");

        typeMap["server.logfile"] = libconfig::Setting::Type::TypeString;
        options.add("",
                    false,
                    1,
                    0,
                    "Path of the (optional) log file.",
                    OPT_SHORT_PREFIX "l",
                    OPT_LONG_PREFIX "server.logfile");

        typeMap["server.logNoColor"] = libconfig::Setting::Type::TypeBoolean;
        options.add("",
                    false,
                    0,
                    0,
                    "If set, terminal log output will not be colored.",
                    OPT_LONG_PREFIX "server.logNoColor");

        //
        // Environment
        //

        typeMap["server.workspace"] = libconfig::Setting::Type::TypeString;
        options.add("",
                    false,
                    1,
                    0,
                    "Path to workspace. Will be used as base for configuration"
                    " files and the trace storage location.",
                    OPT_SHORT_PREFIX "w",
                    OPT_LONG_PREFIX  "server.workspace");

        //
        // Services
        //

        typeMap["server.bindings"] = libconfig::Setting::Type::TypeString;
        options.add("local:/tmp/.simutrace",
                    false,
                    1,
                    0,
                    "The port bindings that the server will establish. "
                    "Multiple bindings can be separated by ','. "
                    "Example: 'local:.simutrace,socket:*:5341'",
                    OPT_SHORT_PREFIX "b",
                    OPT_LONG_PREFIX "server.bindings");

        //
        // Memory Management
        //

        typeMap["server.memmgmt.poolSize"] = libconfig::Setting::Type::TypeInt;
        options.add("4096", //SIMUTRACE_SERVER_MEMMGMT_RECOMMENDED_POOLSIZE
                    false,
                    1,
                    0,
                    "The size of the server internal memory pool to process "
                    "stream segments in MiB. ",
                    OPT_LONG_PREFIX "server.memmgmt.poolSize");

        typeMap["server.memmgmt.disableCache"] = libconfig::Setting::Type::TypeBoolean;
        options.add("",
                    false,
                    0,
                    0,
                    "Disables segment caching. With active cache segments are "
                    "kept in memory until memory pressure requires cached "
                    "segments to be evicted.",
                    OPT_LONG_PREFIX "server.memmgmt.disableCache");

        typeMap["server.memmgmt.retryCount"] = libconfig::Setting::Type::TypeInt;
        options.add("100",
                    false,
                    1,
                    0,
                    "The number of times the memory management tries to "
                    "allocate a segment from a stream buffer on low memory "
                    "condition. See also: server.memmgmt.retrySleep.",
                    OPT_LONG_PREFIX "server.memmgmt.retryCount");

        typeMap["server.memmgmt.retrySleep"] = libconfig::Setting::Type::TypeInt;
        options.add("5000",
                    false,
                    1,
                    0,
                    "The number of milliseconds the memory management waits "
                    "between successive allocation retries on low memory "
                    "condition. See also: server.memmgmt.retryCount.",
                    OPT_LONG_PREFIX "server.memmgmt.retrySleep");

        typeMap["server.memmgmt.readAhead"] = libconfig::Setting::Type::TypeInt;
        options.add("2",
                    false,
                    1,
                    0,
                    "The number of segments that should be read ahead for "
                    "sequential scan stream accesses.",
                    OPT_LONG_PREFIX "server.memmgmt.readAhead");

        //
        // Session Management
        //

        typeMap["server.session.closeTimeout"] = libconfig::Setting::Type::TypeInt;
        options.add("10000",
                    false,
                    1,
                    0,
                    "The number of milliseconds the session manager waits "
                    "for each worker thread to gracefully close.",
                    OPT_LONG_PREFIX "server.session.closeTimeout");


        //
        // Worker Pools
        //

        typeMap["server.workerpool.size"] = libconfig::Setting::Type::TypeInt;
        options.add("0",
                    false,
                    1,
                    0,
                    "The number of worker threads used to process stream "
                    "data. 0 creates one worker for each CPU core (logical "
                    "or physical) in the system.",
                    OPT_LONG_PREFIX "server.workerpool.size");

        typeMap["server.requestworkerpool.size"] = libconfig::Setting::Type::TypeInt;
        options.add("0",
                    false,
                    1,
                    0,
                    "The number of worker threads used to process incoming "
                    "requests. 0 creates one worker for each CPU core "
                    "(logical or physical) in the system.",
                    OPT_LONG_PREFIX "server.requestworkerpool.size");

    };

}
}

#endif