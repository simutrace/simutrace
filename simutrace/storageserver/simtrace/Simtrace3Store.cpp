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

#include "Simtrace3Store.h"

#include "../StorageServer.h"
#include "../ServerStore.h"
#include "../ServerStream.h"
#include "../ServerStreamBuffer.h"
#include "../ServerSession.h"

#include "FileHeader.h"
#include "Simtrace3Format.h"
#include "Simtrace3Frame.h"
#include "Simtrace3GenericEncoder.h"
#include "Simtrace3MemoryEncoder.h"

namespace SimuTrace {
namespace Simtrace
{

    Simtrace3Store::Simtrace3Store(StoreId id, const std::string& path) :
        ServerStore(id, path),
        _fileLock(),
        _file(),
        _readMode(false),
        _loading(false),
        _headerMapping(),
        _header(nullptr),
        _directoryMapping(),
        _directory(nullptr),
        _nextFrameIndex(0)
    {
        _initializeEncoderMap();

        if (File::exists(path)) {
            _openStore(path);
        } else {
            _createStore(path);
        }

        LogInfo("Simtrace store '%s' opened (v%d.%d).", path.c_str(),
                _header->master.majorVersion, _header->master.minorVersion);
    }

    Simtrace3Store::~Simtrace3Store()
    {
        _finalizeHeader();
    }

    void Simtrace3Store::_initializeEncoderMap()
    {
        // !! ADD NEW ENCODER MAPPINGS HERE !!
        EncoderDescriptor desc = {0};

        // Default Encoder --------------------
        // If no special encoder is registered for an entry type, we use a
        // general purpose compression and write the compressed data to
        // the store.
        desc.factoryMethod = Simtrace3GenericEncoder::factoryMethod;
        desc.type = nullptr;

        _registerEncoder(_defaultEncoderTypeId, desc);

        // Memory Encoder --------------------
        // Register built-in memory types with the memory encoder
        static const StreamEncoder::FactoryMethod methodMap[MASTYPETABLE_COUNT] = {
            Simtrace3MemoryEncoder<MemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<MemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess64>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess32>::factoryMethod,
            Simtrace3MemoryEncoder<DataMemoryAccess64>::factoryMethod
        };

        for (uint32_t i = 0; i < MASTYPETABLE_COUNT; i++) {
            desc.factoryMethod = methodMap[i];
            desc.type = &_mastypeTable[i];

            _registerEncoder(_mastypeTable[i].id, desc);
        }

    }

    void Simtrace3Store::_openFrame(FrameDirectoryEntry& entry)
    {
        const FrameHeader& fheader = entry.framelink.frameHeader;

        // Check if this is a zero frame (i.e., it contains meta data
        // for the stream and no actual data). In that case, we just
        // add the meta data to the stream (create it if it does not
        // exist). Otherwise, we add the segment as data to the stream.
        if (fheader.sequenceNumber == INVALID_STREAM_SEGMENT_ID) {
            Simtrace3Frame frame;
            _readFrame(frame, entry.framelink.offset, fheader.totalSize);

            ThrowOn(!frame.validateHash(), Exception,
                    "Corrupted metadata frame detected.");

            // Check if we already registered this stream. If not
            // create it from the frame.
            ServerStream* stream =
                static_cast<ServerStream*>(findStream(fheader.streamId));

            if (stream == nullptr) {
                stream = _openStream(frame);
                assert(stream != nullptr);
            }

            // The zero frame should not contain data.
            assert(frame.findAttribute(
                    Simtrace3AttributeType::SatData) == nullptr);

            // Inform the encoder about the meta data
            Simtrace3Encoder& encoder =
                static_cast<Simtrace3Encoder&>(stream->getEncoder());

            encoder.initialize(frame, true);

        } else {
            // This is a data frame. Add the segment to its stream
            ServerStream& stream =
                static_cast<ServerStream&>(getStream(fheader.streamId));
            Simtrace3Encoder& encoder =
                static_cast<Simtrace3Encoder&>(stream.getEncoder());

            Simtrace3Frame frame(fheader);

            auto location = encoder.makeStorageLocation(frame);

            Simtrace3StorageLocation* sim3location =
                static_cast<Simtrace3StorageLocation*>(location.get());

            sim3location->offset = entry.framelink.offset;
            sim3location->size   = fheader.totalSize;

            stream.addSegment(fheader.sequenceNumber, location);
        }
    }

