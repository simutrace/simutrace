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
#pragma once
#ifndef STREAM_MULTIPLEXER_H
#define STREAM_MULTIPLEXER_H

#include "SimuTraceInternal.h"

#include "SimuTraceX.h"

namespace SimuTrace
{

    class StreamMultiplexer
    {
    private:
        class HandleStreamContext;
        class HandleContext;

    private:
        DISABLE_COPY(StreamMultiplexer);

        StreamId _id;
        bool _temporal;

        const SessionId _session;
        const MultiplexingRule _rule;
        const MultiplexerFlags _flags;
        const std::vector<StreamId> _inputStreams;

        DynamicStreamGetNextEntry _getNextEntryHandler(bool temporal) const;
        void _inferType(const std::string& name,
                        DynamicStreamDescriptor& descOut) const;

        static void _finalize(StreamId id, void* userData);

        static int _open(const DynamicStreamDescriptor* descriptor, StreamId id,
                         QueryIndexType type, uint64_t value,
                         StreamAccessFlags flags, void** userDataOut);
        static void _close(StreamId id, void** userData);

        template<MultiplexingRule rule, bool indirect, bool temporal>
        friend int _getNextEntry(void* userData, void** entryOut);
    public:
        StreamMultiplexer(SessionId session, const std::string& name,
                          MultiplexingRule rule, MultiplexerFlags flags,
                          const std::vector<StreamId>& inputStreams);
        ~StreamMultiplexer();

        StreamId getStreamId() const;
    };

}

#endif