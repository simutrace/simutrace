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
#ifndef SERVER_STREAM_H
#define SERVER_STREAM_H

#include "SimuStor.h"

namespace SimuTrace
{

    class ServerStore;
    class ServerStreamBuffer;
    class StreamEncoder;

    /* Wait context used by server streams */
    typedef WaitContext<StreamSegmentLink> StreamWait;

    class ServerStream : 
        public Stream
    {
    private:
        struct SegmentLocation;

        struct RangeCompare
        {
            bool operator() (const Range* left, const Range* right)
            {
                assert((left != nullptr) && (right != nullptr));
                return (left->start < right->start);
            }
        };

        typedef std::set<Range*, RangeCompare>::reverse_iterator 
            ReverseRangeIterator;

        typedef std::list<SegmentLocation*>::iterator OpenListIterator;
    private:
        DISABLE_COPY(ServerStream);

        ServerStore& _store;

        // The lock order is: _appendLock before _lock
        mutable CriticalSection _appendLock;
        mutable CriticalSection _openLock;
        mutable ReaderWriterLock _lock;

        std::vector<SegmentLocation*> _segments;
        std::list<SegmentLocation*> _openList;
        std::set<Range*, RangeCompare> _trees[QueryIndexType::QMaxTree + 1];

        StreamSegmentId _lastSequenceNumber;
        StreamSegmentId _lastAppendSequenceNumber;
        uint64_t _lastAppendIndex;

        StreamEncoder* _encoder;

        // Statistics
        StreamStatistics _stats;

        // Read ahead
        uint32_t _readAheadAmount;
        std::unique_ptr<StreamSegmentId[]> _readAheadList;

        void _finalize();

        SegmentLocation* _addSegmentLocation(SegmentLocation* loc);
        SegmentLocation* _getPreviousSegment(StreamSegmentId sequenceNumber) const;
        SegmentLocation* _getNextSegment(StreamSegmentId sequenceNumber) const;

        void _addSegment(StreamSegmentId sequenceNumber, 
                         std::unique_ptr<StorageLocation>& location);
        void _completeSegment(StreamSegmentId sequenceNumber,
                              std::unique_ptr<StorageLocation>* location,
                              bool success);
        void _completeSegment(StreamSegmentId sequenceNumber,
                              std::unique_ptr<StorageLocation>* location,
                              std::vector<StreamWait*>* waitList,
                              OpenListIterator* openListIt,
                              bool success, bool synchronous);

        bool _open(SessionId session, StreamSegmentId sequenceNumber,
                   StreamAccessFlags flags, SegmentId& segmentId,
                   bool prefetch, StreamWait* wait = nullptr);

        void _close(SegmentLocation* loc, StreamWait* wait, bool ignoreErrors,
                    OpenListIterator* openListIt);

        StreamSegmentId _findSequenceNumber(QueryIndexType type, 
                                            uint64_t value);
        StreamSegmentId _querySequenceNumber(QueryIndexType type,
                                             uint64_t value);

        ServerStreamBuffer& _getBuffer() const;
        bool _segmentIsAllocated(StreamSegmentId sequenceNumber) const;
    public:
        ServerStream(ServerStore& store, StreamId id, 
                     const StreamDescriptor& desc, StreamBuffer& buffer);
        virtual ~ServerStream() override;

        virtual void queryInformation(
            StreamQueryInformation& informationOut) const override;

        void addSegment(StreamSegmentId sequenceNumber, 
                        std::unique_ptr<StorageLocation>& location);
        void addSegment(SessionId session,
                        StreamSegmentId sequenceNumber,
                        SegmentId* bufferSegmentOut);

        void completeSegment(StreamSegmentId sequenceNumber);
        void completeSegment(StreamSegmentId sequenceNumber, 
                             std::unique_ptr<StorageLocation>* location);

        StreamSegmentId append(SessionId session, SegmentId* bufferSegmentOut, 
                               StreamWait* wait = nullptr);
        StreamSegmentId open(SessionId session, QueryIndexType type, 
                             uint64_t value, StreamAccessFlags flags, 
                             SegmentId* bufferSegmentOut, 
                             StreamWait* wait = nullptr);

        void close(SessionId session, StreamSegmentId sequenceNumber, 
                   StreamWait* wait = nullptr, bool ignoreErrors = false);
        void close(SessionId session, StreamWait* wait = nullptr, 
                   bool ignoreErrors = false);

        SegmentId getBufferMapping(StreamSegmentId sequenceNumber) const;

        const StorageLocation& getStorageLocation(
            StreamSegmentId sequenceNumber) const;

        StreamSegmentId getCurrentSegmentId() const;

        ServerStore& getStore() const;
        StreamEncoder& getEncoder() const;
    };

}

#endif