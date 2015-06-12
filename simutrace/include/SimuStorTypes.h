/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 *             _____ _                 __
 *            / ___/(_)___ ___  __  __/ /__________ _________
 *            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
 *           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
 *          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
 *                         http://simutrace.org
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
/*! \file */
#pragma once
#ifndef SIMUSTOR_TYPES_H
#define SIMUSTOR_TYPES_H

#include "SimuBaseTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

    /* Session and store related definitions */

    /*! \brief Id of a session.
     *
     *  Identifies a session as originally created by StSessionCreate().
     *
     *  \since 3.0
     */
    typedef ObjectId    SessionId;
#define INVALID_SESSION_ID INVALID_OBJECT_ID
#define SERVER_SESSION_ID (INVALID_SESSION_ID - 1)

#ifdef SIMUTRACE
    /*! \internal \brief Id of a store. */
    typedef ObjectId    StoreId;
#define INVALID_STORE_ID INVALID_OBJECT_ID
#define CLIENT_STORE_ID (INVALID_STORE_ID - 1)
#endif

    /* Simulation and tracing environment related definitions */

    /* N.B.: These values must all be defined the same! */
#define INVALID_CYCLE_COUNT INVALID_LARGE_OBJECT_ID
#define INVALID_ENTRY_INDEX INVALID_LARGE_OBJECT_ID
#define INVALID_TIME_STAMP  INVALID_LARGE_OBJECT_ID

    /*! \brief Cycle count type
     *
     *  To measure simulation time Simutrace uses the number of passed CPU
     *  cycles since the start of the simulation. For functional simulation,
     *  the cycle count degenerates to the number of executed instructions.
     *  The cycle count is used as time stamp throughout Simutrace (e.g.,
     *  in temporally ordered streams, see #StreamTypeDescriptor).
     *
     *  \since 3.0
     */
    typedef uint64_t CycleCount;


    /* Stream, stream buffer and data pool related definitions */

    /*! \brief Id of a stream.
     *
     *  Identifies a certain stream. The id is session and store local and is
     *  thus only valid in combination with a #SessionId.
     *
     *  \since 3.0
     */
    typedef ObjectId    StreamId;
#define INVALID_STREAM_ID INVALID_OBJECT_ID

    /*! \brief Id of a stream type.
     *
     *  Identifies a stream type. The id is a globally unique identifier (GUID)
     *  and should be generated randomly and kept fixed for a certain type of
     *  entries.
     *
     *  \since 3.0
     */
    typedef Guid        StreamTypeId;

#ifdef SIMUTRACE
    /*! \internal \brief Id of a stream buffer. */
    typedef ObjectId    BufferId;
#define INVALID_BUFFER_ID INVALID_OBJECT_ID
#define SERVER_BUFFER_ID (INVALID_BUFFER_ID - 1)
#endif

    /*! \internal \brief Id of a stream buffer segment. */
    typedef ObjectId    SegmentId;
#define INVALID_SEGMENT_ID INVALID_OBJECT_ID

    /*! \brief Stream sequence number.
     *
     *  In Simutrace, streams are divided into segments of equal size (e.g.,
     *  64 MiB) to allow partial and multi-threaded compression/decompression.
     *  Each segment is identified via its sequence number. The first segment
     *  in a stream gets the sequence number 0, the second segment gets the
     *  sequence number 1 and so on.
     *
     *  \since 3.0
     */
    typedef ObjectId    StreamSegmentId;
#define INVALID_STREAM_SEGMENT_ID INVALID_OBJECT_ID

    /*! \internal \brief Identifies a segment in a stream. */
    typedef struct _StreamSegmentLink {
        StreamId stream;
        StreamSegmentId sequenceNumber;

    #ifdef __cplusplus
        _StreamSegmentLink() :
            stream(INVALID_STREAM_ID),
            sequenceNumber(INVALID_STREAM_SEGMENT_ID) { }

        _StreamSegmentLink(StreamId stream, StreamSegmentId sequenceNumber) :
            stream(stream),
            sequenceNumber(sequenceNumber) { }
    #endif
    } StreamSegmentLink;

