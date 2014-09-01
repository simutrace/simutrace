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

#include "Port.h"

#include "Exceptions.h"
#include "Utils.h"

#include "Channel.h"
#include "ChannelProvider.h"
#include "RpcInterface.h"

#include "Logging.h"

namespace SimuTrace
{

    Port::Port(bool isServer, const std::string& specifier) :
        _channel(nullptr),
        _specifier(specifier),
        _messagesReceived(0),
        _messagesSent(0)
    {
        _channel = ChannelProvider::createChannelFromSpecifier(isServer, 
                                                               specifier);
    }

    Port::Port(std::unique_ptr<Channel>& channel) :
        _channel(nullptr),
        _messagesReceived(0),
        _messagesSent(0)
    {
        assert(channel != nullptr);
        _channel = std::move(channel);
    }

    Port::~Port()
    {

    }

    void Port::_send(const Message& msg)
    {
        LogRpc("Sending message to %s: <seq: %d, code: 0x%x, type: %d (%s),"
               " flags: %s, lflags: %s, param0: 0x%x, param1: 0x%x,"
               " %s: 0x%x>",
               this->getAddress().c_str(),
               msg.sequenceNumber,
               msg.request.controlCode,
               msg.payloadType,
               payloadTypeToStr(msg.payloadType).c_str(),
               messageFlagsToStr(msg.flags).c_str(),
               localMessageFlagsToStr(msg.localFlags).c_str(),
               msg.parameter0,
               msg.data.parameter1,
               (msg.payloadType == MessagePayloadType::MptData) ? 
                    "len" : "param2",
               msg.data.payloadLength);

        _lastSequenceNumber = msg.sequenceNumber;
        _channel->send(&msg, MESSAGE_SIZE);

        switch (msg.payloadType) 
        {
            case MessagePayloadType::MptEmbedded: {
                break;
            }

            case MessagePayloadType::MptData: {
                if (msg.data.payloadLength == 0) {
                    break;
                }

                assert(msg.data.payload != nullptr);
                size_t bytesSend = _channel->send(msg.data.payload, 
                                                  msg.data.payloadLength);

                ThrowOn(bytesSend != msg.data.payloadLength, 
                        RpcMessageMalformedException);

                break;
            }

            case MessagePayloadType::MptHandles: {
                if (msg.handles.handleCount == 0) {
                    break;
                }

                assert(msg.handles.handles != nullptr);
                _channel->send(*msg.handles.handles);

                break;
            }

            default: {
                _DEBUG_BREAK_

                break;
            }
        }

        _messagesSent++;
    }

    void Port::_receive(Message& msg)
    {
        _channel->receive(&msg, MESSAGE_SIZE);
        _lastSequenceNumber = msg.sequenceNumber;

        try {

            switch (msg.payloadType)
            {
                case MessagePayloadType::MptEmbedded: {
                    break;
                }

                case MessagePayloadType::MptData: {
                    msg.data.payload = nullptr;

                    if (msg.data.payloadLength == 0) {
                        break;
                    }

                    msg.localFlags |= LocalMessageFlags::LmfAllocationOwner;

                    // Use custom memory allocator if possible. However, do not
                    // use it if this is a response and the status indicates a
                    // failure. In that case the payload may be the error text.
                    if ((msg.allocator != nullptr) && 
                        (((msg.flags & MessageFlags::MfResponse) == 0) || 
                         (msg.response.status != RpcApi::SC_Failed))) {

                        msg.allocator(msg, false, msg.allocatorArgs);
                    }

                    if (msg.data.payload == nullptr) {
                        msg.data.payload = malloc(msg.data.payloadLength);

                        if (msg.data.payload == nullptr) {
                            System::ThrowOutOfMemoryException();
                        }
                    } else {
                        msg.localFlags |= LocalMessageFlags::LmfCustomAllocation;
                    }

                    size_t bytesRead = _channel->receive(msg.data.payload, 
                                                         msg.data.payloadLength);

                    ThrowOn(bytesRead != msg.data.payloadLength, 
                            RpcMessageMalformedException);

                    break;
                }

                case MessagePayloadType::MptHandles: {
                    msg.handles.handles = nullptr;

                    if (msg.handles.handleCount == 0) {
                        break;
                    }

                    msg.localFlags |= LocalMessageFlags::LmfAllocationOwner;

                    msg.handles.handles = new std::vector<Handle>();

                    _channel->receive(*msg.handles.handles, 
                                      msg.handles.handleCount);

                    break;
                }

                default: {
                    Throw(NotSupportedException);

                    break;
                }
            }

        } catch (...) {
            msg.discard();

            throw;
        }

        LogRpc("Received message from %s: <seq: %d, code: 0x%x, type: %d (%s),"
               " flags: %s, lflags: %s, param0: 0x%x, param1: 0x%x,"
               " %s: 0x%x>",
               this->getAddress().c_str(),
               msg.sequenceNumber,
               msg.request.controlCode,
               msg.payloadType,
               payloadTypeToStr(msg.payloadType).c_str(),
               messageFlagsToStr(msg.flags).c_str(),
               localMessageFlagsToStr(msg.localFlags).c_str(),
               msg.parameter0,
               msg.data.parameter1,
               (msg.payloadType == MessagePayloadType::MptData) ? 
                    "len" : "param2",
               msg.data.payloadLength);

        _messagesReceived++;
    }

    Channel& Port::_getChannel() const
    {
        assert(_channel != nullptr);
        return *_channel;
    }

    bool Port::isConnected() const
    {
        return _channel->isConnected();
    }

    void Port::close()
    {
        _channel->disconnect();
    }

    const std::string& Port::getAddress() const
    {
        return _channel->getAddress();
    }

    const std::string& Port::getSpecifier() const
    {
        return _specifier;
    }

    uint64_t Port::getMessagesSent() const
    {
        return _messagesSent;
    }

    uint64_t Port::getMessagesReceived() const
    {
        return _messagesReceived;
    }

    uint64_t Port::getBytesSent() const
    {
        return _channel->getBytesSent();
    }

    uint64_t Port::getBytesReceived() const
    {
        return _channel->getBytesReceived();
    }

    uint8_t Port::getLastSequenceNumber() const
    {
        return _lastSequenceNumber;
    }

    ChannelCapabilities Port::getChannelCaps() const
    {
        return _channel->getChannelCaps();
    }

}