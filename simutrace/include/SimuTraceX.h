/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
 *                         http://simutrace.org
 *
 * Simutrace Extensions Library (libsimutraceX) is part of Simutrace.
 *
 * libsimutraceX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutraceX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutraceX. If not, see <http://www.gnu.org/licenses/>.
 */
/*! \file */
#pragma once
#ifndef SIMUTRACEX_H
#define SIMUTRACEX_H

#include "SimuTrace.h"
#include "SimuTraceXTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

#define SIMUTRACEX_API SIMUTRACE_API

    /* Helpers */

    /*! \brief Returns the id of the stream with the specified name
     *
     *  This method searches the list of available streams for a stream with
     *  the given name and returns its id and detailed properties.
     *
     *  \param session The id of the session in which to search for the stream.
     *
     *  \param name The name of the stream to search for.
     *
     *  \param info Optional pointer to a #StreamQueryInformation structure to
     *              receive detailed stream information.
     *
     *  \returns The id of the stream with the given name on success,
     *           \c INVALID_STREAM_ID, otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \since 3.2
     */
    SIMUTRACEX_API
    StreamId StXStreamFindByName(SessionId session, const char* name,
                                 StreamQueryInformation* info);


    /* Stream Multiplexer */

    /*! \brief Creates a new stream multiplexer
     *
     *  A stream multiplexer chooses the next entry from a set of input
     *  streams based on a defined rule. The multiplexer is accessed like a
     *  regular stream.
     *
     *  \param session The session for which the multiplexer should be created.
     *
     *  \param name A friendly name for the multiplexer.
     *
     *  \param rule The multiplexing rule, which determines the next entry.
     *
     *  \param flags A set of flags to customize multiplexing behavior.
     *
     *  \param inputStreams Pointer to an array of streams that should be
     *                      used as input for the multiplexer.
     *
     *  \param count The number of elements in the \p inputStreams array.
     *
     *  \returns The id the dynamic stream representing the created multiplexer.
     *           Read entries from the multiplexer by reading entries from the
     *           stream with the returned id.
     *
     *  \remarks The multiplexer will only be visible in the same session
     *           for the client that created the multiplexer.
     *
     *  \since 3.2
     *
     *  \see StStreamOpen()
     *  \see StGetNextEntry()
     */
    SIMUTRACEX_API
    StreamId StXMultiplexerCreate(SessionId session, const char* name,
                                  MultiplexingRule rule, MultiplexerFlags flags,
                                  StreamId* inputStreams, uint32_t count);

#ifdef __cplusplus
}
}
#endif

#endif