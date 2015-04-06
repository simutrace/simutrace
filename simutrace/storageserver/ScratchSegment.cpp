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

#include "ScratchSegment.h"

#include "StorageServer.h"
#include "ServerStreamBuffer.h"

namespace SimuTrace
{

    ScratchSegment::ScratchSegment() :
        _buffer(StorageServer::getInstance().getMemoryPool()),
        _id(INVALID_SEGMENT_ID),
        _fallbackMemory(nullptr, nullptr)
    {
        _initializeMemory();
    }

    ScratchSegment::ScratchSegment(ServerStreamBuffer& buffer) :
        _buffer(buffer),
        _id(INVALID_SEGMENT_ID),
        _fallbackMemory(nullptr, nullptr)
    {
        _initializeMemory();
    }

    void ScratchSegment::_initializeMemory()
    {
        // We request a new segment from the stream buffer that contains the
        // stream segment we need to compress. This guarantees that we have a
        // buffer with a sufficient size.

        _id = _buffer.requestScratchSegment();
        if (_id == INVALID_SEGMENT_ID) {
            LogWarn("Falling back to private segment memory.");

            _fallbackMemory = PrivateMemoryReference(
                new PrivateMemorySegment(_buffer.getSegmentSize()),
                [](PrivateMemorySegment* instance) {
               delete instance;

               LogMem("Private segment memory released.");
            });

            _fallbackMemory->map();
        }
    }

    ScratchSegment::~ScratchSegment()
    {
        if (_id != INVALID_SEGMENT_ID) {
            assert(_fallbackMemory == nullptr);
            _buffer.purgeSegment(_id);

            _id = INVALID_SEGMENT_ID;
        }
    }

    void* ScratchSegment::getBuffer() const
    {
        return (_id != INVALID_SEGMENT_ID) ?
            _buffer.getSegment(_id) :
            _fallbackMemory->getBuffer();
    }

    size_t ScratchSegment::getLength() const
    {
        return _buffer.getSegmentSize();
    }

}