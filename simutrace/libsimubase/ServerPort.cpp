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

#include "ServerPort.h"

#include "Exceptions.h"
#include "Channel.h"
#include "RpcInterface.h"

#include "Logging.h"

namespace SimuTrace
{

    ServerPort::ServerPort(const std::string& specifier) :
        Port(true, specifier),
        _isConnectionPort(false)
    {

    }

    ServerPort::ServerPort(std::unique_ptr<Channel>& channel) :
        Port(channel),
        _isConnectionPort(true)
    {

    }

    ServerPort::~ServerPort()
    {

    }

    void ServerPort::ret(Message& msg)
    {
        ThrowOn(msg.payloadType >= MessagePayloadType::MptMax, 
                NotSupportedException);
        msg.flags |= MessageFlags::MfResponse;

        _send(msg);
    }

    void ServerPort::ret(Message& request, uint32_t status, uint32_t result0, 
                         uint64_t result1)
    {
        request.discard();

        request.response.status = status;
        request.setupEmbedded(result0, result1);

        ret(request);
    }

    void ServerPort::ret(Message& request, uint32_t status, const void* data, 
                         uint32_t length, uint32_t result0, uint32_t result1)
    {
        request.discard();

        request.response.status = status;
        request.setupData(result0, result1, data, length);        

        ret(request);
    }

    void ServerPort::ret(Message& request, uint32_t status,  
                         std::vector<Handle>& handles, uint32_t result0, 
                         uint32_t result1)
    {
        request.discard();

        request.response.status = status;
        request.setupHandle(result0, result1, handles);

        ret(request);
    }

    void ServerPort::wait(Message& request) 
    {
        request.discard();

        _receive(request);
    }

    std::unique_ptr<Port> ServerPort::accept()
    {
        ThrowOn(_isConnectionPort, InvalidOperationException);

        // Accept new connections on the channel. For each new connection
        // return a connection-specific server port.

        std::unique_ptr<Channel> connection = _getChannel().accept();
        LogDebug("Established new connection with client %s (server: %s).",
                 connection->getAddress().c_str(),
                 this->getAddress().c_str());

        return std::unique_ptr<Port>(new ServerPort(connection));
    }

}