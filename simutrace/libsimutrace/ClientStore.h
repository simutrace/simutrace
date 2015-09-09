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

        StreamId _dynamicStreamIdBoundary;

        ClientStore(ClientSession& session, const std::string& name,
                    bool alwaysCreate, bool open);

        StreamBuffer* _replicateStreamBuffer(BufferId buffer);
        Stream* _replicateStream(StreamId stream);

        void _replicateConfiguration();
        void _updateStaticStreamIdBoundary(const std::vector<StreamId>& ids);

        virtual std::unique_ptr<StreamBuffer> _createStreamBuffer(
            size_t segmentSize, uint32_t numSegments) override;
        virtual std::unique_ptr<Stream> _createStream(StreamId id,
            StreamDescriptor& desc, BufferId buffer) override;

        virtual void _enumerateStreamBuffers(std::vector<BufferId>& out) const override;
        virtual void _enumerateStreams(std::vector<StreamId>& out,
                                       StreamEnumFilter filter) const override;

        virtual StreamBuffer* _getStreamBuffer(BufferId id) override;
        virtual Stream* _getStream(StreamId id) override;
    public:
        virtual ~ClientStore() override;

        static ClientStore* create(ClientSession& session,
                                   const std::string& name,
                                   bool alwaysCreate);
        static ClientStore* open(ClientSession& session,
                                 const std::string& name);
    };

}

#endif