#ifdef SIMUTRACE
    /*! \internal \brief Header of a variable-sized data block. */
    typedef struct _VDataBlockHeader {
        uint16_t reserved : 1;
        uint16_t continuation : 1;
        uint16_t size : 14;
    } VDataBlockHeader;
#else
    typedef uint16_t VDataBlockHeader;
#endif

#define VARIABLE_ENTRY_SIZE_FLAG 0x80000000
#define VARIABLE_ENTRY_MAX_SIZE ((1 << 14) - 1 + sizeof(VDataBlockHeader))
#define VARIABLE_ENTRY_EMPTY_INDEX (INVALID_ENTRY_INDEX - 1)

    /*! \brief Makes the entry size for variable-sized data streams.
     *
     *  Variable-sized data streams are distinguished by fixed-sized data
     *  streams by specifically encoding the entry size property of a stream.
     *  To register a stream for variable-sized data use this method to make
     *  the entry size value.
     *
     *  \param sizeHint Variable-sized data is stored by splitting it into
     *                  specifically encoded fixed-sized blocks. The size hint
     *                  should be chosen so that it is the expected mean length
     *                  of the variable-sized data that should be stored in the
     *                  stream.
     *
     *  \returns The encoded entry size value, which can be used to register
     *           a new stream.
     *
     *  \remarks Variable-sized entries are not supported for dynamic streams.
     *           If the stream should return entries of different size, consider
     *           to use a fixed-size meta entry that contains information on
     *           the size, type, and location of the real entry and return this
     *           instead.
     *
     *  \since 3.0
     *
     *  \see StStreamRegister()
     *  \see isVariableEntrySize()
     *  \see getEntrySize()
     */
    static inline uint32_t makeVariableEntrySize(uint32_t sizeHint)
    {
        STASSERT((sizeHint & VARIABLE_ENTRY_SIZE_FLAG) == 0);
        STASSERT(sizeHint > sizeof(VDataBlockHeader));

        if (sizeHint > VARIABLE_ENTRY_MAX_SIZE) {
            sizeHint = VARIABLE_ENTRY_MAX_SIZE;
        }

        return (sizeHint | VARIABLE_ENTRY_SIZE_FLAG);
    }


    /*! \brief Determines if a given entry size is variable.
     *
     *  This method determines if the given entry size encodes a variable entry
     *  size.
     *
     *  \param entrySize The entry size to check.
     *
     *  \returns \c _true if the entry size is variable, \c _false otherwise.
     *
     *  \since 3.0
     *
     *  \see makeVariableEntrySize()
     *  \see getEntrySize()
     */
    static inline _bool isVariableEntrySize(uint32_t entrySize)
    {
        return ((entrySize & VARIABLE_ENTRY_SIZE_FLAG) != 0) ? _true : _false;
    }

