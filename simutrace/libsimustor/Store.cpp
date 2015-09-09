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
#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "Store.h"

#include "StreamBuffer.h"
#include "Stream.h"

namespace SimuTrace
{

    Store::Store(StoreId id, const std::string& name) :
        _configurationLocked(false),
        _id(id),
        _name(name),
        _numRegularStreams(0),
        _numDynamicStreams(0)
    {
        ThrowOn(id == INVALID_STORE_ID, ArgumentException, "id");
    }

    Store::~Store()
    {
        _freeConfiguration();

        LogInfo("<store: %s> Store closed.", _name.c_str());
    }

    Store::Reference Store::makeOwnerReference(Store* store)
    {
        return Reference(store, [](Store* instance){
            std::string name = instance->getName();
            delete instance;

            LogDebug("Store '%s' released.", name.c_str());
        });
    }

    Store::Reference Store::makeUserReference(Store::Reference& ownerReference)
    {
        return Reference(ownerReference.get(), NullDeleter<Store>::deleter);
    }

    BufferId Store::_addStreamBuffer(std::unique_ptr<StreamBuffer>& buffer)
    {
        assert(!_configurationLocked);

        assert(buffer != nullptr);
        StreamBuffer* buf = buffer.get();

        BufferId id = buffer->getId();
        assert(id != INVALID_BUFFER_ID);
        assert(_buffers.find(id) == _buffers.end());
        assert(_buffers.size() < SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS);

        _buffers[id] = std::move(buffer);

        LogInfo("<store: %s> Registered stream buffer "
                "<id: %d, segs: %d, size: %s (%s), type: %s>.",
                _name.c_str(), id,
                buf->getNumSegments(),
                sizeToString(buf->getSegmentSize()).c_str(),
                sizeToString(buf->getBufferSize()).c_str(),
                (buf->getBufferHandle() == INVALID_HANDLE_VALUE) ?
                    "private" : "shared");

        return id;
    }

    StreamId Store::_addStream(std::unique_ptr<Stream>& stream)
    {
        assert(!_configurationLocked);

        assert(stream != nullptr);
        Stream* str = stream.get();

        StreamId id = stream->getId();
        assert(id != INVALID_STREAM_ID);
        assert(_streams.find(id) == _streams.end());

        _streams[id] = std::move(stream);

        if (IsSet(str->getFlags(), StreamFlags::SfDynamic)) {
            assert(_numDynamicStreams < SIMUTRACE_STORE_MAX_NUM_DYNAMIC_STREAMS);
            _numDynamicStreams++;
        } else {
            assert(_numRegularStreams < SIMUTRACE_STORE_MAX_NUM_STREAMS);
            _numRegularStreams++;
        }

        const StreamTypeDescriptor& type = str->getType();
        LogInfo("<store: %s> Registered %s%s stream "
                "<id: %d, name: '%s', type: %s, esize: %s (%s)>.",
                _name.c_str(),
                IsSet(str->getFlags(), StreamFlags::SfHidden) ?
                    "private" : "public",
                IsSet(str->getFlags(), StreamFlags::SfDynamic) ?
                    " dynamic" : "",
                id, str->getName().c_str(),
                guidToString(type.id).c_str(),
                sizeToString(getEntrySize(&type)).c_str(),
                isVariableEntrySize(type.entrySize) ?
                    "variable" : "fixed");

        return id;
    }

    BufferId Store::_registerStreamBuffer(size_t segmentSize,
                                          uint32_t numSegments)
    {
        ThrowOn(_configurationLocked, InvalidOperationException);
        ThrowOn(_buffers.size() >= SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS,
                Exception, stringFormat("Failed to register stream buffer. "
                    "The number of stream buffers exceeds the maximum (%i).",
                    SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS));

        auto buffer = _createStreamBuffer(segmentSize, numSegments);

        return _addStreamBuffer(buffer);
    }

    StreamId Store::_registerStream(StreamId id, StreamDescriptor& desc,
                                    BufferId buffer)
    {
        ThrowOn(_configurationLocked, InvalidOperationException);

        if (IsSet(desc.flags, StreamFlags::SfDynamic)) {
            ThrowOn(_numDynamicStreams >= SIMUTRACE_STORE_MAX_NUM_DYNAMIC_STREAMS,
                    Exception, stringFormat("Failed to register dynamic stream. "
                        "The number of streams exceeds the maximum (%i).",
                        SIMUTRACE_STORE_MAX_NUM_DYNAMIC_STREAMS));
        } else {
            ThrowOn(_numRegularStreams >= SIMUTRACE_STORE_MAX_NUM_STREAMS,
                    Exception, stringFormat("Failed to register stream. "
                        "The number of streams exceeds the maximum (%i).",
                        SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS));
        }

        auto stream = _createStream(id, desc, buffer);

        return _addStream(stream);
    }

    void Store::_lockConfiguration()
    {
        if (!_supportsWriteAfterOpen() && !_configurationLocked) {
            _configurationLocked = true;

            LogDebug("<store: %s> Locked configuration.", _name.c_str());
        }
    }

