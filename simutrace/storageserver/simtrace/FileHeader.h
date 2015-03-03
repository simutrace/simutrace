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
#ifndef FILE_HEADER_H
#define FILE_HEADER_H

#include "SimuStor.h"
#include "Simtrace3Format.h"

namespace SimuTrace {
namespace Simtrace
{

    /* Master Header */

#define SIMTRACE_DEFAULT_SIGNATURE 0x65636172746d6953 /* 'Simtrace' */
#define SIMTRACE_HEADER_RESERVED_SPACE 0x1000

#define SIMTRACE_DEFAULT_VERSION SIMTRACE_VERSION3

    struct SimtraceMasterHeader {
        union {
            char signature[8];
            uint64_t signatureValue;
        };

        uint16_t majorVersion;
        uint16_t minorVersion;
    };

    inline bool isSimtraceStore(SimtraceMasterHeader& header)
    {
        return (header.signatureValue == SIMTRACE_DEFAULT_SIGNATURE);
    }

    struct SimtraceFileHeader {
        SimtraceMasterHeader master;

        union {
            SimtraceV3Header v3;
        };
    };

}
}

#endif