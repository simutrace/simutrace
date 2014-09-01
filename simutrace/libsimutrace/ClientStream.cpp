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

#include "ClientStream.h"

#include "ClientObject.h"

namespace SimuTrace
{

    ClientStream::ClientStream(StreamId id, const StreamDescriptor& desc,
                               StreamBuffer& buffer,
                               ClientSession& session) :
        Stream(id, desc, buffer),
        ClientObject(session),
        _writeHandle(),
        _readHandles()
    {

    }

    ClientStream::~ClientStream()
    {
        // We do not explicitly close read handles on exit, but instead let
        // the server automatically clean up for us. However, the write handle
        // should be closed by now. If we are using a socket connection, we
        // would loose data otherwise.
        assert(_writeHandle == nullptr);
    }

    byte* ClientStream::_getPayload(SegmentId segment, uint32_t* lengthOut) const
    {
        byte* payload;
        assert(lengthOut != nullptr);

        StreamBuffer& buffer = getStreamBuffer();
        if (buffer.isMaster()) {            
            size_t size;

            payload = buffer.getSegmentAsPayload(segment, size);
            *lengthOut = static_cast<uint32_t>(size);
        } else {
            payload = nullptr;
            *lengthOut = 0;
        }

        return payload;
    }

    void ClientStream::_initializeHandle(StreamHandle handle, SegmentId segment, 
                                         bool readOnly, StreamAccessFlags flags)
    {
        StreamBuffer& buffer = getStreamBuffer();

        handle->isReadOnly  = (readOnly) ? TRUE : FALSE;
        handle->accessFlags = flags;

        handle->stream      = this;
        handle->entrySize   = getType().entrySize;

        handle->segment     = segment;
        handle->control     = buffer.getControlElement(segment);
        handle->entry       = buffer.getSegment(segment);

        handle->sequenceNumber     = handle->control->link.sequenceNumber;

        // If this segment is read-only, we move the segmentEnd pointer right
        // behind the last valid entry.
        size_t endOffset = (readOnly) ?
            getEntrySize(&getType()) * handle->control->rawEntryCount :
            buffer.getSegmentSize();

        assert(endOffset <= buffer.getSegmentSize());

        handle->segmentStart = handle->entry;
        handle->segmentEnd   = handle->segmentStart + endOffset;

        assert(handle->entry != nullptr);
        assert(handle->control != nullptr);
    }

    void ClientStream::_closeHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        assert(handle->stream == this);
        assert(handle->segment != INVALID_SEGMENT_ID);
        assert(handle->control != nullptr);
        assert(handle->control->link.sequenceNumber != INVALID_STREAM_SEGMENT_ID);

        uint32_t payloadLength = 0;
        void *payload = nullptr;

        if (handle == _writeHandle.get()) {
            payload = _getPayload(handle->segment, &payloadLength);
        }

