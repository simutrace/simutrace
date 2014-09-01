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
#ifndef SERVER_STREAM_BUFFER_H
#define SERVER_STREAM_BUFFER_H

#include "SimuStor.h"
#include "WorkItem.h"

namespace SimuTrace
{

    class ServerStore;
    class ServerStream;
    struct Segment;

    class ServerStreamBuffer :
        public StreamBuffer
    {
    private:
        struct StoreStreamSegmentLink :
            public StreamSegmentLink
        {
            StoreId store;

            StoreStreamSegmentLink() :
                StreamSegmentLink(),
                store(INVALID_STORE_ID) { }

            StoreStreamSegmentLink(StoreId store, 
                                   const StreamSegmentLink& link) :
                StreamSegmentLink(link),
                store(store) { }

            StoreStreamSegmentLink(StoreId store, StreamId stream, 
                                   StreamSegmentId sequenceNumber) :
                StreamSegmentLink(stream, sequenceNumber),
                store(store) { }
        };

        struct LinkHash
        {
            size_t operator()(const StoreStreamSegmentLink& seg) const
            {
                uint64_t triple;

                // Hash structure:
                //           32 bit              16 bit       16 bit
                // +-------------------------+------------+------------+
                // |      sequenceNumber     |   stream   |folded store|
                // +-------------------------+------------+------------+

                triple  = static_cast<uint64_t>(seg.sequenceNumber) << 32;

            #if (SIMUTRACE_STORE_MAX_NUM_STREAMS > 0xFFFF)
            #error "The maximum number of streams exceeds the link hash capacity."
            #endif

                triple |= seg.stream << 16;

                StoreId store = seg.store;
                while (store > 0) {
                    triple ^= (store & 0xFFFF);
                    store >>= 16;
                }

                return std::hash<uint64_t>()(triple);
            }
        };

        typedef std::unordered_map<StoreStreamSegmentLink, Segment*, LinkHash>
            StandbyIndex;

    private:
        DISABLE_COPY(ServerStreamBuffer);

        uint64_t _cookie;
        Segment* _segments;

        // Free list - singly-linked lock-free stack
        std::atomic<Segment*> _freeHead;

        // LRU Standby list - circular doubly-linked list + hash map
        bool _enableCache;
        CriticalSection _standbyLock;
        StandbyIndex _standbyIndex;
        Segment* _standbyHead;
  
        void _initializeSegments();

        uint64_t _computeControlCookie(SegmentControlElement& control, 
                                       Segment& segment) const;
        bool _testControlCookie(SegmentControlElement& control, 
                                Segment& segment) const;
        void _notifyEncoderCacheClosed(Segment& segment) const;

        // Free List -----
        Segment* _dequeueFromFreeList();
        void _enqueueToFreeList(Segment& segment);

        void _prepareSegment(SegmentId segment, ServerStream* stream,
                             StreamSegmentId sequenceNumber);
        bool _handleContention(uint32_t tryCount, bool isScratch);

        Segment* _tryAllocateFreeSegment(ServerStream* stream,
                                         StreamSegmentId sequenceNumber,
                                         StorageLocation* location,
                                         StreamAccessFlags flags,
                                         bool prefetch);

        // Standby List -----
        void _dequeueFromStandbyList(Segment& segment);
        void _enqueueToStandbyList(Segment& segment);

        Segment* _findStandbySegment(StoreStreamSegmentLink& link, bool erase);
        Segment* _evictFromStandbyList();

        Segment* _removeStandbySegment(StoreStreamSegmentLink& link);
        void _addStandbySegment(Segment& segment);

        // High-level Segment Management -----
        void _freeSegment(SegmentId segment, bool prefetch = false);
        void _purgeSegment(SegmentId segment);
        bool _submitSegment(SegmentId segment,
                            std::unique_ptr<StorageLocation>& locationOut);
        bool _requestSegment(SegmentId& segment,  
                             ServerStream* stream = nullptr, 
                             StreamSegmentId sequenceNumber = INVALID_STREAM_SEGMENT_ID,
                             StreamAccessFlags flags = SafNone, 
                             StorageLocation* location = nullptr,
                             bool prefetch = false);

    public:
        ServerStreamBuffer(BufferId id, size_t segmentSize, 
                           uint32_t numSegments, bool sharedMemory = true);
        virtual ~ServerStreamBuffer() override;

        SegmentId requestSegment(ServerStream& stream, 
                                 StreamSegmentId sequenceNumber);
        SegmentId requestScratchSegment();

        void freeSegment(SegmentId segment, bool prefetch = false);
        void purgeSegment(SegmentId segment);
        bool submitSegment(SegmentId segment, 
                           std::unique_ptr<StorageLocation>& locationOut);
        bool openSegment(SegmentId& segment, ServerStream& stream,
                         StreamAccessFlags flags, StorageLocation& location,
                         bool prefetch = false);

        void flushStandbyList(StoreId store = INVALID_STORE_ID);

        SegmentControlElement* getControlElement(SegmentId segment) const;
    };

}

#endif