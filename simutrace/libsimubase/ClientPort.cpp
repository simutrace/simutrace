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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "ClientPort.h"

#include "Exceptions.h"

#include "Channel.h"
#include "RpcInterface.h"

namespace SimuTrace
{

    ClientPort::ClientPort(const std::string& specifier) :
        Port(false, specifier)
    {

    }

    ClientPort::~ClientPort()
    {

    }

    uint32_t ClientPort::call(Message* response, Message& msg)
    {
        Message resp = {0};
        if (response == nullptr) {
            response = &resp;
        }

        msg.sequenceNumber = _nextSequenceNumber++;

        _send(msg);
        _receive(*response);

        // Check if this is the response to our request 
        ThrowOn(response->sequenceNumber != msg.sequenceNumber, Exception,
                "Failed to get response from server. Potential "
                "message order violation detected.");

        // Check if the peer could process the request. If not, we forward
        // the error text at this point through an exception.
        if (response->response.status == RpcApi::SC_Failed) {
            ThrowOn(response->payloadType != MessagePayloadType::MptData,
                    RpcMessageMalformedException);

            char* buf = static_cast<char*>(response->data.payload);
            Throw(PeerException, std::string(buf, response->data.payloadLength),
                  static_cast<ExceptionClass>(response->parameter0),
                  response->data.parameter1);
        }

        return response->response.status;
    }

    uint32_t ClientPort::call(Message* response, uint32_t controlCode, 
                              uint32_t parameter0, uint64_t parameter1)
    {
        Message msg = {0};

        msg.request.controlCode = controlCode;
        msg.setupEmbedded(parameter0, parameter1);

        return call(response, msg);
    }

    uint32_t ClientPort::call(Message* response, uint32_t controlCode, 
                              const void* data, uint32_t length, 
                              uint32_t parameter0, uint32_t parameter1)
    {
        Message msg = {0};

        msg.request.controlCode = controlCode;
        msg.setupData(parameter0, parameter1, data, length);

        return call(response, msg);
    }

}