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
        _referenceCount(1)
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
        std::unique_ptr<StreamBuffer> buffer;
        BufferId id = _bufferIdAllocator.getNextId();
    #ifdef _WIN32
    #else
        ThreadBase* thread;
    #endif

        try {
        #ifdef WIN32
            buffer = std::unique_ptr<ServerStreamBuffer>(
                new ServerStreamBuffer(id, segmentSize, numSegments));
        #else
            thread = ThreadBase::getCurrentThread();
            assert(thread != nullptr);

            // In Linux, allocating a shared memory region can be successful, 
            // although the memory for the underlying tmpfs is exhausted. We 
            // therefore touch all pages when creating a shared memory region 
            // to avoid crashes on access later. To fetch exceptions, we need 
            // a pseudo try-catch block for SIGBUS signals.

            SignalJumpBuffer jmp;
            thread->setSignalJmpBuffer(&jmp);
            if (!sigsetjmp(jmp.signalret, 1)) {

                // Try to create and touch shared memory
                buffer = std::unique_ptr<ServerStreamBuffer>(
                    new ServerStreamBuffer(id, segmentSize, numSegments));
                buffer->touch();

            } else { // catch
                const size_t bufferSize = 
                    ServerStreamBuffer::computeBufferSize(segmentSize, 
                                                          numSegments);

                Throw(Exception, stringFormat("Failed to allocate %s of "
                        "shared memory for stream buffer <id: %d>. Increase "
                        "the system's limits for shared memory or reduce the "
                        "stream buffer size (caution: this will also reduce "
                        "the number of streams that can be accessed by the "
                        "client at the same time).",
                        sizeToString(bufferSize, SizeUnit::SuMiB).c_str(), id));
            }

            thread->setSignalJmpBuffer(nullptr);
        #endif
        } catch (...) {
        #ifdef WIN32
        #else
            thread->setSignalJmpBuffer(nullptr);
        #endif
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
            ThrowOnNull(buf, NotFoundException);
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

    std::unique_ptr<DataPool> ServerStore::_createDataPool(PoolId id, 
                                                           StreamId stream)
    {
        std::unique_ptr<DataPool> pool;
        PoolId rid;

        Stream* s = _getStream(stream);
        ThrowOnNull(s, NotFoundException);

        if (id == INVALID_POOL_ID) {
            rid = _dataPoolIdAllocator.getNextId();
        } else {
            rid = id;
            _dataPoolIdAllocator.stealId(id);
        }

        try {
            pool = std::unique_ptr<DataPool>(new DataPool(rid, *s));
        } catch (...) {
            _dataPoolIdAllocator.retireId(rid);

            throw;
        }

        return pool;
    }

    void ServerStore::_enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        std::vector<StreamBuffer*> buffers;
        this->Store::_enumerateStreamBuffers(buffers);

        out.clear();
        out.reserve(buffers.size());
        for (auto buffer : buffers) {
            out.push_back(buffer->getId());
        }
    }

    void ServerStore::_enumerateStreams(std::vector<StreamId>& out, 
                                        bool includeHidden) const
    {
        std::vector<Stream*> streams;
        this->Store::_enumerateStreams(streams, includeHidden);

        out.clear();
        out.reserve(streams.size());
        for (auto stream : streams) {
            out.push_back(stream->getId());
        }
    }

    void ServerStore::_enumerateDataPools(std::vector<PoolId>& out) const
    {
        std::vector<DataPool*> pools;
        this->Store::_enumerateDataPools(pools);

        out.clear();
        out.reserve(pools.size());
        for (auto pool : pools) {
            out.push_back(pool->getId());
        }
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
        LockScopeExclusive(_referenceLock);
        ThrowOn(_findReference(session), InvalidOperationException);

        // The store might be closing at this moment
        ThrowOn(_referenceCount == 0, OperationInProgressException);

        _references.push_back(session);
        _referenceCount++;
    }

    void ServerStore::detach(SessionId session)
    {
        if (_referenceCount == 0) {
            assert(false);
            return;
        }

        uint32_t newRefCount;
        LockExclusive(_referenceLock); {
            StreamWait wait;

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
                this->Store::_enumerateStreams(streams, false);

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

                newRefCount = --_referenceCount;
                session = SERVER_SESSION_ID;
            }

            if (_referenceCount == 1) {
                SwapEnvironment(&StorageServer::getInstance().getEnvironment());
                assert(_references.empty());

                // The store provider always holds the last reference to a 
                // store. In this phase we close all streams that have 
                // references from the imaginary server session, i.e., 
                // references made by encoders. Since we do not know in which 
                // order we should close those streams, we let the encoders do 
                // it themselves.
                ThrowOn(session != SERVER_SESSION_ID, InvalidOperationException);

                // Prevent any further configuration changes and invoke the
                // close method of all encoder instances.
                _lockConfiguration();

                std::vector<Stream*> streams;
                this->Store::_enumerateStreams(streams, true);

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

                newRefCount = --_referenceCount;
            }
        } Unlock();

        // We need to release the lock before we delete ourselves with a
        // call to the store manager's close method. After this call we must
        // not touch anything within the class!
        if (newRefCount == 0) {
            StorageServer::getInstance().getStoreManager()._releaseStore(getId());
        }
    }

    void ServerStore::enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const
    {
        LockScopeShared(_lock);
        this->Store::_enumerateStreamBuffers(out);
    }

    void ServerStore::enumerateStreams(std::vector<Stream*>& out, 
                                       bool includeHidden) const
    {
        LockScopeShared(_lock);
        this->Store::_enumerateStreams(out, includeHidden);
    }

    void ServerStore::enumerateDataPools(std::vector<DataPool*>& out) const
    {
        LockScopeShared(_lock);
        this->Store::_enumerateDataPools(out);
    }

    StreamEncoder::FactoryMethod ServerStore::getEncoderFactory(
        const StreamTypeId& type)
    {
        EncoderDescriptor* enc = _findEncoder(type);
        if (enc == nullptr) {
            // Fall back onto the default encoder
            enc = _findEncoder(_defaultEncoderTypeId);
            ThrowOnNull(enc, NotFoundException);
        }

        return enc->factoryMethod;
    }

}
