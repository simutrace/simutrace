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

    /* Forward declarations */

    struct _DynamicStreamDescriptor;
    typedef struct _DynamicStreamDescriptor DynamicStreamDescriptor;


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
        uint32_t reserved;      /*!< \brief Reserved. Set to 0 */

        const char* message;    /*!< \brief Error message, describing the
                                     exception */
    } ExceptionInformation;


    /* Stream related data structures */

    /*! \brief Dynamic stream initializer
     *  \relates DynamicStreamOperations
     *
     *  This dynamic stream handler is responsible for the initialization of
     *  any user implementation specific data and dependencies for the
     *  dynamic stream. This handler is called once when the stream is
     *  registered.
     *
     *  \param descriptor The descriptor handed to Simutrace for stream
     *                    registration.
     *
     *  \param id The id of the new dynamic stream.
     *
     *  \param userData Pointer to the #DynamicStreamDescriptor#userData field.
     *                  The handler may change the user-defined per-stream data
     *                  by setting a new value with this parameter.
     *
     *  \returns The handler should return 0 on success or any other value
     *           otherwise. The return value is then interpreted as error code,
     *           the stream registration is aborted and the error code can be
     *           retrieved through StGetLastError().
     *
     *  \warning Calling into the Simutrace API from within the handler (e.g.,
     *           to register or open another stream) is undefined behavior and
     *           must be avoided. If such operations are required consider
     *           moving them into the stream open callback
     *           (#DynamicStreamOperations#open) or perform
     *           them prior to the dynamic stream registration. Results from
     *           the operations can be passed to this handler via the
     *           #DynamicStreamDescriptor#userData field.
     *
     *  \remarks The handler does not need to be thread-safe in the context of
     *           a single session.
     *
     *  \since 3.2
     */
    typedef int (*DynamicStreamInitialize)(
        const DynamicStreamDescriptor* descriptor, StreamId id,
        void** userData);


    /*! \brief Dynamic stream finalizer
     *  \relates DynamicStreamOperations
     *
     *  This dynamic stream handler is responsible for cleaning up any
     *  user-defined data that has been allocated for the stream. The handler
     *  is called once, when the stream is destroyed (e.g., when the store
     *  closes).
     *
     *  \param id The id of the dynamic stream that is closed.
     *
     *  \param userData Pointer to the user-defined stream data. This is the
     *                  #DynamicStreamDescriptor#userData field. The handler
     *                  should free any resources associated with the user
     *                  data.
     *
     *  \warning Calling into the Simutrace API from within the handler (e.g.,
     *           to register or open another stream) is undefined behavior and
     *           must be avoided.
     *
     *  \remarks The handler does not need to be thread-safe in the context of
     *           a single session.
     *
     *  \since 3.2
     */
    typedef void (*DynamicStreamFinalize)(StreamId id, void* userData);


    /*! \brief Dynamic stream open handler
     *  \relates DynamicStreamOperations
     *
     *  This dynamic stream handler is responsible for performing any
     *  operations that are necessary to get data from the stream later. The
     *  handler is invoked in the context of a call to StStreamOpen(). The
     *  handler may initialize user-defined data that will be local to
     *  the stream handle that is created during the open operation.
     *
     *  The dynamic stream should adhere to the semantic of the \p type,
     *  \p value, and \p flags parameters as defined for regular streams. If a
     *  combination of flags is not supported, an error should be returned.
     *
     *  \param descriptor The descriptor of the stream for which the handle
     *                    is to be created.
     *
     *  \param id The id of the dynamic stream.
     *
     *  \param type Type of property value. Contains the value passed to
     *              StStreamOpen().
     *
     *  \param value A value used in the query. Contains the value passed to
     *               StStreamOpen().
     *
     *  \param flags Supplies information on how the caller intends to access
     *               the stream. Contains the value passed to StStreamOpen().
     *
     *  \param userDataOut A dynamic stream handle can reference arbitrarily
     *                     user-defined data, which will be available in all
     *                     handlers that operate on the handle
     *                     (e.g., #DynamicStreamGetNextEntry). The parameter
     *                     receives a pointer to the data and may be left
     *                     \c NULL if not required. If the dynamic stream uses
     *                     per-handle user-defined data, the data must only be
     *                     released in #DynamicStreamOperations#close.
     *
     *  \returns The handler should return 0 on success or any other value
     *           otherwise. The return value is then interpreted as error code,
     *           the handle creation is aborted and the error code can be
     *           retrieved through StGetLastError().
     *
     *  \remarks The handler does not need to be thread-safe in the context of
     *           a single stream, but it can be called in parallel for
     *           different streams.
     *
     *  \since 3.2
     */
    typedef int (*DynamicStreamOpen)(const DynamicStreamDescriptor* descriptor,
        StreamId id, QueryIndexType type, uint64_t value,
        StreamAccessFlags flags, void** userDataOut);


    /*! \brief Dynamic stream handle close handler
     *  \relates DynamicStreamOperations
     *
     *  Whenever an open stream handle is closed, this close handler is called
     *  to provide a chance for cleaning up user-defined data.
     *
     *  \param id Id of the stream that is closed.
     *
     *  \param userData Pointer to the user data of the handle. Handle local
     *                  data should be released in this function and the
     *                  variable pointer to by userData should be set to
     *                  \c NULL.
     *
     *  \remarks Note that after this call, there might be further open handles
     *           to the stream. To release any data shared by all handles, you
     *           have to implement some sort of reference counting or place
     *           shared data in the per-stream user-defined data (see
     *           #DynamicStreamInitialize).
     *
     *  \remarks The handler does not need to be thread-safe in the context of
     *           a single stream, but it can be called in parallel for
     *           different streams.
     *
     *  \since 3.2
     */
    typedef void (*DynamicStreamClose)(StreamId id, void** userData);


    /*! \brief Dynamic stream entry delivery handler
     *  \relates DynamicStreamOperations
     *
     *  Entries in dynamic streams are generated or received from other streams
     *  dynamically. Simutrace accomplishes this by calling this handler
     *  method every time StGetNextEntry() is invoked on the stream.
     *
     *  \param userData Pointer to any user-defined data supplied by the
     *                  open handler #DynamicStreamOperations#open.
     *
     *  \param entryOut Pointer that should be set to the location of the
     *                  entry to be returned. The entry must be of the type and
     *                  size specified in the stream descriptor for the
     *                  dynamic stream. The returned value may be \c NULL,
     *                  to indicate the end of the stream. It must be \c NULL
     *                  if an error is returned.
     *
     *  \returns The handler should return 0 on success or any other value
     *           otherwise. The return value is then interpreted as error code,
     *           the handle creation is aborted and the error code can be
     *           retrieved through StGetLastError().
     *
     *  \remarks If the handler should return entries of different type,
     *           consider using a meta entry, which only specifies the type
     *           and storage location of the real entry. Then let the handler
     *           always return the meta entry, instead of the real entry.
     *
     *  \remarks The entry does not have to be available beyond the next call
     *           to this handler or a close with #DynamicStreamOperations#close.
     *           The handler therefore might use a single per-handle entry,
     *           which is updated and returned on each call (effectively always
     *           returning the same pointer).
     *
     *  \remarks To indicate the end of the stream, return 0 and set \p entryOut
     *           to \c NULL.
     *
     *  \remarks The handler does not need to be thread-safe in the context of
     *           a single stream, but it can be called in parallel for
     *           different streams.
     *
     *  \since 3.2
     */
    typedef int (*DynamicStreamGetNextEntry)(void* userData, void** entryOut);


    /*! \brief Describes the handler functions for a dynamic stream
     *
     *  Dynamic streams are operated by user-defined handlers. This descriptor
     *  should be filled with pointers to the various functions. These functions
     *  are called by Simutrace to, for example, dynamically create entries or
     *  perform setup and cleanup for the stream.
     *
     *  \since 3.2
     */
    typedef struct _DynamicStreamOperations {
        DynamicStreamInitialize initialize;     /*!< \brief Pointer to stream
                                                     initializer. This handler
                                                     is optional and may be
                                                     left \c NULL */
        DynamicStreamFinalize finalize;         /*!< \brief Pointer to stream
                                                     finalizer. This handler
                                                     is optional and my be
                                                     left \c NULL */

        DynamicStreamOpen open;                 /*!< \brief Pointer to stream
                                                     open handler. This handler
                                                     is optional and may be
                                                     left \c NULL */
        DynamicStreamClose close;               /*!< \brief Pointer to stream
                                                     handle close handler. This
                                                     handler is optional and
                                                     may be left \c NULL */

        DynamicStreamGetNextEntry getNextEntry; /*!< \brief Pointer to entry
                                                     delivery handler. This
                                                     handler is mandatory */

        void* reserved[4];                      /*!< \brief Reserved for
                                                     future use. Leave \c NULL*/
    } DynamicStreamOperations;


    /*! \typedef DynamicStreamDescriptor
     *  \brief Describes a dynamic stream
     *
     *  A dynamic stream extends on a standard (i.e., static) stream in that
     *  its entries are generated on-demand by user-defined handlers. This
     *  descriptor defines all properties and handlers for a dynamic stream
     *  and can be used for registration with StStreamRegisterDynamic().
     *
     *  \since 3.2
     *
     *  \see StStreamRegisterDynamic()
     *  \see StMakeStreamDescriptorDynamic()
     *  \see StMakeStreamDescriptorDynamicFromType()
     */
    typedef struct _DynamicStreamDescriptor {
        StreamDescriptor base;              /*!< \brief Descriptor for the
                                                 general stream properties.
                                                 The SfDynamic flag must be
                                                 specified */

        DynamicStreamOperations operations; /*!< \brief Descriptor for the
                                                 dynamic stream operations */
        void* userData;                     /*!< \brief May point to
                                                 arbitrary user-defined,
                                                 stream-local data, which
                                                 should be available in the
                                                 handler functions. The data
                                                 may also be allocated in the
                                                 #DynamicStreamInitialize
                                                 handler. It must not be
                                                 deleted for the lifetime of
                                                 the stream */

        uint64_t reserved[4];               /*!< \brief Reserved for future
                                                 use. Leave 0. */
    } DynamicStreamDescriptor;


    /*! \internal */
    typedef enum _StreamStateFlags {
        SsfNone    = 0x00,
        SsfRead    = 0x01,
        SsfDynamic = 0x02
    } StreamStateFlags;

    /*! \internal */
    typedef struct _StreamStateDescriptor {
        StreamStateFlags flags;
        StreamAccessFlags accessFlags;

        void* stream;
        uint32_t entrySize;

        SegmentId segment;

        union {
            /* Static streams */
            struct {
                StreamSegmentId sequenceNumber;
                uint32_t reserved;

                SegmentControlElement* control;
            } stat;

            /* Dynamic streams */
            struct {
                DynamicStreamOperations* operations;
                void* userData;
            } dyn;
        };

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
    /* StreamStateFlags */
    inline StreamStateFlags operator^(StreamStateFlags a, StreamStateFlags b)
    {
        return static_cast<StreamStateFlags>(
            static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline StreamStateFlags operator|(StreamStateFlags a, StreamStateFlags b)
    {
        return static_cast<StreamStateFlags>(
            static_cast<int>(a) | static_cast<int>(b));
    }

    inline StreamStateFlags operator&(StreamStateFlags a, StreamStateFlags b)
    {
        return static_cast<StreamStateFlags>(
            static_cast<int>(a) & static_cast<int>(b));
    }

}
#endif

#endif