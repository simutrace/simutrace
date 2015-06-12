/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
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
#include "SimuStor.h"

#include "ClientStore.h"

#include "ClientObject.h"
#include "ClientSession.h"
#include "StaticStream.h"
#include "DynamicStream.h"

namespace SimuTrace
{

    ClientStore::ClientStore(ClientSession& session, const std::string& name,
                             bool alwaysCreate, bool open) :
        Store(CLIENT_STORE_ID, name),
        ClientObject(session),
        _dynamicStreamIdBoundary(INVALID_STREAM_ID)
    {
        Message response = {0};
        ClientPort& port = _getPort();

        // Send the request to open the store
        port.call(&response, RpcApi::CCV_StoreCreate, name,
                  (alwaysCreate && !open) ? _true : _false,
                  (open) ? _true : _false);

        try {
            _replicateConfiguration();

        } catch (...) {
            port.call(nullptr, RpcApi::CCV_StoreClose);

            throw;
        }
    }

    ClientStore::~ClientStore()
    {

    }

    StreamBuffer* ClientStore::_replicateStreamBuffer(BufferId buffer)
    {
        Message response = {0};
        ClientPort& port = _getPort();
        std::unique_ptr<StreamBuffer> buf;

        port.call(&response, RpcApi::CCV_StreamBufferQuery, buffer, _true);

        uint32_t numSegments = response.parameter0;
        size_t segmentSize   = response.handles.parameter1;

        // If the payload contains handles, the connection between the
        // server and the client supports shared memory buffers.
        if (response.payloadType == MessagePayloadType::MptHandles) {
            assert(response.handles.handles != nullptr);
            assert(response.handles.handles->size() ==
                   response.handles.handleCount);

            ThrowOn(response.handles.handleCount != 1,
                    RpcMessageMalformedException);

            Handle& bufferHandle = response.handles.handles->at(0);

            buf = std::unique_ptr<StreamBuffer>(
                    new StreamBuffer(buffer, segmentSize, numSegments,
                                     bufferHandle));
        } else {
            buf = std::unique_ptr<StreamBuffer>(
                    new StreamBuffer(buffer, segmentSize, numSegments, false));
        }

        StreamBuffer* pbuf = buf.get(); // Make a shortcut for the return value
        _addStreamBuffer(buf);

        return pbuf;
    }

