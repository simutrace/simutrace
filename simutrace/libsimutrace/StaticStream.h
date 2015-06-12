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
#pragma once
#ifndef STATIC_STREAM_H
#define STATIC_STREAM_H

#include "SimuStor.h"

#include "ClientStream.h"

namespace SimuTrace
{
    class StaticStream :
        public ClientStream
    {
    private:
        DISABLE_COPY(StaticStream);

        void _initializeStaticHandle(StreamHandle handle, SegmentId segment,
                                     StreamStateFlags flags = SsfNone,
                                     StreamAccessFlags aflags = SafNone,
                                     size_t offset = 0);
        void _invalidateStaticHandle(StreamHandle handle);

        byte* _getPayload(SegmentId segment, uint32_t* lengthOut) const;

        static void _payloadAllocatorWrite(Message& msg, bool free, void* args);
        static void _payloadAllocatorRead(Message& msg, bool free, void* args);

        virtual StreamHandle _append(StreamHandle handle) override;
        virtual StreamHandle _open(QueryIndexType type, uint64_t value,
                                   StreamAccessFlags flags,
                                   StreamHandle handle) override;

        virtual void _closeHandle(StreamHandle handle) override;
    public:
        StaticStream(StreamId id, const StreamDescriptor& desc,
                     StreamBuffer& buffer, ClientSession& session);
        virtual ~StaticStream() override;

        virtual void queryInformation(
            StreamQueryInformation& informationOut) const override;
    };
}

#endif