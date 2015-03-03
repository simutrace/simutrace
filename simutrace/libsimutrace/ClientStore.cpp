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
#include "ClientStream.h"

namespace SimuTrace
{

    ClientStore::ClientStore(ClientSession& session, const std::string& name,
                             bool alwaysCreate, bool open) :
        Store(CLIENT_STORE_ID, name),
        ClientObject(session)
    {
        Message response = {0};
        ClientPort& port = _getPort();

        // Send the request to open the store
        port.call(&response, RpcApi::CCV31_StoreCreate, name,
                  (alwaysCreate && !open) ? _true : _false,
                  (open) ? _true : _false);

        try {
            _replicateConfiguration(false);

        } catch (...) {
            port.call(nullptr, RpcApi::CCV31_StoreClose);

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

        port.call(&response, RpcApi::CCV31_StreamBufferQuery, buffer, _true);

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
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV31_StreamQuery, stream);

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                (response.data.payloadLength != sizeof(StreamQueryInformation)),
                RpcMessageMalformedException);

        assert(response.data.payload != nullptr);

        StreamBuffer* buffer = _getStreamBuffer(response.parameter0);
        ThrowOn(buffer == nullptr, NotFoundException);

        StreamQueryInformation* desc =
            reinterpret_cast<StreamQueryInformation*>(response.data.payload);

        std::unique_ptr<Stream> str(new ClientStream(stream, desc->descriptor,
                                                     *buffer, getSession()));

        Stream* pstr = str.get(); // Make a shortcut for the return value
        _addStream(str);

        return pstr;
    }

    void ClientStore::_replicateConfiguration(bool update)
    {
        if (!update) {
            _freeConfiguration();
        }

        try {
            // We only replicate the stream buffers to allow an early fail
            // if the client does not have enough buffer space to hold the
            // default stream buffer(s). Streams and other objects are
            // replicated on-demand.

            std::vector<BufferId> buffers;
            _enumerateStreamBuffers(buffers);

            // The server should already created a stream buffer for us.
            assert(!buffers.empty());

            for (auto buffer : buffers) {
                if (this->Store::_getStreamBuffer(buffer) == nullptr) {
                    _replicateStreamBuffer(buffer);
                }
            }

        } catch (...) {
            if (!update) {
                _freeConfiguration();
            }

            throw;
        }
    }

    std::unique_ptr<StreamBuffer> ClientStore::_createStreamBuffer(
        size_t segmentSize, uint32_t numSegments)
    {
        Message response = {0};
        ClientPort& port = _getPort();
        std::unique_ptr<StreamBuffer> buffer = nullptr;

        port.call(&response, RpcApi::CCV31_StreamBufferRegister, numSegments,
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
        Message response = {0};
        ClientPort& port = _getPort();

        StreamBuffer* buf = _getStreamBuffer(buffer);
        ThrowOnNull(buf, NotFoundException);

        desc.hidden = false;

        port.call(&response, RpcApi::CCV31_StreamRegister, &desc,
                  sizeof(StreamDescriptor), buffer);

        StreamId rid = static_cast<StreamId>(response.parameter0);

        return std::unique_ptr<ClientStream>(
                    new ClientStream(rid, desc, *buf, getSession()));
    }

    void ClientStore::_enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV31_StreamBufferEnumerate);

        uint32_t n = response.parameter0;

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                ((response.data.payloadLength % sizeof(BufferId)) != 0) ||
                (response.data.payloadLength / sizeof(BufferId) != n),
                RpcMessageMalformedException);

        if (n > 0) {
            assert(response.data.payload != nullptr);
            BufferId* buffer = reinterpret_cast<BufferId*>(response.data.payload);

            std::vector<BufferId> ids(buffer, buffer + n);
            std::swap(ids, out);
        } else {
            out.clear();
        }
    }

    void ClientStore::_enumerateStreams(std::vector<StreamId>& out,
                                        bool includeHidden) const
    {
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV31_StreamEnumerate,
                  (includeHidden) ? 1 : 0);

        uint32_t n = response.parameter0;

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                ((response.data.payloadLength % sizeof(StreamId)) != 0) ||
                (response.data.payloadLength / sizeof(StreamId) != n),
                RpcMessageMalformedException);

        if (n > 0) {
            assert(response.data.payload != nullptr);
            StreamId* buffer = reinterpret_cast<StreamId*>(response.data.payload);

            std::vector<StreamId> ids(buffer, buffer + n);
            std::swap(ids, out);
        } else {
            out.clear();
        }
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
        this->Store::_enumerateStreams(streams, true);

        for (auto i = 0; i < streams.size(); ++i) {
            assert(streams[i] != nullptr);
            ClientStream* stream = reinterpret_cast<ClientStream*>(streams[i]);

            stream->flush();
        }

        ClientPort& port = _getPort();

        port.call(nullptr, RpcApi::CCV31_StoreClose);
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
