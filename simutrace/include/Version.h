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
#ifndef SIMUTRACE_VERSION_H
#define SIMUTRACE_VERSION_H

/* --------------------------- Software  Version --------------------------- */

/* This version is used throughout Simutrace to denote the current major and 
   minor versions of the whole software package.

   Major Version: Increment the major version if dramatic changes have been 
    made or a big new feature has been incorporated. Reset the minor version
    to 0 for each new major version.

   Minor Version: The minor version is incremented if new features have been
    integrated oder the behavior of components has changed more than a bug
    fix would require. */

#define SIMUTRACE_VERSION_MAJOR 3
#define SIMUTRACE_VERSION_MINOR 0

/* Increment the revision number for bug fixes in a minor version. It does not
   matter in which component (even 3rd party lib) the bug was, as long as this
   fix will trigger a new public release. For feature additions, see major/
   minor versions.

   Reset the revision number to 0 for each new minor version. */

#define SIMUTRACE_REVISION 0


#define SIMUTRACE_VER_MAJOR(ver) (((ver) >> 8) & 0xFF)
#define SIMUTRACE_VER_MINOR(ver) ((ver) & 0xFF)
#define SIMUTRACE_VER_REVISION(ver) ((ver) >> 16)
#define SIMUTRACE_VER(major, minor, revision) \
    (((revision) << 16) | (((major) & 0xFF) << 8) | ((minor) & 0xFF))

#define SIMUTRACE_VERSION \
    SIMUTRACE_VER(SIMUTRACE_VERSION_MAJOR, \
                  SIMUTRACE_VERSION_MINOR, \
                  SIMUTRACE_REVISION)


/* ---------------------- Compile-time Configurations ---------------------- */

/* The size for each segment used in all stream buffers created on the server-
   and client-side. 64 MiB provides a good balance between compression
   effectiveness, decompression speed and segment submission rate. Changing the
   value will break compatibility with traces generated with a different
   segment size, that is, you will no longer be able to open these traces! */
#define SIMUTRACE_MEMMGMT_SEGMENT_SIZE 64
#define SIMUTRACE_MEMMGMT_MAX_NUM_SEGMENTS_PER_BUFFER 1024

/* The following defines provide per-object limits for the number of instances
   the store will create before throwing an InvalidOperationException. The
   limits are not architecture-specific but merely serve as a protection
   against ill-behaving clients. You may increase these limits if you need
   to allocate more objects. However, remind that you cannot open more streams
   simultaneously than the stream buffer provides segments. */
#define SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS 1
#define SIMUTRACE_STORE_MAX_NUM_STREAMS       1024
#define SIMUTRACE_STORE_MAX_NUM_DATAPOOLS     1024

/* Profiling Support --------------- */

/* Enables general support for profiling code within Simutrace. To enable
   profiling, uncomment the respective define. */
//#define SIMUTRACE_PROFILING_ENABLE
#ifdef SIMUTRACE_PROFILING_ENABLE

/* Enables profiling of VPC4 compression code. This will output predictor
   stats during compression. */
#define SIMUTRACE_PROFILING_SIMTRACE3_GENERIC_COMPRESSION_ENABLE
#define SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE

#endif


/* -------------- Minimum and Recommended Server Requirements -------------- */

#define SIMUTRACE_SERVER_MEMMGMT_MINIMUM_POOLSIZE 512
#define SIMUTRACE_SERVER_MEMMGMT_RECOMMENDED_POOLSIZE (2048*2)

#define SIMUTRACE_CLIENT_MEMMGMT_MINIMUM_POOLSIZE 512
#define SIMUTRACE_CLIENT_MEMMGMT_RECOMMENDED_POOLSIZE (2048*2)

#endif