#ifdef SIMUTRACE
    /*! \internal \brief Returns the size hint for a variable entry size. */
    static inline uint32_t getSizeHint(uint32_t variableEntrySize)
    {
        assert((variableEntrySize & VARIABLE_ENTRY_SIZE_FLAG) != 0);
        return variableEntrySize & (~VARIABLE_ENTRY_SIZE_FLAG);
    }

    /*! \internal \brief Computes the block offset in variable-sized data */
    static inline size_t getVBufferOffsetFromIndex(uint64_t rawEntryIndex,
                                                   uint32_t sizeHint)
    {
        assert((sizeHint > sizeof(VDataBlockHeader)) &&
               (sizeHint <= VARIABLE_ENTRY_MAX_SIZE));

        return rawEntryIndex * sizeHint;
    }

    /*! \internal \brief Encodes a buffer as variable-sized data */
    static inline size_t writeVariableData(byte* dest, size_t destSize,
                                           const byte* data, size_t dataSize,
                                           uint32_t sizeHint,
                                           uint32_t* outBlockCount)
    {
        assert(dest != NULL);
        assert(data != NULL);
        assert(dataSize > 0);
        assert((sizeHint > sizeof(VDataBlockHeader)) &&
               (sizeHint <= VARIABLE_ENTRY_MAX_SIZE));

        if (destSize < sizeHint) {
            return 0;
        }

        size_t blockDataSize = sizeHint - sizeof(VDataBlockHeader);
        size_t savedDataSize = dataSize;
        uint32_t blockCount = 0;

        do {
            blockCount++;

            /* Account for the current block in the source and destination */
            destSize -= sizeHint;

            size_t chunkSize = (dataSize >= blockDataSize) ?
                blockDataSize : dataSize;
            dataSize -= chunkSize;

            /* Write block header and data to buffer */
            VDataBlockHeader* header = (VDataBlockHeader*)(dest);
            header->reserved = 0;
            header->size = (uint16_t)chunkSize;

            dest += sizeof(VDataBlockHeader);
            memcpy(dest, data, chunkSize);

            dest += chunkSize;

            if (dataSize == 0) {
                /* We have written out all data. Fill the rest of the block
                   with zeros to help compression. Then exit.               */
                header->continuation = 0;

                memset(dest, 0, blockDataSize - chunkSize);
                break;
            } else {
                header->continuation = 1;

                data += chunkSize;
            }

        } while (destSize >= sizeHint);

        *outBlockCount = blockCount;
        return savedDataSize - dataSize;
    }

    /*! \internal \brief Decodes a buffer of variable-sized data. */
    static inline _bool readVariableData(const byte* src, size_t srcSize,
                                         byte* data, size_t* dataSize)
    {
        assert(src != NULL);
        assert(srcSize > sizeof(VDataBlockHeader));

        assert(dataSize != NULL);

        size_t size = 0;
        do {
            VDataBlockHeader* header = (VDataBlockHeader*)(src);
            size += header->size;

            /* If no destination buffer is supplied, we only calculate the
               size of the encoded variable data                           */
            if (data != NULL) {
                memcpy(data, src + sizeof(VDataBlockHeader), header->size);
                data += header->size;
            }

            if (!header->continuation) {
                *dataSize = size;
                return _true;
            }

            /* Move the source to the next block. */
            size_t sizeHint = sizeof(VDataBlockHeader) + header->size;
            assert(srcSize >= sizeHint);

            src += sizeHint;
            srcSize -= sizeHint;
        } while (srcSize > sizeof(VDataBlockHeader));

        *dataSize = size;
        return _false;
    }

    /*! \internal \brief Finds offset of the first block of the n-th entry */
    static inline size_t findVariableEntry(const byte* buffer, size_t size,
                                           uint64_t localSearchIndex)
    {
        assert(buffer != NULL);
        assert(size > sizeof(VDataBlockHeader));

        uint64_t localIndex = 0;
        const byte* buf = buffer;
        do {
            VDataBlockHeader* header = (VDataBlockHeader*)(buffer);

            if (!header->continuation) {
                localIndex++;

                if (localIndex == localSearchIndex) {
                    return buffer - buf;
                }
            }

            /* Move the buffer to the next block. */
            size_t sizeHint = sizeof(VDataBlockHeader) + header->size;
            assert(size >= sizeHint);

            buffer += sizeHint;
            size -= sizeHint;
        } while (size > sizeof(VDataBlockHeader));

        return -1;
    }
#endif /* SIMUTRACE */

