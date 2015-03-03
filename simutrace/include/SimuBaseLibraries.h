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
#ifndef SIMUBASE_LIBRARIES_H
#define SIMUBASE_LIBRARIES_H

/* Includes for 3rd Party Libraries */

#ifdef _MSC_VER
#pragma warning(push)
/* libconfig */
#pragma warning(disable : 4290) /* exception specification ignored */

/* ezOptionParser */
#pragma warning(disable : 4267) /* possible loss of data */
#pragma warning(disable : 4244) /* possible loss of data */
#pragma warning(disable : 4996) /* unsafe printf */
#pragma warning(disable : 4101) /* unreferenced local variable */
#pragma warning(disable : 4018) /* signed/unsigned mismatch */
#endif

#include <libconfig.h++>
#include <ezOptionParser.h>

#define OPT_SHORT_PREFIX "-"
#define OPT_LONG_PREFIX "--"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif