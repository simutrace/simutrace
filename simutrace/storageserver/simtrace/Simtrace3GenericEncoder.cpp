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
#include "SimuStor.h"

#include "Simtrace3GenericEncoder.h"

#include "../ScratchSegment.h"
#include "../ServerStream.h"
#include "../ServerStreamBuffer.h"

#include "../StorageServer.h"
#include "../WorkItem.h"
#include "../WorkerPool.h"

#include "Simtrace3Store.h"
#include "Simtrace3Frame.h"
#include "Simtrace3Encoder.h"

#include "ProfileSimtrace3GenericEncoder.h"

namespace SimuTrace {
namespace Simtrace
{

    Simtrace3GenericEncoder::Simtrace3GenericEncoder(ServerStore& store,
                                                     ServerStream* stream) :
        Simtrace3Encoder(store, "Simtrace3 LZMA Encoder", stream)
    {
        profileCreateProfiler();
    }

    void Simtrace3GenericEncoder::_encode(Simtrace3Frame& frame, SegmentId id, 
                                         StreamSegmentId sequenceNumber)
    {
        StreamBuffer& buffer = _getStream()->getStreamBuffer();
        SegmentControlElement* ctrl = buffer.getControlElement(id);
        ScratchSegment targetSeg;

        // Compress the input buffer. This may take considerable time!
        void* targetBuffer = targetSeg.getBuffer();
        void* sourceBuffer = buffer.getSegment(id);

        size_t targetLength = targetSeg.getLength();
        size_t sourceLength = getEntrySize(&_getStream()->getType()) * 
            ctrl->rawEntryCount;

        targetLength = Compression::lzmaCompress(sourceBuffer, sourceLength,
                                                 targetBuffer, targetLength,
                                                 defaultCompressionLevel);

        frame.addAttribute(Simtrace3AttributeType::SatData, sourceLength,
                           targetLength, targetBuffer);
    }

    void Simtrace3GenericEncoder::_decode(Simtrace3StorageLocation& location, 
                                          SegmentId id, 
                                          StreamSegmentId sequenceNumber)
    {
        StreamBuffer& buffer = _getStream()->getStreamBuffer();
        Simtrace3Store& store = static_cast<Simtrace3Store&>(_getStore());

        Simtrace3Frame frame;

        // Load frame description and attributes into memory
        store.readFrame(frame, location);

        // Find data attribute
        AttributeHeaderDescription* dataAttr = nullptr;
        dataAttr = frame.findAttribute(Simtrace3AttributeType::SatData);

        ThrowOnNull(dataAttr, Exception, "Unable to find data attribute.");

        ThrowOn(dataAttr->header.uncompressedSize > buffer.getSegmentSize(),
                Exception, stringFormat(
                "The segment size (%s) used to create the trace file "
                "exceeds the current maximum segment size (%s).",
                sizeToString(dataAttr->header.uncompressedSize).c_str(),
                sizeToString(buffer.getSegmentSize()).c_str()));

        // Decompress the input buffer. This may take considerable time!
        void* targetBuffer = buffer.getSegment(id);
        void* sourceBuffer = dataAttr->buffer;

        size_t targetLength = static_cast<size_t>(buffer.getSegmentSize());
        size_t sourceLength = static_cast<size_t>(dataAttr->header.size);

        targetLength = Compression::lzmaDecompress(sourceBuffer, 
                                                    sourceLength,
                                                    targetBuffer, 
                                                    targetLength);

        if (targetLength != dataAttr->header.uncompressedSize) {
            LogWarn("Size mismatch after decompression "
                    "<stream: %d, sqn: %d>", location.link.stream, 
                    location.link.sequenceNumber);
        }
    }

    StreamEncoder* Simtrace3GenericEncoder::factoryMethod(ServerStore& store, 
                                                          ServerStream* stream)
    {
        return new Simtrace3GenericEncoder(store, stream);
    }

}
}