#define MAX_STREAM_NAME_LENGTH 256
#define MAX_STREAM_TYPE_NAME_LENGTH MAX_STREAM_NAME_LENGTH
#define TEMPORAL_ORDER_CYCLE_COUNT_BITS 48ULL
#define TEMPORAL_ORDER_CYCLE_COUNT_MASK \
    ((1ULL << TEMPORAL_ORDER_CYCLE_COUNT_BITS) - 1)

    /*! \brief Stream type flags
     *
     *  Describes specific properties of a stream and its entries.
     *
     *  \since 3.2
     */
    typedef enum _StreamTypeFlags {
        StfNone           = 0x00,

        /*! \brief Temporally ordered stream

            Indicates if the entries in the stream are temporally ordered, that
            is if the entries contain a time stamp. The first member in each
            entry must be a 48 bit (\c TEMPORAL_ORDER_CYCLE_COUNT_BITS)
            monotonically increasing (cycle) counter field. */
        StfTemporalOrder  = 0x01,
        StfBigEndian      = 0x02,  /*!< \brief Value endianess.
                                       Not supported, yet */
        StfArch32Bit      = 0x04   /*!< \brief Indicates if type is meant for
                                       32 bit architectures */
    } StreamTypeFlags;


    /*! \brief Describes the type of entries in a stream.
     *
     *  Each stream must have a type that describes the size and properties of
     *  the entries in a stream. The #StreamTypeDescriptor encapsulates this
     *  information.
     *
     *  \deprecated <b>Before 3.2:</b> A set of bit fields was used to
     *              represent the stream properties. The fields have been
     *              replaced with the flags field to improve interoperability.
     *
     *  \since 3.0
     */
    typedef struct _StreamTypeDescriptor {
        char name[MAX_STREAM_TYPE_NAME_LENGTH]; /*!< \brief Friendly name of
                                                     the type */
        StreamTypeId id;                        /*!< \brief Unique id of the
                                                     type */
        StreamTypeFlags flags;                  /*!< \brief Stream type flags.
                                                     \remarks Replaces set of
                                                     bit fields in a binary
                                                     compatible way.
                                                     \since 3.2 */

        uint32_t reserved1;       /*!< \brief Reserved. Set to 0 */

        /*! \brief Entry size

            Size of a single entry in bytes. To make variable-sized entries use
            makeVariableEntrySize(). If the size itself is desired, do not read
            the entry size directly, instead use getEntrySize() */
        uint32_t entrySize;

        uint32_t reserved2;       /*!< \brief Reserved. Set to 0 */
    } StreamTypeDescriptor;


    /*! \brief Returns the entry size of a stream type.
     *
     *  This method extracts the entry size of the supplied stream type. For
     *  variable-sized stream types it will return the size hint as given to
     *  makeVariableEntrySize().
     *
     *  \param desc Pointer to a stream type description from which to read
     *              the entry size.
     *
     *  \returns The entry size of the given stream type.
     *
     *  \since 3.0
     *
     *  \see makeVariableEntrySize()
     */
    static inline uint32_t getEntrySize(const StreamTypeDescriptor* desc)
    {
        STASSERT(desc != NULL);

        return desc->entrySize & (~VARIABLE_ENTRY_SIZE_FLAG);
    }


#ifdef SIMUTRACE
    /*! \internal \brief Filters for stream enumeration */
    typedef enum _StreamEnumFilter {
        SefRegular  = 0x01,
        SefHidden   = 0x02,
        SefDynamic  = 0x04,

        SefAll      = SefRegular | SefHidden | SefDynamic
    } StreamEnumFilter;