    void Simtrace3Store::_openStore(const std::string& path)
    {
        _loading = true;

        _file = std::unique_ptr<File>(
            new File(path, File::CreateMode::OpenExisting,
                     File::AccessMode::ReadWrite));

        _mapHeader();

        if (_isDirty()) {
            // TODO: Handle dirty stores
        }

        if (_header->v3.directoryCount > 0) {
            // For each directory, we iterate over its entries (i.e., frames)
            // and add them to the corresponding stream. If the stream does not
            // exist, we create it.
            FileOffset dirOffset = _header->v3.directories[0];
            ThrowOn(dirOffset == 0, Exception, "First directory link corrupted.");

            _mapDirectory(dirOffset);

            uint32_t index = 0;
            while (true) {
                ThrowOn(index >= _header->v3.directoryCapacity, Exception,
                        "Directory structure corrupted.");

                assert(_directory != nullptr);
                FrameDirectoryEntry* entry = &_directory[index];

                const uint32_t marker = entry->markerValue;
                if (marker == SIMTRACE_V3_FRAME_MARKER) {
                    _openFrame(*entry);

                    index++;
                } else if (marker == SIMTRACE_V3_DIRECTORY_LINK_MARKER) {
                    _mapDirectory(entry->directoryLink.nextDirectory);

                    index = 0;
                } else {
                    assert(marker == 0);

                    // No further entries in the store
                    break;
                }
            }
        }

        _loading = false;

        // We are currently not support extending an existing store.
        _readMode = true;
    }

    void Simtrace3Store::_createStore(const std::string& path)
    {
        _file = std::unique_ptr<File>(
            new File(path, File::CreateMode::CreateAlways));

        // Reserve space for the file header and map it into memory
        _reserveSpace(SIMTRACE_HEADER_RESERVED_SPACE);

        _mapHeader();
        _initalizeHeader();
    }

    bool Simtrace3Store::_isDirty() const
    {
        assert(_header != nullptr);

        return (_header->v3.dirtyFlag != 0);
    }

    void Simtrace3Store::_markDirty()
    {
        assert(_header != nullptr);

        _header->v3.dirtyFlag = 0xFF;
    }

    void Simtrace3Store::_markClean()
    {
        assert(_header != nullptr);

        _header->v3.dirtyFlag = 0;
    }

    FileOffset Simtrace3Store::_reserveSpace(uint64_t length)
    {
        FileOffset offset;

        offset = _file->commitSpace(length);

        if (_header != nullptr) {
            Interlocked::interlockedAdd(&_header->v3.fileSize, length);
            Interlocked::interlockedAdd(&_header->v3.uncompressedFileSize, length);
        }

        return offset;
    }

    void Simtrace3Store::_mapHeader()
    {
        assert(_headerMapping == nullptr);

        _headerMapping = std::unique_ptr<FileBackedMemorySegment>(
            new FileBackedMemorySegment(getName().c_str(), true));

        _headerMapping->map(0, SIMTRACE_HEADER_RESERVED_SPACE);
        _header = reinterpret_cast<SimtraceFileHeader*>(
            _headerMapping->getBuffer());
    }

    void Simtrace3Store::_initalizeHeader()
    {
        memset(_header, 0, SIMTRACE_HEADER_RESERVED_SPACE);

        _markDirty();

        // Master header
        _header->master.signatureValue   = SIMTRACE_DEFAULT_SIGNATURE;
        _header->master.majorVersion     = SIMTRACE_VERSION3;
        _header->master.minorVersion     = SIMTRACE_VERSION3_MINOR;

        // Simtrace v3 header
        _header->v3.writerVersion        = StorageServer::getVersion();

        _header->v3.fileSize             = SIMTRACE_HEADER_RESERVED_SPACE;
        _header->v3.uncompressedFileSize = SIMTRACE_HEADER_RESERVED_SPACE;

        _header->v3.startTime            = INVALID_TIME_STAMP;
        _header->v3.endTime              = INVALID_TIME_STAMP;
        _header->v3.startCycle           = INVALID_CYCLE_COUNT;
        _header->v3.endCycle             = INVALID_CYCLE_COUNT;

        _header->v3.directoryCapacity    = SIMTRACE_V3_DIRECTORY_SIZE;
    }

    void Simtrace3Store::_finalizeHeader()
    {
        if ((_header == nullptr) || (!_isDirty())) {
            return;
        }

        _header->v3.writerVersion = StorageServer::getVersion();
        _header->v3.endTime = Clock::getTimestamp();

        // Update the hash of the header.
        // Do not modify the header after this point!
        Hash::murmur3_32(_header, SIMTRACE_V3_HEADER_CHECKSUM_DATA_SIZE,
                         &_header->v3.checksum, sizeof(uint32_t), 0);

        _markClean();

        // Log some stats about the new store contents
        _logStoreStats();
    }