        _getPort().call(nullptr, RpcApi::CCV30_StreamClose, payload,
                        payloadLength, getId(),
                        handle->control->link.sequenceNumber);
    }

    void ClientStream::_releaseHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        StreamId id = reinterpret_cast<ClientStream*>(handle->stream)->getId();

        if (handle == _writeHandle.get()) {
            LogDebug("Closing write handle for stream %d.", id);

            _writeHandle = nullptr;
        } else {
            bool found = false;
            for (auto it = _readHandles.begin(); 
                 it != _readHandles.end(); ++it) {

                if ((*it).get() == handle) {
                    LogDebug("Closing read handle for stream %d. "
                             "%d handles left.", id, _readHandles.size() - 1);

                    _readHandles.erase(it);
                    found = true;
                    break;
                }
            }

            if (!found) {
                LogWarn("Could not release handle for stream %d. The handle "
                        "could not be found.", id);
            }
        }
    }

    void ClientStream::_invalidateHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        assert(handle->isReadOnly == 1);

        handle->segment = INVALID_SEGMENT_ID;
        handle->control = nullptr;

        handle->entry = nullptr;
        handle->segmentStart = nullptr;
        handle->segmentEnd = nullptr;
    }

    void ClientStream::_payloadAllocatorWrite(Message& msg, bool free, void* args)
    {
        // Since we directly write into the stream buffer, we do not need to
        // free anything.
        if (free) {
            return;
        }

        ClientStream* stream = reinterpret_cast<ClientStream*>(args);
        StreamBuffer& buffer = stream->getStreamBuffer();

        assert(msg.response.status == RpcApi::SC_Success);

        // The server should send only the segment control element to us, not
        // the entire segment as we have to do on submit.
        ThrowOn(msg.data.payloadLength != sizeof(SegmentControlElement),
                RpcMessageMalformedException);

        SegmentId id = static_cast<SegmentId>(msg.data.parameter1);
        msg.data.payload = buffer.getControlElement(id);
    }

    void ClientStream::_payloadAllocatorRead(Message& msg, bool free, void* args)
    {
        // Since we directly write into the stream buffer, we do not need to
        // free anything.
        if (free) {
            return;
        }

        ClientStream* stream = reinterpret_cast<ClientStream*>(args);
        StreamBuffer& buffer = stream->getStreamBuffer();

        assert(msg.response.status == RpcApi::SC_Success);

        SegmentId id = static_cast<SegmentId>(msg.data.parameter1);

        size_t size;
        void* segment = buffer.getSegmentAsPayload(id, size);

        // The server should send us the whole segment.
        ThrowOn(msg.data.payloadLength != size,
                RpcMessageMalformedException);

        msg.data.payload = segment;
    }

    void ClientStream::queryInformation(StreamQueryInformation& informationOut) const
    {
        Message response = {0};

        _getPort().call(&response, RpcApi::CCV30_StreamQuery, getId());

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                (response.data.payloadLength != sizeof(StreamQueryInformation)),
                RpcMessageMalformedException);

        assert(response.data.payload != nullptr);

        StreamQueryInformation* desc = 
            reinterpret_cast<StreamQueryInformation*>(response.data.payload);

        informationOut = *desc;
    }

    StreamHandle ClientStream::append(StreamHandle handle)
    {
        uint32_t payloadLength = 0;
        void* payload = nullptr;

        if (_writeHandle != nullptr) {
            ThrowOn(handle != _writeHandle.get(), InvalidOperationException);
            assert(_writeHandle->segment != INVALID_SEGMENT_ID);
            assert(!_writeHandle->isReadOnly);

            payload = _getPayload(_writeHandle->segment, &payloadLength);
        } else {
            ThrowOn(handle != nullptr, InvalidOperationException);
        }

        // Set the payload allocator so we accept segment control elements in 
        // remote connections.
        Message response = {0};
        response.allocator = _payloadAllocatorWrite;
        response.allocatorArgs = this;

        try {
            _getPort().call(&response, RpcApi::CCV30_StreamAppend, payload,
                            payloadLength, getId());

        } catch (...) {
            if (handle != nullptr) {
                _releaseHandle(handle);
            }

            throw;
        }

        SegmentId id = response.data.parameter1;
        if (id == INVALID_SEGMENT_ID) {
            if (handle != nullptr) {
                _releaseHandle(handle);
            }

            return nullptr;
        }

        if (_writeHandle == nullptr) {
            std::unique_ptr<StreamStateDescriptor> desc(
                new StreamStateDescriptor());

            _initializeHandle(desc.get(), id, false, StreamAccessFlags::SafNone);

            // Set the handle only after we could successfully initialize
            // it. Otherwise, we might throw and would need to manually
            // release it again.
            _writeHandle = std::move(desc);

            LogDebug("Created new write handle for stream %d.", getId());
        } else {
            _initializeHandle(_writeHandle.get(), id, false,
                              StreamAccessFlags::SafNone);
        }

        return _writeHandle.get();
    }

    StreamHandle ClientStream::open(QueryIndexType type, uint64_t value,
                                    StreamAccessFlags flags, StreamHandle handle)
    {
        // We do not check here, if the supplied handle is in our list or if
        // it is a manually crafted one by the caller. However, we do not need
        // to care.
        if (handle != nullptr) {
            ThrowOn(handle->stream != this, InvalidOperationException);

            // We need to release the handle if the caller specified the
            // write handle. Otherwise, we have a stale reference to the write
            // handle.

            if (handle == _writeHandle.get()) {
                _closeHandle(handle);
                _releaseHandle(handle);
                handle = nullptr;
            }
        }

        // Set the payload allocator so we accept segment data in remote
        // connections.
        Message response = {0};
        response.allocator = _payloadAllocatorRead;
        response.allocatorArgs = this;

        StreamOpenQuery query;
        query.type  = type;
        query.value = value;

        // Specifying no flags and a handle will lead us to copy the
        // handle's flags. Otherwise, we use the specified flags.
        if ((handle != nullptr) && (flags == StreamAccessFlags::SafNone)) {
            query.flags = handle->accessFlags;
        } else {
            query.flags = flags;
            if (handle != nullptr) {
                handle->accessFlags = flags;
            }
        }

        StreamSegmentId closeSqn = 
            ((handle != nullptr) && (handle->control != nullptr)) ? 
                handle->control->link.sequenceNumber :
                INVALID_STREAM_SEGMENT_ID;

        try {
            _getPort().call(&response, RpcApi::CCV30_StreamCloseAndOpen, &query,
                            sizeof(StreamOpenQuery), getId(), closeSqn);

        } catch (...) {
            // If the client requests a segment that is still in progress or
            // does not yet exists (e.g., in a producer/consumer scenario) the
            // server will throw according exceptions. In these cases we do not
            // want the handle to be deleted, to facilitate retrying the
            // operation at a later time.

            if (handle != nullptr) {
                _invalidateHandle(handle);
            }

            throw;
        } 

        SegmentId id = response.data.parameter1;
        if (id == INVALID_SEGMENT_ID) {
            if (handle != nullptr) {
                _invalidateHandle(handle);
            }
            return handle;
        }

        if (handle == nullptr) {
            std::unique_ptr<StreamStateDescriptor> desc(
                new StreamStateDescriptor());

            _initializeHandle(desc.get(), id, true, flags);

            handle = desc.get();

            // Add the handle only after we could successfully initialize
            // it. Otherwise, we might throw and would need to manually
            // remove it from the handle list again.
            _readHandles.push_back(std::move(desc));

            LogDebug("Created new read handle for stream %d.", getId());
        } else {
            _initializeHandle(handle, id, true, flags);
        }

        return handle;
    }

    void ClientStream::close(StreamHandle handle)
    {
        ThrowOnNull(handle, ArgumentNullException);

        if (handle->control != nullptr) {
            _closeHandle(handle);
        }

        _releaseHandle(handle);
    }

    void ClientStream::flush()
    {
        if (_writeHandle == nullptr) {
            return;
        }

        // For a stream to contain new data, a corresponding stream handle has
        // to be allocated. A write handle therefore points to a segment that
        // needs to be submitted. However, for segments backed by a 
        // shared-memory buffer, we let the server submit it automatically.

        if (getStreamBuffer().isMaster()) {
            _closeHandle(_writeHandle.get());
        }

        _releaseHandle(_writeHandle.get());
    }

}