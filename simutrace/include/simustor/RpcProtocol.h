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
#ifndef RPC_PROTOCOL_H
#define RPC_PROTOCOL_H

#include "SimuPlatform.h"
#include "SimuStorTypes.h"

#include "RpcInterface.h"
#include "Version.h"

namespace SimuTrace
{

#define RPC_VERSION_MAJOR SIMUTRACE_VERSION_MAJOR
#define RPC_VERSION_MINOR SIMUTRACE_VERSION_MINOR

#define RPC_VERSION RPC_VER(RPC_VERSION_MAJOR, RPC_VERSION_MINOR)

// Incompatible v3.1 function
#define RPC_CALL_V31(code, name, payloadType, expectedLength) \
    RPC_CALL(3, 1, code, name, payloadType, expectedLength)

// Compatible function
#define RPC_CALL_V31C(code, name, payloadType, expectedLength) \
    RPC_CALL(3, 1, code, name, payloadType, expectedLength) \
    RPC_CALL(3, 0, code, name, payloadType, expectedLength)

#define TEST_REQUEST_V31(rpcname, msg) \
    TEST_REQUEST(3, 1, rpcname, msg)

#define RPC_CALL_V30(code, name, payloadType, expectedLength) \
    RPC_CALL(3, 0, code, name, payloadType, expectedLength)

#define TEST_REQUEST_V30(rpcname, msg) \
    TEST_REQUEST(3, 0, rpcname, msg)


    /* ==========  Simutrace Storage Server - Main API  ========== */

    ///
    ///    Null
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Empty message. Can be used to ping the server or check the
    ///        server's API version.
    ///
    /// Arguments:
    ///        Parameter0: Client API version
    ///
    /// Return Value:
    ///        Parameter0: Server API version
    ///
    RPC_CALL_V31C(0x0000, Null, Embedded, 0)


    ///
    ///    EnumerateSessions
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Enumerates the ids of all currently active sessions in the
    ///        server.
    ///
    /// Arguments:
    ///        Parameter0<uint16_t>: Client API version (with RPC_VER macro)
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<uint16_t>: Server API version
    ///        Parameter1<uint32_t>: Number of sessions
    ///        Payload<data>: Array of SessionId
    ///
    RPC_CALL_V31C(0x0001, EnumerateSessions, Embedded, 0)


    /* ==========  Simutrace Storage Server - Session API  ========== */

    ///
    ///    SessionCreate
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Create a new session in the storage server.
    ///
    /// Arguments:
    ///        Parameter0<uint16_t>: Client API version (with RPC_VER macro)
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<SessionId> Server Side Session Id
    ///
    RPC_CALL_V31C(0x0010, SessionCreate, Embedded, 0)


