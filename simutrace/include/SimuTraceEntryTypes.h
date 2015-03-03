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
 * Simutrace Client Library (libsimutrace) is part of Simutrace.
 *
 * libsimutrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimutrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
 */
/*! \file */
#pragma once
#ifndef SIMUTRACE_ENTRY_TYPES_H
#define SIMUTRACE_ENTRY_TYPES_H

#include "SimuStorTypes.h"

#ifdef __cplusplus
namespace SimuTrace {
extern "C"
{
#endif

    /* Basic type definitions */

    /*! \brief 32 bit memory address
     *
     *  Holds virtual or physical addresses accessed during a memory operation.
     *  This type is used on 32 bit architectures as defined by
     *  #ArchitectureSize.
     *
     *  \since 3.0
     */
    typedef uint32_t Address32;


    /*! \brief 64 bit memory address
     *
     *  Holds virtual or physical addresses accessed during a memory operation.
     *  This type is used on 64 bit architectures as defined by
     *  #ArchitectureSize.
     *
     *  \since 3.0
     */
    typedef uint64_t Address64;


    /*! \brief 32 bit data
     *
     *  Holds data read or written during a memory operation. This type
     *  is used on 32 bit architectures as defined by #ArchitectureSize.
     *
     *  \remarks The larger fields (\c data32) include the smaller fields
     *           (\c data8). Thus assigning a full 32 bit double word to
     *           \c data32 will also set all other fields accordingly.
     *
     *  \since 3.0
     *
     *  \see #Data64
     *  \see #MemoryAccessMetaData
     */
    typedef union _Data32 {
        uint32_t data32;         /*!< \brief Full 32 bit data value */

        /*! \cond */
        struct {
            union {
        /*! \endcond */

                uint8_t data8;   /*!< \brief Lower first byte (8 bits) */
                uint16_t data16; /*!< \brief Lower first word (16 bits) */

        /*! \cond */
            };
        /*! \endcond */

            uint16_t size;      /*!< \brief Memory access size

                                     For memory operations with a data size
                                     less than 32 bits, this field contains
                                     the exact size. Otherwise, this field
                                     contains the high 16 bit word. */

        /*! \cond */
        };
        /*! \endcond */
    } Data32;


    /*! \brief 64 bit data
     *
     *  Encapsulates data read or written during a memory operation. This type
     *  is used on 64 bit architectures as defined by #ArchitectureSize.
     *
     *  \remarks The larger fields (\c data64) include the smaller fields
     *           (\c data8). Thus assigning a full 64 bit quad word to
     *           \c data64 will also set all other fields accordingly.
     *
     *  \since 3.0
     *
     *  \see #Data32
     *  \see #MemoryAccessMetaData
     */
    typedef union _Data64 {
        uint64_t data64;         /*!< \brief Full 64 bit data value */

        /*! \cond */
        struct {
            union {
        /*! \endcond */

                uint8_t data8;   /*!< \brief Lower first byte (8 bits) */
                uint16_t data16; /*!< \brief Lower first word (16 bits) */
                uint32_t data32; /*!< \brief Lower first double word
                                      (32 bits) */

        /*! \cond */
            };
        /*! \endcond */

            uint32_t size;      /*!< \brief Memory access size

                                     For memory operations with a data size
                                     less than 64 bits, this field contains the
                                     exact size. Otherwise, this field contains
                                     the high 32 bit double word. */

        /*! \cond */
        };
        /*! \endcond */
    } Data64;


    /* Entry definitions for built-in memory access types */

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

#if TEMPORAL_ORDER_CYCLE_COUNT_BITS != 48
#error "MemoryAccessMetaData expects 48 bits of cycle count data."
#endif

    /*! \brief Memory trace entry meta data
     *
     *  Each memory trace entry starts with a meta data field. This field holds
     *  a time stamp indicating the time when the memory operation happened
     *  and saves information on the size of the operation. The meta data field
     *  has also space for user-defined data.
     *
     *  \since 3.0
     *
     *  \see #Write32
     *  \see #DataWrite32
     *  \see #Write64
     *  \see #DataWrite64
     */
    typedef union _MemoryAccessMetaData {
        /*! \cond */
        struct {
        /*! \endcond */

            /*! \brief Simulation time

                Indicates when the operation took place (48 bits). The
                cycle count is often expressed as the number of instructions
                executed by one or more virtual CPU(s) since the start of the
                simulation or tracing session. */
            uint64_t cycleCount : TEMPORAL_ORDER_CYCLE_COUNT_BITS;

            /* Custom memory access fields, not interpreted by Simutrace */

            /*! \brief Indicates a full size memory access

                If set, the operation accessed the memory with the full
                architecture size (e.g., 32 or 64 bits). The \c size field
                in the #Data32 and #Data64 types will then not be valid but
                contain actual data. */
            uint64_t fullSize : 1;

            /*! \brief User information

                The tag can be freely set by the user to encode additional
                information with every memory operation. */
            uint64_t tag : 15;
    /*! \cond */
        };

        uint64_t value;
    /*! \endcond */
    } MemoryAccessMetaData;


    /*! \brief 32 bit memory trace entry with data
     *
     *  Memory trace entry for 32 bit memory operations (read or write)
     *  including the read or written data. To register a stream using this
     *  entry type see StStreamFindMemoryType().
     *
     *  \since 3.0
     *
     *  \see StStreamFindMemoryType()
     *  \see #MemoryAccessMetaData
     *  \see #Write32
     *  \see #Write64
     *  \see #DataWrite64
     */
    typedef struct _DataMemoryAccess32 {
        MemoryAccessMetaData metadata;  /*!< \brief Operation meta data */

        Address32 ip;                   /*!< \brief Instruction pointer (IP) in
                                             virtual memory of the CPU
                                             instruction that issued the
                                             memory operation */
    /*! \cond */
        union {
            struct {
    /*! \endcond */
                Address32 address;      /*!< \brief Physical or virtual address
                                             accessed by the memory operation*/
                Data32    data;         /*!< \brief Read or written data. See
                                             the meta data field and #Data32
                                             for information on the access size
                                             */
    /*! \cond */
            };

            uint32_t dataFields[2];
        };
    /*! \endcond */
    } DataMemoryAccess32, DataRead32, DataWrite32;


    /*! \brief 32 bit memory trace entry
     *
     *  Memory trace entry for 32 bit memory operations (read or write).
     *  To register a stream using this entry type see StStreamFindMemoryType().
     *
     *  \since 3.0
     *
     *  \see StStreamFindMemoryType()
     *  \see #MemoryAccessMetaData
     *  \see #DataWrite32
     *  \see #Write64
     *  \see #DataWrite64
     */
    typedef struct _MemoryAccess32 {
        MemoryAccessMetaData metadata;  /*!< \brief Operation meta data */

        Address32 ip;                   /*!< \brief Instruction pointer (IP) in
                                             virtual memory of the CPU
                                             instruction that issued the
                                             memory operation */
    /*! \cond */
        union {
            struct {
    /*! \endcond */
                Address32 address;      /*!< \brief Physical or virtual address
                                             accessed by the memory operation*/
    /*! \cond */
            };

            uint32_t dataFields[1];
        };
    /*! \endcond */
    } MemoryAccess32, Read32, Write32;


    /*! \brief 64 bit memory trace entry with data
     *
     *  Memory trace entry for 64 bit memory operations (read or write)
     *  including the read or written data. To register a stream using this
     *  entry type see StStreamFindMemoryType().
     *
     *  \since 3.0
     *
     *  \see StStreamFindMemoryType()
     *  \see #MemoryAccessMetaData
     *  \see #Write32
     *  \see #DataWrite32
     *  \see #Write64
     */
    typedef struct _DataMemoryAccess64 {
        MemoryAccessMetaData metadata;  /*!< \brief Operation meta data */

        Address64 ip;                   /*!< \brief Instruction pointer (IP) in
                                             virtual memory of the CPU
                                             instruction that issued the
                                             memory operation */
    /*! \cond */
        union {
            struct {
    /*! \endcond */
                Address64 address;      /*!< \brief Physical or virtual address
                                             accessed by the memory operation*/
                Data64    data;         /*!< \brief Read or written data. See
                                             the meta data field and #Data64
                                             for information on the access size
                                             */
    /*! \cond */
            };

            uint64_t dataFields[2];
        };
    /*! \endcond */
    } DataMemoryAccess64, DataRead64, DataWrite64;


    /*! \brief 64 bit memory trace entry
     *
     *  Memory trace entry for 64 bit memory operations (read or write).
     *  To register a stream using this entry type see StStreamFindMemoryType().
     *
     *  \since 3.0
     *
     *  \see StStreamFindMemoryType()
     *  \see #MemoryAccessMetaData
     *  \see #Write32
     *  \see #DataWrite32
     *  \see #DataWrite64
     */
    typedef struct _MemoryAccess64 {
        MemoryAccessMetaData metadata;  /*!< \brief Operation meta data */

        Address64 ip;                   /*!< \brief Instruction pointer (IP) in
                                             virtual memory of the CPU
                                             instruction that issued the
                                             memory operation */
    /*! \cond */
        union {
            struct {
    /*! \endcond */
                Address64 address;      /*!< \brief Physical or virtual address
                                             accessed by the memory operation*/
    /*! \cond */
            };

            uint64_t dataFields[1];
        };
    /*! \endcond */
    } MemoryAccess64, Read64, Write64;

 #pragma pack(pop)


    /* Stream type descriptions for built-in memory accesses types */

    /*! \brief The bit size of the architecture.
     *
     *  Specifies the size of the memory accesses in bits that a stream
     *  should receive. This will determine the size of the instruction
     *  pointer, memory address and data fields.
     *
     *  \since 3.0
     */
    typedef enum _ArchitectureSize {
        As32Bit = 0x00, /*!< 32 bit memory operations */
        As64Bit = 0x01, /*!< 64 bit memory operations */

        AsMax           /*!< Internal, do not use. */
    } ArchitectureSize;


    /*! \brief The type of a memory operation.
     *
     * Specifies the type of memory operation that a stream should receive.
     *
     *  \since 3.0
     */
    typedef enum _MemoryAccessType {
        MatRead  = 0x00,    /*!< Memory read operations */
        MatWrite = 0x01,    /*!< Memory write operations */

        MatMax              /*!< Internal, do not use. */
    } MemoryAccessType;


    /*! \brief The type of a memory address.
     *
     *  Specifies the semantic of the address field in a memory entry.
     *
     *  \since 3.0
     */
    typedef enum _MemoryAddressType {
        AtPhysical = 0x00,      /*!< Address in physical memory, i.e., RAM */
        AtVirtual  = 0x01,      /*!< Address in virtual memory, e.g., the
                                     address space of a process            */

        AtLogical  = AtVirtual, /*!< Same as AtVirtual */
        AtLinear   = AtVirtual, /*!< Same as AtVirtual */

        AtMax                   /*!< Internal, do not use. */
    } MemoryAddressType;

#ifdef SIMUTRACE
#ifdef __GNUC__
// Protecting against GCC's false reporting of missing braces
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

#define MASTYPETABLE_COUNT AsMax*MatMax*AtMax*2
    /*! \internal */
    static const StreamTypeDescriptor _mastypeTable[MASTYPETABLE_COUNT] = {
        /* 32bit read (physical address) (0)
           {3F759BFB-2C25-470C-9646-1698CDF9D835} */
        {"32bit memory read (physical address)",
         {0x3f759bfb,0x2c25,0x470c,{0x96,0x46,0x16,0x98,0xcd,0xf9,0xd8,0x35}},
         _true, _false, _true, 0, 0, sizeof(Read32), 0 },

         /* 64bit read (physical address) (1)
           {03CFACB5-D490-4D5D-B5BE-6BB4A3235DD1} */
        {"64bit memory read (physical address)",
         {0x03cfacb5,0xd490,0x4d5d,{0xb5,0xbe,0x6b,0xb4,0xa3,0x23,0x5d,0xd1}},
         _true, _false, _false, 0, 0, sizeof(Read64), 0 },

        /* 32bit write (physical address) (2)
           {624C5632-3ACC-4B39-871F-33F739E6F93F} */
        {"32bit memory write (physical address)",
         {0x624c5632,0x3acc,0x4b39,{0x87,0x1f,0x33,0xf7,0x39,0xe6,0xf9,0x3f}},
         _true, _false, _true, 0, 0, sizeof(Write32), 0 },

         /* 64bit write (physical address) (3)
           {4CE4D04D-BDDC-4640-B50D-00B35FC08133} */
        {"64bit memory write (physical address)",
         {0x4ce4d04d,0xbddc,0x4640,{0xb5,0x0d,0x00,0xb3,0x5f,0xc0,0x81,0x33}},
         _true, _false, _false, 0, 0, sizeof(Write64), 0 },

         /* 32bit read (virtual address) (4)
           {7CC9A0C4-41BC-4A9E-8F9B-553C0AAE8BA8}*/
        {"32bit memory read (virtual address)",
         {0x7cc9a0c4,0x41bc,0x4a9e,{0x8f,0x9b,0x55,0x3c,0x0a,0xae,0x8b,0xa8}},
         _true, _false, _true, 0, 0, sizeof(Read32), 0 },

         /* 64bit read (virtual address) (5)
           {C81B6CE6-47A0-482B-B38E-283A4CA3ED8C} */
        {"64bit memory read (virtual address)",
         {0xc81b6ce6,0x47a0,0x482b,{0xb3,0x8e,0x28,0x3a,0x4c,0xa3,0xed,0x8c}},
         _true, _false, _false, 0, 0, sizeof(Read64), 0 },

        /* 32bit write (virtual address) (6)
           {22D8A589-16A8-4069-B7DF-DFEE23BA55B6} */
        {"32bit memory write (virtual address)",
         {0x22d8a589,0x16a8,0x4069,{0xb7,0xdf,0xdf,0xee,0x23,0xba,0x55,0xb6}},
         _true, _false, _true, 0, 0, sizeof(Write32), 0 },

         /* 64bit write (virtual address) (7)
           {DE27CF5E-E16B-499C-B101-14062F578AA7} */
        {"64bit memory write (virtual address)",
         {0xde27cf5e,0xe16b,0x499c,{0xb1,0x01,0x14,0x06,0x2f,0x57,0x8a,0xa7}},
         _true, _false, _false, 0, 0, sizeof(Write64), 0 },

         /* 32bit read (data, physical address) (8)
           {74A5E8D8-F8B3-4831-AB0E-ACB004556C0A} */
        {"32bit memory read (data, physical address)",
         {0x74a5e8d8,0xf8b3,0x4831,{0xab,0x0e,0xac,0xb0,0x04,0x55,0x6c,0x0a}},
         _true, _false, _true, 0, 0, sizeof(DataRead32), 0 },

         /* 64bit read (data, physical address) (9)
           {2B58F45A-C2F6-495B-A7D2-9FBBEBCD4710} */
        {"64bit memory read (data, physical address)",
         {0x2b58f45a,0xc2f6,0x495b,{0xa7,0xd2,0x9f,0xbb,0xeb,0xcd,0x47,0x10}},
         _true, _false, _false, 0, 0, sizeof(DataRead64), 0 },

        /* 32bit write (data, physical address) (10)
           {F8F41EE8-EEB1-4C02-B997-C91B35D5D426} */
        {"32bit memory write (data, physical address)",
         {0xf8f41ee8,0xeeb1,0x4c02,{0xb9,0x97,0xc9,0x1b,0x35,0xd5,0xd4,0x26}},
         _true, _false, _true, 0, 0, sizeof(DataWrite32), 0 },

         /* 64bit write (data, physical address) (11)
           {6E943CDD-D2DA-4E83-984F-585C47EB0E36} */
        {"64bit memory write (data, physical address)",
         {0x6e943cdd,0xd2da,0x4e83,{0x98,0x4f,0x58,0x5c,0x47,0xeb,0x0e,0x36}},
         _true, _false, _false, 0, 0, sizeof(DataWrite64), 0 },

         /* 32bit read (data, virtual address) (12)
           {BFCCBB37-A37A-4007-BA04-53B7F1B85746}*/
        {"32bit memory read (data, virtual address)",
         {0xbfccbb37,0xa37a,0x4007,{0xba,0x04,0x53,0xb7,0xf1,0xb8,0x57,0x46}},
         _true, _false, _true, 0, 0, sizeof(DataRead32), 0 },

         /* 64bit read (data, virtual address) (13)
           {62629E18-267D-4AF9-A5EE-ED485893513A} */
        {"64bit memory read (data, virtual address)",
         {0x62629e18,0x267d,0x4af9,{0xa5,0xee,0xed,0x48,0x58,0x93,0x51,0x3a}},
         _true, _false, _false, 0, 0, sizeof(DataRead64), 0 },

        /* 32bit write (data, virtual address) (14)
           {9ABE7322-339E-420D-8224-23113AA05B8C} */
        {"32bit memory write (data, virtual address)",
         {0x9abe7322,0x339e,0x420d,{0x82,0x24,0x23,0x11,0x3a,0xa0,0x5b,0x8c}},
         _true, _false, _true, 0, 0, sizeof(DataWrite32), 0 },

         /* 64bit write (data, virtual address) (15)
           {157F6FAE-0E88-48BA-B829-5C1C818F56A9} */
        {"64bit memory write (data, virtual address)",
         {0x157f6fae,0x0e88,0x48ba,{0xb8,0x29,0x5c,0x1c,0x81,0x8f,0x56,0xa9}},
         _true, _false, _false, 0, 0, sizeof(DataWrite64), 0 }
    };
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    static inline const StreamTypeDescriptor* streamFindMemoryType(
        ArchitectureSize size, MemoryAccessType accessType,
        MemoryAddressType addressType, _bool hasData)
    {
        if ((size >= AsMax) ||
            (accessType >= MatMax) ||
            (addressType >= AtMax)) {

            return NULL;
        }

        /* Caution: This will break when new types are added. */
        hasData = (hasData) ? 1 : 0;
        return &_mastypeTable[(size) |
                              (accessType * AsMax) |
                              (addressType * AsMax * MatMax) |
                              (hasData * AsMax * MatMax * AtMax)];
    }
#endif /* SIMUTRACE */

#ifdef __cplusplus
}
}
#endif

#endif