    void Simtrace3Store::_mapDirectory(uint64_t offset)
    {
        _directory = nullptr;

        // To map a directory, we copy construct a new file backed memory
        // segment from the header mapping. This will duplicate all internal
        // handles. If a directory mapping already exists, we just remap.
        if (_directoryMapping == nullptr) {
            _directoryMapping = std::unique_ptr<FileBackedMemorySegment>(
                new FileBackedMemorySegment(*_headerMapping));
        } else {
            _directoryMapping->unmap();
        }

        _directoryMapping->updateSize();

        // Map the directory into memory
        _directoryMapping->map(offset, _header->v3.directoryCapacity *
            sizeof(FrameDirectoryEntry));

        _directory = reinterpret_cast<FrameDirectory>(
            _directoryMapping->getBuffer());
    }

    void Simtrace3Store::_addDirectory()
    {
        _markDirty();

        // Reserve space for directory, map it into memory and add it to the
        // directory table in the header (if possible).
        size_t directorySize = _header->v3.directoryCapacity *
            sizeof(FrameDirectoryEntry);
        FileOffset offset = _reserveSpace(directorySize);

        if (_header->v3.directoryCount < _header->v3.directoryCapacity) {
            _header->v3.directories[_header->v3.directoryCount] = offset;
        }

        _header->v3.directoryCount++;

        // Link the last directory to the new one
        if (_directory != nullptr) {
            FrameDirectoryEntry* last;

            last = &_directory[_header->v3.directoryCapacity - 1];
            assert(last->markerValue == 0);

            last->markerValue = SIMTRACE_V3_DIRECTORY_LINK_MARKER;
            last->directoryLink.nextDirectory = offset;
        }

        _mapDirectory(offset);

        // Reset the free slot pointer to the first entry in the new directory.
        _nextFrameIndex = 0;

        LogDebug("<store: %s> Added directory %d to store.", getName().c_str(),
                 _header->v3.directoryCount - 1);
    }

    void Simtrace3Store::_addFrameToDirectory(Simtrace3Frame& frame,
                                              FileOffset offset)
    {
        FrameHeader& frameHeader = frame.getHeader();
        assert(_header != nullptr);

        _markDirty();

        // The current directory is full or none has been setup in the first
        // place. We have to add a new directory to the store.
        if ((_directory == nullptr) ||
            (_nextFrameIndex == _header->v3.directoryCapacity - 1)) {

            _addDirectory();
        }

        // Get the next free slot in the current directory and copy the
        // frame header into it. The frame must not be changed anymore!
        assert(_directory != nullptr);
        FrameDirectoryEntry& entry = _directory[_nextFrameIndex++];

        assert(frameHeader.markerValue == SIMTRACE_V3_FRAME_MARKER);
        entry.framelink.frameHeader = frameHeader;
        entry.framelink.offset      = offset;

        _header->v3.frameCount++;
    }

    void Simtrace3Store::_addFrameToStoreInformation(
        Simtrace3Frame& frame, uint64_t uncompressedSize)
    {
        FrameHeader& fheader = frame.getHeader();

        _header->v3.fileSize += fheader.totalSize;
        _header->v3.uncompressedFileSize += uncompressedSize;

        if (fheader.startTime < _header->v3.startTime) {
            _header->v3.startTime = fheader.startTime;
        }

        if ((_header->v3.endTime == INVALID_TIME_STAMP) ||
            ((fheader.endTime != INVALID_TIME_STAMP) &&
             (fheader.endTime > _header->v3.endTime))) {
            _header->v3.endTime = fheader.endTime;
        }

        if (fheader.startCycle < _header->v3.startCycle) {
            _header->v3.startCycle = fheader.startCycle;
        }

        if ((_header->v3.endCycle == INVALID_CYCLE_COUNT) ||
            ((fheader.endCycle != INVALID_CYCLE_COUNT) &&
             (fheader.endCycle > _header->v3.endCycle))) {
            _header->v3.endCycle = fheader.endCycle;
        }

        _header->v3.entryCount    += fheader.entryCount;
        _header->v3.rawEntryCount += fheader.rawEntryCount;
    }

