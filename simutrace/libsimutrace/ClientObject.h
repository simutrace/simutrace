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
#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include "SimuStor.h"
#include "SimuTraceTypes.h"

#include "ClientSession.h"

namespace SimuTrace
{

    class ClientObject
    {
    private:
        ClientSession& _session;

    protected:

        ClientPort& _getPort() const { return _session.getPort(); }
    public:
        ClientObject(ClientSession& session) :
            _session(session) { }
        ~ClientObject() { }

        ClientSession& getSession() const { return _session; }
    };

}

#endif