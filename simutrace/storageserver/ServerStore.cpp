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

#include "ServerStore.h"

#include "StorageServer.h"
#include "ServerSession.h"
#include "ServerStreamBuffer.h"
#include "ServerSessionWorker.h"
#include "StreamEncoder.h"
#include "ServerStream.h"
#include "ServerStoreManager.h"

#include "Version.h"

namespace SimuTrace
{

    const StreamTypeId ServerStore::_defaultEncoderTypeId =
        DefGuid(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

    ServerStore::ServerStore(StoreId id, const std::string& name) :
        Store(id, name),
        _referenceCount(1),
        _references()
    {
        // The server store automatically creates a first stream buffer that
        // the client will map into its address space.
        uint32_t poolSize = 0;
        if (!Configuration::tryGet("client.memmgmt.poolSize", poolSize)) {
            poolSize = SIMUTRACE_CLIENT_MEMMGMT_RECOMMENDED_POOLSIZE;
        }

        _registerStreamBuffer(SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB,
                              poolSize / SIMUTRACE_MEMMGMT_SEGMENT_SIZE);
    }

    ServerStore::~ServerStore()
    {
        // Before we close all streams and buffers in the destructor of the
        // base class, we need to flush each buffer's standby list so any
        // references to the streams are removed.
        std::vector<StreamBuffer*> buffers;
        this->Store::_enumerateStreamBuffers(buffers);

        for (auto buffer : buffers) {
            ServerStreamBuffer* sbuffer =
                dynamic_cast<ServerStreamBuffer*>(buffer);

            sbuffer->flushStandbyList();
        }

        // The same is true for the global storage pool
        ServerStreamBuffer& pool = StorageServer::getInstance().getMemoryPool();
        pool.flushStandbyList(getId());

        // Clear the encoder map. Finding encoders for a stream type is not
        // possible anymore after this point.
        _encoderMap.clear();
    }

    ServerStore::EncoderDescriptor* ServerStore::_findEncoder(
        const StreamTypeId& type)
    {
        auto it = _encoderMap.find(type);
        if (it == _encoderMap.end()) {
            return nullptr;
        }

        return &it->second;
    }

    bool ServerStore::_findReference(SessionId session) const
    {
        bool found = false;
        for (auto ref : _references) {
            if (ref == session) {
                found = true;
                break;
            }
        }

        return found;
    }

    std::unique_ptr<StreamBuffer> ServerStore::_createStreamBuffer(
        size_t segmentSize, uint32_t numSegments)
    {
        bool sharedMemory = StorageServer::getInstance().hasLocalBindings();
        std::unique_ptr<StreamBuffer> buffer;
        BufferId id = _bufferIdAllocator.getNextId();

        try {
            buffer = std::unique_ptr<ServerStreamBuffer>(
                new ServerStreamBuffer(id, segmentSize, numSegments,
                    sharedMemory));
        } catch (...) {
            _bufferIdAllocator.retireId(id);

            throw;
        }

        return buffer;
    }

    std::unique_ptr<Stream> ServerStore::_createStream(StreamId id,
                                                       StreamDescriptor& desc,
                                                       BufferId buffer)
    {
        std::unique_ptr<Stream> stream;
        StreamId rid;
        StreamBuffer* buf;

        if (buffer == SERVER_BUFFER_ID) {
            buf = &StorageServer::getInstance().getMemoryPool();
        } else {
            buf = _getStreamBuffer(buffer);
            ThrowOnNull(buf, NotFoundException,
                        stringFormat("stream buffer with id %d", buffer));
        }

        if (id == INVALID_STREAM_ID) {
            rid = _streamIdAllocator.getNextId();
        } else {
            rid = id;
            _streamIdAllocator.stealId(id);
        }

        try {
            stream = std::unique_ptr<ServerStream>(
                new ServerStream(*this, rid, desc, *buf));
        } catch (...) {
            _streamIdAllocator.retireId(rid);

            throw;
        }

        return stream;
    }

    void ServerStore::_enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        std::vector<StreamBuffer*> buffers;
        std::vector<BufferId> ids;
        this->Store::_enumerateStreamBuffers(buffers);

        ids.reserve(buffers.size());
        for (auto buffer : buffers) {
            ids.push_back(buffer->getId());
        }

        std::swap(ids, out);
    }

    void ServerStore::_enumerateStreams(std::vector<StreamId>& out,
                                        StreamEnumFilter filter) const
    {
        std::vector<Stream*> streams;
        std::vector<StreamId> ids;
        this->Store::_enumerateStreams(streams, filter);

        ids.reserve(streams.size());
        for (auto stream : streams) {
            ids.push_back(stream->getId());
        }

        std::swap(ids, out);
    }