    FileOffset Simtrace3Store::_writeFrame(Simtrace3Frame& frame,
                                           uint64_t& uncompressedBytesWritten)
    {
        assert(!_readMode && !_loading);

        // We first try to commit space in the store before we make
        // any changes. This guarantees us that we won't run out of
        // disk space half way in the frame submission. It also
        // ensures that all data will be contiguous in the file.

        _markDirty();

        const FrameHeader& header = frame.getHeader();
        FileOffset frameOffset = _file->commitSpace(header.totalSize);

        // Write frame header to store
        FileOffset offset = frameOffset;
        offset += _file->write(&header, offset);

        // A frame consists of multiple attributes. All attributes are
        // saved in the order they were added. Each attribute has a
        // header and associated data.
        uncompressedBytesWritten = sizeof(FrameHeader);
        AttributeList& attributes = frame.getAttributeList();
        for (auto i = 0; i < attributes.size(); ++i) {
            AttributeHeaderDescription& description = attributes[i];

            offset += _file->write(&description.header, offset);
            offset += _file->write(description.buffer,
                                   description.header.size,
                                   offset);

            uncompressedBytesWritten += description.header.uncompressedSize +
                                        sizeof(AttributeHeader);
        }

        LogDebug("<store: %s> Written frame to store <stream: %d, sqn: %d, "
                 "size: %s (%s), attr#: %d>.", getName().c_str(),
                 header.streamId, header.sequenceNumber,
                 sizeToString(header.totalSize).c_str(),
                 sizeToString(uncompressedBytesWritten).c_str(),
                 header.attributeCount);

        return frameOffset;
    }

    uint64_t Simtrace3Store::_readFrame(Simtrace3Frame& frame,
                                        FileOffset offset, size_t size)
    {
        // We map the frame into memory and then rebuild the list of
        // attributes.
        frame.map(*_headerMapping, offset, size);

        unsigned char* buffer =
            reinterpret_cast<unsigned char*>(frame.getBuffer());

        // We update the frame header by copying the header from the mapping
        // into the supplied frame structure.
        FrameHeader& header = frame.getHeader();
        header = reinterpret_cast<FrameHeader&>(*buffer);

        ThrowOn(!frame.validateHash(), Exception, "Corrupted frame header.");
        assert(header.attributeCount <= SIMTRACE_V3_FRAME_ATTRIBUTE_TABLE_SIZE);

        // A frame consists of multiple attributes. All attributes are
        // loaded in the order they were originally added. Each attribute
        // consists of a header and associated data. The data points to the
        // frame's mapping so we do not need to allocate any extra buffer
        // space.
        uint64_t position = sizeof(FrameHeader);
        for (auto i = 0; i < header.attributeCount; ++i) {
            AttributeHeaderDescription description;

            assert(position < header.totalSize);

            description.header =
                reinterpret_cast<AttributeHeader&>(buffer[position]);
            position += sizeof(AttributeHeader);

            description.buffer = &buffer[position];
            position += description.header.size;

            frame.addAttribute(description);
        }

        // Return the total number of uncompressed bytes read
        return position;
    }

    ServerStream* Simtrace3Store::_openStream(Simtrace3Frame& frame)
    {
        assert(_loading);

        AttributeHeaderDescription* attrDesc = frame.findAttribute(
            Simtrace3AttributeType::SatStreamDescription);

        ThrowOnNull(attrDesc, Exception,
                    "Unable to find stream description attribute.");

        StreamDescriptor* desc =
            reinterpret_cast<StreamDescriptor*>(attrDesc->buffer);

        // We are using the server's memory pool for hidden
        // streams and the shared memory pool for public ones.
        BufferId bufId = IsSet(desc->flags, StreamFlags::SfHidden) ?
            SERVER_BUFFER_ID : 0;

        FrameHeader& header = frame.getHeader();
        std::unique_ptr<Stream> stream =
            this->ServerStore::_createStream(header.streamId, *desc, bufId);

        assert(stream != nullptr);

        ServerStream* sstream = static_cast<ServerStream*>(stream.get());

        this->_addStream(stream);

        return sstream;
    }

    std::unique_ptr<Stream> Simtrace3Store::_createStream(StreamId id,
        StreamDescriptor& desc, BufferId buffer)
    {
        std::unique_ptr<Stream> stream =
            this->ServerStore::_createStream(id, desc, buffer);
        assert(stream != nullptr);

        if (!_loading) {
            // If we are creating a new stream that is not loaded from the
            // store we need to write frame 0 to save the stream's meta data.
            // This will also give the encoder the chance to write out any
            // additional attributes it needs to decode the stream later.

            ServerStream* sstream = static_cast<ServerStream*>(stream.get());
            Simtrace3Encoder& encoder = static_cast<Simtrace3Encoder&>(
                sstream->getEncoder());

            Simtrace3Frame frame(sstream);

            frame.addAttribute(Simtrace3AttributeType::SatStreamDescription,
                               sizeof(StreamDescriptor), &desc);

            encoder.initialize(frame, false);

            // Write the frame 0 into the store.
            commitFrame(frame);
        }

        return stream;
    }