    ///
    ///    SessionOpen
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Opens an existing session in the storage server.
    ///
    /// Arguments:
    ///        Parameter0<uint16_t>: Client API version (with RPC_VER macro)
    ///        Parameter1<SessionId>: Session Id
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0011, SessionOpen, Embedded, 0)


    ///
    ///    SessionQuery
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Returns basic information about a specific session.
    ///
    /// Arguments:
    ///        Parameter0<uint16_t>: Client API version (with RPC_VER macro)
    ///        Parameter1<SessionId>: Id of session
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Payload<data>: SessionInformation
    ///

    struct SessionInformation
    {
        int dummy;
    };

    RPC_CALL_V31C(0x0012, SessionQuery, Embedded, 0)


    ///
    ///    SessionClose
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Closes the current session
    ///
    /// Arguments:
    ///        None.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0013, SessionClose, Embedded, 0)


    ///
    ///    SessionSetConfiguration
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Applies the given configuration string to the session's
    ///        configuration.
    ///
    /// Arguments:
    ///        Payload<data>: configuration string. See documentation of
    ///                       libconfig for more information.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0014, SessionSetConfiguration, Data, 0)


    /* ==========  Simutrace Storage Server - Store API  ========== */

    ///
    ///    StoreCreate
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Opens or creates the store with the supplied specifier
    ///
    /// Arguments:
    ///        Parameter0<bool>: Overwrite store if it exists
    ///        Parameter1<bool>: Open the store, instead of create it
    ///        Payload<data>: Specifier
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0020, StoreCreate, Data, 0)


    ///
    ///    StoreClose
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Closes the open store
    ///
    /// Arguments:
    ///        None
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0021, StoreClose, Embedded, 0)


    ///
    ///    StreamBufferRegister
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Creates a new stream buffer. If the connection between the
    ///        server and the client does not support shared memory, the
    ///        buffer content is copied on submission.
    ///
    /// Arguments:
    ///        Parameter0<uint32_t>: Number of segments
    ///        Parameter1<size_t>: Segment size
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<BufferId>: Server-side Id of the buffer
    ///
    ///      Only for local connections:
    ///        Payload<Handles>: [0] Stream Buffer
    ///
    RPC_CALL_V31C(0x0022, StreamBufferRegister, Embedded, 0)


    ///
    ///    StreamBufferEnumerate
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Enumerates the ids of all registered stream buffers
    ///        in the store.
    ///
    /// Arguments:
    ///        None.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<uint32_t>: Number of stream buffers
    ///        Payload<data>: Array of BufferIds
    ///
    RPC_CALL_V31C(0x0023, StreamBufferEnumerate, Embedded, 0)


    ///
    ///    StreamBufferQuery
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Queries the configuration of a stream buffer.
    ///
    /// Arguments:
    ///        Parameter0<BufferId>: Id of the buffer to query
    ///
    ///      Only for local connections:
    ///        Parameter1<bool>: Duplicate handles
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<uint32_t>: Number of segments
    ///        Parameter1<size_t>:  Segment size
    ///
    ///      Only for local connections:
    ///        Payload<Handles>:  [0] Stream Buffer
    ///
    RPC_CALL_V31C(0x0024, StreamBufferQuery, Embedded, 0)


    /* ==========  Simutrace Storage Server - Stream RPC  ========== */

    ///
    ///    StreamRegister
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Create a new stream to back trace data.
    ///
    /// Arguments:
    ///        Parameter0<BufferId>: Id of the stream buffer from which to
    ///                              allocate segments.
    ///        Payload<StreamDescriptor>: Description of the stream and the
    ///                            entries that are to be stored in the stream.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<StreamId>: Id of the newly created stream.
    ///
    RPC_CALL_V31C(0x0030, StreamRegister, Data, sizeof(StreamDescriptor))


    ///
    ///    StreamEnumerate
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Enumerates the ids of all registered streams in the
    ///        current store.
    ///
    /// Arguments:
    ///        Parameter<bool>: Specifies, if hidden streams should be
    ///                         included in the results.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<uint32_t>: Number of streams
    ///        Payload<data>: Array of StreamIds
    ///
    RPC_CALL_V31C(0x0031, StreamEnumerate, Embedded, 0)


    ///
    ///    StreamQuery
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Queries the configuration of a stream.
    ///
    /// Arguments:
    ///        Parameter0<StreamId>: Id of the stream to query
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<BufferId>: Id of the buffer, which should back the
    ///                              stream.
    ///
    ///        Payload<StreamQueryInformation>: Description of the stream and
    ///                              the entries that are to be stored in the
    ///                              stream.
    ///
    RPC_CALL_V31C(0x0032, StreamQuery, Embedded, 0)


    ///
    ///    StreamAppend
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Appends a new segment at the end of the stream and returns the
    ///        id of the segment as well as the id of the allocated buffer
    ///        segment.
    ///
    /// Arguments:
    ///        Parameter0<StreamId>: Stream to apply the operation on.
    ///
    ///      Only for remote connections:
    ///        Payload<Segment>: Control element of the segment + the
    ///                          segment data
    ///
    ///      Local connections send a data packet with zero payload size.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Parameter0<StreamSegmentId>: Id of the newly allocated segment
    ///        Parameter1<SegmentId>: Id of the segment in the stream buffer
    ///
    ///      Only for remote connections:
    ///         Payload<SegmentControlElement>: The initialized control element
    ///                                         of the new segment.
    ///
    ///      Local connections send a data packet with zero payload size.
    ///
    RPC_CALL_V31C(0x0033, StreamAppend, Data, 0)


    ///
    ///    StreamCloseAndOpen
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Closes the specified segment and opens the segment that fulfills
    ///        the specified query. The query may point to a certain entry by
    ///        index, by cycle count, real time or it may directly specify the
    ///        segment by sequence number. If the query points to an entry the
    ///        returned segment contains the requested entry, but does not need
    ///        to start with this entry. The caller therefore needs to perform
    ///        a manual search for the entry within the segment.
    ///
    /// Arguments:
    ///        Parameter0<StreamId>: Stream to apply the operation on.
    ///        Parameter1<StreamSegmentId>: Sequence number to close.
    ///
    ///        Payload<StreamOpenQuery>: Query value
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    ///        Version 3.0:
    ///        Parameter0<StreamSegmentId>: Id of the opened segment
    ///        Parameter1<SegmentId>: Id of the segment in the stream buffer
    ///
    ///        Version 3.1:
    ///        Parameter0<SegmentId>: Id of the segment in the stream buffer
    ///        Parameter1<uint32_t>: Offset into segment to use for handle
    ///
    ///      Only for remote connections:
    ///         Payload<SegmentControlElement>: The initialized control element
    ///                                         of the segment.
    ///
    ///      Local connections send a data packet with zero payload size.
    ///
    struct StreamOpenQuery {
        QueryIndexType type;
        uint64_t value;

        StreamAccessFlags flags;
    };

    RPC_CALL_V31(0x0134, StreamCloseAndOpen, Data, sizeof(StreamOpenQuery))
    RPC_CALL_V30(0x0034, StreamCloseAndOpen, Data, sizeof(StreamOpenQuery))


    ///
    ///    StreamClose
    /// -----------------------------------------------------------
    /// Routine Description:
    ///        Closes the specified stream segment. If the segment contains
    ///        new data (i.e., it is the segment created with AppendStream),
    ///        the segment is submitted.
    ///
    /// Arguments:
    ///        Parameter0<StreamId>: Id of the stream to which the segment
    ///                              belongs
    ///        Parameter1<StreamSegmentId>: Id of the stream segment
    ///
    ///      Only for remote connections:
    ///        Payload<Segment>: Control element of the segment + the
    ///                          segment data
    ///
    ///      Local connections send a data packet with zero payload size.
    ///
    /// Return Value:
    ///        SC_Success on success, SC_Failed otherwise.
    ///
    RPC_CALL_V31C(0x0035, StreamClose, Data, 0)

}

#endif