/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
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

#include "StaticStream.h"

#include "ClientStream.h"

namespace SimuTrace
{

    StaticStream::StaticStream(StreamId id, const StreamDescriptor& desc,
                               StreamBuffer& buffer,
                               ClientSession& session) :
        ClientStream(id, desc, buffer, session)
    {

    }

    StaticStream::~StaticStream()
    {

    }

    byte* StaticStream::_getPayload(SegmentId segment, uint32_t* lengthOut) const
    {
        byte* payload;
        assert(lengthOut != nullptr);

        StreamBuffer& buffer = getStreamBuffer();
        if (buffer.isMaster()) {
            size_t size;

            // Since our RPC interface does not support scatter/gather I/O, we
            // send the whole segment here and not just the used part plus the
            // control element. The assumption is that in the common case we
            // will have to send full segments anyway and thus do not transfer
            // unnecessary data here.
            payload = buffer.getSegmentAsPayload(segment, size);
            *lengthOut = static_cast<uint32_t>(size);

            assert(buffer.dbgSanityCheck(segment, getType().entrySize) < 2);
        } else {

            // We use shared memory, the server already has the data and we
            // do not need to send it over network.
            payload = nullptr;
            *lengthOut = 0;
        }

        return payload;
    }

    void StaticStream::_initializeStaticHandle(StreamHandle handle,
                                               SegmentId segment,
                                               StreamStateFlags flags,
                                               StreamAccessFlags aflags,
                                               size_t offset)
    {
        assert(handle != nullptr);
        assert(!IsSet(flags, StreamStateFlags::SsfDynamic));

        StreamBuffer& buffer = getStreamBuffer();

        handle->flags               = flags;
        handle->accessFlags         = aflags;

        handle->stream              = this;
        handle->entrySize           = getType().entrySize;

        handle->segment             = segment;
        handle->stat.control        = buffer.getControlElement(segment);

        handle->stat.sequenceNumber = handle->stat.control->link.sequenceNumber;

        handle->segmentStart        = buffer.getSegment(segment);

        // If this segment is read-only, we move the segmentEnd pointer right
        // behind the last valid entry.
        handle->segmentEnd = IsSet(flags, StreamStateFlags::SsfRead) ?
            buffer.getSegmentEnd(segment, handle->entrySize) :
            handle->segmentStart + buffer.getSegmentSize();

        assert(handle->segmentEnd > handle->segmentStart);

        // Position the handle at the offset that the server determined for
        // the query.
        handle->entry = handle->segmentStart + offset;

        assert((handle->entry != nullptr) &&
               (handle->entry >= handle->segmentStart) &&
               (handle->entry <= handle->segmentEnd -
                (isVariableEntrySize(handle->entrySize) ?
                getSizeHint(handle->entrySize) : handle->entrySize)));
        assert(handle->stat.control != nullptr);
    }

    void StaticStream::_invalidateStaticHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        assert(IsSet(handle->flags, StreamStateFlags::SsfRead));
        assert(!IsSet(handle->flags, StreamStateFlags::SsfDynamic));

        handle->segment = INVALID_SEGMENT_ID;
        handle->stat.control = nullptr;

