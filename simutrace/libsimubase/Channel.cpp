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

#include "Channel.h"

#include "Exceptions.h"

#define MESSAGE_SIZE sizeof(Message) - sizeof(void*)

namespace SimuTrace
{

    Channel::Channel(bool isServer, const std::string& address) :
        _address(address),
        _isServer(isServer),
        _bytesSent(0),
        _bytesReceived(0)
    {

    }

    Channel::~Channel()
    {

    }

    bool Channel::_isServerChannel() const
    {
        return _isServer;
    }

    std::unique_ptr<Channel> Channel::accept()
    {
        ThrowOn(!_isServer || !isConnected(), InvalidOperationException);

        return _accept();
    }

    void Channel::disconnect()
    {
        if (!isConnected()) {
            return;
        }

        _disconnect();
    }

    size_t Channel::send(const void* data, size_t size)
    {
        ThrowOn(!isConnected(), InvalidOperationException);
        ThrowOn(data == nullptr, ArgumentNullException, "data");
        ThrowOn(size == 0, ArgumentNullException, "size");

        size_t bytesSent = _send(data, size);

        _bytesSent += bytesSent;
        return bytesSent;
    }

    size_t Channel::send(const std::vector<Handle>& handles)
    {
        ThrowOn(!isConnected(), InvalidOperationException);

        size_t bytesSent = _send(handles);

        _bytesSent += bytesSent;
        return bytesSent;
    }

    size_t Channel::receive(void* data, size_t size)
    {
        ThrowOn(!isConnected(), InvalidOperationException);
        ThrowOn(data == nullptr, ArgumentNullException, "data");
        ThrowOn(size == 0, ArgumentNullException, "size");

        size_t bytesReceived = _receive(data, size);

        _bytesReceived += bytesReceived;
        return bytesReceived;
    }

    size_t Channel::receive(std::vector<Handle>& handles,
        uint32_t handleCount)
    {
        ThrowOn(!isConnected(), InvalidOperationException);

        size_t bytesReceived =_receive(handles, handleCount);

        _bytesReceived += bytesReceived;
        return bytesReceived;
    }

    const std::string& Channel::getAddress() const
    {
        return _address;
    }

    uint64_t Channel::getBytesSent() const
    {
        return _bytesSent;
    }

    uint64_t Channel::getBytesReceived() const
    {
        return _bytesReceived;
    }

}