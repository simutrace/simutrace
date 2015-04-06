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
#ifndef LOCAL_CHANNEL_H
#define LOCAL_CHANNEL_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Channel.h"

#include "SafeHandle.h"

namespace SimuTrace
{

    class LocalChannel :
        public Channel
    {
    private:
        DISABLE_COPY(LocalChannel);

        SafeHandle _endpoint;

        LocalChannel(SafeHandle& endpoint, const std::string& address);

    #if defined(_WIN32)
        void _createServerNamedPipe();
        void _connectServerNamedPipe();

        void _duplicateHandles(const std::vector<Handle>& local,
                               std::vector<Handle>& remote);
    #else
        void _createDomainSocket(SafeHandle& handle,
                                 struct sockaddr_un& sockaddr);
        void _createServerDomainSocket();
        void _connectServerDomainSocket();

        static struct msghdr* _allocateHandleTransferMessage(uint32_t handleCount);
        static void _freeHandleTransferMessage(struct msghdr* msg);

        uint64_t _sendHandleTransferMessage(struct msghdr* msg);
        uint64_t _receiveHandleTransferMessage(struct msghdr* msg,
                                               uint32_t handleCount);
    #endif

        void _createServerEndpoint();
        void _connectServerEndpoint();

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
        LocalChannel(bool isServer, const std::string& address);
        virtual ~LocalChannel();

        virtual bool isConnected() const override;
        virtual ChannelCapabilities getChannelCaps() const override;
    };

}

#endif