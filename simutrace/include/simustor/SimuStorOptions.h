/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef SIMUSTOR_OPTIONS_H
#define SIMUSTOR_OPTIONS_H

#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "SimuBaseOptions.h"

#include <ezOptionParser.h>
#include <libconfig.h++>

namespace SimuTrace {
namespace Options
{

    //
    // ---------- Add new options for the SimuStor library here! ----------
    //
    // Important: The last option name must be the long name. Only one long
    //            name may be specified!
    //

    static inline void addSimuStorOptions(ez::ezOptionParser& options,
                                          SettingsTypeMap& typeMap)
    {

    }

}
}

#endif