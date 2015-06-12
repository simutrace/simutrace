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
#ifndef SIMUTRACEX_TYPES_H
#define SIMUTRACEX_TYPES_H

#include "SimuTraceTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

    /*! \brief Describes the logic for choosing the next entry in a multiplexer
     *
     *  A stream multiplexer chooses the next entry from a set of input
     *  streams. The rule used in the selection process is defined with the
     *  multiplexing rule.
     *
     *  \since 3.2
     */
    typedef enum _MultiplexingRule {
        MxrRoundRobin = 0x00,   /*!< \brief Entries are selected from the
                                     input streams in a round robin fashion */
        MxrCycleCount = 0x01    /*!< \brief The multiplexer selects the
                                     entry with the smallest next cycle count.
                                     For this rule, all input streams must be
                                     temporally ordered. */
    } MultiplexingRule;


    /*! \brief Basic multiplexer flags
     *
     *  Flags for the customization of stream multiplexers.
     *
     *  \since 3.2
     */
    typedef enum _MultiplexerFlags {
        MxfNone     = 0x00,     /*!< \brief No special configuration */
        MxfIndirect = 0x01      /*!< \brief The multiplexer will output
                                     entries of type #MultiplexerEntry. This
                                     flag is required if the input streams
                                     have different types. */
    } MultiplexerFlags;

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

    /*! \brief Entry returned by multiplexer
     *
     *  A multiplexer returns entries from a set of streams according to a
     *  certain rule. If the #MxfIndirect flag is specified (required if the
     *  streams are not of the same type), the multiplexer outputs this type
     *  of entry. It is independent of the types configured for the input
     *  streams and in addition provides the index of the stream from which
     *  the entry originates.
     *
     *  \remarks If all input streams are temporally ordered, the output stream
     *           will also be temporally ordered.
     *
     *  \since 3.2
     */
    typedef struct _MultiplexerEntry {
        CycleCount cycleCount;  /*!< \brief Cycle count taken from the current
                                     entry. Only valid if all input streams
                                     are temporally ordered */
        StreamId streamId;      /*!< \brief Id of the stream from which the
                                     entry originates */
        uint32_t streamIdx;     /*!< \brief Index of the stream in the list of
                                     input streams used to create the
                                     multiplexer */

        void* entry;            /*!< \brief Pointer to the entry as returned
                                     by StGetNextEntry() */
    } MultiplexerEntry;

#pragma pack(pop)

#ifdef SIMUTRACE
#ifdef __GNUC__
// Protecting against GCC's false reporting of missing braces
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

    static const StreamTypeDescriptor _multiplexerEntryTypes[2] = {
        /* Multiplexer entry (temporal order)
           {80C8C1BB-32C9-4283-B95F-BA37C62095CB} */
        {"Multiplexer entry (temporal order)",
         {0x80c8c1bb,0x32c9,0x4283,{0xb9,0x5f,0xba,0x37,0xc6,0x20,0x95,0xcb}},
         StfTemporalOrder, 0, sizeof(MultiplexerEntry), 0 },

         /* Multiplexer entry
           {FA258383-9EC8-4E05-8239-9F260DDA8AD2} */
        {"Multiplexer entry",
         {0xfa258383,0x9ec8,0x4e05,{0x82,0x39,0x9f,0x26,0xd,0xda,0x8a,0xd2}},
         StfNone, 0, sizeof(MultiplexerEntry), 0 }
    };

    static inline const StreamTypeDescriptor* streamFindMultiplexerType(
        _bool temporalOrder)
    {
        return (temporalOrder == _true) ?
            &_multiplexerEntryTypes[0] : &_multiplexerEntryTypes[1];
    }

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif /* SIMUTRACE */

#ifdef __cplusplus
}
}
#endif

#endif