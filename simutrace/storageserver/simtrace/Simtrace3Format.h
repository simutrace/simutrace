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
#ifndef SIMTRACE3_FORMAT_H
#define SIMTRACE3_FORMAT_H

#include "SimuStor.h"

namespace SimuTrace {
namespace Simtrace
{

#define INVALID_FILE_OFFSET std::numeric_limits<uint64_t>::max()
    typedef uint64_t FileOffset;

    struct MasterHeader;
    struct SegmentDirectoryTable;

    /* Header - Version 3.0 */
#define SIMTRACE_VERSION3 3
#define SIMTRACE_VERSION3_MINOR 0

    enum Simtrace3AttributeType
    {
        /* Built-in attribute types */
        SatData              = 0x00,
        SatStreamDescription = 0x01,
        SatAssociatedStreams = 0x02,

        /* Encoders can freely use the types from this base on */
        SatEncoderSpecific   = 0x20,

        /* Has to be set for the last attribute in the segment */
        SatFlagLast          = 0x80,

        MaxValue             = 0xff
    };

    template<uint32_t numStreams>
    struct AttributeAssociatedStreams {
        uint32_t streamCount;
        StreamId streams[numStreams];

        AttributeAssociatedStreams() :
            streamCount(0) 
        { 
            memset(streams, INVALID_STREAM_ID, sizeof(streams));
        }
    };

#define SIMTRACE_V3_ATTRIBUTE_MARKER 0x52545441 /* 'ATTR' */
    struct AttributeHeader {
        /* Magic marker to identify the start of an attribute header */
        union {
            char marker[4];
            uint32_t markerValue;
        };

        uint8_t type;

        uint8_t reserved0[3];
        uint64_t reserved1;

        uint64_t size;
        uint64_t uncompressedSize;
    };

    struct AttributeHeaderLink {
        uint64_t type               : 8;
        uint64_t reserved0          : 8;

        uint64_t relativeFileOffset : 48;
    };

    struct AttributeHeaderDescription {
        AttributeHeader header;
        void*           buffer;
    };

    /* Keep the file header within a 4KiB page boundary - the header keeps
       360 bytes unused, reserving it for the master header and future use */
#define SIMTRACE_V3_DIRECTORY_TABLE_SIZE 448
#define SIMTRACE_V3_ATTRIBUTE_TABLE_SIZE 8
#define SIMTRACE_V3_HEADER_CHECKSUM_DATA_SIZE offsetof(SimtraceV3Header, checksum)
    struct SimtraceV3Header {
        uint32_t writerVersion;
        uint32_t reserved0;

        uint64_t fileSize;
        uint64_t uncompressedFileSize; 
        uint64_t frameCount;

        uint64_t entryCount;
        uint64_t rawEntryCount;

        Timestamp startTime;
        Timestamp endTime;
        CycleCount startCycle;
        CycleCount endCycle;

        /* The directory table provides O(1) lookup for the file offsets for 
           the "first" directories. With the recommended 64 MiB frame size,
           the table will overflow for (uncompressed) traces larger than 
           28 TiB. The directories are linked as a singly linked list, allowing
           for more than <tableSize> directories. */
        uint32_t directoryCount;
        uint16_t directoryCapacity;
        uint16_t attributeCount;

        AttributeHeaderLink attributes[SIMTRACE_V3_ATTRIBUTE_TABLE_SIZE];
        FileOffset directories[SIMTRACE_V3_DIRECTORY_TABLE_SIZE];

        // -- Members beyond this point are not included in the checksum --

        uint32_t checksum; /* MurmurHash3 */

        uint8_t dirtyFlag;
        uint8_t reserved1[3];
    };

#define SIMTRACE_V3_HEADER_SIZE \
    (sizeof(MasterHeader) + sizeof(SimtraceV3Header))

    /* The size of the frame header is tuned to be 120 bytes. A frame
       directory link occupies 8 extra bytes - for a total of 128 bytes per
       frame in a directory and 32 frames per directory page */

#define SIMTRACE_V3_FRAME_ATTRIBUTE_TABLE_SIZE 2
#define SIMTRACE_V3_FRAME_HEADER_CHECKSUM_DATA_SIZE offsetof(FrameHeader, checksum)
#define SIMTRACE_V3_FRAME_MARKER 0x454D5246 /* 'FRME' */

    struct FrameHeader {
        /* Magic marker to identify the start of a frame header */
        union {
            char marker[4];
            uint32_t markerValue;
        };

        uint32_t reserved0;

        StreamSegmentId sequenceNumber;
        StreamId streamId;

        StreamTypeId typeId;

        uint32_t entryCount;
        uint32_t rawEntryCount;

        Timestamp startTime;
        Timestamp endTime;
        CycleCount startCycle;
        CycleCount endCycle;

        uint64_t startIndex;

        uint64_t totalSize;

        uint64_t reserved1;

        /* Storing a table of the first attributes improves the chance that we
           are able to recover meta data in case of corruption. This way, the 
           attribute table is included in the checksum (to detect corruption) 
           and replicated into the frame directory. */

        AttributeHeaderLink attributes[SIMTRACE_V3_FRAME_ATTRIBUTE_TABLE_SIZE];
        uint8_t attributeCount;
        uint8_t reserved2[3];

        // -- Members beyond this point are not included in the checksum --

        uint32_t checksum; /* MurmurHash3 */
    };

    struct FrameHeaderLink {
        FrameHeader frameHeader;

        FileOffset offset;
    };

    /* The last entry in a directory may be a directory link entry to
       lead the reader to the next directory. */
#define SIMTRACE_V3_DIRECTORY_LINK_MARKER 0x4B4E4C44 /* 'DLNK' */
    struct FrameDirectoryLink {
        union {
            char marker[4];
            uint32_t markerValue;
        };

        FileOffset nextDirectory;
    };

    union FrameDirectoryEntry {
        /* The markerValue specifies the type of entry */
        struct {
            union {
                char marker[4];
                uint32_t markerValue;
            };
        };

        FrameHeaderLink framelink;
        FrameDirectoryLink directoryLink;
    };

    /* A frame directory is an array of directory entries */
#define SIMTRACE_V3_DIRECTORY_SIZE 1024
    /* 128KiB - 32 links per page */
#define SIMTRACE_V3_DIRECTORY_DATA_SIZE \
    SIMTRACE_V3_DIRECTORY_SIZE * sizeof(FrameDirectoryEntry)

    typedef FrameDirectoryEntry* FrameDirectory;

}
}

#endif