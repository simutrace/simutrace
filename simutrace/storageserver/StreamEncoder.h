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
#ifndef STREAM_ENCODER_H
#define STREAM_ENCODER_H

#include "SimuStor.h"
#include "ServerStream.h"

namespace SimuTrace
{

    class ServerStore;
    class ServerStreamBuffer;

    class StreamEncoder
    {
    public:
        typedef StreamEncoder* (*FactoryMethod)(ServerStore&, ServerStream*);

    private:
        DISABLE_COPY(StreamEncoder);

        const std::string _friendlyName;
        ServerStore& _store;

    protected:
        ServerStore& _getStore() const { return _store; }

    public:
        StreamEncoder(ServerStore& store, const std::string& friendlyName) :
            _friendlyName(friendlyName),
            _store(store) { }
        virtual ~StreamEncoder() { }

        virtual void close(StreamWait* wait = nullptr) { };

        virtual bool read(ServerStreamBuffer& buffer, SegmentId segment,
                          StreamAccessFlags flags, StorageLocation& location,
                          bool prefetch) = 0;
        virtual bool write(ServerStreamBuffer& buffer, SegmentId segment,
                           std::unique_ptr<StorageLocation>& locationOut) = 0;

        virtual void notifySegmentClosed(StreamSegmentId segment) { };
        virtual void notifySegmentCacheClosed(StreamSegmentId segment) { };

        virtual void queryInformationStream(
            StreamQueryInformation& informationOut) { };

        const std::string& getFriendlyName() { return _friendlyName; }
    };

}

#endif