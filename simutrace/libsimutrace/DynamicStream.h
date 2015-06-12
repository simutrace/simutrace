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
#ifndef DYNAMIC_STREAM_H
#define DYNAMIC_STREAM_H

#include "SimuStor.h"

#include "ClientStream.h"

namespace SimuTrace
{
    class DynamicStream :
        public ClientStream
    {
    private:
        DISABLE_COPY(DynamicStream);

        DynamicStreamDescriptor _desc;

        void _initializeDynamicHandle(StreamHandle handle,
                                      StreamStateFlags flags,
                                      StreamAccessFlags aflags,
                                      void* userData);

        virtual StreamHandle _append(StreamHandle handle) override;
        virtual StreamHandle _open(QueryIndexType type, uint64_t value,
                                   StreamAccessFlags flags,
                                   StreamHandle handle) override;

        virtual void _closeHandle(StreamHandle handle) override;
    public:
        DynamicStream(StreamId id, const DynamicStreamDescriptor& desc,
                      StreamBuffer& buffer, ClientSession& session);
        virtual ~DynamicStream() override;

        virtual void queryInformation(
            StreamQueryInformation& informationOut) const override;
    };
}

#endif