    void ServerStore::_registerEncoder(const StreamTypeId& type,
                                       EncoderDescriptor& descriptor)
    {
        assert(descriptor.factoryMethod != nullptr);
        _encoderMap[type] = descriptor;

    #ifdef _DEBUG
        std::unique_ptr<StreamEncoder> encoder(
            descriptor.factoryMethod(*this, nullptr));
        assert(encoder != nullptr);

        LogDebug("<store: %s> Registered '%s' encoder for type %s%s.",
                 getName().c_str(),
                 encoder->getFriendlyName().c_str(),
                 guidToString(type).c_str(),
                 (type == _defaultEncoderTypeId) ? " (default)" : "");
    #endif
    }

    void ServerStore::attach(SessionId session)
    {
        LockScopeExclusive(_lock);
        ThrowOn(_findReference(session), InvalidOperationException);

        // The store might be closing at this moment
        ThrowOn(_referenceCount == 0, OperationInProgressException);

        _references.push_back(session);
        _referenceCount++;
    }

    uint32_t ServerStore::detach(SessionId session)
    {
        LockScopeExclusive(_lock);
        ThrowOn(_referenceCount == 0, InvalidOperationException);

        StreamWait wait;
    #ifdef _DEBUG
        uint32_t savedRefCount = _referenceCount;
    #endif
        if (_referenceCount > 1) {
            // When we enter this branch, a session closes its store
            // reference. First check if the session holds a reference to
            // this store.

            ThrowOn(!_findReference(session), InvalidOperationException);

            // The client leaves it to the server to finalize shared memory
            // -based segments. We therefore, iterate over all streams to
            // submit any pending changes or purge read segments.
            // It is ok if new streams are registered after the enum,
            // because the new streams cannot contain references from this
            // session.
            std::vector<Stream*> streams;
            this->Store::_enumerateStreams(streams, SefRegular);

            for (auto stream : streams) {
                ServerStream* sstream = dynamic_cast<ServerStream*>(stream);
                assert(sstream != nullptr);

                assert(sstream->getStreamBuffer().getId() != SERVER_BUFFER_ID);

                sstream->close(session, &wait, true);
            }

            LogInfo("<store: %s> Session %d rundown (%d pending "
                    "segment(s)).", getName().c_str(), session,
                    wait.getCount());

            // Wait for the triggered operations to finish. Closing a store
            // is a synchronous operation. Ignore any errors.
            wait.wait();

            // TODO: Add error handling for wait. What happens if a
            // segment is still open?

            _references.remove(session);

            --_referenceCount;
        }

        // Execution might fall through. Do not refactor to else if

        if (_referenceCount == 1) {
            SwapEnvironment(&StorageServer::getInstance().getEnvironment());
            assert(_references.empty());

            // The store provider always holds the last reference to a
            // store. In case this is the last user session to the store
            // we lock the configuration, write out any last data that
            // the encoders might still hold and switch the store to
            // read-only mode. If the store was already in read-only mode,
            // the following operations should only close any references
            // to (hidden) streams. Since we do not know in which order
            // such encoder private streams should be closed, we let the
            // encoders do the work. In case the close comes from the
            // server session, we free the last reference and return.
            // Streams and encoders should already be closed.
            if (session == SERVER_SESSION_ID) {
                assert(savedRefCount == 1);

                _referenceCount = 0;
            } else {
                assert(savedRefCount > 1);

                // Prevent any further configuration changes and invoke the
                // close method of all encoder instances.
                _lockConfiguration();

                std::vector<Stream*> streams;
                this->Store::_enumerateStreams(streams, SefAll);

                for (auto stream : streams) {
                    ServerStream* sstream = dynamic_cast<ServerStream*>(stream);
                    assert(sstream != nullptr);

                    StreamEncoder& encoder = sstream->getEncoder();
                    encoder.close(&wait);
                }

                LogInfo("<store: %s> Store rundown (%d pending "
                        "segment(s)).", getName().c_str(), wait.getCount());

                // Wait for the the encoders to finish. Ignore any errors.
                wait.wait();

                // TODO: Add error handling for wait. What happens if a
                // segment is still open?
            }
        }

        return _referenceCount;
    }

    void ServerStore::enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const
    {
        LockScopeShared(_lock);
        this->Store::_enumerateStreamBuffers(out);
    }

    void ServerStore::enumerateStreams(std::vector<Stream*>& out,
                                       StreamEnumFilter filter) const
    {
        LockScopeShared(_lock);
        this->Store::_enumerateStreams(out, filter);
    }

    StreamEncoder::FactoryMethod ServerStore::getEncoderFactory(
        const StreamTypeId& type)
    {
        EncoderDescriptor* enc = _findEncoder(type);
        if (enc == nullptr) {
            // Fall back onto the default encoder
            enc = _findEncoder(_defaultEncoderTypeId);
            ThrowOnNull(enc, NotFoundException,
                        stringFormat("encoder for type %s",
                            guidToString(type).c_str()));
        }

        return enc->factoryMethod;
    }

}
