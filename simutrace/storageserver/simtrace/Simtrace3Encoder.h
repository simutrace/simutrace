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
#ifndef SIMTRACE3_ENCODER_H
#define SIMTRACE3_ENCODER_H

#include "SimuStor.h"
#include "../StreamEncoder.h"
#include "../WorkItem.h"

#include "Simtrace3Store.h"

namespace SimuTrace {
namespace Simtrace
{

    class Simtrace3Frame;

    class Simtrace3Encoder :
        public StreamEncoder
    {
    private:
        DISABLE_COPY(Simtrace3Encoder);

        ServerStream* _stream;

        struct WorkerContext;

        virtual void _encode(Simtrace3Frame& frame, SegmentId id,
                             StreamSegmentId sequenceNumber) = 0;
        virtual void _decode(Simtrace3StorageLocation& location, SegmentId id,
                             StreamSegmentId sequenceNumber) = 0;

        static void _writerMain(WorkItem<WorkerContext>& workItem,
                                WorkerContext& context);

        static void _readerMain(WorkItem<WorkerContext>& workItem,
                                WorkerContext& context);

    protected:
        ServerStream* _getStream() const;

    public:
        Simtrace3Encoder(ServerStore& store, const std::string& friendlyName,
                         ServerStream* stream);
        virtual ~Simtrace3Encoder() override;

        virtual void initialize(Simtrace3Frame& frame, bool isOpen);

        virtual bool read(ServerStreamBuffer& buffer, SegmentId segment,
                          StreamAccessFlags flags, StorageLocation& location,
                          bool prefetch) override;
        virtual bool write(ServerStreamBuffer& buffer, SegmentId segment,
                           std::unique_ptr<StorageLocation>& locationOut) override;

        std::unique_ptr<StorageLocation> makeStorageLocation(
            Simtrace3Frame& frame);
    };

}
}

#endif