    Stream* ClientStore::_replicateStream(StreamId stream)
    {
        assert(stream < _dynamicStreamIdBoundary);

        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV_StreamQuery, stream);

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                (response.data.payloadLength != sizeof(StreamQueryInformation)),
                RpcMessageMalformedException);

        assert(response.data.payload != nullptr);

        StreamBuffer* buffer = _getStreamBuffer(response.parameter0);
        ThrowOn(buffer == nullptr, NotFoundException);

        StreamQueryInformation* desc =
            reinterpret_cast<StreamQueryInformation*>(response.data.payload);

        std::unique_ptr<Stream> str(new StaticStream(stream, desc->descriptor,
                                                     *buffer, getSession()));

        Stream* pstr = str.get(); // Make a shortcut for the return value
        _addStream(str);

        return pstr;
    }

    void ClientStore::_replicateConfiguration()
    {
        // We only replicate the stream buffers to allow an early fail
        // if the client does not have enough buffer space to hold the
        // default stream buffer(s). Streams and other objects are
        // replicated on-demand.
        std::vector<BufferId> buffers;
        _enumerateStreamBuffers(buffers);

        // The server should already created a stream buffer for us. This
        // exception should never be thrown with a good behaving server.
        ThrowOn(buffers.empty(), Exception, "Expected at least one "
            "stream buffer during replication of the session configuration, "
            "but the server did not return one.");

        for (auto buffer : buffers) {
            if (this->Store::_getStreamBuffer(buffer) == nullptr) {
                _replicateStreamBuffer(buffer);
            }
        }
    }

    std::unique_ptr<StreamBuffer> ClientStore::_createStreamBuffer(
        size_t segmentSize, uint32_t numSegments)
    {
        Message response = {0};
        ClientPort& port = _getPort();
        std::unique_ptr<StreamBuffer> buffer = nullptr;

        port.call(&response, RpcApi::CCV_StreamBufferRegister, numSegments,
                  static_cast<uint64_t>(segmentSize));

        BufferId id = static_cast<BufferId>(response.parameter0);

        // If the payload contains handles, the connection between the
        // server and the client supports shared memory buffers.
        if (response.payloadType == MessagePayloadType::MptHandles) {
            assert(response.handles.handles != nullptr);
            assert(response.handles.handles->size() ==
                   response.handles.handleCount);

            ThrowOn(response.handles.handleCount != 1,
                    RpcMessageMalformedException);

            Handle& bufferHandle = response.handles.handles->at(0);

            buffer = std::unique_ptr<StreamBuffer>(
                     new StreamBuffer(id, segmentSize, numSegments,
                                      bufferHandle));
        } else {
            buffer = std::unique_ptr<StreamBuffer>(
                     new StreamBuffer(id, segmentSize, numSegments, false));
        }

        return buffer;
    }

    std::unique_ptr<Stream> ClientStore::_createStream(StreamId id,
        StreamDescriptor& desc, BufferId buffer)
    {
        assert(id == INVALID_STREAM_ID);

        Message response = {0};
        ClientPort& port = _getPort();

        StreamBuffer* buf = _getStreamBuffer(buffer);
        ThrowOnNull(buf, NotFoundException);

        if (IsSet(desc.flags, SfDynamic)) {
            const DynamicStreamDescriptor& dyndesc =
                reinterpret_cast<DynamicStreamDescriptor&>(desc);




            // For regular streams the server generates a valid id. For
            // dynamic streams we have to do this on our own, as dynamic
            // streams are local to the client. The ids must not collide!
            // The server generates ids for regular streams in the lower
            // positive integer range. We therefore generate the ids for
            // dynamic streams in the high positive integer range. The
            // limit for the number of streams (see Version.h) prevents a
            // collision.
            StreamId sid = --_dynamicStreamIdBoundary;

        #ifdef _DEBUG
            std::vector<StreamId> ids;
            _enumerateStreams(ids, StreamEnumFilter::SefRegular);

            StreamId max = *std::max_element(ids.cbegin(), ids.cend());
            assert(max < sid);
        #endif

            return std::unique_ptr<Stream>(
                    new DynamicStream(sid, dyndesc, *buf, getSession()));
        } else {
            port.call(&response, RpcApi::CCV_StreamRegister, &desc,
                      sizeof(StreamDescriptor), buffer);

            StreamId rid = static_cast<StreamId>(response.parameter0);

            return std::unique_ptr<Stream>(
                        new StaticStream(rid, desc, *buf, getSession()));
        }
    }

    void ClientStore::_enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV_StreamBufferEnumerate);

        uint32_t n = response.parameter0;

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                ((response.data.payloadLength % sizeof(BufferId)) != 0) ||
                (response.data.payloadLength / sizeof(BufferId) != n),
                RpcMessageMalformedException);

        if (n > 0) {
            assert(response.data.payload != nullptr);
            assert(n <= SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS);
            BufferId* buffer = reinterpret_cast<BufferId*>(response.data.payload);

            std::vector<BufferId> ids(buffer, buffer + n);
            std::swap(ids, out);
        } else {
            out.clear();
        }
    }

    void ClientStore::_enumerateStreams(std::vector<StreamId>& out,
                                        StreamEnumFilter filter) const
    {
        std::vector<StreamId> ids;

        // Request the ids for the static streams. Since we are replicating
        // static streams on-demand, we cannot just query our local store but
        // must ask the server.
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV_StreamEnumerate, filter);

        uint32_t n = response.parameter0;

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                ((response.data.payloadLength % sizeof(StreamId)) != 0) ||
                (response.data.payloadLength / sizeof(StreamId) != n),
                RpcMessageMalformedException);

        if (n > 0) {
            assert(response.data.payload != nullptr);
            assert(n <= SIMUTRACE_STORE_MAX_NUM_STREAMS);
            StreamId* buffer = reinterpret_cast<StreamId*>(response.data.payload);

            ids.assign(buffer, buffer + n);
        }

        // Now add the dynamic streams that this client session possesses.
        std::vector<Stream*> dynStreams;
        this->Store::_enumerateStreams(dynStreams, StreamEnumFilter::SefDynamic);
        for (auto stream : dynStreams) {
            assert(IsSet(stream->getFlags(), StreamFlags::SfDynamic));
            assert(!IsSet(stream->getFlags(), StreamFlags::SfHidden));

            ids.push_back(stream->getId());
        }

        std::swap(ids, out);
    }

    StreamBuffer* ClientStore::_getStreamBuffer(BufferId id)
    {
        StreamBuffer* buffer = this->Store::_getStreamBuffer(id);
        if (buffer == nullptr) {
            // We might not have the buffer replicated, yet. Check the server.
            // As this can fail if the id is not valid, we catch any errors
            // here and just return nullptr.
            try {
                buffer = _replicateStreamBuffer(id);
            } catch (...) { }
        }

        return buffer;
    }

    Stream* ClientStore::_getStream(StreamId id)
    {
        Stream* stream = this->Store::_getStream(id);
        if (stream == nullptr) {
            // We might not have the stream replicated, yet. Check the server.
            // As this can fail if the id is not valid, we catch any errors
            // here and just return nullptr.
            try {
                stream = _replicateStream(id);
            } catch (...) { }
        }

        return stream;
    }

    void ClientStore::detach(SessionId session)
    {
        LockScopeExclusive(_lock);

        // We first flush all streams to get any pending data to the server.
        // Afterwards, we can close the store. The server will process the
        // data we send here (and any other pending data), before closing the
        // server instance of the store.
        std::vector<Stream*> streams;
        this->Store::_enumerateStreams(streams, StreamEnumFilter::SefAll);

        for (auto i = 0; i < streams.size(); ++i) {
            assert(streams[i] != nullptr);
            ClientStream* stream = reinterpret_cast<ClientStream*>(streams[i]);

            stream->flush();
        }

        ClientPort& port = _getPort();

        port.call(nullptr, RpcApi::CCV_StoreClose);
    }

    ClientStore* ClientStore::create(ClientSession& session,
                                     const std::string& name,
                                     bool alwaysCreate)
    {
        return new ClientStore(session, name, alwaysCreate, false);
    }

    ClientStore* ClientStore::open(ClientSession& session,
                                   const std::string& name)
    {
        return new ClientStore(session, name, false, true);
    }

}
