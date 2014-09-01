/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef SERVER_PORT_H
#define SERVER_PORT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Port.h"

namespace SimuTrace
{

    class ServerPort :
        public Port
    {
    private:
        DISABLE_COPY(ServerPort);

        bool _isConnectionPort;

        ServerPort(std::unique_ptr<Channel>& channel);

    public:
        ServerPort(const std::string& specifier);
        virtual ~ServerPort();

        void ret(Message& msg);
        void ret(Message& request, uint32_t status, uint32_t result0 = 0, 
                 uint64_t result1 = 0);
        void ret(Message& request, uint32_t status, const void* data, 
                 uint32_t length, uint32_t result0 = 0, uint32_t result1 = 0);
        void ret(Message& request, uint32_t status, std::vector<Handle>& handles, 
                 uint32_t result0 = 0, uint32_t result1 = 0);

        void wait(Message& request);

        std::unique_ptr<Port> accept();
    };

}

#endif