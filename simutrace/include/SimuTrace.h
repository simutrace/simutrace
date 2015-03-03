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
#ifndef SIMUTRACE_H
#define SIMUTRACE_H

#include "SimuTraceTypes.h"
#include "SimuTraceEntryTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

#ifdef WIN32
#ifdef SIMUTRACE
#define SIMUTRACE_API __declspec(dllexport)
#else
#define SIMUTRACE_API __declspec(dllimport)
#endif
#else
#define SIMUTRACE_API
#endif

    /* Base API */

    /*! \brief Version of the Simutrace client.
     *
     *  Version number of the Simutrace client. This should ideally match the
     *  version of the server. The version number has the form:
     *  <Revision (16 bits), Major Version (8 bits), Minor Version (8 bits)>
     *  with the revision being stored in the high word.
     *
     *  \since 3.0
     *
     *  \see SIMUTRACE_VER_MAJOR
     *  \see SIMUTRACE_VER_MINOR
     *  \see SIMUTRACE_VER_REVISION
     */
    SIMUTRACE_API
    extern const uint32_t StClientVersion;


    /*! \brief Version of the Simutrace client.
     *
     *  Wrapper function to receive the client version. See #StClientVersion
     *  for more information.
     *
     *  \since 3.1
     *
     *  \see #StClientVersion
     */
    SIMUTRACE_API
    uint32_t StGetClientVersion();


    /*! \brief Returns the last exception.
     *
     *  Returns extended exception information on the last error during a
     *  Simutrace API call.
     *
     *  \param informationOut A pointer to an #ExceptionInformation structure
     *                        that will receive the error information. The
     *                        parameter is optional and may be \c NULL. Freeing
     *                        the message member in the structure is not
     *                        necessary, however the string will only be valid
     *                        until the next API call.
     *
     *  \returns The last error code. For platform exceptions this is the
     *           operating system error code.
     *
     *  \remarks The exception information is thread local. StGetLastError()
     *           thus always returns error information on API calls from the
     *           same thread only.
     *
     *  \since 3.0
     */
    SIMUTRACE_API
    int StGetLastError(ExceptionInformation* informationOut);


    /* Session API */

    /*! \brief Creates a new session.
     *
     *  Connections of a client to a Simutrace storage server are represented
     *  as a session. This method connects to the specified server and creates
     *  a new local session as well as a new session on the server side.
     *  Calling this function is the first step, when working with Simutrace.
     *  To successfully establish a connection, the server process must already
     *  be running and accept connections on the address specified by @p server.
     *
     *  \param server Specifies the connection string used to contact the
     *                server. The specifier must be supplied in the form
     *                <tt>channel:address</tt>. Supported specifiers are:
     *                - <tt>local:pipe</tt> (recommended)
     *
     *                  The server runs on the same host as the client.
     *                  This is the fastest connection with a Simutrace server,
     *                  because the server and client can use shared memory
     *                  to communicate trace data. To exchange commands a
     *                  named pipe is utilized. The named pipe must be set
     *                  in the server configuration and will be created by the
     *                  server at startup.
     *
     *                  Example connecting through a named pipe to a server
     *                  running on the same host. On Windows, the "\\.\Pipe\"-
     *                  prefix must be omitted.
     *                  \code
     *                  local:/tmp/mypipe
     *                  \endcode
     *
     *                - <tt>socket:ipv4|[ipv6]|dns:port</tt>
     *
     *                  The server runs on a remote host. This channel will
     *                  use a regular TCP/IP connection. The remote host
     *                  may be specified through its IPv4, IPv6 or its dns
     *                  name. Since Simutrace does not have a default port,
     *                  the address must always include the destination port.
     *                  The socket connection can also be used to connect to
     *                  a local server, however in that case a local channel
     *                  connection is recommended to improve performance.
     *
     *                  Examples connecting through a TCP socket to a server
     *                  running on the same or a remote host.
     *                  \code
     *                  socket:10.1.1.10:7000
     *                  socket:simutrace.org:7000
     *                  socket:[2001:0db8:0000:0000:0000:ff00:0042:8329]:7000
     *                  \endcode
     *
     *                  When setting up a socket binding in the server the
     *                  binding string may also contain a "*"-wildcard for (a)
     *                  the interface address to denote all installed
     *                  interfaces, and (b) for the port number to let the
     *                  operating system choose a port. However, keep in mind
     *                  that you need to specify the exact address and port on
     *                  the client. Examples:
     *                  \code
     *                  socket:*:7000
     *                  socket:*:*
     *                  socket:10.1.1.10:*
     *                  \endcode
     *
     *  \returns A #SessionId, identifying the newly created session if
     *           successful, \c INVALID_SESSION_ID otherwise.
     *           For a more detailed error description call StGetLastError().
     *
     *  \remarks The connection to the server is established by the thread
     *           calling StSessionCreate(). Only the very same thread is able
     *           to work with the session. To connect further threads to a
     *           session call StSessionOpen() from within the respective
     *           threads that should participate in the tracing session.
     *
     *  \remarks Before a thread connected to a Simutrace server exits,
     *           it must release its connection. For more details see
     *           StSessionClose().
     *
     *  \since 3.0
     *
     *  \see StSessionOpen()
     *  \see StSessionClose()
     */
    SIMUTRACE_API
    SessionId StSessionCreate(const char* server);


    /*! \brief Opens an existing local session.
     *
     *  Each thread that wants to participate in a tracing session (e.g., read
     *  or write data via the tracing API), must be connected to a session.
     *  This method connects a thread to an existing local session.
     *
     *  \param session The id of the session to be opened as originally
     *                 returned by StSessionCreate().
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks A thread may not connect to a session created in a different
     *           process or on a remote host running the Simutrace client. A
     *           session is always process local and thus accessible for
     *           threads of the same process only. To share a tracing session
     *           between multiple processes or hosts, create a new session and
     *           open the \b same store in/on all processes/machines. For more
     *           details see StSessionCreateStore().
     *
     *  \remarks Before a thread connected to a Simutrace server exits,
     *           it must release its connection. For more details see
     *           StSessionClose().
     *
     *  \since 3.0
     *
     *  \see StSessionCreate()
     *  \see StSessionClose()
     */
    SIMUTRACE_API
    _bool StSessionOpen(SessionId session);


    /*! \brief Closes an open session.
     *
     *  This method closes a thread's reference to an open session. If this is
     *  the last reference, the session is run down. After closing the session,
     *  the executing thread may connect to a different session again.
     *
     *  \param session The id of the session to be closed as originally
     *                 returned by StSessionCreate().
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks Before a thread connected to a Simutrace server exits,
     *           it must call StSessionClose() to release its connection. A
     *           session is automatically closed as soon as the last reference
     *           is closed, that is all threads participating in a session have
     *           called StSessionClose(). If a thread fails to do so, the
     *           connection and hence the session is dangling until the client
     *           application terminates. If the connection to the server is
     *           lost (e.g., due to a network failure), the server treats this
     *           as an implicit close and, if all connections are lost, frees
     *           the session. If the client program terminates without closing
     *           its open sessions, the server will treat this as a loss of all
     *           connections and consequently close the corresponding sessions.
     *
     *  \remarks Closing a session will also close its reference to the open
     *           store (if any). For more details about closing a store and the
     *           effects, see StSessionCloseStore().
     *
     *  \since 3.0
     *
     *  \see StSessionCreate()
     *  \see StSessionOpen()
     */
    SIMUTRACE_API
    _bool StSessionClose(SessionId session);


    /*! \brief Creates a new store.
     *
     *  A store is the data back-end for a Simutrace tracing session. All trace
     *  data recorded or read in a session is stored in the currently open
     *  store. Creating or opening a store is thus typically the next operation
     *  after creating a new session. This method creates a new store.
     *
     *  \param session The id of the session to open a store in as originally
     *                 returned by StSessionCreate().
     *
     *  \param specifier Simutrace comes with a modular storage back-end that
     *                   can employ custom trace formats to save trace data.
     *                   The specifier must be supplied in the form
     *                   <tt>format:path</tt>. Supported specifiers are:
     *                   - <tt>simtrace:path</tt> (recommended)
     *
     *                     Simutrace's native storage format (v 3.0). It
     *                     fully supports all features, provides fast access
     *                     even for very large traces and comes with a
     *                     built-in aggressive but fast compressor for memory
     *                     traces (see StStreamFindMemoryType()).
     *
     *                     The \c simtrace storage back-end saves trace data
     *                     in a regular file on disk. The path can be
     *                     absolute (e.g., <tt>C:\\simutrace\\traces\\mytrace.sim
     *                     </tt> in Windows or <tt>/traces/mytrace.sim</tt> in
     *                     Linux) or a relative path. Relative paths use the
     *                     configured root for the \c simtrace storage format
     *                     (\c store.simtrace.root) or the active workspace
     *                     (\c server.workspace) as base path. For more
     *                     For more information on how to configure these
     *                     settings see StSessionSetConfiguration() and the
     *                     sample server configuration file.
     *
     *                     The \c simtrace storage format uses the extension
     *                     \c sim. Although a trace can have any valid file
     *                     name, enumerating \c simtrace stores will list only
     *                     files with the \c sim extension.
     *
     *                     Example creating a store using a relative path.
     *                     \code
     *                     simtrace:mytrace.sim
     *                     \endcode
     *
     *                     The \c simtrace format is the only supported format
     *                     in this release. This may change in the future.
     *
     *  \param alwaysCreate Indicates if the store should be overwritten, if
     *                      it already exists.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks A store may receive new data only directly after its creation.
     *           Closing a store marks it read-only, denying any modification
     *           afterwards. This may change in the future.
     *
     *  \remarks Simutrace uses a global store concept, where the same store
     *           may be open in multiple sessions (in the same server). This
     *           allows writing to and reading from the same trace file from
     *           different processes or physical machines (each one having its
     *           own session). Calling this method with \p alwaysCreate set to
     *           \c _true will fail if the store is still open in another
     *           session.
     *
     *  \warning Be sure to set the \p alwaysCreate parameter as intended to
     *           avoid data loss. Simutrace won't present a confirmation
     *           before deleting an existing store!
     *
     *  \deprecated <b>Before 3.1:</b> If \p alwaysCreate was set to \c _false
     *              and the store already existed, this method was used to open
     *              a store. The method now fails in this scenario.
     *              Use StSessionOpenStore() instead.
     *
     *  \since 3.0
     *
     *  \see StSessionCloseStore()
     *  \see StSessionSetConfiguration()
     */
    SIMUTRACE_API
    _bool StSessionCreateStore(SessionId session, const char* specifier,
                               _bool alwaysCreate);


    /*! \brief Opens an existing store.
     *
     *  A store is the data back-end for a Simutrace tracing session. All trace
     *  data recorded or read in a session is stored in the currently open
     *  store. Creating or opening a store is thus typically the next operation
     *  after creating a new session. This method opens an existing store.
     *
     *  \param session The id of the session to open a store in as originally
     *                 returned by StSessionCreate().
     *
     *  \param specifier Simutrace comes with a modular storage back-end that
     *                   can employ custom trace formats to save trace data.
     *                   The specifier must be supplied in the form
     *                   <tt>format:path</tt>. For more information on
     *                   storage specifier please refer to
     *                   StSessionCreateStore().
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks A store may receive new data only directly after its creation.
     *           Closing a store marks it read-only, denying any modification
     *           afterwards. This may change in the future.
     *
     *  \since 3.1
     *
     *  \see StSessionCreateStore()
     *  \see StSessionCloseStore()
     *  \see StSessionEnumerateStores()
     */
    SIMUTRACE_API
    _bool StSessionOpenStore(SessionId session, const char* specifier);


    /*! \brief Closes a session's store.
     *
     *  Each session may have only one store open at a time. Use this function
     *  to close the currently open store. This allows you to switch to a
     *  different store without creating a new session.
     *
     *  \param session The id of the session whose store should be closed as
     *                 originally returned by StSessionCreate().
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks Simutrace uses a global store concept, where the same store
     *           may be open in multiple sessions (in the same server). This
     *           allows writing to and reading from the same trace file from
     *           different processes or physical machines (each one having its
     *           own session). Closing a session's reference to a store will
     *           only effectively close the store, if this is the last
     *           reference.
     *
     *  \remarks A session closes its reference to a store automatically when
     *           the session itself is closed.
     *
     *  \remarks Closing a store will implicitly close all handles to streams
     *           of this store. For more details on closing stream handles
     *           and the effects see StStreamClose().
     *
     *  \since 3.0
     *
     *  \see StSessionCreateStore()
     */
    SIMUTRACE_API
    _bool StSessionCloseStore(SessionId session);


    /*! \brief Sets session configurations
     *
     *  This method allows configuring various parameters for the specified
     *  session. For a reference of all configurable parameters see the
     *  sample server configuration file, which comes with Simutrace.
     *
     *  \param session The id of the session to be configured as
     *                 originally returned by StSessionCreate().
     *
     *  \param configuration A string describing the configuration to set. For
     *                       information on the format see the documentation
     *                       of libconfig.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks The Simutrace storage server has a main configuration, which
     *           can be set by specifying a configuration file or by supplying
     *           command line parameters when starting the server executable.
     *           The server replicates its entire configuration when it creates
     *           a new session in behalf of the user. This method allows
     *           modifying the session configuration. The underlying server
     *           configuration cannot be changed with this method.
     *
     *  \since 3.0
     */
    SIMUTRACE_API
    _bool StSessionSetConfiguration(SessionId session, const char* configuration);


    /* Stream API */

    /*! \brief Creates a new stream descriptor.
     *
     *  This method is a helper function to quickly create a new stream
     *  description needed to register a new stream. The description contains
     *  information about the stream's name and layout of trace entries,
     *  that is records written to or read from a stream.
     *
     *  \param name A friendly name of the new stream (e.g.,
     *              "CPU Memory Accesses").
     *
     *  \param entrySize The size of a single trace entry in bytes. To specify
     *                   a variable-sized entry type, use
     *                   makeVariableEntrySize().
     *
     *  \param temporalOrder Indicates if the entries in the stream will
     *                       contain a time stamp. The time stamp must be the
     *                       first field in each entry and be of type
     *                       \c CycleCount, at least
     *                       \c TEMPORAL_ORDER_CYCLE_COUNT_BITS (default 48)
     *                       bits wide. The time stamp must further be
     *                       monotonic increasing.
     *
     *  \param descOut Pointer to a #StreamDescriptor structure that will
     *                 receive the new stream information.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks Simutrace internally identifies entry types through a
     *           #StreamTypeId. This method will generate a random id on every
     *           call. Using type information generated by this method is thus
     *           not recommended if a constant id is required, for example to
     *           find a certain stream of data by type when opening an existing
     *           store.
     *
     *  \remarks Simutrace uses a stream's type information to find an
     *           appropriate encoder. The encoder processes (usually compresses)
     *           the data in a stream before it is written to disk. To
     *           utilize the built-in memory trace encoder use
     *           StStreamFindMemoryType() and StMakeStreamDescriptorFromType().
     *
     *  \since 3.0
     *
     *  \see StMakeStreamDescriptorFromType()
     *  \see StStreamRegister()
     */
    SIMUTRACE_API
    _bool StMakeStreamDescriptor(const char* name, uint32_t entrySize,
                                 _bool temporalOrder, StreamDescriptor* descOut);


    /*! \brief Creates a new stream descriptor from existing type information.
     *
     *  This method is a helper function to quickly create a new stream
     *  description needed to register a new stream. The description contains
     *  information about the stream's name and layout of trace entries,
     *  that is records written to or read from a stream. This method uses
     *  the supplied type information.
     *
     *  \param name A friendly name of the new stream (e.g.,
     *              "CPU Memory Accesses").
     *
     *  \param type Pointer to a valid #StreamTypeDescriptor structure,
     *              defining the desired type of the new stream.
     *
     *  \param descOut Pointer to a #StreamDescriptor structure that will
     *                 receive the new stream information.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks Simutrace uses a stream's type information to find an
     *           appropriate encoder. The encoder processes (usually compresses)
     *           the data in a stream before it is written to disk. To
     *           utilize the built-in memory trace encoder get the type by
     *           calling StStreamFindMemoryType().
     *
     *  \since 3.0
     *
     *  \see StStreamFindMemoryType()
     *  \see StMakeStreamDescriptor()
     *  \see StStreamRegister()
     */
    SIMUTRACE_API
    _bool StMakeStreamDescriptorFromType(const char* name,
                                         const StreamTypeDescriptor* type,
                                         StreamDescriptor* descOut);


    /*! Returns stream descriptions for built-in memory entry types
     *
     *  Simutrace comes with an integrated aggressive, but fast compressor for
     *  memory traces. To utilize the compressor, the stream must use one of
     *  the types returned by this method and employ the \c simtrace storage
     *  format (see StSessionCreateStore()).
     *
     *  \param size Architecture size of the simulated system.
     *
     *  \param accessType Type of memory operations that the new stream should
     *                    hold.
     *
     *  \param addressType The semantic of the address field.
     *
     *  \param hasData Indicates if the memory entry will contain the data
     *                 read or written with the memory entry.
     *
     *  \returns Stream descriptor identifying the desired memory entry type.
     *           Do not modify the returned stream descriptor.
     *
     *  \remarks The memory trace compressor makes heavy use of the server's
     *           internal memory pool. For example to compress a 64 MiB segment
     *           of raw memory references (#DataWrite64), the server may
     *           allocate over 1 GiB of additional internal pool memory. When
     *           reading a memory stream with read-ahead active, the amount of
     *           consumed memory increases. Ensure you run the server with
     *           enough pool memory configured.
     *
     *  \since 3.0
     *
     *  \see StMakeStreamDescriptor()
     *  \see StStreamRegister()
     */
    SIMUTRACE_API
    const StreamTypeDescriptor* StStreamFindMemoryType(ArchitectureSize size,
        MemoryAccessType accessType, MemoryAddressType addressType,
        _bool hasData);


    /*! \brief Registers a new stream.
     *
     *  Streams are the basic interface to write or read data with Simutrace.
     *  Streams always belong to a store. This method registers a new stream in
     *  the session's active store. Before doing so, the caller must initially
     *  create a new store with StSessionCreateStore().
     *
     *  \param session The id of the session, whose store should register the
     *                 stream.
     *
     *  \param desc Pointer to a stream descriptor defining the properties of
     *              the new stream (e.g., the desired type of data entries).
     *              To create a descriptor see StMakeStreamDescriptor() or
     *              StMakeStreamDescriptorFromType().
     *
     *  \returns The id of the new stream if successful, \c INVALID_STREAM_ID
     *           otherwise. For a more detailed error description call
     *           StGetLastError().
     *
     *  \remarks Registering streams is only possible for newly created stores.
     *           This may change in the future.
     *
     *  \remarks When opening an existing store, Simutrace automatically
     *           restores all streams under the same ids. To retrieve a list of
     *           all registered stream call StStreamEnumerate().
     *
     *  \warning Once a stream is registered, it cannot be removed. The
     *           operation is irreversible.
     *
     *  \since 3.0
     *
     *  \see StMakeStreamDescriptor()
     *  \see StMakeStreamDescriptorFromType()
     *  \see StStreamEnumerate()
     *  \see StStreamQuery()
     */
    SIMUTRACE_API
    StreamId StStreamRegister(SessionId session, StreamDescriptor* desc);


    /*! \brief Returns a list of all registered streams.
     *
     *  After registering streams or opening an existing store, all streams
     *  are available through their corresponding ids. This method enumerates
     *  all registered streams by returning a list of valid stream ids. To get
     *  information on a certain stream call StStreamQuery().
     *
     *  \param session The id of the session to enumerate all streams from.
     *
     *  \param bufferSize Total size in bytes of the buffer supplied to receive
     *                    the stream ids. The function fails when the buffer is
     *                    too small.
     *
     *  \param streamIdsOut Supplies the destination buffer, which receives the
     *                      list of stream ids. Each element in the buffer will
     *                      be of type #StreamId. The buffer must be at least
     *                      \p bufferSize bytes in size.
     *
     *  \returns The number of valid stream ids, that is registered streams.
     *           The method will return \c -1 on error. For a more detailed
     *           error description call StGetLastError().
     *
     *  \remarks To determine the correct size for the destination buffer, call
     *           this method with \p bufferSize and \p streamIdsOut set to
     *           \c 0 and \c NULL respectively. The return value indicates the
     *           number of stream ids (#StreamId) the buffer must be able to
     *           accommodate.
     *
     *  \since 3.0
     *
     *  \see StStreamRegister()
     *  \see StStreamQuery()
     */
    SIMUTRACE_API
    int StStreamEnumerate(SessionId session, size_t bufferSize,
                          StreamId* streamIdsOut);


    /*! \brief Returns detailed information on a stream.
     *
     *  This method returns detailed information on the properties of a stream.
     *  The information includes the #StreamDescriptor originally supplied to
     *  create the stream as well as a set of statistics such as the compressed
     *  size of the stream. See #StreamQueryInformation for furthers details.
     *
     *  \param session The id of the session that holds the stream of interest.
     *
     *  \param stream The id of the stream to query.
     *
     *  \param informationOut A pointer to a #StreamQueryInformation structure
     *                        which will receive the stream information.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \since 3.0
     *
     *  \see StStreamRegister()
     *  \see StStreamEnumerate()
     */
    SIMUTRACE_API
    _bool StStreamQuery(SessionId session, StreamId stream,
                        StreamQueryInformation* informationOut);


    /*! \brief Opens a stream for appending write access.
     *
     *  This method opens a write handle to the specified stream. The handle
     *  can be used with StGetNextEntryFast() to append new entries to a stream.
     *
     *  \param session The id of the session that holds the stream of interest.
     *                 Must be \c INVALID_SESSION_ID, when supplying
     *                 \p handle.
     *
     *  \param stream The id of the stream to open a handle for. Must be
     *                \c INVALID_STREAM_ID, when supplying \p handle.
     *
     *  \param handle Existing write handle or \c NULL.
     *
     *  \returns A write handle to the specified stream if successful,
     *           \c NULL otherwise. For a more detailed error description call
     *           StGetLastError().
     *
     *  \remarks While each stream may have multiple independent read handles,
     *           a stream may have only one write handle. If the caller already
     *           created the write handle and it is still open, it must be
     *           passed with \p handle.
     *
     *  \remarks Use this method only to create an initial write handle to
     *           a stream, that is supply the \p session and \p stream
     *           parameters and set handle to \c NULL. StGetNextEntryFast()
     *           will take care of proceeding the handle along the stream.
     *
     *  \remarks Writing to a stream at arbitrary positions is not supported.
     *           Entries can only be appended to a stream.
     *
     *  \remarks Use StStreamClose() to close the write handle at the end of
     *           the tracing session. It is valid to create a new write handle
     *           afterwards, as long as the store has not been closed and
     *           re-opened in the mean time.
     *
     *  \warning Do not attempt to append a stream after opening an existing
     *           store. The data will be lost. This may change in the future.
     *
     *  \since 3.0
     *
     *  \see StStreamRegister()
     *  \see StStreamOpen()
     *  \see StStreamClose()
     */
    SIMUTRACE_API
    StreamHandle StStreamAppend(SessionId session, StreamId stream,
                                StreamHandle handle);


    /*! \brief Opens a stream for read access.
     *
     *  This method opens a new read handle to the specified stream. The handle
     *  can be used with StGetNextEntryFast() and StGetPreviousEntryFast() to
     *  read entries.
     *
     *  \param session The id of the session that holds the stream of interest.
     *                 Must be \c INVALID_SESSION_ID, when supplying
     *                 \p handle.
     *
     *  \param stream The id of the stream to open a handle for. Must be
     *                \c INVALID_STREAM_ID, when supplying \p handle.
     *
     *  \param type Type of property value that indicates which position in
     *              the stream should be opened. For further information on the
     *              possible query types see #QueryIndexType.
     *
     *  \param value A value used in the query specified by \p type. See
     *               QueryIndexType for more details.
     *
     *  \param flags Supplies information on how the caller intends to access
     *               the stream with the requested handle. For more information
     *               see #StreamAccessFlags. If \p handle is supplied and
     *               \p flags is set to \c StreamAccessFlags::SafNone, the
     *               flags are taken from the handle.
     *
     *  \param handle An existing handle to the same stream, which will be
     *                closed or reused for the request. The handle may be a
     *                write handle or a read handle. Leave this parameter
     *                \c NULL.
     *
     *  \returns A read handle to the specified stream if successful,
     *           \c NULL otherwise (if \p handle has been specified, an
     *           invalidated version of the handle will be returned). For a
     *           more detailed error description call StGetLastError().
     *
     *           The method returns a #NotFoundException, when the supplied
     *           query did not deliver any results and the target element or
     *           area in the stream could not be identified. <b>Note that
     *           stream segments are only visible to a query after the
     *           processing by the server has been completed!<\b>. For that
     *           reason you may encounter a #NotFoundException for stream
     *           elements that were only recently send to the server.
     *           Queries working on sequence numbers return an
     *           #OperationInProgressException in that case.
     *
     *           If successful, the handle will point to the exact entry
     *           requested by the <\c type, \c value>-pair.
     *
     *  \remarks A stream may have an arbitrary number of open stream handles.
     *
     *  \remarks Do not attempt to write data with a handle received from this
     *           method. The data will be lost.
     *
     *  \remarks Use this method only to create an initial read handle to
     *           a stream, that is supply the \p session and \p stream
     *           parameters and set handle to \c NULL. StGetNextEntryFast() and
     *           StGetPreviousEntryFast() will take care of proceeding the
     *           handle along the stream.
     *
     *  \deprecated <b>Before 3.1:</b> If successful, the handle will \b NOT
     *           point to the exact entry requested by the
     *           <\c type, \c value>-pair. The method only guarantees that the
     *           requested entry is in the data delivered by the server for
     *           this request. To find the desired entry, use
     *           StGetNextEntryFast() on the returned handle and scan over the
     *           entries manually.
     *
     *  \since 3.0
     *
     *  \see StStreamRegister()
     *  \see StStreamAppend()
     *  \see StStreamClose()
     *  \see StGetNextEntryFast()
     *  \see StGetPreviousEntryFast()
     */
    SIMUTRACE_API
    StreamHandle StStreamOpen(SessionId session, StreamId stream,
                              QueryIndexType type, uint64_t value,
                              StreamAccessFlags flags, StreamHandle handle);


    /*! \brief Closes a handle to a stream.
     *
     *  This method closes the supplied stream handle.
     *
     *  \param handle The handle to be closed.
     *
     *  \returns \c _true if successful, \c _false otherwise. For a more
     *           detailed error description call StGetLastError().
     *
     *  \remarks When closing a write handle, the client will automatically
     *           pass any pending new entries to the server. This will persist
     *           the entries in the open store.
     *
     *  \remarks Failing to explicitly call this method can lead to data loss
     *           for remote clients. While local clients use shared memory to
     *           communicate new data to the server, remote clients
     *           (i.e., socket connections) have to actively send new data to
     *           the server.
     *
     *  \remarks Simutrace will close all handles when the active session or
     *           store is closed.
     *
     *  \since 3.0
     *
     *  \see StStreamAppend()
     *  \see StStreamOpen()
     */
    SIMUTRACE_API
    _bool StStreamClose(StreamHandle handle);


    /* Tracing API */

    /*! \brief Returns a pointer to the next entry in a stream.
     *
     *  This method moves the read/write pointer of the supplied handle to the
     *  next entry in the stream. The pointer returned by this method can be
     *  used to write or read (depending on the handle) a single trace entry.
     *
     *  \param handlePtr A pointer to the stream handle. The method may
     *                   allocate a new handle and update the supplied pointer.
     *
     *  \returns A pointer to the next trace entry if successful, \c NULL
     *           otherwise. For a more detailed error description call
     *           StGetLastError(). On error, the supplied handle will be
     *           invalidated. In contrast to a closed handle
     *           (see StStreamClose()), an invalidated handle is not freed and
     *           may be used to retry the operation. If successful, the caller
     *           must cast the pointer to the data type (e.g., #DataWrite64)
     *           for the type supplied at stream registration.
     *
     *  \remarks This method is not thread-safe when operating on the same
     *           handle.
     *
     *  \remarks When \b writing a new entry, call StSubmitEntryFast()
     *           afterwards. Each call to StGetNextEntryFast() must be paired
     *           with a matching call to StSubmitEntryFast(). When \b reading
     *           entries from the stream the caller must not invoke
     *           StSubmitEntryFast().
     *
     *  \remarks Simutrace reads and writes trace data in segments of fixed
     *           size (e.g., 64MiB). Whenever the handle reaches the end of a
     *           segment, Simutrace will proceed the handle to the next
     *           segment. This will include loading/submitting data from/to the
     *           server, causing a noticeable delay. Using appropriate access
     *           flags in the call to StStreamOpen() can significantly reduce
     *           latency and increase transfer rate for reads. See
     *           #StreamAccessFlags for more information.
     *
     *  \remarks Use StWriteVariableData() and StReadVariableData() to work
     *           with variable-sized data types.
     *
     *  \remarks For optimal performance, the compiler should run with inlining
     *           permitted.
     *
     *  \remarks When writing to a stream, the buffer space returned by this
     *           method is not zeroed and may contain arbitrary data. Do not
     *           attempt to read or interpret the data. Instead, simply
     *           overwrite it.
     *
     *  \warning Writing data to a stream with a read handle will lead to
     *           data loss.
     *
     *  \since 3.0
     *
     *  \see StGetNextEntry()
     *  \see StStreamAppend()
     *  \see StStreamOpen()
     *  \see StSubmitEntryFast()
     *  \see StWriteVariableData()
     *  \see StReadVariableData()
     */
    static inline void* StGetNextEntryFast(StreamHandle* handlePtr)
    {
        STASSERT(handlePtr != NULL);
        StreamHandle handle = *handlePtr;
        STASSERT(handle != NULL);
        STASSERT(!isVariableEntrySize(handle->entrySize));

        byte* entry = handle->entry;

        /* If the next entry points behind the segment's buffer end, we do not
           have enough space left for another entry. In that case, request
           a new segment from the server                                     */
        if (entry >= handle->segmentEnd) {
            if (handle->isReadOnly) {
                handle->accessFlags = handle->accessFlags &
                    (StreamAccessFlags)(~SafReverseRead);

                handle = StStreamOpen(INVALID_SESSION_ID, INVALID_STREAM_ID,
                                      QNextValidSequenceNumber,
                                      handle->sequenceNumber,
                                      handle->accessFlags, handle);
            } else {
                handle = StStreamAppend(INVALID_SESSION_ID, INVALID_STREAM_ID,
                                        handle);
            }

            *handlePtr = handle;
            if ((handle == NULL) || (handle->control == NULL)) {
                return NULL;
            }
            entry = handle->entry;
        }

        STASSERT((entry >= handle->segmentStart) &&
                 (entry < handle->segmentEnd));

        handle->entry = entry + handle->entrySize;
        return entry;
    }


    /*! \brief Returns a pointer to the next entry in a stream.
     *
     *  This method performs the same operation as StGetNextEntryFast() as part
     *  of the client library exported interface. It is slower than
     *  StGetNextEntryFast() but does not require C function inlining.
     *
     *  \since 3.1
     *
     *  \see StGetNextEntryFast()
     */
    SIMUTRACE_API
    void* StGetNextEntry(StreamHandle* handlePtr);


    /*! \brief Returns a pointer to the previous entry in a stream.
     *
     *  This method moves the read/write pointer of the supplied handle to the
     *  previous entry in the stream. The pointer returned by this method can
     *  only be used to \b read a single trace entry.
     *
     *  \param handlePtr A pointer to the stream read handle. The method may
     *                   allocate a new handle and update the supplied pointer.
     *
     *  \returns A pointer to the previous trace entry if successful, \c NULL
     *           otherwise. For a more detailed error description call
     *           StGetLastError(). On error, the supplied handle will be
     *           invalidated. In contrast to a closed handle
     *           (see StStreamClose()), an invalidated handle is not freed and
     *           may be used to retry the operation. If successful, the caller
     *           must cast the pointer to the data type (e.g., #DataWrite64)
     *           for the type supplied at stream registration.
     *
     *  \remarks This method is not thread-safe when operating on the same
     *           handle.
     *
     *  \remarks The caller must not invoke StSubmitEntryFast().
     *
     *  \remarks Simutrace reads and writes trace data in segments of fixed
     *           size (e.g., 64MiB). Whenever the handle reaches the end of a
     *           segment, Simutrace will proceed the handle to the next
     *           segment. This will include loading/submitting data from/to the
     *           server, causing a noticeable delay. Using appropriate access
     *           flags in the call to StStreamOpen() can significantly reduce
     *           latency and increase transfer rate for reads. See
     *           #StreamAccessFlags for more information.
     *
     *  \remarks Use StWriteVariableData() and StReadVariableData() to work
     *           with variable-sized data types. Reading variable-sized data
     *           backwards is not supported.
     *
     *  \remarks For optimal performance, the compiler should run with inlining
     *           permitted.
     *
     *  \warning Writing data to a stream with a read handle will lead to
     *           data loss.
     *
     *  \since 3.1
     *
     *  \see StStreamOpen()
     *  \see StReadVariableData()
     */
    static inline void* StGetPreviousEntryFast(StreamHandle* handlePtr)
    {
        STASSERT(handlePtr != NULL);
        StreamHandle handle = *handlePtr;
        STASSERT(handle != NULL);
        STASSERT(!isVariableEntrySize(handle->entrySize));
        STASSERT(handle->isReadOnly == _true);

        byte* entry = handle->entry;

        /* If the previous entry points before the segment's buffer start, we
           do not have enough space left for another entry. In that case,
           request a new segment from the server                             */
        if (entry < handle->segmentStart) {
            handle->accessFlags = handle->accessFlags | SafReverseRead;

            handle = StStreamOpen(INVALID_SESSION_ID, INVALID_STREAM_ID,
                                  QPreviousValidSequenceNumber,
                                  handle->sequenceNumber,
                                  handle->accessFlags, handle);

            *handlePtr = handle;
            if ((handle == NULL) || (handle->control == NULL)) {
                return NULL;
            }
            entry = handle->entry;
        }

        STASSERT((entry >= handle->segmentStart) &&
                 (entry <= handle->segmentEnd - handle->entrySize));

        handle->entry = entry - handle->entrySize;
        return entry;
    }


    /*! \brief Returns a pointer to the previous entry in a stream.
     *
     *  This method performs the same operation as StGetPreviousEntryFast() as
     *  part of the client library exported interface. It is slower than
     *  StGetNextPreviousFast() but does not require C function inlining.
     *
     *  \since 3.1
     *
     *  \see StGetPreviousEntryFast()
     */
    SIMUTRACE_API
    void* StGetPreviousEntry(StreamHandle* handlePtr);


    /*! \brief Completes an entry.
     *
     *  This method builds the closing operation when writing a new entry to
     *  a stream, signaling Simutrace that the entry has been fully written and
     *  is valid.
     *
     *  \param handle The write handle of the stream, which has been used in
     *                matching call to StGetNextEntryFast().
     *
     *  \remarks When \b writing a new entry, call StSubmitEntryFast()
     *           afterwards. Each call to StGetNextEntryFast() must be paired
     *           with a matching call to StSubmitEntryFast(). When \b reading
     *           entries from the stream the caller must not invoke
     *           StSubmitEntryFast().
     *
     *  \since 3.0
     *
     *  \see StSubmitEntry()
     *  \see StGetNextEntryFast()
     */
    static inline void StSubmitEntryFast(StreamHandle handle)
    {
        handle->control->rawEntryCount++;
    }


    /*! \brief Completes an entry.
     *
     *  This method performs the same operation as StSubmitEntryFast() as part
     *  of the client library exported interface. It is slower than
     *  StSubmitEntryFast() but does not require C function inlining.
     *
     *  \since 3.1
     *
     *  \see StSubmitEntryFast()
     */
    SIMUTRACE_API
    void StSubmitEntry(StreamHandle handle);


    /*! \brief Writes variable-sized data to a stream.
     *
     *  In Simutrace, entries are required to be of fixed size. This method
     *  enables tracing of variable-sized data such as strings. To trace
     *  entries that contain variable-sized fields, the data fields should be
     *  replaced with fixed-size reference fields. The data itself can then be
     *  traced with this method.
     *
     *  \param handlePtr Pointer to a write handle for the stream that will
     *                   receive the variable-sized data.
     *
     *  \param sourceBuffer Pointer to a buffer containing the variable-sized
     *                      data.
     *
     *  \param sourceLength Length of the variable-sized data.
     *
     *  \param referenceOut Pointer to a reference field receiving the
     *                      reference to the variable-sized data in the given
     *                      stream. This parameter may be NULL.
     *
     *  \returns The number of bytes written, \p sourceLength if successful.
     *           For a more detailed error description call StGetLastError().
     *
     *  \remarks The returned reference is only valid for the supplied stream.
     *           The caller thus have to remember the stream for which the
     *           reference is valid when reading the data.
     *
     *  \remarks The destination stream must be created with a variable
     *           entry size, see StStreamRegister() and makeVariableEntrySize().
     *
     *  \deprecated <b>Before 3.1:</b> The parameter \p referenceOut must not
     *           be NULL.
     *
     *  \since 3.0
     *
     *  \see makeVariableEntrySize()
     *  \see StReadVariableData()
     */
    SIMUTRACE_API
    size_t StWriteVariableData(StreamHandle* handlePtr, byte* sourceBuffer,
                               size_t sourceLength, uint64_t* referenceOut);


    /*! \brief Reads variable-sized data.
     *
     *  This method reads variable-sized data from the supplied stream given
     *  a reference, originally returned by StWriteVariableData().
     *
     *  \param handlePtr Pointer to a read handle for the stream that stores
     *                   the variable-sized data.
     *
     *  \param reference Reference to the variable-sized data in the given
     *                   stream.
     *
     *  \param destinationBuffer Pointer to a buffer receiving the
     *                           variable-sized data. The buffer needs provide
     *                           enough space. Supply \c NULL to determine
     *                           the length of the data.
     *
     *  \returns The length of the referenced variable-sized data in bytes, if
     *           successful, \c -1 otherwise. For a more detailed error
     *           description call StGetLastError().
     *
     *  \since 3.0
     *
     *  \see StWriteVariableData()
     */
    SIMUTRACE_API
    size_t StReadVariableData(StreamHandle* handlePtr, uint64_t reference,
                              byte* destinationBuffer);

#ifdef __cplusplus
}
}
#endif

#endif