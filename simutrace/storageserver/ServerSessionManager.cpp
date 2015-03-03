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
#include "SimuStor.h"

#include "ServerSessionManager.h"

#include "ServerSession.h"
#include "StorageServer.h"

namespace SimuTrace
{

    ServerSessionManager::ServerSessionManager() :
        SessionManager()
    {

    }

    ServerSessionManager::~ServerSessionManager()
    {

    }

    std::unique_ptr<Session> ServerSessionManager::_startSession(
        SessionId localId, std::unique_ptr<Port>& sessionPort,
        uint16_t peerApiVersion)
    {
        const Environment& env = StorageServer::getInstance().getEnvironment();

        // Create a server session. This will also start the thread that
        // processes client requests for this session/connection pair.
        return std::unique_ptr<ServerSession>(
            new ServerSession(*this, sessionPort, peerApiVersion, localId, env));
    }

    SessionId ServerSessionManager::createSession(
        std::unique_ptr<Port>& sessionPort, uint16_t peerApiVersion)
    {
        return _createSession(sessionPort, peerApiVersion);
    }

    void ServerSessionManager::openLocalSession(SessionId session,
        std::unique_ptr<Port>& sessionPort, uint16_t peerApiVersion)
    {
        return _openLocalSession(session, sessionPort, peerApiVersion);
    }

}
