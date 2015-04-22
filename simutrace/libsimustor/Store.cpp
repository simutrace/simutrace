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
        _name(name)
    {
        ThrowOn(id == INVALID_STORE_ID, ArgumentException);
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
        assert(buffer != nullptr);
        StreamBuffer* buf = buffer.get();

        BufferId id = buffer->getId();
        assert(id != INVALID_BUFFER_ID);
        assert(_buffers.find(id) == _buffers.end());

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
        assert(stream != nullptr);
        Stream* str = stream.get();

        StreamId id = stream->getId();
        assert(id != INVALID_STREAM_ID);
        assert(_streams.find(id) == _streams.end());

        _streams[id] = std::move(stream);

        const StreamTypeDescriptor& type = str->getType();
        LogInfo("<store: %s> Registered %s stream "
                "<id: %d, name: '%s', type: %s, esize: %s (%s)>.",
                _name.c_str(), (str->isHidden()) ? "private" : "public",
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
        ThrowOn(_buffers.size() > SIMUTRACE_STORE_MAX_NUM_STREAMBUFFERS,
                InvalidOperationException);

        auto buffer = _createStreamBuffer(segmentSize, numSegments);

        return _addStreamBuffer(buffer);
    }

    StreamId Store::_registerStream(StreamId id, StreamDescriptor& desc,
                                    BufferId buffer)
    {
        ThrowOn(_streams.size() > SIMUTRACE_STORE_MAX_NUM_STREAMS,
                InvalidOperationException);

        auto stream = _createStream(id, desc, buffer);

        return _addStream(stream);
    }

    void Store::_lockConfiguration()
    {
        _configurationLocked = true;

        LogDebug("<store: %s> Locked configuration.", _name.c_str());
    }

    void Store::_freeConfiguration()
    {
        _streams.clear();
        _buffers.clear();

        _configurationLocked = false;
    }

    void Store::_enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const
    {
        out.clear();
        out.reserve(_buffers.size());

        for (auto& pair : _buffers) {
            out.push_back(pair.second.get());
        }
    }

    void Store::_enumerateStreams(std::vector<Stream*>& out,
                                  bool includeHidden) const
    {
        out.clear();
        out.reserve(_streams.size());

        for (auto& pair : _streams) {
            Stream* stream = pair.second.get();

            if ((!stream->isHidden()) || includeHidden) {
                out.push_back(stream);
            }
        }
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
        ThrowOn(_configurationLocked, InvalidOperationException);

        return _registerStreamBuffer(segmentSize, numSegments);
    }

    StreamId Store::registerStream(StreamDescriptor& desc,
                                   BufferId buffer)
    {
        LockScopeExclusive(_lock);
        ThrowOn(_configurationLocked, InvalidOperationException);

        return _registerStream(INVALID_STREAM_ID, desc, buffer);
    }

    void Store::enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        LockScopeShared(_lock);
        _enumerateStreamBuffers(out);
    }

    void Store::enumerateStreams(std::vector<StreamId>& out,
                                 bool includeHidden) const
    {
        LockScopeShared(_lock);
        _enumerateStreams(out, includeHidden);
    }

    uint32_t Store::queryTotalStreamStats(StreamStatistics& stats,
                                          uint64_t& uncompressedSize,
                                          bool includeHidden) const
    {
        memset(&stats, 0, sizeof(StreamStatistics));
        memset(&stats.ranges, static_cast<int>(INVALID_LARGE_OBJECT_ID),
               sizeof(StreamRangeInformation));
        uncompressedSize = 0;

        std::vector<Stream*> streams;
        this->_enumerateStreams(streams, includeHidden);
        for (const auto stream : streams) {
            StreamQueryInformation info;
            stream->queryInformation(info);

            stats.compressedSize += info.stats.compressedSize;
            stats.entryCount     += info.stats.entryCount;
            stats.rawEntryCount  += info.stats.rawEntryCount;
            uncompressedSize     += info.stats.rawEntryCount *
                getEntrySize(&info.descriptor.type);

            for (int i = 0; i < 3; ++i) {
                Range& in  = info.stats.ranges.ranges[i];
                Range& sum = stats.ranges.ranges[i];

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
        ThrowOnNull(buffer, NotFoundException);

        return *buffer;
    }

    Stream& Store::getStream(StreamId id)
    {
        LockScopeShared(_lock);

        Stream* stream = _getStream(id);
        ThrowOnNull(stream, NotFoundException);

        return *stream;
    }

    Stream* Store::findStream(StreamId id)
    {
        LockScopeShared(_lock);

        return _getStream(id);
    }

}