#endif


    /*! \brief Basic stream flags
     *
     *  Describes general properties of the stream that are independent of the
     *  specified entry type.
     *
     *  \since 3.2
     */
    typedef enum _StreamFlags {
        SfNone    = 0x00,    /*!< Regular stream for recording events */
        SfHidden  = 0x01,    /*!< Hidden stream. Internal, do not set */
        SfDynamic = 0x02     /*!< Dynamic stream. Entries are generated
                                  dynamically. Stream descriptor must be of
                                  type #DynamicStreamDescriptor.
                                  \see StStreamRegisterDynamic() */
    } StreamFlags;


    /*! \brief Describes a stream.
     *
     *  Contains all information necessary to register a new regular stream.
     *  For dynamic streams see #DynamicStreamDescriptor.
     *
     *  \deprecated <b>Before 3.2:</b> The <i>hidden</i> field was the only
     *              possible flag. The field has been replaced to add further
     *              flags.
     *
     *  \since 3.0
     */
    typedef struct _StreamDescriptor {
        char name[MAX_STREAM_NAME_LENGTH]; /*!< \brief Friendly name of the
                                                stream */

        StreamFlags flags;                 /*!< \brief Stream flags
                                                \remarks Replaces solitary
                                                <i>hidden</i> flag in a binary
                                                compatible way.
                                                \since 3.2 */
        uint32_t reserved1;                /*!< \brief Reserved. Set to 0 */

        StreamTypeDescriptor type;         /*!< \brief Type of the stream */
    } StreamDescriptor;


    /*! \brief Describes data ranges in a stream.
     *
     *  The entries in a stream or stream segment cover ranges of certain
     *  properties, for example a time span. Simutrace builds indices over
     *  these ranges to allow the user to search for supplied points (e.g.,
     *  a certain point in simulation time) when reading trace data. This
     *  structure contains all ranges that Simutrace uses for index generation.
     *
     *  \since 3.0
     */
    typedef union _StreamRangeInformation {
    /*! \cond */
        struct {
    /*! \endcond */
            uint64_t startIndex;    /*!< \brief Index of the first entry */
            uint64_t endIndex;      /*!< \brief Index of the last entry  */

            CycleCount startCycle;  /*!< \brief Simulation time of the first
                                         entry */
            CycleCount endCycle;    /*!< \brief Simulation time of the last
                                         entry */

            Timestamp startTime;    /*!< \brief Wall clock time at the
                                         allocation of the range in the server
                                         during recording */
            Timestamp endTime;      /*!< \brief Wall clock time at the start of
                                         the processing by the server */
    /*! \cond */
        };

    #ifdef SIMUTRACE
        Range ranges[3]; /*!< Internal, do not use. */
    #endif
    /*! \endcond */

    #ifdef __cplusplus
        _StreamRangeInformation() :
            startIndex(INVALID_ENTRY_INDEX),
            endIndex(INVALID_ENTRY_INDEX),
            startCycle(INVALID_CYCLE_COUNT),
            endCycle(INVALID_CYCLE_COUNT),
            startTime(INVALID_TIME_STAMP),
            endTime(INVALID_TIME_STAMP) { };
    #endif
    } StreamRangeInformation;


    /*! \brief Statistic information on a stream.
     *
     *  This structure contains a set of summarizing statistics on a stream and
     *  can be used to get an impression of the size and range of data covered
     *  by a stream.
     *
     *  \since 3.0
     */
    typedef struct _StreamStatistics {
        uint64_t compressedSize;        /*!< \brief Total size of the stream in
                                             bytes after compression with the
                                             type-specific encoder */

        uint64_t entryCount;            /*!< \brief Number of entries in the
                                             stream */
        uint64_t rawEntryCount;         /*!< \brief Number of raw entries in the
                                             stream.

                                             This value differs from
                                             \c entryCount only in streams with
                                             variable-sized data and is the
                                             number of data blocks. */

        StreamRangeInformation ranges;  /*!< \brief Range information over the
                                             entire stream */
    #ifdef __cplusplus
        _StreamStatistics() :
            compressedSize(0),
            entryCount(0),
            rawEntryCount(0),
            ranges() { };
    #endif
    } StreamStatistics;


    /*! \brief Stream query result.
     *
     *  Contains stream registration and type information as well as
     *  statistics about the encapsulated data.
     *
     *  \since 3.0
     *
     *  \see StStreamQuery()
     */
    typedef struct _StreamQueryInformation {
        StreamDescriptor descriptor; /*!< \brief Stream registration
                                          information */
        StreamStatistics stats;      /*!< \brief Stream data statistics */
    } StreamQueryInformation;


    /*! \internal \brief Segment control element. */
    typedef struct _SegmentControlElement {
        /* Modified by the server - do not touch! */
        uint64_t cookie;
        StreamSegmentLink link;

        /* Modified by the client (Writer) */
        uint32_t entryCount;
        uint32_t rawEntryCount;

        /* The server may set startIndex to INVALID_ENTRY_INDEX to indicate
           that no index-based addressing is needed. The client must not
           touch this field */
        uint64_t startIndex;

        /* Modified by the server - do not touch! */
        CycleCount endCycle;
        CycleCount startCycle;

        Timestamp endTime;
        Timestamp startTime; /* Always included in cookie */
    } SegmentControlElement;


    /*! \brief Type for stream open queries.
     *
     *  The query type specifies the type of position the caller requests in a
     *  call to StStreamOpen(). Depending on the type, the supplied query value
     *  is interpreted differently.
     *
     *  \remarks Dynamic streams are technically not forced to adhere to the
     *           semantic. See the dynamic stream's documentation for
     *           more information.
     *
     *  \since 3.0
     *
     *  \see StStreamOpen()
     */
    typedef enum _QueryIndexType {
        /* We build individual index trees
           for the following index types   */
        QIndex          = 0x00,      /*!< Index of an entry. The first entry
                                          in a stream has index 0, the second
                                          one has index 1 and so on          */
        QCycleCount     = 0x01,      /*!< Cycle count of an entry            */
        QRealTime       = 0x02,      /*!< Wall clock time. Simutrace
                                          automatically records the wall clock
                                          time for each segment (e.g., 64 MiB)
                                          of trace data in a stream          */

        _QMaxTree       = QRealTime, /*!< Internal, do not use               */

        /* Not tree-based index types */
        QSequenceNumber              = _QMaxTree + 0x01, /*!< The sequence
                                                             number of a stream
                                                             segment         */
        QNextValidSequenceNumber     = _QMaxTree + 0x02, /*!< Points to the next
                                                             valid sequence
                                                             number          */

        /* Since 3.1 */
        QPreviousValidSequenceNumber = _QMaxTree + 0x03, /*!< Points to the
                                                             previous sequence
                                                             number
                                                             \since 3.1      */

        /* Since 3.2 - User-defined query types for dynamic streams */
        QUserIndex0 = _QMaxTree + 0x04,  /*!< Available for free use with
                                              dynamic streams \since 3.2     */

        QUserIndex1 = _QMaxTree + 0x05,  /*!< Available for free use with
                                              dynamic streams \since 3.2     */

        QUserIndex2 = _QMaxTree + 0x06,  /*!< Available for free use with
                                              dynamic streams \since 3.2     */

        QUserIndex3 = _QMaxTree + 0x07,  /*!< Available for free use with
                                              dynamic streams \since 3.2     */
    } QueryIndexType;


    /*! \brief Stream access behavior
     *
     *  Describes the intended stream access behavior. Simutrace uses this as
     *  hint to adapt caching, read-ahead policies and interpret query values.
     *
     *  \remarks Dynamic streams are technically not forced to adhere to the
     *           semantic. See the dynamic stream's documentation for
     *           more information.
     *
     *  \since 3.0
     *
     *  \see StStreamOpen()
     */
    typedef enum _StreamAccessFlags {
        SafNone             = 0x00, /*!< No information. Trace data is cached
                                         with LRU, no read-ahead             */
        SafSequentialScan   = 0x01, /*!< The caller intends to read the stream
                                         in sequential order. Segments are
                                         cached, but reused as soon as possible
                                         to avoid cache pollution. Maximum
                                         read-ahead. This access pattern and
                                         settings provide best read
                                         performance                         */
        SafRandomAccess     = 0x02, /*!< Data is accessed in random order.
                                         Segments are cached, but reused as
                                         soons as possible to avoid cache
                                         pollution, no read-ahead            */

        /* Implict in client */
        SafSynchronous      = 0x04, /*!< Internal, do not use */

        /* Since 3.1 */
        SafReverseQuery     = 0x08, /*!< The query information specified in the
                                         call to StStreamOpen() are relative to
                                         the end of the stream
                                         \since 3.1                          */

        SafReverseRead      = 0x10, /*!< The caller intends to read the stream
                                         in reverse order. In a call to
                                         StStreamOpen(), this flag prepares the
                                         new read handle for use with
                                         StGetPreviousEntryFast()
                                         \since 3.1                          */

        /* Since 3.2 - User-defined flags for dynamic streams */
        SafUserFlag0        = 0x20, /*!< Available for free use with dynamic
                                         streams \since 3.2                  */
        SafUserFlag1        = 0x40, /*!< Available for free use with dynamic
                                         streams \since 3.2                  */
        SafUserFlag2        = 0x80, /*!< Available for free use with dynamic
                                         streams \since 3.2                  */
        SafUserFlag3        = 0x100 /*!< Available for free use with dynamic
                                         streams \since 3.2                  */
    } StreamAccessFlags;

