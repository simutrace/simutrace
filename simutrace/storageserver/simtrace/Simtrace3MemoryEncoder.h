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
#ifndef SIMTRACE3_MEMORY_ENCODER_H
#define SIMTRACE3_MEMORY_ENCODER_H

#include "SimuStor.h"

#include "../StreamEncoder.h"
#include "../ScratchSegment.h"
#include "../ServerStreamBuffer.h"
#include "../ServerStream.h"
#include "../ServerSession.h"
#include "../StorageServer.h"
#include "../WorkItem.h"
#include "../WorkerPool.h"
#include "../WorkerStreamWait.h"

#include "VPC4/IpPredictor.h"
#include "VPC4/ValuePredictor.h"
#include "VPC4/CyclePredictor.h"

#include "Simtrace3Store.h"
#include "Simtrace3Encoder.h"
#include "Simtrace3Format.h"
#include "Simtrace3Frame.h"

#include "SimuTraceEntryTypes.h"

namespace SimuTrace {
namespace Simtrace
{

    template<typename accessType> struct MemoryEntryLayout {};

    template<> struct MemoryEntryLayout<MemoryAccess32>
    {
        typedef uint32_t dataType;
        typedef Address32 addressType;

        static const bool arch32Bit = true;

        static const uint32_t dataFieldCount = 1; // Address

        // Line 1: <Meta (16 bit)>
        // Line 2: <Ip, Addr, Cycle - Id (8 bit)>
        // Line 3: <Ip, Addr - Data (32 bit)>
        // Line 4: <Cycle - Data (64 bit)>
        static const uint32_t lineCount = 4;
    };

    template<> struct MemoryEntryLayout<DataMemoryAccess32>
    {
        typedef uint32_t dataType;
        typedef Address32 addressType;

        static const bool arch32Bit = true;

        static const uint32_t dataFieldCount = 2; // Address, Data

        // Line 1: <Meta (16 bit)>
        // Line 2: <Ip, Addr, Data, Cycle - Id (8 bit)>
        // Line 3: <Ip, Addr, Data - Data (32 bit)>
        // Line 4: <Cycle - Data (64 bit)>
        static const uint32_t lineCount = 4;
    };

    template<> struct MemoryEntryLayout<MemoryAccess64>
    {
        typedef uint64_t dataType;
        typedef Address64 addressType;

        static const bool arch32Bit = false;

        static const uint32_t dataFieldCount = 1; // Address

        // Line 1: <Meta (16 bit)>
        // Line 2: <Ip, Addr, Cycle - Id (8 bit)>
        // Line 3: <Ip, Addr, Cycle - Data (64 bit)>
        static const uint32_t lineCount = 3;
    };

    template<> struct MemoryEntryLayout<DataMemoryAccess64>
    {
        typedef uint64_t dataType;
        typedef Address64 addressType;

        static const bool arch32Bit = false;

        static const uint32_t dataFieldCount = 2; // Address, Data

        // Line 1: <Meta (16 bit)>
        // Line 2: <Ip, Addr, Data, Cycle - Id (8 bit)>
        // Line 3: <Ip, Addr, Data, Cycle - Data (64 bit)>
        static const uint32_t lineCount = 3;
    };

