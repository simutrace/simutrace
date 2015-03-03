/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "SessionManager.h"

#include "Session.h"

namespace SimuTrace
{

    SessionManager::SessionManager() :
        _canCreateSession(true)
    {

    }

    SessionManager::~SessionManager()
    {
        assert(_sessions.empty());
    }

    void SessionManager::_releaseSession(SessionId id)
    {
        LockScopeExclusive(_lock);

        // Find the session with the specified id and remove it from
        // the session list. This will call its destructor.
        auto it = _sessions.find(id);
        assert(it != _sessions.end());

        _sessions.erase(it);
        _sessionIdAllocator.retireId(id);
    }

    SessionId SessionManager::_createSession(std::unique_ptr<Port>& sessionPort,
                                             uint16_t peerApiVersion)
    {
        LockScopeExclusive(_lock);
        ThrowOn(!_canCreateSession, InvalidOperationException);

        SessionId id = _sessionIdAllocator.getNextId();
        assert(id != INVALID_SESSION_ID);
        assert(_getSession(id) == nullptr);

        std::string address = sessionPort->getAddress();

        // Create the session based on the specified port
        auto session = _startSession(id, sessionPort, peerApiVersion);
        assert(session != nullptr);

        _sessions.insert(std::pair<SessionId, Session::Reference>(id,
            Session::makeOwnerReference(session.release())));

        LogInfo("Created session for peer '%s' (RPCv%d.%d) <id: %d>.",
                address.c_str(),
                RPC_VER_MAJOR(peerApiVersion),
                RPC_VER_MINOR(peerApiVersion),
                id);

        return id;
    }

    void SessionManager::_openLocalSession(SessionId session,
                                           std::unique_ptr<Port>& sessionPort,
                                           uint16_t peerApiVersion)
    {
        Session* s = _getSession(session);
        ThrowOnNull(s, NotFoundException);

        assert(_canCreateSession);

        ThrowOn(s->getPeerApiVersion() != peerApiVersion, Exception,
                "Incompatible API version.");

        std::string address = sessionPort->getAddress();

        s->attach(sessionPort);

        LogInfo("<client: %s> Attached client to session %d.",
                address.c_str(), session);
    }

    Session* SessionManager::_getSession(SessionId id) const
    {
        auto it = _sessions.find(id);
        if (it == _sessions.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    void SessionManager::close()
    {
        std::vector<Session*> sessions;

        LockExclusive(_lock); {
            _canCreateSession = false;

            for (auto& pair : _sessions) {
                sessions.push_back(pair.second.get());
            }
        } Unlock();

        // At this point, we can be sure that no one is currently creating a
        // session and that no new sessions will be created in the future.
        for (auto session : sessions) {
            session->close();
        }
    }

    void SessionManager::enumerateSessions(std::vector<SessionId>& out) const
    {
        LockScopeShared(_lock);

        out.clear();
        out.reserve(_sessions.size());

        for (auto& pair : _sessions) {
            out.push_back(pair.second->getLocalId());
        }
    }

    Session& SessionManager::getSession(SessionId id) const
    {
        LockScopeShared(_lock);

        Session* session = _getSession(id);
        ThrowOnNull(session, NotFoundException);

        return *session;
    }

    uint32_t SessionManager::getOpenSessionsCount() const
    {
        LockScopeShared(_lock);
        return static_cast<uint32_t>(_sessions.size());
    }

}