#ifdef __cplusplus
}
    /* StreamTypeId */
    inline bool operator==(const StreamTypeId& id1, const StreamTypeId& id2)
    {
        return (memcmp(&id1, &id2, sizeof(StreamTypeId)) == 0);
    }

    /* StreamSegmentLink */
    inline bool operator==(const StreamSegmentLink& l1, const StreamSegmentLink& l2)
    {
        return (memcmp(&l1, &l2, sizeof(StreamSegmentLink)) == 0);
    }

#ifdef SIMUTRACE
    /* StreamEnumFilter */
    inline StreamEnumFilter operator^(StreamEnumFilter a, StreamEnumFilter b)
    {
        return static_cast<StreamEnumFilter>(
            static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline StreamEnumFilter operator|(StreamEnumFilter a, StreamEnumFilter b)
    {
        return static_cast<StreamEnumFilter>(
            static_cast<int>(a) | static_cast<int>(b));
    }

    inline StreamEnumFilter operator&(StreamEnumFilter a, StreamEnumFilter b)
    {
        return static_cast<StreamEnumFilter>(
            static_cast<int>(a) & static_cast<int>(b));
    }
#endif

    /* StreamTypeFlags */
    inline StreamTypeFlags operator^(StreamTypeFlags a, StreamTypeFlags b)
    {
        return static_cast<StreamTypeFlags>(
            static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline StreamTypeFlags operator|(StreamTypeFlags a, StreamTypeFlags b)
    {
        return static_cast<StreamTypeFlags>(
            static_cast<int>(a) | static_cast<int>(b));
    }

    inline StreamTypeFlags operator&(StreamTypeFlags a, StreamTypeFlags b)
    {
        return static_cast<StreamTypeFlags>(
            static_cast<int>(a) & static_cast<int>(b));
    }

    /* StreamFlags */
    inline StreamFlags operator^(StreamFlags a, StreamFlags b)
    {
        return static_cast<StreamFlags>(
            static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline StreamFlags operator|(StreamFlags a, StreamFlags b)
    {
        return static_cast<StreamFlags>(
            static_cast<int>(a) | static_cast<int>(b));
    }

    inline StreamFlags operator&(StreamFlags a, StreamFlags b)
    {
        return static_cast<StreamFlags>(
            static_cast<int>(a) & static_cast<int>(b));
    }

    /* StreamAccessFlags */
    inline StreamAccessFlags operator^(StreamAccessFlags a, StreamAccessFlags b)
    {
        return static_cast<StreamAccessFlags>(
            static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline StreamAccessFlags operator|(StreamAccessFlags a, StreamAccessFlags b)
    {
        return static_cast<StreamAccessFlags>(
            static_cast<int>(a) | static_cast<int>(b));
    }

    inline StreamAccessFlags operator&(StreamAccessFlags a, StreamAccessFlags b)
    {
        return static_cast<StreamAccessFlags>(
            static_cast<int>(a) & static_cast<int>(b));
    }

}
#endif

#endif
