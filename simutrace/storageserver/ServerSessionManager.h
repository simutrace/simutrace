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
#ifndef _SERVER_SESSION_MANAGER_
#define _SERVER_SESSION_MANAGER_

#include "SimuStor.h"

namespace SimuTrace
{

    class ServerSessionManager :
        public SessionManager
    {
    private:
        DISABLE_COPY(ServerSessionManager);
        
        virtual std::unique_ptr<Session> _startSession(SessionId localId, 
            std::unique_ptr<Port>& sessionPort, uint16_t peerApiVersion) override;

    public:
        ServerSessionManager();
        virtual ~ServerSessionManager() override;

        SessionId createSession(std::unique_ptr<Port>& sessionPort, 
                                uint16_t peerApiVersion);
        void openLocalSession(SessionId session, 
                              std::unique_ptr<Port>& sessionPort, 
                              uint16_t peerApiVersion);
    };

}

#endif