    void Simtrace3Store::_logStreamStats(std::ostringstream& str,
                                         StreamStatistics& stats,
                                         uint64_t size, uint64_t usize)
    {
        str << " Entries: " << stats.entryCount
            << " (raw: " << stats.rawEntryCount
            << ")";

        str << std::endl << " Size: " << sizeToString(size);
        if (stats.rawEntryCount > 0) {
            uint64_t usizediv = (usize > 0) ? usize : 1;
            str << " (uncomp.: " << sizeToString(usize)
                << " ratio: " << (size * 100) / usizediv
                << "%)";
        }

        str << std::endl << " Wall Time (";
        if (stats.ranges.startTime != INVALID_TIME_STAMP) {
            assert(stats.ranges.endTime != INVALID_TIME_STAMP);

            str << "start: " << Clock::formatTimeIso8601(stats.ranges.startTime)
                << " end: " << Clock::formatTimeIso8601(stats.ranges.endTime)
                << " ";
        }

        Timestamp dur = stats.ranges.endTime - stats.ranges.startTime;
        str << "duration: " << Clock::formatDuration(dur)
            << ")";

        if (stats.ranges.startCycle != INVALID_CYCLE_COUNT) {
            assert(stats.ranges.endCycle != INVALID_CYCLE_COUNT);

            str << std::endl << " Cycle Time"
                << " (start: " << stats.ranges.startCycle
                << " end: " << stats.ranges.endCycle
                << ")";
        }
    }

    void Simtrace3Store::_logStoreStats()
    {
        StreamQueryInformation info;

        uint64_t usize;
        uint32_t nstreams = summarizeStreamStats(info.stats, usize, SefRegular);

        std::ostringstream str;

        // Print store summary
        str << "<store: " << this->getName() << ">" << std::endl
            << " Streams: " << nstreams << std::endl;

        _logStreamStats(str, info.stats, _header->v3.fileSize, usize);

        // Print per-stream summary
        if ((Configuration::get<bool>("store.simtrace.logStreamStats")) &&
            (nstreams > 1)) {

            std::vector<Stream*> streams;
            this->Store::_enumerateStreams(streams, SefRegular);

            for (auto stream : streams) {
                stream->queryInformation(info);

                str << std::endl
                    << "---- Stream: " << stream->getName() << std::endl;

                uint64_t size = info.stats.rawEntryCount *
                    getEntrySize(&info.descriptor.type);

                _logStreamStats(str, info.stats, info.stats.compressedSize, size);
            }
        }

        LogInfo("%s", str.str().c_str());
    }

    FileOffset Simtrace3Store::commitFrame(Simtrace3Frame& frame)
    {
        ThrowOn(_readMode || _loading, InvalidOperationException);

        frame.updateHash();

        ThrowOn(frame.getHeader().streamId == INVALID_STREAM_ID,
                ArgumentException, "frame");

        _markDirty();

        // We first write the frame to the store and then make it visible
        // in the directory.
        uint64_t uncompressedBytesWritten;
        FileOffset offset;
        try {
            offset = _writeFrame(frame, uncompressedBytesWritten);

            LockExclusive(_fileLock); {
                // Make frame visible in the store and update global info
                _addFrameToDirectory(frame, offset);
                _addFrameToStoreInformation(frame, uncompressedBytesWritten);
            } Unlock();

        } catch (const std::exception& e) {
            _readMode = true;

            LogError("<store: %s> Failed to write frame to store. "
                     "The error message is '%s'. The store might be corrupted."
                     " Switched to read-only mode.",
                     getName().c_str(), e.what());

            throw;
        }

        return offset;
    }

    void Simtrace3Store::readFrame(Simtrace3Frame& frame,
                                   Simtrace3StorageLocation& location)
    {
        ThrowOn(_loading, InvalidOperationException);

        _readFrame(frame, location.offset, location.size);
    }

    void Simtrace3Store::readAttribute(Simtrace3Frame& frame, uint32_t index,
                                       void* buffer)
    {
        ThrowOn(_loading, InvalidOperationException);

        AttributeList& list = frame.getAttributeList();
        ThrowOn(index >= list.size(), ArgumentOutOfBoundsException, "index");
        ThrowOnNull(buffer, ArgumentNullException, "buffer");

        assert(index < SIMTRACE_V3_ATTRIBUTE_TABLE_SIZE);

        assert(list[index].buffer >= frame.getBuffer());
        FileOffset offset = reinterpret_cast<FileOffset>(list[index].buffer) -
            reinterpret_cast<FileOffset>(frame.getBuffer());

        _file->read(buffer, frame.getOffset() + offset,
                    list[index].header.size);
    }
}
}
