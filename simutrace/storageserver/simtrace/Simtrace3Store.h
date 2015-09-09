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
#ifndef SIMTRACE3_STORE_H
#define SIMTRACE3_STORE_H

#include "SimuStor.h"
#include "../ServerStore.h"

#include "FileHeader.h"
#include "Simtrace3Format.h"
#include "Simtrace3Frame.h"

namespace SimuTrace {

    class ServerSession;

namespace Simtrace
{

    struct Simtrace3StorageLocation :
        public StorageLocation
    {
        FileOffset offset;
        uint64_t size;

        Simtrace3StorageLocation(Simtrace3Frame& frame) :
            StorageLocation(StreamSegmentLink(
                                frame.getHeader().streamId,
                                frame.getHeader().sequenceNumber)),
            offset(INVALID_FILE_OFFSET),
            size(0)
        {
            const FrameHeader& header = frame.getHeader();
            ranges.startCycle = header.startCycle;
            ranges.endCycle   = header.endCycle;

            if ((header.entryCount > 0) &&
                (header.startIndex != INVALID_ENTRY_INDEX)) {
                ranges.startIndex = header.startIndex;
                ranges.endIndex   = header.startIndex + header.entryCount - 1;
            } else {
                assert(ranges.startIndex == INVALID_ENTRY_INDEX);
                assert(ranges.endIndex == INVALID_ENTRY_INDEX);
            }

            ranges.startTime = header.startTime;
            ranges.endTime   = header.endTime;

            compressedSize   = header.totalSize;
            rawEntryCount    = header.rawEntryCount;
        }
    };

    class Simtrace3Store :
        public ServerStore
    {
    private:
        DISABLE_COPY(Simtrace3Store);

        ReaderWriterLock _fileLock;

        std::unique_ptr<File> _file;
        bool _readMode;
        bool _loading;

        std::unique_ptr<FileBackedMemorySegment> _headerMapping;
        SimtraceFileHeader* _header;

        std::unique_ptr<FileBackedMemorySegment> _directoryMapping;
        FrameDirectory _directory;
        uint32_t _nextFrameIndex;

        void _initializeEncoderMap();

        void _openFrame(FrameDirectoryEntry& entry);
        void _openStore(const std::string& path);
        void _createStore(const std::string& path);

        bool _isDirty() const;
        void _markDirty();
        void _markClean();

        FileOffset _reserveSpace(uint64_t length);

        void _mapHeader();
        void _initalizeHeader();
        void _finalizeHeader();

        void _mapDirectory(uint64_t offset);
        void _addDirectory();
        void _addFrameToDirectory(Simtrace3Frame& frame,
                                  FileOffset offset);
        void _addFrameToStoreInformation(Simtrace3Frame& frame,
                                         uint64_t uncompressedSize);

        FileOffset _writeFrame(Simtrace3Frame& frame,
                               uint64_t& uncompressedBytesWritten);
        uint64_t _readFrame(Simtrace3Frame& frame,
                            FileOffset offset, size_t size);

        ServerStream* _openStream(Simtrace3Frame& frame);
        virtual std::unique_ptr<Stream> _createStream(StreamId id,
            StreamDescriptor& desc, BufferId buffer) override;

        void _logStreamStats(std::ostringstream& str,
                             StreamStatistics& stats,
                             uint64_t size, uint64_t usize);
        void _logStoreStats();
    public:
        Simtrace3Store(StoreId store, const std::string& path);
        virtual ~Simtrace3Store() override;

        FileOffset commitFrame(Simtrace3Frame& frame);

        void readFrame(Simtrace3Frame& frame,
                       Simtrace3StorageLocation& location);

        void readAttribute(Simtrace3Frame& frame, uint32_t index,
                           void* buffer);
    };

}
}

#endif