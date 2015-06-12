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
#include "SimuStor.h"

#include "ServerSessionWorker.h"

#include "ServerSession.h"
#include "ServerStore.h"
#include "ServerStream.h"
#include "ServerStreamBuffer.h"

namespace SimuTrace
{

    struct ServerSessionWorker::MessageContext
    {
        ServerSessionWorker& worker;

        uint32_t code;
        Message msg;

        MessageContext(ServerSessionWorker& sessionWorker) :
            worker(sessionWorker) { }
    };

    ServerSessionWorker::ServerSessionWorker(ServerSession& session,
                                             std::unique_ptr<Port>& port) :
        _session(session),
        _port()
    {
        _initializeHandlerMap();

        ThrowOnNull(dynamic_cast<ServerPort*>(port.get()),
                    ArgumentNullException, "port");

        // Take ownership of port. Do not throw exceptions afterwards!
        _port = std::unique_ptr<ServerPort>(
            static_cast<ServerPort*>(port.release()));
    }

    ServerSessionWorker::~ServerSessionWorker()
    {

    }

    void ServerSessionWorker::_initializeHandlerMap()
    {
        std::map<int, MessageHandler>& m = _handlers; // short alias

        /* =======  Session API  ======= */
        m[RpcApi::CCV32_SessionClose]            = _handleSessionClose;
        m[RpcApi::CCV32_SessionSetConfiguration] = _handleSessionSetConfiguration;

        /* =======  Store API  ======= */
        m[RpcApi::CCV32_StoreCreate]            = _handleStoreCreate;
        m[RpcApi::CCV32_StoreClose]             = _handleStoreClose;

        m[RpcApi::CCV32_StreamBufferRegister]   = _handleStreamBufferRegister;
        m[RpcApi::CCV32_StreamBufferEnumerate]  = _handleStreamBufferEnumerate;
        m[RpcApi::CCV32_StreamBufferQuery]      = _handleStreamBufferQuery;

        /* =======  Stream API  ======= */
        m[RpcApi::CCV32_StreamRegister]         = _handleStreamRegister;
        m[RpcApi::CCV31_StreamEnumerate]        = _handleStreamEnumerate;
        m[RpcApi::CCV32_StreamEnumerate]        = _handleStreamEnumerate;
        m[RpcApi::CCV32_StreamQuery]            = _handleStreamQuery;
        m[RpcApi::CCV32_StreamAppend]           = _handleStreamAppend;
        m[RpcApi::CCV30_StreamCloseAndOpen]     = _handleStreamCloseAndOpen;
        m[RpcApi::CCV32_StreamCloseAndOpen]     = _handleStreamCloseAndOpen;
        m[RpcApi::CCV32_StreamClose]            = _handleStreamClose;
    }

    void ServerSessionWorker::_acknowledgeSessionCreate()
    {
        Message msg = {0};

        msg.setupEmpty();
        msg.sequenceNumber = _port->getLastSequenceNumber();
        msg.response.status = RpcApi::SC_Success;
        msg.parameter0 = _session.getLocalId();

        _port->ret(msg);
    }

    bool ServerSessionWorker::_handleSessionClose(MessageContext& ctx)
    {
        TEST_REQUEST_V32(SessionClose, ctx.msg);

        // We break out of the processing loop and return from the thread
        // function. This will invoke the thread finish handler, which
        // closes the session.
        ctx.worker.stop();
        return true;
    }

    bool ServerSessionWorker::_handleSessionSetConfiguration(
        MessageContext& ctx)
    {
        TEST_REQUEST_V32(SessionSetConfiguration, ctx.msg);
        ServerSession& session = ctx.worker._session;

        std::string setting = std::string(
            static_cast<char*>(ctx.msg.data.payload),
            ctx.msg.data.payloadLength);

        session.applySetting(setting);
        return true;
    }

