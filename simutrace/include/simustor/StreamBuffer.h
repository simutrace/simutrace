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
        DISABLE_COPY(StreamBuffer);

        const BufferId _id;
        const bool _master;

        std::unique_ptr<MemorySegment> _buffer;
        const size_t _segmentSize;
        const size_t _lineSize;
        const uint32_t _numSegments;

        static size_t _computeLineSize(size_t segmentSize);

        void _prepareSegment(SegmentId id, StreamId owner);
        SegmentId _submitAndRequest(SegmentId segment, StreamId stream);
    public:
        StreamBuffer(BufferId id, size_t segmentSize, uint32_t numSegments,
                     bool sharedMemory);
        StreamBuffer(BufferId id, size_t segmentSize, uint32_t numSegments,
                     Handle& buffer);
        virtual ~StreamBuffer();

        void touch();

        byte* getSegmentAsPayload(SegmentId segment, size_t& outSize) const;
        byte* getSegment(SegmentId segment) const;
        byte* getSegmentEnd(SegmentId segment, uint32_t entrySize) const;
        SegmentControlElement* getControlElement(SegmentId segment) const;

        BufferId getId() const;
        bool isMaster() const;

        Handle getBufferHandle() const;

        size_t getBufferSize() const;
        size_t getSegmentSize() const;
        uint32_t getNumSegments() const;

        static size_t computeBufferSize(size_t segmentSize,
                                        uint32_t numSegments);
    };

}

#endif