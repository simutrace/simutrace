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
                             bool alwaysCreate) :
        Store(CLIENT_STORE_ID, name),
        ClientObject(session)
    {
        Message response = {0};
        ClientPort& port = _getPort();

        // Send the request to create the store
        port.call(&response, RpcApi::CCV30_StoreCreate, name, 
                  (alwaysCreate) ? TRUE : FALSE, 0);

        try {
            _replicateConfiguration(false);

        } catch (...) {
            port.call(nullptr, RpcApi::CCV30_StoreClose);

            throw;
        }
    }

    ClientStore::~ClientStore()
    {

    }

    void ClientStore::_replicateStreamBuffer(BufferId buffer)
    {
        Message response = {0};
        ClientPort& port = _getPort();
        std::unique_ptr<StreamBuffer> buf;

        port.call(&response, RpcApi::CCV30_StreamBufferQuery, buffer, TRUE);

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
                    new StreamBuffer(buffer, segmentSize, numSegments));
        }

        _addStreamBuffer(buf);
    }

    void ClientStore::_replicateStream(StreamId stream)
    {
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV30_StreamQuery, stream);

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

        _addStream(str);
    }

    void ClientStore::_replicateConfiguration(bool update)
    {
        if (!update) {
            _freeConfiguration();
        }

        try {
            // Replicate stream buffers -----
            std::vector<BufferId> buffers;
            _enumerateStreamBuffers(buffers);

            // The server should already created a stream buffer for us.
            assert(!buffers.empty());

            for (auto buffer : buffers) {
                if (_getStreamBuffer(buffer) == nullptr) {
                    _replicateStreamBuffer(buffer);
                }
            }

            // Replicate streams -----
            std::vector<StreamId> streams;
            _enumerateStreams(streams, false);

            for (auto stream : streams) {
                if (_getStream(stream) == nullptr) {
                    _replicateStream(stream);
                }
            }

            // Replicate data pools -----
            // TODO

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

        port.call(&response, RpcApi::CCV30_StreamBufferRegister, numSegments,
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
                     new StreamBuffer(id, segmentSize, numSegments, bufferHandle));
        } else {
            buffer = std::unique_ptr<StreamBuffer>(
                     new StreamBuffer(id, segmentSize, numSegments));
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

        port.call(&response, RpcApi::CCV30_StreamRegister, &desc, 
                  sizeof(StreamDescriptor), buffer);

        StreamId rid = static_cast<PoolId>(response.parameter0);

        return std::unique_ptr<ClientStream>(
                    new ClientStream(rid, desc, *buf, getSession()));
    }

    std::unique_ptr<DataPool> ClientStore::_createDataPool(PoolId id, 
                                                           StreamId stream)
    {
        Message response = {0};
        ClientPort& port = _getPort();

        Stream* s = _getStream(stream);
        ThrowOnNull(s, NotFoundException);

        port.call(&response, RpcApi::CCV30_DataPoolRegister, stream);

        PoolId rid = static_cast<PoolId>(response.parameter0);

        return std::unique_ptr<DataPool>(
               new DataPool(rid, *s)); 
    }

    void ClientStore::_enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        Message response = {0};
        ClientPort& port = _getPort();

        port.call(&response, RpcApi::CCV30_StreamBufferEnumerate);

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

        port.call(&response, RpcApi::CCV30_StreamEnumerate, 
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

    void ClientStore::_enumerateDataPools(std::vector<PoolId>& out) const
    {
        Throw(NotImplementedException);
    }

    void ClientStore::detach(SessionId session)
    {
        LockScopeExclusive(_lock);

        std::vector<Stream*> streams;
        this->Store::_enumerateStreams(streams, true);

        for (auto i = 0; i < streams.size(); ++i) {
            assert(streams[i] != nullptr);
            ClientStream* stream = reinterpret_cast<ClientStream*>(streams[i]);

            stream->flush();
        }
    }

}
