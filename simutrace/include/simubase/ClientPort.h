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
#ifndef CLIENT_PORT_H
#define CLIENT_PORT_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Port.h"
#include "CriticalSection.h"

namespace SimuTrace
{

    class ClientPort :
        public Port
    {
    private:
        DISABLE_COPY(ClientPort);

        uint8_t _nextSequenceNumber;

    public:
        ClientPort(const std::string& specifier);
        virtual ~ClientPort();

        uint32_t call(Message* response, Message& msg);
        uint32_t call(Message* response, uint32_t controlCode,
                      uint32_t parameter0 = 0, uint64_t parameter1 = 0);
        uint32_t call(Message* response, uint32_t controlCode,
                      const void* data, uint32_t length,
                      uint32_t parameter0 = 0, uint32_t parameter1 = 0);

        uint32_t call(Message* response, uint32_t controlCode,
                      const std::string& data, uint32_t parameter0 = 0,
                      uint32_t parameter1 = 0)
        {
            return call(response, controlCode, data.c_str(),
                        static_cast<uint32_t>(data.length()),
                        parameter0, parameter1);
        }

    };

}

#endif