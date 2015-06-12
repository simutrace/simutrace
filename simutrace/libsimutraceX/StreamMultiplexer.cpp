/*
 * Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus
 *
 * Simutrace Extensions Library (libsimutraceX) is part of Simutrace.
 *
 * libsimutraceX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutraceX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutraceX. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuTraceInternal.h"

#include "StreamMultiplexer.h"

namespace SimuTrace
{

    class StreamMultiplexer::HandleStreamContext
    {
    private:
        DISABLE_COPY(HandleStreamContext);

    public:
        StreamHandle handle;
        MultiplexerEntry entry;

        HandleStreamContext(const StreamMultiplexer& multiplexer,
                            uint32_t streamIndex, QueryIndexType type,
                            uint64_t value, StreamAccessFlags flags) :
            handle(nullptr),
            entry()
        {
            assert(streamIndex < multiplexer._inputStreams.size());
            entry.streamIdx = streamIndex;
            entry.streamId  = multiplexer._inputStreams[streamIndex];

            handle = StStreamOpen(multiplexer._session, entry.streamId,
                                  type, value, flags, nullptr);
            ThrowOnNull(handle, SimutraceException);
        }

        ~HandleStreamContext()
        {
            StStreamClose(handle);
        }

        inline bool update(bool temporal)
        {
            // Get the first entry from the stream. This might fail and return
            // nullptr instead. We treat this as end of stream.
            entry.entry = StGetNextEntryFast(&handle);
            if (entry.entry == nullptr) {
                return false;
            }

            entry.cycleCount = (temporal) ?
                *(reinterpret_cast<CycleCount*>(entry.entry)) & TEMPORAL_ORDER_CYCLE_COUNT_MASK :
                INVALID_CYCLE_COUNT;

            return true;
        }

        inline static bool compareByCycleCountWithPointer(
            const HandleStreamContext* ctx1, const HandleStreamContext* ctx2)
        {
            if (ctx2 == nullptr) {
                return true;
            } else if (ctx1 == nullptr) {
                return false;
            } else {
                assert(ctx1->entry.cycleCount != INVALID_CYCLE_COUNT);
                assert(ctx2->entry.cycleCount != INVALID_CYCLE_COUNT);

                return ctx1->entry.cycleCount < ctx2->entry.cycleCount;
            }
        }
    };

    class StreamMultiplexer::HandleContext
    {
    private:
        DISABLE_COPY(HandleContext);

    public:
        const StreamMultiplexer& multiplexer;
        std::vector<std::unique_ptr<HandleStreamContext>> streams;

        std::vector<HandleStreamContext*> list;
        int index;

        HandleContext(const StreamMultiplexer& multiplexer) :
            multiplexer(multiplexer),
            streams(),
            list(),
            index(-1) { }
    };

    StreamMultiplexer::StreamMultiplexer(SessionId session,
        const std::string& name, MultiplexingRule rule, MultiplexerFlags flags,
        const std::vector<StreamId>& inputStreams) :
        _id(INVALID_STREAM_ID),
        _session(session),
        _rule(rule),
        _flags(flags),
        _inputStreams(inputStreams)
    {
        ThrowOn(_inputStreams.size() < 2, ArgumentException, "inputStreams");

        // Build the type that we will return with our multiplexer based on
        // the flags and input streams. Then we can register a new dynamic
        // stream with this type.
        DynamicStreamDescriptor desc;
        _inferType(name, desc);

        _temporal = IsSet(desc.base.type.flags, StreamTypeFlags::StfTemporalOrder);

        _id = StStreamRegisterDynamic(session, &desc);
        ThrowOn(_id == INVALID_STREAM_ID, SimutraceException);
    }

    StreamMultiplexer::~StreamMultiplexer()
    {

    }

    DynamicStreamGetNextEntry StreamMultiplexer::_getNextEntryHandler(
        bool temporal) const
    {
        switch (_rule) {
            case MultiplexingRule::MxrRoundRobin:
                if (IsSet(_flags, MultiplexerFlags::MxfIndirect)) {
                    if (temporal) {
                        return _getNextEntry<MxrRoundRobin, true, true>;
                    } else {
                        return _getNextEntry<MxrRoundRobin, true, false>;
                    }
                } else {
                    if (temporal) {
                        return _getNextEntry<MxrRoundRobin, false, true>;
                    } else {
                        return _getNextEntry<MxrRoundRobin, false, false>;
                    }
                }

                break;

            case MultiplexingRule::MxrCycleCount:
                ThrowOn(!temporal, Exception, "Cannot use cyclecount-based "
                    "multiplexing for streams that are not temporally ordered.");

                if (IsSet(_flags, MultiplexerFlags::MxfIndirect)) {
                    return _getNextEntry<MxrCycleCount, true, true>;
                } else {
                    return _getNextEntry<MxrCycleCount, false, true>;
                }

                break;

            default:
                Throw(NotSupportedException);
                break;
        }
    }

    void StreamMultiplexer::_inferType(const std::string& name,
                                       DynamicStreamDescriptor& descOut) const
    {
        const StreamTypeDescriptor* type = nullptr;
        bool sameType = true;
        bool temporal = true;

        assert(_inputStreams.size() > 0);
        for (int i = 0; i < _inputStreams.size(); ++i) {
            StreamQueryInformation query;

            _bool success = StStreamQuery(_session, _inputStreams[i], &query);
            ThrowOn(!success, SimutraceException);

            // We take the type from the first stream and check if all
            // streams share the same type. In that case we can take the
            // same type for the multiplexer output. Otherwise, we have to
            // take an indirection type.
            if (i == 0) {
                type = &query.descriptor.type;
            } else {
                assert(type != nullptr);

                if (memcmp(type, &query.descriptor.type,
                           sizeof(StreamTypeDescriptor)) != 0) {
                    sameType = false;
                }
            }

            // We check if all types use temporal order. If not, we cannot
            // apply the CycleCount multiplexing policy (regardless if the
            // types are the same or not).
            if (!IsSet(query.descriptor.type.flags,
                       StreamTypeFlags::StfTemporalOrder)) {
                temporal = false;
            }
        }

        // Setup the multiplexer handler functions according to the requested
        // multiplexing rule.
        DynamicStreamOperations operations;
        memset(&operations, 0, sizeof(DynamicStreamOperations));

        operations.finalize     = _finalize;
        operations.open         = _open;
        operations.close        = _close;
        operations.getNextEntry = _getNextEntryHandler(temporal);

        // Choose the type we will use for the multiplexer
        if ((!sameType) || (IsSet(_flags, MultiplexerFlags::MxfIndirect))) {
            ThrowOn(!IsSet(_flags, MultiplexerFlags::MxfIndirect),
                Exception, "The input streams posses different types. To "
                "build a multiplexer from this set of streams you must "
                "specify the MxfIndirect flag.");

            type = streamFindMultiplexerType(temporal);
        }

        // Build the type for the multiplexer. We will use this multiplexer
        // class instance as user data so we can still have a reference to our
        // full context in the handler functions.
        _bool success = StMakeStreamDescriptorDynamicFromType(name.c_str(),
                            this, type, &operations, &descOut);
        ThrowOn(!success, SimutraceException);
    }

    void StreamMultiplexer::_finalize(StreamId id, void* userData)
    {
        // ---- This function must not throw ----

        assert(userData != nullptr);

        StreamMultiplexer* multiplexer =
            reinterpret_cast<StreamMultiplexer*>(userData);
        assert(multiplexer != nullptr);
        assert(multiplexer->_id == id);

        delete multiplexer;
    }

    int StreamMultiplexer::_open(const DynamicStreamDescriptor* descriptor,
        StreamId id, QueryIndexType type, uint64_t value,
        StreamAccessFlags flags, void** userDataOut)
    {
        // ---- This function must not throw ----

        assert(descriptor != nullptr);
        assert(userDataOut != nullptr);

        StreamMultiplexer* multiplexer =
            reinterpret_cast<StreamMultiplexer*>(descriptor->userData);
        assert(multiplexer != nullptr);
        assert(multiplexer->_id == id);

        try {
            std::unique_ptr<HandleContext> ctx(new HandleContext(*multiplexer));
            ctx->streams.reserve(multiplexer->_inputStreams.size());
            ctx->list.reserve(multiplexer->_inputStreams.size());

            for (uint32_t i = 0; i < multiplexer->_inputStreams.size(); ++i) {
                std::unique_ptr<HandleStreamContext> sctx(
                    new HandleStreamContext(*multiplexer, i, type, value, flags));

                // Load the first entry from the stream. If the entry is null,
                // we assume the stream to be empty
                if (sctx->update(multiplexer->_temporal)) {
                    ctx->list.push_back(sctx.get());
                }

                ctx->streams.push_back(std::move(sctx));
            }

            // For the cycle count rule we use a sorted list. This way we can
            // always take the first entry (lowest cycle count). Inserting the
            // next entry should be cheap in most cases, too, because often we
            // have only have to compare the first few entries to find the
            // right position.
            if (multiplexer->_rule == MultiplexingRule::MxrCycleCount) {
                std::sort(ctx->list.begin(), ctx->list.end(),
                          HandleStreamContext::compareByCycleCountWithPointer);
            }

            *userDataOut = ctx.get();

            ctx.release();
        } catch (const Exception& e) {
            return e.getErrorCode();
        }

        return 0;
    }

    void StreamMultiplexer::_close(StreamId id, void** userData)
    {
        // ---- This function must not throw ----

        assert(userData != nullptr);

        HandleContext* ctx = reinterpret_cast<HandleContext*>(*userData);
        assert(ctx != nullptr);

        delete ctx;
    }

    template<MultiplexingRule rule, bool indirect, bool temporal>
    int _getNextEntry(void* userData, void** entryOut)
    {
        // ---- This function must not throw ----

        assert(userData != nullptr);
        assert(entryOut != nullptr);

        auto ctx = reinterpret_cast<StreamMultiplexer::HandleContext*>(userData);

        if (ctx->index == -1) {
            // This indicates that this is the first access or that all
            // input streams are exhausted.
            if (ctx->list.size() > 0) {
                ctx->index = 0;
            } else {
                *entryOut = nullptr;
                return 0;
            }
        } else {
            // We give the user the guarantee that the entry returned in the
            // last call is valid up to this call. We therefore can update the
            // entry only now.
            assert(ctx->index < ctx->list.size());
            assert(ctx->list[ctx->index] != nullptr);
            auto sctx = ctx->list[ctx->index];

            if (!sctx->update(temporal)) {
                ctx->list.erase(ctx->list.begin() + ctx->index);

                // Check if there are still input streams. If not, signal the
                // end of the dynamic stream. Otherwise, the list can be left
                // unmodified as we automatically moved to the next element by
                // removing the current one.
                if (ctx->list.size() == 0) {
                    ctx->index = -1;
                    *entryOut = nullptr;
                    return 0;
                }

                assert(ctx->index <= ctx->list.size());
                if (ctx->index == ctx->list.size()) {
                    ctx->index--;
                }
            } else {
                // Switch should be optimized out by the compiler
                switch (rule) {
                    case MultiplexingRule::MxrRoundRobin: {
                        // The round-robin rule increments the index and leaves
                        // the elements at their original position in the list.
                        ctx->index = (ctx->index + 1) % ctx->list.size();

                        break;
                    }

                    case MultiplexingRule::MxrCycleCount: {
                        // With the cycle count rule, we modify the list to
                        // keep it sorted. We then always take the first
                        // element from the list as this will have the lowest
                        // cycle count. The list must be sorted right from the
                        // start.
                        assert(ctx->index == 0);

                        int i = 0;
                        while (true) {
                            if ((++i >= ctx->list.size()) ||
                                (sctx->entry.cycleCount <=
                                ctx->list[i]->entry.cycleCount)) {

                                ctx->list[i - 1] = sctx;
                                break;
                            }

                            ctx->list[i - 1] = ctx->list[i];
                        }

                        break;
                    }

                    default: {
                        assert(false); // Should never happen
                        break;
                    }
                }
            }
        }

        assert(ctx->index < ctx->list.size());
        assert(ctx->list[ctx->index] != nullptr);
        assert(ctx->list[ctx->index]->entry.entry != nullptr);
        *entryOut = (indirect) ? &ctx->list[ctx->index]->entry :
            ctx->list[ctx->index]->entry.entry;

        return 0;
    }

    StreamId StreamMultiplexer::getStreamId() const
    {
        assert(_id != INVALID_STREAM_ID);
        return _id;
    }

}