    template<typename T>
    class Simtrace3MemoryEncoder :
        public Simtrace3Encoder
    {
    private:
        //DISABLE_COPY(Simtrace3MemoryEncoder);

        struct TypeInfo :
            MemoryEntryLayout<T>
        {
            // cycleCount, ip + data fields (data does not include cycle)
            static const uint32_t idStreamCount = 2 + MemoryEntryLayout<T>::dataFieldCount;
            static const uint32_t dataStreamCount = 1 + MemoryEntryLayout<T>::dataFieldCount;

            // ids + data (incl. cycle) + meta
            static const uint32_t totalStreamCount =
                idStreamCount + (dataStreamCount + 1) + 1;
        };

        typedef typename TypeInfo::dataType DataType;
        typedef typename TypeInfo::addressType AddressType;

        struct MemoryLayout
        {
            static const size_t segmentSize = SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB;
            static const size_t maxEntryCount = segmentSize / sizeof(T);

            // For each entry in the source segment, we need to store at most
            // only a single data field in each hidden stream. We thus have
            // space to map multiple source segments to one hidden stream
            // segment. For 64 bit, we get for example 4 sub segments.
            static const size_t dataSubSegmentSize =
                maxEntryCount * sizeof(DataType);
            static const uint32_t dataSubSegmentCount =
                static_cast<uint32_t>(segmentSize / dataSubSegmentSize);

            // We always use 64 bits for the cycle counts and 16 bits for the
            // meta data.
            static const size_t cycleSubSegmentSize =
                maxEntryCount * sizeof(CycleCount);
            static const uint32_t cycleSubSegmentCount =
                static_cast<uint32_t>(segmentSize / cycleSubSegmentSize);

            static const size_t metaSubSegmentSize =
                maxEntryCount * sizeof(uint16_t);
            static const uint32_t metaSubSegmentCount =
                static_cast<uint32_t>(segmentSize / metaSubSegmentSize);

            // The predictor ids take even less space than the data fields
            // (just one byte). We therefore can fit more source segments into
            // a single segment of a hidden stream.
        #ifdef VPC_HALF_BYTE_ENCODING
            static const size_t idSubSegmentSize =
                (maxEntryCount * sizeof(byte)) / 2;
        #else
            static const size_t idSubSegmentSize =
                maxEntryCount * sizeof(byte);
        #endif

            static const uint32_t idSubSegmentCount =
                static_cast<uint32_t>(segmentSize / idSubSegmentSize);

            // This char is used to fill unused space in sub segments to
            // improve second stage compression
            static const unsigned char fillChar = 0xFF;
        };

        typedef AttributeAssociatedStreams<
            TypeInfo::totalStreamCount
        > AssociatedStreams;

        class BufferContext
        {
        private:
            CriticalSection _lock;
            bool _initialized;

            // Wait object used for implicit completes
            StreamWait* _wait;
            WorkerStreamWait _openWait;

            // Sequence number in the hidden streams
            const StreamSegmentId _sequenceNumber;

            const uint32_t _subSegmentCount;
            const size_t _subSegmentSize;

            // Buffers for sub segments. These point at the beginnings of the
            // segments in the hidden streams.
            std::vector<ServerStream*>& _streams;
            std::unique_ptr<SegmentId[]> _segmentIds;
            std::unique_ptr<byte*[]> _buffers;

            volatile uint32_t _usedBuffers;
            bool _isLoad;

            void _initialize(bool isLoad, StreamWait* wait)
            {
                assert(!_initialized);

                for (int i = 0; i < _streams.size(); ++i) {

                    // Depending on the mode of operation, we open the
                    // segments or we add new segments to the buffer streams.
                    if (isLoad) {
                        _streams[i]->open(SERVER_SESSION_ID,
                                          QueryIndexType::QSequenceNumber,
                                          _sequenceNumber,
                                          StreamAccessFlags::SafNone,
                                          &_segmentIds[i], nullptr, wait);
                    } else {
                        _streams[i]->addSegment(SERVER_SESSION_ID,
                                                _sequenceNumber,
                                                &_segmentIds[i]);
                    }

                    ThrowOn(_segmentIds[i] == INVALID_SEGMENT_ID, Exception,
                            stringFormat("Out of segment memory "
                                "<stream: %d, sqn: %d>", _streams[i]->getId(),
                                _sequenceNumber));

                    StreamBuffer& buffer = _streams[i]->getStreamBuffer();
                    _buffers[i] = buffer.getSegment(_segmentIds[i]);
                }

                _isLoad = isLoad;
                _initialized = true;
            }

            void _close(StreamWait* wait)
            {
                // We have to commit/close the segments that our buffers
                // reference in this context.
                for (int i = 0; i < _streams.size(); ++i) {

                    // If the buffer has not been initialized, all the
                    // following buffers won't be initialized too and we can
                    // bail out.
                    if (_buffers[i] == nullptr) {
                        break;
                    }

                    try {
                        if (!_isLoad) {
                            // This is a new segment. Before we write it
                            // back, we need to set the number of raw
                            // entries so the generic encoding knows the
                            // amount of valid data.
                            ServerStreamBuffer& buffer =
                                static_cast<ServerStreamBuffer&>(
                                _streams[i]->getStreamBuffer());

                            SegmentControlElement* ctrl =
                                buffer.getControlElement(_segmentIds[i]);

                            assert(_usedBuffers <= _subSegmentCount);
                            ctrl->rawEntryCount = static_cast<uint32_t>(
                                _subSegmentSize * _usedBuffers);
                        }

                        _streams[i]->close(SERVER_SESSION_ID, _sequenceNumber,
                                           wait);

                        _buffers[i] = nullptr;
                        _segmentIds[i] = INVALID_SEGMENT_ID;

                    } catch (const std::exception& e) {
                        LogError("Failed to close stream segment during "
                                 "buffer context destruction <stream: %d, "
                                 "sqn: %d>. The exception is: '%s'.",
                                 _streams[i]->getId(), _sequenceNumber, e.what());

                        // Do not rethrow the exception. This method guarantees
                        // to not throw any exceptions. If the data could not
                        // be saved, it will be lost.
                    }
                }
            }

        public:
            BufferContext(StreamSegmentId hiddenStreamSqn,
                          std::vector<ServerStream*>& streams,
                          uint32_t subSegmentCount,
                          StreamWait* completeWait) :
                _initialized(false),
                _wait(completeWait),
                _sequenceNumber(hiddenStreamSqn),
                _subSegmentCount(subSegmentCount),
                _subSegmentSize(MemoryLayout::segmentSize / subSegmentCount),
                _streams(streams),
                _segmentIds(new SegmentId[_streams.size()]),
                _buffers(new byte*[_streams.size()]),
                _usedBuffers(0),
                _isLoad(false)
            {
                assert(_sequenceNumber != INVALID_STREAM_SEGMENT_ID);
                assert(_subSegmentCount > 0);
                assert(_wait != nullptr);

                ThrowOn(_streams.size() == 0, Exception, "Missing associated "
                        "streams. Cannot work on incomplete memory stream.");

                for (int i = 0; i < _streams.size(); ++i) {
                    assert(_streams[i] != nullptr);

                    _segmentIds[i] = INVALID_SEGMENT_ID;
                    _buffers[i] = nullptr;
                }
            }

            ~BufferContext()
            {
            #ifdef _DEBUG
                for (int i = 0; i < _streams.size(); ++i) {
                    assert(_buffers[i] == nullptr);
                    assert(_segmentIds[i] == INVALID_SEGMENT_ID);
                }
            #endif
            }

            void append()
            {
                if (!_initialized) {
                    LockScope(_lock);
                    if (!_initialized) {
                        _initialize(false, nullptr);
                    }
                }

                ThrowOn(_isLoad, InvalidOperationException);
            }

            void open()
            {
                if (!_initialized) {
                    LockScope(_lock);
                    if (!_initialized) {
                        _initialize(true, &_openWait);
                    }
                }

                ThrowOn(!_isLoad, InvalidOperationException);
            }

            bool completeSubSegment()
            {
                assert(_initialized);
                assert(_usedBuffers < _subSegmentCount);

                uint32_t count = Interlocked::interlockedAdd(&_usedBuffers, 1);
                if (count == _subSegmentCount - 1) {
                    _close(_wait); // Does not throw

                    return true;
                }

                return false;
            }

            void close(StreamWait* wait)
            {
                assert(_initialized);

                // Only a single thread will ever call close, when no other
                // completes can be expected. We therefore need no locking.
                _close(wait);
                _initialized = false;
            }

            template<typename D>
            D* getSubSegment(uint32_t buffer, StreamSegmentId memoryStreamSqn)
            {
                assert(_initialized);
                assert(buffer < _streams.size());
                assert(memoryStreamSqn != INVALID_STREAM_SEGMENT_ID);
                assert(memoryStreamSqn / _subSegmentCount == _sequenceNumber);

                uint32_t index = memoryStreamSqn % _subSegmentCount;

                return reinterpret_cast<D*>(&_buffers[buffer][index * _subSegmentSize]);
            }

            StreamSegmentId getSequenceNumber()
            {
                return _sequenceNumber;
            }

            uint32_t getStreamCount()
            {
                return _streams.size();
            }

            uint32_t getSubSegmentCount()
            {
                return _subSegmentCount;
            }

            size_t getSubSegmentSize()
            {
                return _subSegmentSize;
            }

            WorkerStreamWait& getOpenWait()
            {
                return _openWait;
            }
        };

        // A SegmentLine represents a set of hidden streams and their active
        // buffer contexts that share a common number of sub segments per
        // stream segment. The number varies with the size of the data written
        // to the hidden stream (e.g., predictor id vs. data).
        struct SegmentLine
        {
            uint32_t subSegmentCount;

            std::map<StreamSegmentId, std::unique_ptr<BufferContext>> contexts;
            std::vector<ServerStream*> streams;
        };

        // The SubSegmentContext represents a wrapper around the buffer
        // space of the hidden streams and the input/output memory stream. The
        // context directly touches internal fields of the memory encoder!
        class SubSegmentContext
        {
        private:
            Simtrace3MemoryEncoder& _encoder;

            BufferContext* _contexts[TypeInfo::lineCount];

            uint32_t _entryCount;
            CycleCount _startCycle;

            // Sequence number in memory stream
            const StreamSegmentId _sequenceNumber;

            const bool _isLoad;

            BufferContext* _getBufferContext(SegmentLine& line, bool& created)
            {
                BufferContext* context;

                StreamSegmentId sqn = _sequenceNumber / line.subSegmentCount;

                auto it = line.contexts.find(sqn);
                if (it == line.contexts.end()) {
                    // The context for the sequence number is not present.
                    // Create one.

                    std::unique_ptr<BufferContext> ctx(new BufferContext(sqn,
                        line.streams, line.subSegmentCount,
                        &_encoder._globalWaitContext));

                    context = ctx.get();

                    line.contexts.insert(std::pair<StreamSegmentId,
                        std::unique_ptr<BufferContext>>(sqn, std::move(ctx)));

                    created = true;
                } else {
                    context = it->second.get();
                    created = false;
                }

                return context;
            }

            void _closeBufferContext(BufferContext** context,
                                     SegmentLine& line,
                                     bool force = false)
            {
                if (force || (*context)->completeSubSegment()) {
                    LockScope(_encoder._lock);

                    line.contexts.erase((*context)->getSequenceNumber());
                }

                *context = nullptr;
            }

            void _setupBufferContexts(bool isLoad)
            {
                bool allCreated = false;
                bool createdContext[TypeInfo::lineCount] = { false };

                try {

                    // Get the buffer contexts. If one fails to initialize, we
                    // throw them all away (if we created it).
                    Lock(_encoder._lock); {
                        for (int i = 0; i < TypeInfo::lineCount; ++i) {
                            _contexts[i] = _getBufferContext(_encoder._lines[i],
                                                             createdContext[i]);
                        }
                    } Unlock();

                    allCreated = true;

                    // Open or append the hidden streams depending on the
                    // requested operation. On a load, wait for it to complete.
                    if (isLoad) {

                        for (int i = 0; i < TypeInfo::lineCount; ++i) {
                            _contexts[i]->open();
                        }

                        for (int i = 0; i < TypeInfo::lineCount; ++i) {
                            WorkerStreamWait& wait = _contexts[i]->getOpenWait();

                            if (!wait.wait()) {
                                std::stringstream str;
                                str << "Could not establish load context. "
                                       "One or more segments failed to load. "
                                       "The segments are: ";

                                StreamSegmentLink link;
                                while (!wait.popError(link)) {
                                    str << "<stream: " << link.stream << ", sqn: "
                                        << link.sequenceNumber << "> ";
                                }

                                Throw(Exception, str.str());
                            }
                        }

                    } else {
                        for (int i = 0; i < TypeInfo::lineCount; ++i) {
                            _contexts[i]->append();
                        }
                    }

                } catch (...) {

                    for (int i = 0; i < TypeInfo::lineCount; ++i) {
                        _closeBufferContext(&_contexts[i], _encoder._lines[i],
                                            !allCreated && createdContext[i]);
                    }

                    throw;
                }
            }

            void _mapSubSegments()
            {
                assert(sizeof(_contexts) / sizeof(BufferContext*) >= 3);

                metaDataBuffer =
                    _contexts[0]->template getSubSegment<uint16_t>(0,
                        _sequenceNumber);

                for (int i = 0; i < TypeInfo::idStreamCount; ++i) {
                    idBuffers[i] =
                        _contexts[1]->template getSubSegment<PredictorId>(i,
                            _sequenceNumber);
                }

                for (int i = 0; i < TypeInfo::dataStreamCount; ++i) {
                    dataBuffers[i] =
                        _contexts[2]->template getSubSegment<DataType>(i,
                            _sequenceNumber);
                }

                if (TypeInfo::arch32Bit) {
                    assert(sizeof(_contexts) / sizeof(BufferContext*) >= 4);

                    cycleDataBuffer =
                        _contexts[3]->template getSubSegment<CycleCount>(
                            0, _sequenceNumber);
                } else {
                    cycleDataBuffer =
                        _contexts[2]->template getSubSegment<CycleCount>(
                            TypeInfo::dataStreamCount, _sequenceNumber);
                }
            }

        public:
            // As these fields are heavily used by the compress/decompress
            // logic, we provide public access to them.
            T* entryBuffer;

            PredictorId* idBuffers[TypeInfo::idStreamCount];
            DataType* dataBuffers[TypeInfo::dataStreamCount];
            CycleCount* cycleDataBuffer;
            uint16_t* metaDataBuffer;

        #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
            ProfileContext profileContext;
        #endif

            SubSegmentContext(Simtrace3MemoryEncoder& encoder,
                              ServerStream* memoryStream, SegmentId id,
                              StreamSegmentId sequenceNumber, bool isLoad) :
                _encoder(encoder),
                _entryCount(0),
                _startCycle(INVALID_CYCLE_COUNT),
                _sequenceNumber(sequenceNumber),
                _isLoad(isLoad),
                entryBuffer(nullptr),
                metaDataBuffer(nullptr)
            {
                assert(memoryStream != nullptr);
                assert(id != INVALID_SEGMENT_ID);
                assert(sequenceNumber != INVALID_STREAM_SEGMENT_ID);
                ServerStreamBuffer& buffer = static_cast<ServerStreamBuffer&>(
                    memoryStream->getStreamBuffer());
                SegmentControlElement* ctrl = buffer.getControlElement(id);

                // Our goal is to create a context which gives us access to
                // a specific sub segment in the data and id streams. This
                // context can then be supplied to the compression and
                // decompression routines.

                // Step 1) Set up the entry buffer pointer and meta data. These
                // point to the corresponding segment in the memory stream.
                entryBuffer = reinterpret_cast<T*>(buffer.getSegment(id));

                _entryCount = ctrl->rawEntryCount;
                _startCycle = ctrl->startCycle;

                // Step 2) We first have to create the buffer contexts. These
                // will give access to allocated segments in the server stream
                // buffer for the current sequence number in the respective
                // hidden streams
                _setupBufferContexts(isLoad);

                // Step 3) Now, we need to point our buffers to the requested
                // sub segments in the prepared buffer contexts.
                _mapSubSegments();
            }

            ~SubSegmentContext()
            {
                // As soon as the sub segment context goes out of scope, we
                // treat this as a completion of the segment (regardless if an
                // error is responsible for that). The buffer contexts will
                // stay alive until all sub segments have been completed. As
                // data is generated only sequentially, this is not a problem
                // regarding leaked contexts.
                for (int i = 0; i < TypeInfo::lineCount; ++i) {
                    _closeBufferContext(&_contexts[i], _encoder._lines[i]);
                }

            #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
                if (!_isLoad) {
                    _encoder._profiler->collect(profileContext);
                }
            #endif
            }

            uint32_t getEntryCount()
            {
                return _entryCount;
            }

            CycleCount getStartCycle()
            {
                return _startCycle;
            }
        };

        struct WorkerContext
        {
            Simtrace3MemoryEncoder& encoder;
            ServerStreamBuffer& buffer;
            SegmentId segment;
            StreamAccessFlags flags;
            StorageLocation* location;

            WorkerContext(Simtrace3MemoryEncoder& encoder,
                          ServerStreamBuffer& buffer,
                          SegmentId segment,
                          StreamAccessFlags flags,
                          StorageLocation* location = nullptr) :
                encoder(encoder),
                buffer(buffer),
                segment(segment),
                flags(flags),
                location(location) { }
        };

    private:
        StreamWait _globalWaitContext;

        CriticalSection _lock;

        bool _initialized;
        SegmentLine _lines[TypeInfo::lineCount];

    #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
        std::unique_ptr<Profiler> _profiler;
    #endif

        ServerStream* _registerHiddenStream(std::vector<ServerStream*>& streams,
                                            uint32_t index, std::string name)
        {
            assert(index <= streams.size());
            if (index == streams.size()) {
                streams.push_back(nullptr);
            } else if (streams[index] != nullptr) {
                return streams[index];
            }

            StreamDescriptor desc;
            memset(&desc, 0, sizeof(StreamDescriptor));

            desc.hidden             = true;
            desc.type.entrySize     = 1;
            desc.type.temporalOrder = false;

            assert(name.length() <= MAX_STREAM_NAME_LENGTH);
            memcpy(&desc.name, name.c_str(), name.length());

            StreamId id = _getStore().registerStream(desc, SERVER_BUFFER_ID);
            assert(id != INVALID_STREAM_ID);

            streams[index] = &dynamic_cast<ServerStream&>(_getStore().getStream(id));

            return streams[index];
        }

        void _initializeLine(SegmentLine& line, const char* prefix,
                             uint32_t streamCount, uint32_t subSegmentCount,
                             AssociatedStreams& assocStreams)
        {
            line.subSegmentCount = subSegmentCount;

            // Register the hidden streams. This will also write a zero frame
            // per stream to the store, saving the stream's description.
            for (uint32_t i = 0; i < streamCount; ++i) {
                assert(assocStreams.streamCount < TypeInfo::totalStreamCount);
                StreamId id = assocStreams.streams[assocStreams.streamCount];

                if (id != INVALID_STREAM_ID) {
                    Stream* stream = _getStore().findStream(id);

                    ThrowOnNull(stream, Exception, stringFormat(
                                    "Failed to initialize memory encoder for "
                                    "stream %d. Could not find associated "
                                    "stream 'stream%d:%s%d' (%d).",
                                    _getStream()->getId(),
                                    _getStream()->getId(),
                                    prefix, i, i));

                    line.streams.push_back(static_cast<ServerStream*>(stream));
                } else {
                    _registerHiddenStream(line.streams, i, stringFormat(
                                            "stream%d:%s%d", _getStream()->getId(),
                                            prefix, i));

                    assert(assocStreams.streamCount < TypeInfo::totalStreamCount);
                    assocStreams.streams[assocStreams.streamCount] =
                        line.streams[i]->getId();
                }

                assocStreams.streamCount++;
            }
        }

        void _initializeLines(AssociatedStreams* assoc)
        {
            assert(sizeof(_lines) / sizeof(SegmentLine) >= 3);
            AssociatedStreams assocStreams;

            if (assoc != nullptr) {
                // We are opening a store and should only restore the
                // stream association. The streams are already registered.
                ThrowOn(assoc->streamCount != TypeInfo::totalStreamCount,
                        Exception, stringFormat("Corrupted or incompatible "
                            "stream association data for stream %d. Expected "
                            "%d streams, found %d.", _getStream()->getId(),
                            TypeInfo::totalStreamCount, assoc->streamCount));

                // We have to reset the stream counter so we make a private
                // copy of the attribute.
                assocStreams = *assoc;
                assocStreams.streamCount = 0;
            }

            // Meta data
            _initializeLine(_lines[0], "meta", 1,
                            MemoryLayout::metaSubSegmentCount,
                            assocStreams);

            // Predictor ids (Ip, Addr, (Data), Cycle)
            _initializeLine(_lines[1], "ids", TypeInfo::idStreamCount,
                            MemoryLayout::idSubSegmentCount,
                            assocStreams);

            if (TypeInfo::arch32Bit) {
                assert(sizeof(_lines) / sizeof(SegmentLine) >= 4);

                // Not predicted data (Ip, Addr, (Data))
                _initializeLine(_lines[2], "data",
                                TypeInfo::dataStreamCount,
                                MemoryLayout::dataSubSegmentCount,
                                assocStreams);

                // Not predicted cycle counts
                _initializeLine(_lines[3], "cycle", 1,
                                MemoryLayout::cycleSubSegmentCount,
                                assocStreams);

            } else {
                // Not predicted data (Ip, Addr, (Data), Cycle)
                _initializeLine(_lines[2], "data",
                                TypeInfo::dataStreamCount + 1,
                                MemoryLayout::dataSubSegmentCount,
                                assocStreams);
            }

            if (assoc == nullptr) {
                // We need to associate the hidden streams with the memory
                // stream so we know from which hidden streams to read later
                Simtrace3Frame frame(_getStream());

                frame.addAttribute(Simtrace3AttributeType::SatAssociatedStreams,
                                   sizeof(AssociatedStreams), &assocStreams);

                Simtrace3Store& store = static_cast<Simtrace3Store&>(_getStore());
                store.commitFrame(frame);
            }
        }

        virtual void _encode(Simtrace3Frame& frame, SegmentId id,
                             StreamSegmentId sequenceNumber) override
        {
            SubSegmentContext ctx(*this, _getStream(), id, sequenceNumber, false);

            // Copy the start pointers for the data buffers. We need them later
            // to compute the unused sub segment size.
            byte* orgDataBuffers[TypeInfo::dataStreamCount];
            memcpy(orgDataBuffers, ctx.dataBuffers, sizeof(orgDataBuffers));
            byte* orgCycleBuffer = (byte*)ctx.cycleDataBuffer;

            // Create and initialize the predictors.
            // N.B. We create the cycle predictor with the data type and NOT
            // with the cyclecount type. This may
            std::unique_ptr<IpPredictor<AddressType>> ipPredictor(
                new IpPredictor<AddressType>());
            std::unique_ptr<CyclePredictor<CycleCount, AddressType>> cyclePredictor(
                new CyclePredictor<CycleCount, AddressType>(ctx.getStartCycle()));
            std::unique_ptr<ValuePredictor<DataType, AddressType>[]> valuePredictor(
                new ValuePredictor<DataType, AddressType>[TypeInfo::dataFieldCount]);

        #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
            cyclePredictor.addToProfileContext(ctx.profileContext, "cc");
            ipPredictor.addToProfileContext(ctx.profileContext, "ip");
            for (int i = 0; i < TypeInfo::dataFieldCount; ++i) {
                valuePredictor[i].addToProfileContext(ctx.profileContext,
                    stringFormat("v[%d]",i).c_str());
            }
        #endif

            uint32_t entryCount = ctx.getEntryCount();
            for (uint32_t j = 0; j < entryCount; ++j, ++ctx.entryBuffer) {
                const T* entry = ctx.entryBuffer;

                // Encode instruction pointer
                ipPredictor->encodeIp(&ctx.idBuffers[0],
                                      &ctx.dataBuffers[0],
                                      entry->ip);

                // Encode value fields (address and potentially data)
                for (uint32_t i = 0; i < TypeInfo::dataFieldCount; ++i) {
                    valuePredictor[i].encodeValue(&ctx.idBuffers[i + 1],
                                                  &ctx.dataBuffers[i + 1],
                                                  entry->dataFields[i],
                                                  entry->ip);
                }

                // Encode metadata (incl. cycle count)
                static const uint32_t ccidx = 1 + TypeInfo::dataFieldCount;
                cyclePredictor->encodeCycle(&ctx.idBuffers[ccidx],
                                            &ctx.cycleDataBuffer,
                                            entry->metadata.cycleCount,
                                            entry->ip);

                *ctx.metaDataBuffer = static_cast<uint16_t>(
                    (entry->metadata.value & ~TEMPORAL_ORDER_CYCLE_COUNT_MASK)
                    >> TEMPORAL_ORDER_CYCLE_COUNT_BITS);
                ctx.metaDataBuffer++;
            }

            // For every successful prediction, the data space is not used and
            // there is still the data present from the last use of the
            // hidden stream segment in the stream buffer. To improve
            // compression, we fill up the remaining space in the sub segments
            // with a special char.
            // N.B. We do not fill remaining space for id buffers and the meta
            // buffer. For these only the last segment will not be entirely
            // filled. Doing checks here all the time is thus in 99% useless
            // and we are better off with less compression of the single last
            // segment.

            for (uint32_t i = 0; i < TypeInfo::dataStreamCount; ++i) {
                size_t usedSpace, remainingSpace;

                usedSpace = (size_t)ctx.dataBuffers[i] - (size_t)orgDataBuffers[i];
                remainingSpace = MemoryLayout::dataSubSegmentSize - usedSpace;
                memset(ctx.dataBuffers[i], MemoryLayout::fillChar, remainingSpace);
            }

            size_t usedCycleSpace = (byte*)ctx.cycleDataBuffer - orgCycleBuffer;
            size_t remainingCycleSpace = MemoryLayout::cycleSubSegmentSize - usedCycleSpace;
            memset(ctx.cycleDataBuffer, MemoryLayout::fillChar, remainingCycleSpace);
        }

        virtual void _decode(Simtrace3StorageLocation& location, SegmentId id,
                             StreamSegmentId sequenceNumber) override
        {
            SubSegmentContext ctx(*this, _getStream(), id, sequenceNumber, true);

            CycleCount cycleCount = 0;

            std::unique_ptr<IpPredictor<AddressType>> ipPredictor(
                new IpPredictor<AddressType>());
            std::unique_ptr<CyclePredictor<CycleCount, AddressType>> cyclePredictor(
                new CyclePredictor<CycleCount, AddressType>(ctx.getStartCycle()));
            std::unique_ptr<ValuePredictor<DataType, AddressType>[]> valuePredictor(
                new ValuePredictor<DataType, AddressType>[TypeInfo::dataFieldCount]);

            uint32_t entryCount = ctx.getEntryCount();
            for (uint32_t j = 0; j < entryCount; ++j, ++ctx.entryBuffer) {
                T* entry = ctx.entryBuffer;

                // Decode the instruction pointer. We use this as key for
                // most of the other members.
                ipPredictor->decodeIp(&ctx.idBuffers[0],
                                      &ctx.dataBuffers[0],
                                      entry->ip);

                // Decode the value fields (address and potentially data)
                for (uint32_t i = 0; i < TypeInfo::dataFieldCount; ++i) {
                    valuePredictor[i].decodeValue(&ctx.idBuffers[i + 1],
                                                  &ctx.dataBuffers[i + 1],
                                                  entry->ip,
                                                  entry->dataFields[i]);
                }

                // Decode metadata (incl. cycle count).
                entry->metadata.value =
                    (static_cast<uint64_t>(*ctx.metaDataBuffer)
                    << TEMPORAL_ORDER_CYCLE_COUNT_BITS);
                ctx.metaDataBuffer++;

                static const uint32_t ccidx = 1 + TypeInfo::dataFieldCount;
                cyclePredictor->decodeCycle(&ctx.idBuffers[ccidx],
                                            &ctx.cycleDataBuffer,
                                            entry->ip, cycleCount);

                entry->metadata.cycleCount = cycleCount;
            }
        }

    public:
        Simtrace3MemoryEncoder(ServerStore& store, ServerStream* stream) :
            Simtrace3Encoder(store, "Simtrace3 Memory Encoder", stream),
            _initialized(false)
        {
            // If this is just an initialization to get the friendly name,
            // we do not have to proceed further.
            if (stream == nullptr) {
                return;
            }

            const StreamTypeDescriptor& desc = stream->getType();
            assert(desc.entrySize == sizeof(T));
            assert(desc.arch32Bit == TypeInfo::arch32Bit);
            assert(stream->getStreamBuffer().getSegmentSize() ==
                   MemoryLayout::segmentSize);

            ThrowOn(desc.bigEndian, NotSupportedException);
            ThrowOn(!desc.temporalOrder, NotSupportedException);

            // Initialize profiling code, if necessary. The profiler gives us
            // stats on predictor usage when writing.
        #ifdef SIMUTRACE_PROFILING_SIMTRACE3_VPC4_PREDICTORS_ENABLE
            std::string file = stringFormat("profile_vpc4_stream%d.csv",
                stream->getId());

            _profiler = std::unique_ptr<Profiler>(
                new Profiler(StorageServer::makePath(nullptr, &file)));
        #endif
        }

        virtual ~Simtrace3MemoryEncoder() override
        {
        #ifdef _DEBUG
            for (int i = 0; i < TypeInfo::lineCount; ++i) {
                assert(_lines[i].contexts.empty());
            }
        #endif
        }

        virtual void initialize(Simtrace3Frame& frame, bool isOpen) override
        {
            if (isOpen) {
                AttributeHeaderDescription* attrDesc = frame.findAttribute(
                    Simtrace3AttributeType::SatAssociatedStreams);

                if (attrDesc == nullptr) {
                    return;
                }

                AssociatedStreams* assocStreams =
                    reinterpret_cast<AssociatedStreams*>(attrDesc->buffer);

                _initializeLines(assocStreams);

            } else {
                // We create the hidden streams lazily on the first write.
                return;
            }
        }

        virtual void close(StreamWait* wait = nullptr) override
        {
            // There are probably open segments in the hidden streams
            // we have to close. We use the supplied wait object so the caller
            // can proceed to signal other encoders before the wait is
            // performed.
            for (int i = 0; i < TypeInfo::lineCount; ++i) {
                for (auto& ctx : _lines[i].contexts) {
                    ctx.second->close(wait);
                }

                _lines[i].contexts.clear();
            }

            // It is possible that there are currently operations on hidden
            // stream segments pending. Since they already have been started
            // we cannot use the supplied wait context and have to wait here
            // for the operations to finish. Usually, this does not take very
            // long.
            _globalWaitContext.wait();
        }

        virtual bool write(ServerStreamBuffer& buffer, SegmentId segment,
                           std::unique_ptr<StorageLocation>& locationOut) override
        {
            // We lazily create the hidden streams to avoid running into
            // softlocks with the stream registering process. We do not need to
            // perform locking on our own, because the server stream will do
            // the job.
            if (!_initialized) {
                _initializeLines(nullptr);
                _initialized = true;
            }

            return this->Simtrace3Encoder::write(buffer, segment, locationOut);
        }

        virtual void queryInformationStream(
            StreamQueryInformation& informationOut) override
        {
            // The memory stream which is the input for this encoder does
            // not account the compressed size for all the hidden streams
            // backing it. We will add this size to the query information here.
            for (int i = 0; i < TypeInfo::lineCount; ++i) {
                for (auto stream : _lines[i].streams) {
                    StreamQueryInformation info;
                    stream->queryInformation(info);

                    informationOut.stats.compressedSize +=
                        info.stats.compressedSize;
                }
            }
        }

        static StreamEncoder* factoryMethod(ServerStore& store,
                                            ServerStream* stream)
        {
            return new Simtrace3MemoryEncoder<T>(store, stream);
        }
    };

}
}

#endif