    void Store::_freeConfiguration()
    {
        _streams.clear();
        _buffers.clear();

        _numDynamicStreams = 0;
        _numRegularStreams = 0;

        _configurationLocked = false;
    }

    void Store::_enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const
    {
        std::vector<StreamBuffer*> buffers;
        buffers.reserve(_buffers.size());

        for (auto& pair : _buffers) {
            buffers.push_back(pair.second.get());
        }

        std::swap(buffers, out);
    }

    void Store::_enumerateStreams(std::vector<Stream*>& out,
                                  StreamEnumFilter filter) const
    {
        std::vector<Stream*> streams;
        streams.reserve(_streams.size());

        StreamFlags mask = StreamFlags::SfNone;
        if (IsSet(filter, StreamEnumFilter::SefHidden)) {
            mask = mask | StreamFlags::SfHidden;
        }
        if (IsSet(filter, StreamEnumFilter::SefDynamic)) {
            mask = mask | StreamFlags::SfDynamic;
        }

        bool includeRegular = IsSet(filter, StreamEnumFilter::SefRegular);
        static const StreamFlags notRegularMask = SfDynamic | SfHidden;

        for (auto& pair : _streams) {
            Stream* stream = pair.second.get();
            StreamFlags flags = stream->getFlags();

            if ( ((flags & mask) != 0) ||
                (((flags & notRegularMask) == 0) && includeRegular)) {

                streams.push_back(stream);
            }
        }

        std::swap(streams, out);
    }

    StreamBuffer* Store::_getStreamBuffer(BufferId id)
    {
        auto it = _buffers.find(id);
        if (it == _buffers.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    Stream* Store::_getStream(StreamId id)
    {
        auto it = _streams.find(id);
        if (it == _streams.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    BufferId Store::registerStreamBuffer(size_t segmentSize,
                                         uint32_t numSegments)
    {
        LockScopeExclusive(_lock);
        return _registerStreamBuffer(segmentSize, numSegments);
    }

    StreamId Store::registerStream(StreamDescriptor& desc,
                                   BufferId buffer)
    {
        LockScopeExclusive(_lock);
        return _registerStream(INVALID_STREAM_ID, desc, buffer);
    }

    void Store::enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        LockScopeShared(_lock);
        _enumerateStreamBuffers(out);
    }

    void Store::enumerateStreams(std::vector<StreamId>& out,
                                 StreamEnumFilter filter) const
    {
        LockScopeShared(_lock);
        _enumerateStreams(out, filter);
    }

    uint32_t Store::summarizeStreamStats(StreamStatistics& stats,
                                         uint64_t& uncompressedSize,
                                         StreamEnumFilter filter) const
    {
        StreamStatistics lstats;
        uint64_t uncomprSize = 0;
        memset(&lstats, 0, sizeof(StreamStatistics));
        memset(&lstats.ranges, static_cast<int>(INVALID_LARGE_OBJECT_ID),
               sizeof(StreamRangeInformation));

        if (IsSet(filter, StreamEnumFilter::SefDynamic)) {
            filter = static_cast<StreamEnumFilter>(
                filter ^ StreamEnumFilter::SefDynamic);
        }

        std::vector<Stream*> streams;
        LockShared(_lock); {
            this->_enumerateStreams(streams, filter);
        } Unlock();

        for (const auto stream : streams) {
            assert(!IsSet(stream->getFlags(), StreamFlags::SfDynamic));

            StreamQueryInformation info;
            stream->queryInformation(info);

            lstats.compressedSize += info.stats.compressedSize;
            lstats.entryCount     += info.stats.entryCount;
            lstats.rawEntryCount  += info.stats.rawEntryCount;
            uncomprSize           += info.stats.rawEntryCount *
                getEntrySize(&info.descriptor.type);

            for (int i = 0; i < 3; ++i) {
                Range& in  = info.stats.ranges.ranges[i];
                Range& sum = lstats.ranges.ranges[i];

                if (in.start < sum.start) {
                    sum.start = in.start;
                }

                if ((sum.end == INVALID_LARGE_OBJECT_ID) ||
                    ((in.end != INVALID_LARGE_OBJECT_ID) &&
                     (in.end > sum.end))) {
                    sum.end = in.end;
                }
            }
        }

        stats = lstats;
        uncompressedSize = uncomprSize;

        return static_cast<uint32_t>(streams.size());
    }

    StoreId Store::getId() const
    {
        return _id;
    }

    const std::string& Store::getName() const
    {
        return _name;
    }

    StreamBuffer& Store::getStreamBuffer(BufferId id)
    {
        LockScopeShared(_lock);

        StreamBuffer* buffer = _getStreamBuffer(id);
        ThrowOnNull(buffer, NotFoundException,
                    stringFormat("stream buffer with id %d", id));

        return *buffer;
    }

    Stream& Store::getStream(StreamId id)
    {
        LockScopeShared(_lock);

        Stream* stream = _getStream(id);
        ThrowOnNull(stream, NotFoundException,
                    stringFormat("stream with id %d", id));

        return *stream;
    }

    Stream* Store::findStream(StreamId id)
    {
        LockScopeShared(_lock);

        return _getStream(id);
    }

}
