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
#ifndef CLIENT_STORE_H
#define CLIENT_STORE_H

#include "SimuStor.h"
#include "ClientObject.h"

namespace SimuTrace
{

    class ClientSession;

    class ClientStore :
        public Store,
        public ClientObject
    {
    private:
        DISABLE_COPY(ClientStore);

        void _replicateStreamBuffer(BufferId buffer);
        void _replicateStream(StreamId stream);

        void _replicateConfiguration(bool update);

        virtual std::unique_ptr<StreamBuffer> _createStreamBuffer(
            size_t segmentSize, uint32_t numSegments) override;
        virtual std::unique_ptr<Stream> _createStream(StreamId id, 
            StreamDescriptor& desc, BufferId buffer) override;
        virtual std::unique_ptr<DataPool> _createDataPool(PoolId id, 
            StreamId stream) override;

        virtual void _enumerateStreamBuffers(std::vector<BufferId>& out) const override;
        virtual void _enumerateStreams(std::vector<StreamId>& out,
                                       bool includeHidden) const override;
        virtual void _enumerateDataPools(std::vector<PoolId>& out) const override;
    public:
        ClientStore(ClientSession& session, const std::string& name, 
                    bool alwaysCreate);
        virtual ~ClientStore() override;

        virtual void detach(SessionId session) override;
    };

}

#endif