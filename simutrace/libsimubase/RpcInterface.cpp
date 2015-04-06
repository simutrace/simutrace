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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "RpcInterface.h"

#include "Exceptions.h"

namespace SimuTrace
{

    Message::~Message()
    {
        discard();
    }

    void Message::setupEmpty()
    {
        payloadType = MessagePayloadType::MptEmbedded;

        Message::parameter0          = 0;
        Message::embedded.parameter1 = 0;
    }

    void Message::setupEmbedded(uint32_t parameter0, uint64_t parameter1)
    {
        payloadType = MessagePayloadType::MptEmbedded;

        Message::parameter0          = parameter0;
        Message::embedded.parameter1 = parameter1;
    }

    void Message::setupData(uint32_t parameter0, uint32_t parameter1,
                            const void* data, uint32_t length)
    {
        payloadType = MessagePayloadType::MptData;

        Message::parameter0         = parameter0;
        Message::data.parameter1    = parameter1;
        Message::data.payloadLength = length;
        Message::data.payload       = const_cast<void*>(data);
    }

    void Message::setupHandle(uint32_t parameter0, uint32_t parameter1,
                              std::vector<Handle>& handles)
    {
        payloadType = MessagePayloadType::MptHandles;

        Message::parameter0          = parameter0;
        Message::handles.parameter1  = parameter1;
        Message::handles.handleCount = (uint32_t)handles.size();
        Message::handles.handles     = new std::vector<Handle>(handles);
    }

    bool Message::test(MessagePayloadType payloadType, size_t expectedLength)
    {
        if (Message::payloadType != payloadType) {
            return false;
        }

        if (expectedLength > 0) {

            switch (Message::payloadType)
            {
                case MessagePayloadType::MptEmbedded: {
                    break;
                }

                case MessagePayloadType::MptData: {
                    if (Message::data.payloadLength != expectedLength) {
                        return false;
                    }

                    break;
                }

                case MessagePayloadType::MptHandles: {
                    if (Message::handles.handleCount != expectedLength) {
                        return false;
                    }

                    break;
                }

                default: {
                    return false;

                    break;
                }
            }

        }

        return true;
    }

    void Message::discard()
    {
        switch (payloadType)
        {
            case MessagePayloadType::MptEmbedded: {
                embedded.parameter1 = 0;
                break;
            }

            case MessagePayloadType::MptData: {
                if ((data.payloadLength > 0) &&
                    (data.payload != nullptr) &&
                    ((localFlags & LocalMessageFlags::LmfAllocationOwner) != 0)) {

                    if ((localFlags & LocalMessageFlags::LmfCustomAllocation) != 0) {
                        assert(allocator != nullptr);
                        allocator(*this, true, allocatorArgs);

                        localFlags ^= LocalMessageFlags::LmfCustomAllocation;
                    } else {
                        free(data.payload);
                    }

                    localFlags ^= LocalMessageFlags::LmfAllocationOwner;
                }

                data.payloadLength = 0;
                data.payload = nullptr;
                break;
            }

            case MessagePayloadType::MptHandles: {
                if ((handles.handleCount > 0) &&
                    (handles.handles!= nullptr) &&
                    ((localFlags & LocalMessageFlags::LmfAllocationOwner) != 0)) {

                    for (auto i = 0; i < handles.handles->size(); ++i) {
                        Handle& handle = handles.handles->at(i);

                        if (handle == INVALID_HANDLE_VALUE) {
                            continue;
                        }

                    #if defined(_WIN32)
                        ::CloseHandle(handle);
                    #else
                        ::close(handle);
                    #endif
                    }

                    delete handles.handles;

                    localFlags ^= LocalMessageFlags::LmfAllocationOwner;
                }

                handles.handleCount = 0;
                handles.handles = nullptr;
                break;
            }

            default: {
                Throw(NotSupportedException);

                break;
            }
        }

        localFlags = 0;
        flags = 0;
    }

}