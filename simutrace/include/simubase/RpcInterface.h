/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef RPC_INTERFACE_H
#define RPC_INTERFACE_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Version.h"

namespace SimuTrace
{

    //
    // Basic format of messages sent with server port or client port.
    //

#define MESSAGE_SIZE offsetof(Message, data.payload)

    enum MessagePayloadType {
        MptEmbedded            = 0x00,
        MptData                = 0x01,
        MptHandles             = 0x02,

        MptMax
    };

    enum MessageFlags {
        MfResponse             = 0x01,

        MfMax
    };

    enum LocalMessageFlags {
        LmfCustomAllocation    = 0x01,
        LmfAllocationOwner     = 0x02,

        LmfMax
    };

#ifdef _DEBUG
    //
    // The following functions are used by the RPC logging mechanism, which is
    // only available in DEBUG builds.
    //

    attribute_unused
    static std::string payloadTypeToStr(uint8_t payloadType)
    {
        switch (payloadType)
        {
            case MessagePayloadType::MptEmbedded:
                return std::string("Embedded");

            case MessagePayloadType::MptData:
                return std::string("Data");

            case MessagePayloadType::MptHandles:
                return std::string("Handles");

            default:
                return std::string("Unknown");
        }
    }

    attribute_unused
    static std::string messageFlagsToStr(uint8_t flags)
    {
        static const char* flagChars[] = {
            "r" // LmfResponse
        };

        std::stringstream str;
        for (uint32_t i = 0; ; ++i) {
            const uint32_t flag = (1 << i);
            if (flag >= MessageFlags::MfMax) {
                break;
            }

            assert(i < sizeof(flagChars) / sizeof(char*));
            str << (((flags && flag) != 0) ? flagChars[i] : "-");
        }

        return str.str();
    }

    attribute_unused
    static std::string localMessageFlagsToStr(uint8_t flags)
    {
        static const char* flagChars[] = {
            "c", // LmfCustomAllocation
            "o"  // LmfAllocationOwner
        };

        std::stringstream str;
        for (uint32_t i = 0; ; ++i) {
            const uint32_t flag = (1 << i);
            if (flag >= LocalMessageFlags::LmfMax) {
                break;
            }

            assert(i < sizeof(flagChars) / sizeof(char*));
            str << (((flags && flag) != 0) ? flagChars[i] : "-");
        }

        return str.str();
    }
#endif

    struct Message;
    typedef void (*PayloadAllocator)(Message& msg, bool free, void* args);

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

    struct Message
    {
        uint8_t sequenceNumber;
        uint8_t payloadType : 2;
        uint8_t flags : 6;

        union {
            struct {
                uint16_t controlCode;
            } request;
            struct {
                uint16_t status;
            } response;
        };

        uint32_t parameter0;

        union {

            // MESSAGE_PAYLOAD_EMBEDDED
            struct {
                uint64_t parameter1;

            } embedded;

            // MESSAGE_PAYLOAD_DATA
            struct {
                uint32_t parameter1;
                uint32_t payloadLength;

                void* payload;
            } data;

            // MESSAGE_PAYLOAD_HANDLES
            struct {
                uint32_t parameter1;
                uint32_t handleCount;

                std::vector<Handle>* handles;
            } handles;

        };

        // ^                                                    ^
        // | All fields up to this point are send over the wire |
        // +----------------------------------------------------+

        void* allocatorArgs;
        PayloadAllocator allocator;
        uint8_t localFlags;

        ~Message();

        void setupEmpty();
        void setupEmbedded(uint32_t parameter0, uint64_t parameter1);
        void setupData(uint32_t parameter0, uint32_t parameter1,
                       const void* data, uint32_t length);
        void setupHandle(uint32_t parameter0, uint32_t parameter1,
                         std::vector<Handle>& handles);

        bool test(MessagePayloadType payloadType, size_t expectedLength);

        void discard();
    };

#pragma pack(pop)   /* restore original alignment from stack */


    //
    // RPC Helper Functions and Macros
    //

#define RPC_VER_MAJOR(ver) SIMUTRACE_VER_MAJOR(ver)
#define RPC_VER_MINOR(ver) SIMUTRACE_VER_MINOR(ver)
#define RPC_VER(major, minor) SIMUTRACE_VER(major, minor, 0)
#define RPC_VER_AND_CODE(ver, code) (((ver) << 16) | (code))

#define RPC_CALL(vmajor, vminor, code, name, payloadType, expectedLength)     \
    namespace RpcApi {                                                        \
        enum {                                                                \
            CCV##vmajor##vminor##_##name =                                    \
                RPC_VER_AND_CODE(RPC_VER(vmajor, vminor), code)               \
        };                                                                    \
        inline bool _testRequest_V##vmajor##vminor##_##name(Message& msg)     \
        {                                                                     \
            return msg.test(MessagePayloadType::Mpt##payloadType, (expectedLength)); \
        }                                                                     \
    }

#define TEST_REQUEST(vmajor, vminor, rpcname, msg)                            \
    if (!RpcApi::_testRequest_V##vmajor##vminor##_##rpcname(msg)) {           \
        Throw(RpcMessageMalformedException);                                  \
    }

#define RPC_STATUS(code, name)                                                \
    namespace RpcApi {                                                        \
        enum { SC_##name = code };                                            \
    }

    //
    // Default RPC Status codes
    //

    RPC_STATUS(0x0000, Success)
    RPC_STATUS(0x0001, Failed)
}

#endif