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
#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "SimuStor.h"

namespace SimuTrace 
{

    class SessionManager;
    class ServerSessionWorker;

    class ServerSession :
        public Session
    {
    private:
        DISABLE_COPY(ServerSession);

        std::vector<std::unique_ptr<ServerSessionWorker>> _workers;

        LogCategory _workerLog;
        Environment _workerEnvironment;
    private:
        virtual void _attach(std::unique_ptr<Port>& port) override;
        virtual bool _detach(bool whatif) override;

        virtual Store::Reference _createStore(const std::string& specifier,
                                              bool alwaysCreate) override;

        virtual void _close() override;

        virtual void _enumerateStores(std::vector<std::string>& out) const override;
    public:
        ServerSession(SessionManager& manager, std::unique_ptr<Port>& port, 
                      uint16_t clientApiVersion, SessionId localId,
                      const Environment& root);
        virtual ~ServerSession() override;

        void enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const;
        void enumerateStreams(std::vector<Stream*>& out, 
                              bool includeHidden) const;   
        void enumerateDataPools(std::vector<DataPool*>& out) const;

        const Environment& getWorkerEnvironment() const;
    };

}

#endif