    bool ServerSessionWorker::_handleStoreCreate(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StoreCreate, ctx.msg);
        ServerSession& session = ctx.worker._session;

        bool alwaysCreate = (ctx.msg.parameter0 == _true);
        bool open = (ctx.msg.data.parameter1 == _true);
        std::string specifier = std::string(
            static_cast<char*>(ctx.msg.data.payload),
            ctx.msg.data.payloadLength);

        if (open) {
            assert(!alwaysCreate);
            session.openStore(specifier);
        } else {
            session.createStore(specifier, alwaysCreate);
        }

        return true;
    }

    bool ServerSessionWorker::_handleStoreClose(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StoreClose, ctx.msg);
        ServerSession& session = ctx.worker._session;

        session.closeStore();
        return true;
    }

    bool ServerSessionWorker::_handleStreamBufferRegister(
        MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamBufferRegister, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        uint32_t numSegments = ctx.msg.parameter0;
        size_t segmentSize = static_cast<size_t>(ctx.msg.embedded.parameter1);

        BufferId id = session.registerStreamBuffer(segmentSize, numSegments);

        // Send Response
        // If the channel between the client and the server supports
        // handle transfer, we can use shared memory based stream buffers,
        // otherwise we have to utilize slower copy based buffers.
        if (ctx.worker.channelSupportsSharedMemory()) {
            std::vector<Handle> handleList;
            const ServerStreamBuffer& buffer =
                dynamic_cast<const ServerStreamBuffer&>(
                session.getStreamBuffer(id));

            handleList.push_back(buffer.getBufferHandle());

            port->ret(ctx.msg, RpcApi::SC_Success, handleList, id);
        } else {
            port->ret(ctx.msg, RpcApi::SC_Success, id);
        }

        return false;
    }

    bool ServerSessionWorker::_handleStreamBufferEnumerate(
        MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamBufferEnumerate, ctx.msg);
        Session& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        std::vector<BufferId> buffers;
        session.enumerateStreamBuffers(buffers);

        port->ret(ctx.msg, RpcApi::SC_Success, buffers.data(),
                  static_cast<uint32_t>(buffers.size() * sizeof(BufferId)),
                  static_cast<uint32_t>(buffers.size()));

        return false;
    }

    bool ServerSessionWorker::_handleStreamBufferQuery(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamBufferQuery, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        const ServerStreamBuffer& buffer =
            dynamic_cast<const ServerStreamBuffer&>(
            session.getStreamBuffer(ctx.msg.parameter0));

        if (ctx.worker.channelSupportsSharedMemory()) {
            std::vector<Handle> handleList;

            handleList.push_back(buffer.getBufferHandle());

            port->ret(ctx.msg, RpcApi::SC_Success, handleList,
                      buffer.getNumSegments(),
                      static_cast<uint32_t>(buffer.getSegmentSize()));
        } else {
            port->ret(ctx.msg, RpcApi::SC_Success, buffer.getNumSegments(),
                      static_cast<uint32_t>(buffer.getSegmentSize()));
        }

        return false;
    }

    bool ServerSessionWorker::_handleStreamRegister(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamRegister, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        StreamDescriptor* desc =
            static_cast<StreamDescriptor*>(ctx.msg.data.payload);
        BufferId buffer = ctx.msg.parameter0;

        // Dynamic flag should always be unset, because dynamic streams are
        // handled solely on the client-side and the library should not
        // pass the request to the server.
        assert(!IsSet(desc->flags, SfDynamic));

        // Forbid the client to create hidden streams by always overriding it.
        // Also dynamic streams are not supported on the server-side. Since
        // these are the only flags at the moment, we simply clear the whole
        // flag field.
        desc->flags = SfNone;

        StreamId id = session.registerStream(*desc, buffer);

        port->ret(ctx.msg, RpcApi::SC_Success, id);
        return false;
    }

    bool ServerSessionWorker::_handleStreamEnumerate(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamBufferEnumerate, ctx.msg);
        Session& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        // In this version, we do not allow any client to receive a list of
        // hidden streams.
        std::vector<StreamId> streams;
        session.enumerateStreams(streams, StreamEnumFilter::SefRegular);

        port->ret(ctx.msg, RpcApi::SC_Success, streams.data(),
                  static_cast<uint32_t>(streams.size() * sizeof(BufferId)),
                  static_cast<uint32_t>(streams.size()));

        return false;
    }

    bool ServerSessionWorker::_handleStreamQuery(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamQuery, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        Stream& stream = session.getStream(ctx.msg.parameter0);

        StreamQueryInformation information;
        stream.queryInformation(information);

        port->ret(ctx.msg, RpcApi::SC_Success, &information,
                  static_cast<uint32_t>(sizeof(StreamQueryInformation)),
                  stream.getStreamBuffer().getId());

        return false;
    }

    bool ServerSessionWorker::_handleStreamAppend(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamAppend, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        StreamId id = ctx.msg.parameter0;
        ServerStream& stream = dynamic_cast<ServerStream&>(
            session.getStream(id));

        SegmentId seg = INVALID_SEGMENT_ID;
        StreamSegmentId sqn;

        // At this point the control element already needs to be
        // updated via shared memory or through a payload allocator.

        sqn = stream.append(session.getLocalId(), &seg);

        try {
            if (ctx.worker.channelSupportsSharedMemory() ||
                (seg == INVALID_SEGMENT_ID)) {

                port->ret(ctx.msg, RpcApi::SC_Success, nullptr, 0, sqn, seg);
            } else {
                // If the channel does not support shared memory, we send the
                // initialized control element to the client.
                StreamBuffer& buffer = stream.getStreamBuffer();
                SegmentControlElement* control = buffer.getControlElement(seg);
                port->ret(ctx.msg, RpcApi::SC_Success, control,
                          sizeof(SegmentControlElement), sqn, seg);
            }

        } catch (...) {
            assert(sqn != INVALID_STREAM_SEGMENT_ID);
            stream.close(session.getLocalId(), sqn, nullptr, true);

            throw;
        }

        return false;
    }

    bool ServerSessionWorker::_handleStreamCloseAndOpen(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamCloseAndOpen, ctx.msg);
        ServerSession& session = ctx.worker._session;
        ServerPort* port = ctx.worker._port.get();

        // Close the specified sequence number first. Note that we do not
        // accept any segment data for this close. The operation is thus only
        // suited for read-only segments! Use a dedicated close instead.
        StreamId id = ctx.msg.parameter0;
        StreamSegmentId closeSqn = ctx.msg.data.parameter1;

        ServerStream& stream = dynamic_cast<ServerStream&>(
            session.getStream(id));

        if (closeSqn != INVALID_STREAM_SEGMENT_ID) {
            stream.close(session.getLocalId(), closeSqn, nullptr);
        }

        // Do the open query
        StreamOpenQuery* query = reinterpret_cast<StreamOpenQuery*>(
            ctx.msg.data.payload);

        SegmentId seg = INVALID_SEGMENT_ID;
        StreamSegmentId sqn;
        size_t offset;

        // The open must be performed synchronously, because the remote
        // call is synchronous. However, the open may be performed
        // asynchronously, despite the specified flag. This may be the case if
        // there is already a read in progress. We then need to wait for it to
        // finish.
        query->flags = static_cast<StreamAccessFlags>(
            query->flags | StreamAccessFlags::SafSynchronous);

        ctx.worker._wait.reset();

        sqn = stream.open(session.getLocalId(), query->type,
                          query->value, query->flags, &seg,
                          &offset, &ctx.worker._wait);

        StreamBuffer& buffer = stream.getStreamBuffer();

        assert((ctx.worker._wait.getCount() == 0));
        assert((offset <= buffer.getSegmentSize()) &&
               (offset <= std::numeric_limits<uint32_t>::max()));

        // We now have the segment in memory and need to find the exact entry
        // that the caller requests. The open operation has already transformed
        // the query to be prepared for findOffset().
        try {
            if (ctx.worker.channelSupportsSharedMemory() ||
                (seg == INVALID_SEGMENT_ID)) {

                if (ctx.code == RpcApi::CCV30_StreamCloseAndOpen) {   // v3.0
                    port->ret(ctx.msg, RpcApi::SC_Success, nullptr, 0, sqn,
                              seg);
                } else {                                              // v3.1+
                    port->ret(ctx.msg, RpcApi::SC_Success, nullptr, 0, seg,
                              static_cast<uint32_t>(offset));
                }
            } else {
                // If the channel does not support shared memory, we send whole
                // segment to the client.

                size_t size;
                void* segment = buffer.getSegmentAsPayload(seg, size);

                if (ctx.code == RpcApi::CCV30_StreamCloseAndOpen) {   // v3.0
                    port->ret(ctx.msg, RpcApi::SC_Success, segment,
                              static_cast<uint32_t>(size), sqn, seg);
                } else {                                              // v3.1+
                    port->ret(ctx.msg, RpcApi::SC_Success, segment,
                              static_cast<uint32_t>(size), seg,
                              static_cast<uint32_t>(offset));
                }

            }

        } catch (...) {
            assert(sqn != INVALID_STREAM_SEGMENT_ID);
            stream.close(session.getLocalId(), sqn, nullptr, true);

            throw;
        }

        return false;
    }

    bool ServerSessionWorker::_handleStreamClose(MessageContext& ctx)
    {
        TEST_REQUEST_V32(StreamClose, ctx.msg);
        ServerSession& session = ctx.worker._session;

        StreamId id = ctx.msg.parameter0;
        StreamSegmentId sseg = ctx.msg.data.parameter1;
        ServerStream& stream = dynamic_cast<ServerStream&>(
            session.getStream(id));

        // At this point the control element already needs to be
        // updated via shared memory or through a payload allocator.

        stream.close(session.getLocalId(), sseg);
        return true;
    }

    void ServerSessionWorker::_messagePayloadAllocator(Message& msg, bool free,
                                                       void* args)
    {
        // We employ the custom allocator to use the real segment
        // buffer for copy stream buffers as payload receive buffer. We
        // therefore do not need to free the buffer.
        if (free) {
            return;
        }

        ServerSession* session = reinterpret_cast<ServerSession*>(args);
        uint16_t ver = session->getPeerApiVersion();
        uint32_t code = RPC_VER_AND_CODE(ver, msg.request.controlCode);

        switch (code)
        {
            case RpcApi::CCV32_StreamAppend:
            case RpcApi::CCV32_StreamClose: {
                TEST_REQUEST_V32(StreamAppend, msg); // Same characteristics

                StreamId id = msg.parameter0;
                ServerStream& stream = dynamic_cast<ServerStream&>(
                    session->getStream(id));

                // It is important to NOT use the ServerStreamBuffer here.
                // Otherwise, getControlElement() might return a pointer to the
                // copied control element (in the internal data structures of
                // the buffer), if the segment has already been submitted.
                StreamBuffer& buffer = stream.getStreamBuffer();

                StreamSegmentId sqn = (code == RpcApi::CCV32_StreamAppend) ?
                    stream.getCurrentSegmentId() :
                    msg.data.parameter1;

                SegmentId seg = stream.getBufferMapping(sqn);

                size_t lineSize;

                // This will throw if the mapping is not established, because
                // then seg is INVALID_SEGMENT_ID.
                msg.data.payload = buffer.getSegmentAsPayload(seg, lineSize);

                ThrowOn(msg.data.payloadLength != lineSize,
                        RpcMessageMalformedException);

                break;
            }
        }
    }

    int ServerSessionWorker::_run()
    {
        Environment::set(&_session.getEnvironment());

        LogInfo("<client: %s> Session worker thread "
                "successfully created (id: %d).",
                _port->getAddress().c_str(),
                ThreadBase::getCurrentSystemThreadId());

        try {
            uint16_t ver = _session.getPeerApiVersion();

            // Complete the RPC call, which the client send to signal a
            // successful initialization of the session.
            _acknowledgeSessionCreate();

            // Main request processing loop

            MessageContext ctx(*this);
            while (!shouldStop()) {
                memset(&ctx.msg, 0, sizeof(Message));

                // If the channel does not support shared memory, we use a
                // payload allocator to speed up segment transfer.
                if (!channelSupportsSharedMemory()) {
                    ctx.msg.allocatorArgs = &_session;
                    ctx.msg.allocator     = _messagePayloadAllocator;
                }

                _port->wait(ctx.msg);

                try {

                    ctx.code = RPC_VER_AND_CODE(ver,
                        ctx.msg.request.controlCode);

                    auto it = _handlers.find(ctx.code);
                    ThrowOn(it == _handlers.end(), NotSupportedException);

                    // Call the handler method to dispatch the message
                    MessageHandler handler = it->second;
                    assert(handler != nullptr);

                    if (handler(ctx)) {
                        _port->ret(ctx.msg, RpcApi::SC_Success);
                    }

                } catch (const SocketException& e) {

                    if (shouldStop()) {
                        return 0;
                    }

                    // We encountered a socket exception. This is most probably
                    // caused by a failed transmission of the response. We drop
                    // the session.

                    LogError("<client: %s, code: 0x%x> SocketException: '%s'. "
                             "Dropping connection.",
                             _port->getAddress().c_str(),
                             ctx.msg.request.controlCode,
                             e.what());

                    return -1;

                } catch (const Exception& e) {

                    if (shouldStop()) {
                        return 0;
                    }

                    // We catch all other Simutrace related exceptions and
                    // return these to the caller.
                    //
                    // We do not catch std::exception here, because we expect
                    // std::exceptions to denote internal server errors. In
                    // that case, we better close the session.

                    try {
                        LogWarn("<client: %s, code: 0x%x> Exception: '%s'",
                                _port->getAddress().c_str(),
                                ctx.msg.request.controlCode,
                                e.what());

                        _port->ret(ctx.msg, RpcApi::SC_Failed, e.what(),
                                   static_cast<uint32_t>(e.whatLength()),
                                   e.getErrorClass(), e.getErrorCode());

                    } catch (const std::exception& e) {

                        if (shouldStop()) {
                            return 0;
                        }

                        // This is a second level exception. Drop the connection.

                        LogError("<client: %s, code: 0x%x> Failed to transmit "
                                 "exception: '%s'. Dropping connection.",
                                 _port->getAddress().c_str(),
                                 ctx.msg.request.controlCode,
                                 e.what());

                        return -1;
                    }
                }

            } // end processing loop

        } catch (const std::exception& e) {

            if (shouldStop()) {
                return 0;
            }

            LogError("<client: %s> Exception in session "
                     "worker thread: '%s'. Dropping connection.",
                     _port->getAddress().c_str(),
                     e.what());

            return -1;
        }

        return 0;
    }

    void ServerSessionWorker::_onFinalize()
    {
        LogDebug("Finalizing session worker thread <id: %d>.",
                 ThreadBase::getCurrentSystemThreadId());

        _session.detach();
    }

    void ServerSessionWorker::close()
    {
        // Set shouldStop
        stop();

        // Disconnect the port so the worker will break out of any
        // communication or wait for new messages.
        _port->close();
    }

    ServerSession& ServerSessionWorker::getSession() const
    {
        return _session;
    }

    bool ServerSessionWorker::channelSupportsSharedMemory() const
    {
        return IsSet(_port->getChannelCaps(), CCapHandleTransfer);
    }

}