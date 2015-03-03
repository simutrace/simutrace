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
#ifndef CHANNEL_H
#define CHANNEL_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    class Channel
    {
    private:
        DISABLE_COPY(Channel);

        std::string _address;
        bool _isServer;

        uint64_t _bytesSent;
        uint64_t _bytesReceived;
    protected:
        Channel(bool isServer, const std::string& address);

        virtual std::unique_ptr<Channel> _accept() = 0;

        virtual void _disconnect() = 0;

        virtual size_t _send(const void* data, size_t size) = 0;
        virtual size_t _send(const std::vector<Handle>& handles) = 0;
        virtual size_t _receive(void* data, size_t size) = 0;
        virtual size_t _receive(std::vector<Handle>& handles,
                                uint32_t handleCount) = 0;

        bool _isServerChannel() const;

    public:
        virtual ~Channel();

        std::unique_ptr<Channel> accept();

        void disconnect();

        size_t send(const void* data, size_t size);
        size_t send(const std::vector<Handle>& handles);
        size_t receive(void* data, size_t size);
        size_t receive(std::vector<Handle>& handles, uint32_t handleCount);

        virtual bool isConnected() const = 0;
        virtual ChannelCapabilities getChannelCaps() const = 0;

        const std::string& getAddress() const;

        uint64_t getBytesSent() const;
        uint64_t getBytesReceived() const;
    };

}

#endif