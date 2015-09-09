/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef SERVER_STORE_H
#define SERVER_STORE_H

#include "SimuStor.h"
#include "StreamEncoder.h"

namespace SimuTrace
{

    class ServerSession;
    class ServerStreamBuffer;

    class ServerStore :
        public Store
    {
    protected:
        struct EncoderDescriptor {
            StreamEncoder::FactoryMethod factoryMethod;
            const StreamTypeDescriptor* type;
        };
    private:
        DISABLE_COPY(ServerStore);

        uint32_t _referenceCount;
        std::list<SessionId> _references;

        IdAllocator<BufferId> _bufferIdAllocator;
        IdAllocator<StreamId> _streamIdAllocator;

        std::map<StreamTypeId, EncoderDescriptor> _encoderMap;

        EncoderDescriptor* _findEncoder(const StreamTypeId& type);
        bool _findReference(SessionId session) const;
    protected:
        static const StreamTypeId _defaultEncoderTypeId;

        ServerStore(StoreId id, const std::string& name);

        void _registerEncoder(const StreamTypeId& type,
                              EncoderDescriptor& descriptor);

        virtual std::unique_ptr<StreamBuffer> _createStreamBuffer(
            size_t segmentSize, uint32_t numSegments) override;
        virtual std::unique_ptr<Stream> _createStream(StreamId id,
            StreamDescriptor& desc, BufferId buffer) override;

        virtual void _enumerateStreamBuffers(std::vector<BufferId>& out) const override;
        virtual void _enumerateStreams(std::vector<StreamId>& out,
                                       StreamEnumFilter filter) const override;
    public:
        virtual ~ServerStore() override;

        void attach(SessionId session);
        uint32_t detach(SessionId session);

        void enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const;
        void enumerateStreams(std::vector<Stream*>& out,
                              StreamEnumFilter filter) const;

        StreamEncoder::FactoryMethod getEncoderFactory(const StreamTypeId& type);
    };

}

#endif