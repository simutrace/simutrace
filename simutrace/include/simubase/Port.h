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
#ifndef PORT_H
#define PORT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "RpcInterface.h"

namespace SimuTrace
{

    class Channel;

    class Port
    {
    private:
        DISABLE_COPY(Port);

        std::unique_ptr<Channel> _channel;
        const std::string _specifier;

        uint64_t _messagesSent;
        uint64_t _messagesReceived;

        uint8_t _lastSequenceNumber;

    protected:
        Port(bool isServer, const std::string& specifier);
        Port(std::unique_ptr<Channel>& channel);

        void _send(const Message& msg);
        void _receive(Message& msg);

        Channel& _getChannel() const;
    public:
        virtual ~Port();

        bool isConnected() const;
        void close();

        const std::string& getAddress() const;
        const std::string& getSpecifier() const;

        uint64_t getMessagesSent() const;
        uint64_t getMessagesReceived() const;
        uint64_t getBytesSent() const;
        uint64_t getBytesReceived() const;

        uint8_t getLastSequenceNumber() const;

        ChannelCapabilities getChannelCaps() const;
    };

}

#endif