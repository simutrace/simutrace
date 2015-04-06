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
#ifndef SOCKET_CHANNEL_H
#define SOCKET_CHANNEL_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Channel.h"

namespace SimuTrace
{

#if defined(_WIN32)
#else
    typedef Handle SOCKET;

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET INVALID_HANDLE_VALUE
#endif

// Enable/disable ipv6 support
#define SIMUTRACE_SOCKET_ENABLE_IPV6

    class SocketChannel :
        public Channel
    {
    private:
        DISABLE_COPY(SocketChannel);

        SOCKET _endpoint;

        struct addrinfo* _info;
        std::string _ipaddress;
        std::string _port;

        SocketChannel(SOCKET endpoint, const std::string& address);

        void _initChannel(bool isServer, const std::string& address,
                          SOCKET endpoint);
        void _getAddressInfo(struct addrinfo** info, const char* host,
                             const char* port);
        void _updateSocketAddress(const std::string& address);

        void _createServerSocket();
        void _bind();
        void _listen();

        void _createClientSocket();
        void _connect();

        void _createServerEndpoint();
        void _connectServerEndpoint();

        static void _closeSocket(SOCKET socket);

    private:
        virtual std::unique_ptr<Channel> _accept() override;

        virtual void _disconnect() override;

        virtual size_t _send(const void* data, size_t size) override;
        virtual size_t _send(const std::vector<Handle>& handles) override;
        virtual size_t _receive(void* data, size_t size) override;
        virtual size_t _receive(std::vector<Handle>& handles,
                                uint32_t handleCount) override;

    public:
        static std::unique_ptr<Channel> factoryMethod(bool isServer,
            const std::string& address);

    public:
        SocketChannel(bool isServer, const std::string& address);
        virtual ~SocketChannel();

        virtual bool isConnected() const override;
        virtual ChannelCapabilities getChannelCaps() const override;

        const std::string& getIpAddress() const;
        const std::string& getPort() const;
    };

}

#endif