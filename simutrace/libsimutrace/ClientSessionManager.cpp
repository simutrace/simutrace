/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
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
#include "SimuStor.h"
#include "SimuTrace.h"

#include "ClientSessionManager.h"

#include "ClientSession.h"

namespace SimuTrace
{

    ClientSessionManager::ClientSessionManager() :
        SessionManager(),
        _config(),
        _log(""),
        _environment()
    {
        _environment.log = &_log;
        _environment.config = &_config;

        _initializeEnvironment();
    }

    ClientSessionManager::~ClientSessionManager()
    {

    }

    void ClientSessionManager::_initializeEnvironment()
    {
        static const char* logPattern =
            "<Simutrace> [%d{%Y-%m-%d %H:%M:%S.%~}] %c %s%m%n";

        std::unique_ptr<LogLayout> layout =
            std::unique_ptr<LogLayout>(new PatternLogLayout(logPattern));
        std::shared_ptr<LogAppender> consoleLog =
            std::make_shared<FileLogAppender>("simutraceclient.log", layout, true);

        // In the client we set the Debug level as default level.
        _log.setPriorityThreshold(LogPriority::Debug);

        _log.registerAppender(consoleLog);

        Environment::set(&_environment);

        // Apply default configuration
        int recPoolSize = SIMUTRACE_CLIENT_MEMMGMT_RECOMMENDED_POOLSIZE;
        Configuration::set<int>("client.memmgmt.poolSize", recPoolSize);
    }

    std::unique_ptr<Session> ClientSessionManager::_startSession(
        SessionId localId, std::unique_ptr<Port>& sessionPort,
        uint16_t peerApiVersion)
    {
        Message response = {0};
        ClientPort& port = dynamic_cast<ClientPort&>(*sessionPort);

        // Create the server session
        port.call(&response, RpcApi::CCV31_SessionCreate, RPC_VERSION);

        ThrowOn((response.payloadType != MessagePayloadType::MptEmbedded) ||
                (response.parameter0 == INVALID_SESSION_ID),
                RpcMessageMalformedException);

        // Create the local client session. If this fails, we will close the
        // port. This will also close the session on the server side.
        return std::unique_ptr<Session>(
            new ClientSession(*this, sessionPort, peerApiVersion, localId,
                              response.parameter0, _environment));
    }

    uint16_t ClientSessionManager::_getServerApiVersion(
        const std::string& specifier) const
    {
        Message response = {0};
        ClientPort port(specifier);

        port.call(&response, RpcApi::CCV31_Null, RPC_VERSION);

        ThrowOn(response.payloadType != MessagePayloadType::MptEmbedded,
                RpcMessageMalformedException);

        // Return server API version
        return static_cast<uint16_t>(response.parameter0);
    }

    SessionId ClientSessionManager::createSession(const std::string& specifier)
    {
        SwapEnvironment(&_environment);

        // Creating a new client port will also connect to the storage server.
        // We create two connections. The first one is used to retrieve the
        // server's API version. The second connection is used to create the
        // new session.

        uint16_t serverApiVersion = _getServerApiVersion(specifier);

        // We only connect to servers that have the same or higher version
        ThrowOn((RPC_VER_MAJOR(serverApiVersion) < RPC_VERSION_MAJOR) ||
                ((RPC_VER_MAJOR(serverApiVersion) == RPC_VERSION_MAJOR) &&
                 (RPC_VER_MINOR(serverApiVersion) < RPC_VERSION_MINOR)),
                Exception, stringFormat("Incompatible server version %s.%s "
                "(client %s.%s).",
                RPC_VER_MAJOR(serverApiVersion),
                RPC_VER_MINOR(serverApiVersion),
                RPC_VERSION_MAJOR, RPC_VERSION_MINOR));

        std::unique_ptr<Port> clientPort(new ClientPort(specifier));
        return _createSession(clientPort, serverApiVersion);
    }

    void ClientSessionManager::openLocalSession(SessionId session)
    {
        LockScopeShared(_lock);
        SwapEnvironment(&_environment);

        ClientSession* cs = dynamic_cast<ClientSession*>(_getSession(session));
        ThrowOnNull(cs, NotFoundException);

        uint16_t apiVersion = cs->getPeerApiVersion();
        std::unique_ptr<Port> port(new ClientPort(cs->getAddress()));

        _openLocalSession(session, port, apiVersion);
    }

}
