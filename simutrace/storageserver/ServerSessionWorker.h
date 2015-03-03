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
#ifndef SERVER_SESSION_WORKER_H
#define SERVER_SESSION_WORKER_H

#include "SimuStor.h"
#include "ServerStream.h"

namespace SimuTrace
{

    class ServerSession;

    class ServerSessionWorker :
        public ThreadBase
    {
    private:
        DISABLE_COPY(ServerSessionWorker);

        struct MessageContext;
        typedef bool (*MessageHandler)(MessageContext& ctx);

        ServerSession& _session;
        std::unique_ptr<ServerPort> _port;

        StreamWait _wait;

        std::map<int, MessageHandler> _handlers;

        void _initializeHandlerMap();
        void _acknowledgeSessionCreate();

    private:
        static bool _handleSessionClose(MessageContext& ctx);
        static bool _handleSessionSetConfiguration(MessageContext& ctx);

        static bool _handleStoreCreate(MessageContext& ctx);
        static bool _handleStoreClose(MessageContext& ctx);

        static bool _handleStreamBufferRegister(MessageContext& ctx);
        static bool _handleStreamBufferEnumerate(MessageContext& ctx);
        static bool _handleStreamBufferQuery(MessageContext& ctx);

        static bool _handleStreamRegister(MessageContext& ctx);
        static bool _handleStreamEnumerate(MessageContext& ctx);
        static bool _handleStreamQuery(MessageContext& ctx);
        static bool _handleStreamAppend(MessageContext& ctx);
        static bool _handleStreamCloseAndOpen(MessageContext& ctx);
        static bool _handleStreamClose(MessageContext& ctx);

        static void _messagePayloadAllocator(Message& msg, bool free,
                                             void* args);
    private:
        virtual int _run() override;
        virtual void _onFinalize() override;

    public:
        ServerSessionWorker(ServerSession& session, std::unique_ptr<Port>& port);
        virtual ~ServerSessionWorker() override;

        void close();

        ServerSession& getSession() const;
        bool channelSupportsSharedMemory() const;
    };

}

#endif