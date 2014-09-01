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
#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "SimuStor.h"
#include "SimuTraceTypes.h"

namespace SimuTrace 
{

    class ClientSession :
        public Session
    {
    private:
        struct ClientThreadContext;

    private:
        DISABLE_COPY(ClientSession);

        std::string _address;
        SessionId _serverSideId;

        static __thread ClientThreadContext* _context;
        std::vector<ClientThreadContext*> _clients;

        void _detachFromContext(ClientThreadContext* context);

        virtual void _attach(std::unique_ptr<Port>& port) override;
        virtual bool _detach(bool whatif) override;

        virtual Store::Reference _createStore(const std::string& specifier, 
                                              bool alwaysCreate) override;

        virtual void _close() override;

        virtual void _enumerateStores(std::vector<std::string>& out) const override;

        virtual void _applySetting(const std::string& setting) override;

        void _determineStreamBufferConfiguration(uint32_t& numSegments);
    public:
        ClientSession(SessionManager& manager, std::unique_ptr<Port>& port, 
                      uint16_t serverApiVersion, SessionId localId, 
                      SessionId serverSideId, const Environment& root);
        virtual ~ClientSession() override;

        StreamId registerStream(StreamDescriptor& desc);

        const std::string& getAddress() const;
        SessionId getServerSideId() const;
        ClientPort& getPort() const;
    };

}

#endif