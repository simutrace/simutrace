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
 * Simutrace Client Library (libsimutrace) is part of Simutrace.
 *
 * libsimutrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
 */
/*! \file */
#pragma once
#ifndef SIMUTRACE_TYPES_H
#define SIMUTRACE_TYPES_H

#include "SimuStorTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

    /* General data structures */

    /*! \brief Extended exception information.
     *
     *  Contains extended information on exceptions. The information can be
     *  retrieved by calling StGetLastError().
     *
     *  \since 3.0
     *
     *  \see StGetLastError()
     */
    typedef struct _ExceptionInformation {
        ExceptionClass type;    /*!< \brief Class of the exception */
        ExceptionSite site;     /*!< \brief Site, where the exception
                                     occurred */
        int code;               /*!< \brief Error code. For platform exceptions
                                     this is the system error code. */
        uint32_t reserved;      /*!< \internal \brief Internal. Do not use. */

        const char* message;    /*!< \brief Error message, describing the
                                     exception */
    } ExceptionInformation;


    /* Stream related data structures */

    /*! \internal */
    typedef struct _StreamStateDescriptor {
        _bool isReadOnly;
        StreamAccessFlags accessFlags;

        void* stream;
        uint32_t entrySize;

        SegmentId segment;
        StreamSegmentId sequenceNumber;

        SegmentControlElement* control;

        byte* segmentStart;
        byte* segmentEnd;
        byte* entry;
    } StreamStateDescriptor;

    /*! \brief Handle to a stream.
     *
     *  Represents a handle to a stream which can be passed to routines of the
     *  stream API to read or write entries. A handle may either be for read or
     *  write access only.
     *
     *  \remarks Do not access the members of a stream handle directly.
     *           Instead assume it to be opaque and use the stream API to
     *           work with streams and entries.
     *
     *  \since 3.0
     *
     *  \see StStreamAppend()
     *  \see StStreamOpen()
     *  \see StStreamClose()
     *  \see StGetNextEntryFast()
     *  \see StGetPreviousEntryFast()
     *  \see StWriteVariableData()
     *  \see StReadVariableData()
     */
    typedef StreamStateDescriptor* StreamHandle;

#ifdef __cplusplus
}
}
#endif

#endif