/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include "SimuBase.h"
#include "SimuStorTypes.h"

namespace SimuTrace
{
    class Store;
    class Stream;

    class StreamBuffer
    {
    private:
        static const byte FENCE_MEMORY_FILL = (byte)0xFD;
        static const byte CLEAR_MEMORY_FILL = (byte)0xCD;
        static const byte DEAD_MEMORY_FILL  = (byte)0xDD;
    private:
        DISABLE_COPY(StreamBuffer);

        const BufferId _id;
        const bool _master;

        std::unique_ptr<MemorySegment> _buffer;
        const size_t _segmentSize;
        const size_t _lineSize;
        const uint32_t _numSegments;

        byte* _getFence(SegmentId segment) const;
        size_t _getFenceSize() const;

        static size_t _computeLineSize(size_t segmentSize);
        static byte _testMemory(byte* buffer, size_t size);
    protected:
        void _touch();
    public:
        StreamBuffer(BufferId id, size_t segmentSize, uint32_t numSegments,
                     bool sharedMemory);
        StreamBuffer(BufferId id, size_t segmentSize, uint32_t numSegments,
                     Handle& buffer);
        virtual ~StreamBuffer();

        byte* getSegmentAsPayload(SegmentId segment, size_t& outSize) const;
        byte* getSegment(SegmentId segment) const;
        byte* getSegmentEnd(SegmentId segment, uint32_t entrySize) const;
        SegmentControlElement* getControlElement(SegmentId segment) const;

        void dbgSanityFill(SegmentId segment, bool dead);
        int dbgSanityCheck(SegmentId segment, uint32_t entrySize) const;

        BufferId getId() const;
        bool isMaster() const;

        Handle getBufferHandle() const;

        size_t getBufferSize() const;
        size_t getSegmentSize() const;
        uint32_t getNumSegments() const;

        inline static std::string bufferIdToString(BufferId id)
        {
            return (id == SERVER_BUFFER_ID) ?
                "'server'" : stringFormat("%d", id);
        }
    };

}

#endif