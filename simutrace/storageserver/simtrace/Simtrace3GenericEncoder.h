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
#ifndef SIMTRACE3_ZLIB_ENCODER_H
#define SIMTRACE3_ZLIB_ENCODER_H

#include "SimuStor.h"
#include "../WorkItem.h"

#include "Simtrace3Encoder.h"

namespace SimuTrace {
namespace Simtrace
{

    class Simtrace3GenericEncoder :
        public Simtrace3Encoder
    {
    private:
        DISABLE_COPY(Simtrace3GenericEncoder);

        static const int defaultCompressionLevel = 4;

        virtual void _encode(Simtrace3Frame& frame, SegmentId id, 
                             StreamSegmentId sequenceNumber) override;
        virtual void _decode(Simtrace3StorageLocation& location, SegmentId id, 
                             StreamSegmentId sequenceNumber) override;

    public:
        Simtrace3GenericEncoder(ServerStore& store, ServerStream* stream);

        static StreamEncoder* factoryMethod(ServerStore& store,
                                            ServerStream* stream);
    };

}
}
#endif