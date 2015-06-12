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
#pragma once
#ifndef CLIENT_STREAM_H
#define CLIENT_STREAM_H

#include "SimuStor.h"

#include "ClientObject.h"

namespace SimuTrace
{
    class ClientStream :
        public Stream,
        public ClientObject
    {
    private:
        DISABLE_COPY(ClientStream);

        CriticalSection _lock;

        std::unique_ptr<StreamStateDescriptor> _writeHandle;
        std::list<std::unique_ptr<StreamStateDescriptor>> _readHandles;
    protected:
        void _addHandle(std::unique_ptr<StreamStateDescriptor>& handle);
        void _releaseHandle(StreamHandle handle);

        virtual StreamHandle _append(StreamHandle handle) = 0;
        virtual StreamHandle _open(QueryIndexType type, uint64_t value,
                                   StreamAccessFlags flags,
                                   StreamHandle handle) = 0;

        virtual void _closeHandle(StreamHandle handle) = 0;
    public:
        ClientStream(StreamId id, const StreamDescriptor& desc,
                     StreamBuffer& buffer, ClientSession& session);
        virtual ~ClientStream() override;

        virtual void queryInformation(
            StreamQueryInformation& informationOut) const = 0;

        StreamHandle append(StreamHandle handle);
        StreamHandle open(QueryIndexType type, uint64_t value,
                          StreamAccessFlags flags,
                          StreamHandle handle);

        void close(StreamHandle handle);

        void flush();
    };
}

#endif