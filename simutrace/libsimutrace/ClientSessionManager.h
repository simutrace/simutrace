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
#pragma once
#ifndef CLIENT_SESSION_MANAGER_H
#define CLIENT_SESSION_MANAGER_H

#include "SimuStor.h"

namespace SimuTrace
{

    class ClientSessionManager :
        public SessionManager
    {
    private:
        DISABLE_COPY(ClientSessionManager);

        libconfig::Config _config;
        LogCategory _log;
        Environment _environment;

        void _initializeEnvironment();

        virtual std::unique_ptr<Session> _startSession(SessionId localId,
            std::unique_ptr<Port>& sessionPort, uint16_t peerApiVersion) override;

        uint16_t _getServerApiVersion(const std::string& specifier) const;

    public:
        ClientSessionManager();
        virtual ~ClientSessionManager() override;

        SessionId createSession(const std::string& specifier); 
        void openLocalSession(SessionId session);
    };

}

#endif
