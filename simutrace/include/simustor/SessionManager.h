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
#pragma once
#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "SimuStor.h"
#include "Session.h"

namespace SimuTrace
{

    class SessionManager
    {
        friend class Session;
    private:
        DISABLE_COPY(SessionManager);

        IdAllocator<SessionId> _sessionIdAllocator;
        std::map<SessionId, Session::Reference> _sessions;

        bool _canCreateSession;

        void _releaseSession(SessionId session); // Called from session
    protected:
        mutable ReaderWriterLock _lock;

        SessionManager();
        virtual std::unique_ptr<Session> _startSession(SessionId localId,
            std::unique_ptr<Port>& sessionPort, uint16_t peerApiVersion) = 0;

        SessionId _createSession(std::unique_ptr<Port>& sessionPort,
                                 uint16_t peerApiVersion);
        void _openLocalSession(SessionId session,
                               std::unique_ptr<Port>& sessionPort,
                               uint16_t peerApiVersion);

        Session* _getSession(SessionId id) const;
    public:
        virtual ~SessionManager();

        void close();

        // Get a list of all session ids currently associated with this manager
        void enumerateSessions(std::vector<SessionId>& out) const;

        // Returns the session with the specified id
        Session& getSession(SessionId id) const;

        uint32_t getOpenSessionsCount() const;
    };

}

#endif