        handle->entry = nullptr;
        handle->segmentStart = nullptr;
        handle->segmentEnd = nullptr;
    }

    void StaticStream::_payloadAllocatorWrite(Message& msg, bool free, void* args)
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

    #ifdef _DEBUG
        // For debugging builds we clear the whole data area of the segment
        // with 0xCD (clean memory) to allow for later checks if the user
        // filled the segment in the correct way. If we use shared memory, the
        // server has already done the job for us.
        if (buffer.isMaster()) {
            buffer.dbgSanityFill(id, false);
        }
    #endif
    }

    void StaticStream::_payloadAllocatorRead(Message& msg, bool free, void* args)
    {
        // Since we directly write into the stream buffer, we do not need to
        // free anything.
        if (free) {
            return;
        }

        ClientStream* stream = reinterpret_cast<ClientStream*>(args);
        StreamBuffer& buffer = stream->getStreamBuffer();

        assert(msg.response.status == RpcApi::SC_Success);

        SegmentId id = static_cast<SegmentId>(msg.parameter0);

        size_t size;
        void* segment = buffer.getSegmentAsPayload(id, size);

        // The server should send us the whole segment.
        ThrowOn(msg.data.payloadLength != size,
                RpcMessageMalformedException);

        msg.data.payload = segment;
    }

    StreamHandle StaticStream::_append(StreamHandle handle)
    {
        uint32_t payloadLength = 0;
        void* payload = nullptr;

        if (handle != nullptr) {
            assert(handle->stream == this);
            assert(!IsSet(handle->flags, StreamStateFlags::SsfRead));
            assert(!IsSet(handle->flags, StreamStateFlags::SsfDynamic));
            assert(handle->segment != INVALID_SEGMENT_ID);

            payload = _getPayload(handle->segment, &payloadLength);
        }

        // Set the payload allocator so we accept segment control elements in
        // remote connections.
        Message response = {0};
        response.allocator = _payloadAllocatorWrite;
        response.allocatorArgs = this;

        try {
            _getPort().call(&response, RpcApi::CCV_StreamAppend, payload,
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

        if (handle == nullptr) {
            std::unique_ptr<StreamStateDescriptor> desc(
                new StreamStateDescriptor());

            _initializeStaticHandle(desc.get(), id);

            handle = desc.get();

            // Set the handle only after we could successfully initialize
            // it. Otherwise, we might throw and would need to manually
            // release it again.
            _addHandle(desc);

            LogDebug("Created new write handle for stream %d.", getId());
        } else {
            _initializeStaticHandle(handle, id);
        }

        return handle;
    }

    StreamHandle StaticStream::_open(QueryIndexType type, uint64_t value,
                                     StreamAccessFlags flags, StreamHandle handle)
    {
    #ifdef _DEBUG
        if (handle != nullptr) {
            assert(handle->stream == this);
            assert(IsSet(handle->flags, StreamStateFlags::SsfRead));
            assert(!IsSet(handle->flags, StreamStateFlags::SsfDynamic));
        }
    #endif
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
            ((handle != nullptr) && (handle->stat.control != nullptr)) ?
                handle->stat.control->link.sequenceNumber :
                INVALID_STREAM_SEGMENT_ID;

        try {
            _getPort().call(&response, RpcApi::CCV_StreamCloseAndOpen, &query,
                            sizeof(StreamOpenQuery), getId(), closeSqn);

        } catch (...) {
            // If the client requests a segment that is still in progress or
            // does not yet exists (e.g., in a producer/consumer scenario) the
            // server will throw according exceptions. In these cases we do not
            // want the handle to be deleted, to facilitate retrying the
            // operation at a later time.

            if (handle != nullptr) {
                _invalidateStaticHandle(handle);
            }

            throw;
        }

        SegmentId id = response.parameter0;
        if (id == INVALID_SEGMENT_ID) {
            if (handle != nullptr) {
                _invalidateStaticHandle(handle);
            }
            return handle;
        }

        size_t offset = response.data.parameter1;
        if (handle == nullptr) {
            std::unique_ptr<StreamStateDescriptor> desc(
                new StreamStateDescriptor());

            handle = desc.get();

            _initializeStaticHandle(handle, id, SsfRead, flags, offset);

            // Add the handle only after we could successfully initialize
            // it. Otherwise, we might throw and would need to manually
            // remove it from the handle list again.
            _addHandle(desc);

            LogDebug("Created new read handle for stream %d.", getId());
        } else {
            _initializeStaticHandle(handle, id, SsfRead, flags, offset);
        }

        return handle;
    }

    void StaticStream::_closeHandle(StreamHandle handle)
    {
        assert(handle != nullptr);
        assert(handle->stream == this);
        assert(handle->segment != INVALID_SEGMENT_ID);
        assert(!IsSet(handle->flags, StreamStateFlags::SsfDynamic));
        assert(handle->stat.control != nullptr);
        assert(handle->stat.control->link.sequenceNumber !=
            INVALID_STREAM_SEGMENT_ID);

        uint32_t payloadLength = 0;
        void *payload = nullptr;

        if (!IsSet(handle->flags, StreamStateFlags::SsfRead)) {
            // This is a write handle. Add any new data to the RPC call
            payload = _getPayload(handle->segment, &payloadLength);
        }

        _getPort().call(nullptr, RpcApi::CCV_StreamClose, payload,
                        payloadLength, getId(),
                        handle->stat.control->link.sequenceNumber);
    }

    void StaticStream::queryInformation(StreamQueryInformation& informationOut) const
    {
        Message response = {0};

        _getPort().call(&response, RpcApi::CCV_StreamQuery, getId());

        ThrowOn((response.payloadType != MessagePayloadType::MptData) ||
                (response.data.payloadLength != sizeof(StreamQueryInformation)),
                RpcMessageMalformedException);

        assert(response.data.payload != nullptr);

        StreamQueryInformation* desc =
            reinterpret_cast<StreamQueryInformation*>(response.data.payload);

        informationOut = *desc;
    }

}