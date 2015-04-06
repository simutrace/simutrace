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

#include "SocketChannel.h"

#include "Exceptions.h"
#include "Interlocked.h"
#include "Utils.h"
#include "Logging.h"

namespace SimuTrace {

#if defined(_WIN32)
namespace System
{

    static volatile uint32_t _winsockReferences = 0;

    static void _initializeWinsock()
    {
        WSADATA _wsaData;

        if (Interlocked::interlockedAdd(&_winsockReferences, 1) == 0) {
            if (::WSAStartup(MAKEWORD(2,2), &_wsaData) != 0) {
                Throw(PlatformException);
            }
        }
    }

    static void _cleanupWinsock()
    {
        assert(_winsockReferences > 0);

        if (Interlocked::interlockedSub(&_winsockReferences, 1) == 1) {
            WSACleanup();
        }
    }

}
#else
#endif

    SocketChannel::SocketChannel(bool isServer, const std::string& address) :
        Channel(isServer, address),
        _endpoint(INVALID_SOCKET),
        _info(nullptr)
    {
        _initChannel(isServer, address, INVALID_SOCKET);
    }

    SocketChannel::SocketChannel(SOCKET endpoint, const std::string& address) :
        Channel(false, address),
        _endpoint(INVALID_SOCKET),
        _info(nullptr)
    {
        _initChannel(false, address, endpoint);
    }

    SocketChannel::~SocketChannel()
    {
        try {
            disconnect();
            assert(!isConnected());
        } catch (const std::exception& e) {
            LogWarn("Could not disconnect connection to '%s'. "
                    "The exception is '%s'.", this->getAddress().c_str(),
                    e.what());
        }

        if (_info != nullptr) {
            ::freeaddrinfo(_info);
            _info = nullptr;
        }

    #if defined(_WIN32)
        System::_cleanupWinsock();
    #else
    #endif
    }

    void SocketChannel::_initChannel(bool isServer, const std::string& address,
                                     SOCKET endpoint)
    {
    #if defined(_WIN32)
        System::_initializeWinsock();
    #else
    #endif

        try {
            _updateSocketAddress(address);

            if (endpoint == INVALID_SOCKET) {

                if (isServer) {
                    _createServerEndpoint();
                } else {
                    _connectServerEndpoint();
                }

            } else {
                _endpoint = endpoint;
            }

        } catch (...) {
            if (_info != nullptr) {
                ::freeaddrinfo(_info);
                _info = nullptr;
            }

        #if defined(_WIN32)
            System::_cleanupWinsock();
        #else
        #endif

            throw;
        }

    }

    void SocketChannel::_getAddressInfo(struct addrinfo** info,
                                        const char* host, const char* port)
    {
        struct addrinfo hints = {0};
        hints.ai_flags = (_isServerChannel()) ? AI_PASSIVE : 0;
    #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6
        hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    #else
        hints.ai_family = AF_INET;       // Allow IPv4 only
    #endif
        hints.ai_socktype = SOCK_STREAM; // Reliable socket
        hints.ai_protocol = IPPROTO_TCP;

        int result = ::getaddrinfo(host, port, &hints, info);
        if (result != 0) {
        #if defined(_WIN32)
        #else
            ThrowOn(result == EAI_SYSTEM, PlatformException);
        #endif
            Throw(SocketException, result);
        }
    }

    void SocketChannel::_updateSocketAddress(const std::string& address)
    {
        std::string host;
        std::string port;

        assert(!isConnected());

        if (_info != nullptr) {
            ::freeaddrinfo(_info);
            _info = nullptr;
        }

        // We expect addresses in the form: host:port or [host]:port
        // Wild-carding the host or port is allowed for a server channel port.
        // If the port is set to "*", the system chooses a port from the
        // dynamic port range. If the host is set to "*", the server will listen
        // on all interfaces (ipv6 and ipv4).

        size_t pos = address.find_last_of(':');
        if (pos == std::string::npos) {
            port = "*"; // Let the system choose a dynamic server port
            host = address;
        } else {
            port = address.substr(pos + 1, std::string::npos);
            host = address.substr(0, pos);

            // Cut off the [] braces if we have an host address in the form
            // [host]:port, e.g. ipv6.
        #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6
            std::string::size_type len = host.length();
            if ((len > 0) && (host[0] == '[') && (host[len - 1] == ']')) {
                host = host.substr(1, len - 2);
            }
        #else
        #endif
        }

        ThrowOn(host.empty() || port.empty(), Exception,
                "Missing host or port in address. Expected format "
                "is: host:port");

        bool anyHost = (host.compare("*") == 0);
        bool anyPort = (port.compare("*") == 0);

        ThrowOn(!_isServerChannel() && (anyHost || anyPort), Exception,
                "Using wildcard addressing is not allowed for client "
                "connections. Expected format is: host:port.");

        // Resolve the IP address and port number
        struct addrinfo* info;
        struct sockaddr_in* addrv4  = nullptr;
        struct sockaddr_in6* addrv6 = nullptr;
        char str[INET6_ADDRSTRLEN];
        uint32_t portNumber;

        _getAddressInfo(&info, (anyHost) ? nullptr : host.c_str(),
                        (anyPort) ? "0" : port.c_str());

        switch (info->ai_family)
        {
            case AF_INET6: {
                addrv6 = reinterpret_cast<struct sockaddr_in6*>(info->ai_addr);

                ::inet_ntop(AF_INET6, &(addrv6->sin6_addr), str, sizeof(str));
                portNumber = ntohs(addrv6->sin6_port);

                break;
            }

            case AF_INET: {
                addrv4 = reinterpret_cast<struct sockaddr_in*>(info->ai_addr);

                ::inet_ntop(AF_INET, &(addrv4->sin_addr), str, sizeof(str));
                portNumber = ntohs(addrv4->sin_port);

                break;
            }

            default: {
                ::freeaddrinfo(info);
                Throw(NotSupportedException);

                break;
            }
        }

        _info = info;
        _ipaddress = std::string(str);
        _port = std::string(std::to_string(portNumber));
    }

    void SocketChannel::_createServerSocket()
    {
        SOCKET endpoint;
        uint32_t ipv6only = 0;

        assert(_isServerChannel() && !isConnected() && _info != nullptr);

        switch (_info->ai_family)
        {
            case AF_INET6: {
            #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6

                // Try to create an ipv4 compatible ipv6 socket.

                endpoint = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                if (endpoint != INVALID_SOCKET) {

                    int result = ::setsockopt(endpoint,
                                              IPPROTO_IPV6, IPV6_V6ONLY,
                                              (const char*)&ipv6only,
                                              sizeof(ipv6only));

                    if (result != SOCKET_ERROR) {
                        break;
                    }

                    // We failed to bring the socket into ipv4 compatible mode.
                    // The current solution is to use the pure ipv6 socket.

                    // Nothing to do!

                } else {
                    Throw(SocketException);
                }

            #else
                Throw(NotSupportedException);
            #endif
                break;
            }

            case AF_INET: {
                endpoint = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                ThrowOn(endpoint == INVALID_SOCKET, SocketException);

                break;
            }

            default: {
                Throw(NotSupportedException);

                break;
            }
        }

        _endpoint = endpoint;
    }

    void SocketChannel::_bind()
    {
        assert(_isServerChannel() && isConnected() && _info != nullptr);

        int result = ::bind(_endpoint, _info->ai_addr, (int)_info->ai_addrlen);
        ThrowOn(result == SOCKET_ERROR, SocketException);
    }

    void SocketChannel::_listen()
    {
        assert(_isServerChannel() && isConnected() && _info != nullptr);

        int result = ::listen(_endpoint, SOMAXCONN);
        ThrowOn(result == SOCKET_ERROR, SocketException);

        // Get the real port number if the system has provided us one

        if (_port.compare("0") == 0) {
            uint32_t portNumber;

            switch (_info->ai_family)
            {
                case AF_INET6: {
                #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6
                    struct sockaddr_in6 sin6;
                    socklen_t len6 = sizeof(sin6);

                    result = ::getsockname(_endpoint, (struct sockaddr*)&sin6,
                                           &len6);
                    ThrowOn(result == SOCKET_ERROR, SocketException);

                    portNumber = ntohs(sin6.sin6_port);
                #else
                    Throw(NotSupportedException);
                #endif
                    break;
                }

                case AF_INET: {
                    struct sockaddr_in sin;
                    socklen_t len = sizeof(sin);

                    result = ::getsockname(_endpoint, (struct sockaddr*)&sin,
                                           &len);
                    ThrowOn(result == SOCKET_ERROR, SocketException);

                    portNumber = ntohs(sin.sin_port);
                    break;
                }

                default: {
                    Throw(NotSupportedException);

                    break;
                }
            }

            _port = std::to_string(portNumber);
        }
    }

    void SocketChannel::_createClientSocket()
    {
        SOCKET endpoint;

        assert(!_isServerChannel() && !isConnected() && _info != nullptr);

        switch (_info->ai_family)
        {
            case AF_INET6: {
            #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6
                endpoint = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                ThrowOn(endpoint == INVALID_SOCKET, SocketException);

            #else
                Throw(NotSupportedException);
            #endif
                break;
            }

            case AF_INET: {
                endpoint = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                ThrowOn(endpoint == INVALID_SOCKET, SocketException);

                break;
            }

            default: {
                Throw(NotSupportedException);

                break;
            }
        }

        _endpoint = endpoint;
    }

    void SocketChannel::_connect()
    {
        assert(!_isServerChannel() && isConnected() && _info != nullptr);

        int result = ::connect(_endpoint, _info->ai_addr,
                               static_cast<int>(_info->ai_addrlen));

        ThrowOn(result == SOCKET_ERROR, SocketException);
    }

    void SocketChannel::_createServerEndpoint()
    {
        _createServerSocket();

        try {
            _bind();
            _listen();

        } catch (...) {
            _closeSocket(_endpoint);
            _endpoint = INVALID_SOCKET;

            throw;
        }
    }

    void SocketChannel::_connectServerEndpoint()
    {
        _createClientSocket();

        try {
            _connect();

        } catch (...) {
            _closeSocket(_endpoint);
            _endpoint = INVALID_SOCKET;

            throw;
        }
    }

    void SocketChannel::_closeSocket(SOCKET socket)
    {
        assert(socket != INVALID_SOCKET);
        int result;

    #if defined(_WIN32)
        result = ::closesocket(socket);
    #else
        result = ::close(socket);
    #endif

        ThrowOn(result == SOCKET_ERROR, SocketException);
    }

    std::unique_ptr<Channel> SocketChannel::_accept()
    {
        SOCKET endpoint;
        std::string address;
        char str[INET6_ADDRSTRLEN];

        assert(_isServerChannel());

        switch (_info->ai_family)
        {
            case AF_INET6: {
            #ifdef SIMUTRACE_SOCKET_ENABLE_IPV6
                struct sockaddr_in6 sin6;
                socklen_t len6 = sizeof(sin6);

                endpoint = ::accept(_endpoint, (struct sockaddr*)&sin6, &len6);
                ThrowOn(endpoint == INVALID_SOCKET, SocketException);

                ::inet_ntop(AF_INET6, &(sin6.sin6_addr), str, sizeof(str));

                address = "[" + std::string(str) + "]:" +
                    std::to_string(ntohs(sin6.sin6_port));

            #else
                Throw(NotSupportedException);
            #endif
                break;
            }

            case AF_INET: {
                struct sockaddr_in sin;
                socklen_t len = sizeof(sin);

                endpoint = ::accept(_endpoint, (struct sockaddr*)&sin, &len);
                ThrowOn(endpoint == INVALID_SOCKET, SocketException);

                ::inet_ntop(AF_INET, &(sin.sin_addr), str, sizeof(str));

                address = std::string(str) + ":" +
                    std::to_string(ntohs(sin.sin_port));

                break;
            }

            default: {
                Throw(NotSupportedException);

                break;
            }
        }

        return std::unique_ptr<SocketChannel>(
            new SocketChannel(endpoint, address));
    }

    void SocketChannel::_disconnect()
    {
        if (_endpoint != INVALID_SOCKET) {
            _closeSocket(_endpoint);
        }

        _endpoint = INVALID_SOCKET;
    }

    size_t SocketChannel::_send(const void* data, size_t size)
    {
        assert(data != nullptr && size > 0);

    #if defined(_WIN32)
        int bytesWritten = 0;
        int result;
    #else
        ssize_t bytesWritten = 0;
        ssize_t result;

        System::maskSignal(SIGPIPE); {
    #endif

            do {
                void* buf = reinterpret_cast<void*>(
                    reinterpret_cast<size_t>(data) + bytesWritten);

            #if defined(_WIN32)
                result = ::send(_endpoint, (const char*)buf,
                                (int)(size - bytesWritten), 0);
                if (result == SOCKET_ERROR) {
                    Throw(SocketException);
            #else
                result = ::send(_endpoint, buf, size - bytesWritten, 0);
                if (result < 0) {
                    int error = System::getLastErrorCode();

                    // If error is interrupted system-call we just continue
                    if (error != EINTR) {
                        System::unmaskSignal(SIGPIPE);

                        Throw(PlatformException, error);
                    }
            #endif
                } else if (result > 0) {
                    bytesWritten += result;
                } else {
                    assert(result == 0);

                #if defined(_WIN32)
                    Throw(PlatformException, ERROR_NO_DATA);
                #else
                    System::unmaskSignal(SIGPIPE);
                    Throw(PlatformException, ECONNRESET);
                #endif
                }

            } while((result < 0) || (bytesWritten < size));

    #if defined(_WIN32)
    #else
        }
        System::unmaskSignal(SIGPIPE);
    #endif

        ThrowOn(bytesWritten != size, Exception, "The amount of data written "
                "to the connection endpoint does not match the supplied "
                "buffer size.");

        return bytesWritten;
    }

    size_t SocketChannel::_send(const std::vector<Handle>& handles)
    {
        Throw(NotSupportedException);
    }

    size_t SocketChannel::_receive(void* data, size_t size)
    {
        assert(data != nullptr && size > 0);

    #if defined(_WIN32)
        int bytesRead = 0;
        int result;
    #else
        ssize_t bytesRead = 0;
        ssize_t result;

        System::maskSignal(SIGPIPE); {
    #endif

            do {
                void* buf = reinterpret_cast<void*>(
                    reinterpret_cast<size_t>(data) + bytesRead);

            #if defined(_WIN32)
                result = ::recv(_endpoint, (char*)buf,
                                (int)(size - bytesRead), 0);
                if (result == SOCKET_ERROR) {
                    Throw(SocketException);
            #else
                result = ::recv(_endpoint, buf, size - bytesRead, 0);
                if (result < 0) {
                    int error = System::getLastErrorCode();

                    // If error is interrupted system-call we just continue
                    if (error != EINTR) {
                        System::unmaskSignal(SIGPIPE);

                        Throw(PlatformException, error);
                    }
            #endif
                } else if (result > 0) {
                    bytesRead += result;
                } else {
                    assert(result == 0);

                #if defined(_WIN32)
                    Throw(PlatformException, ERROR_NO_DATA);
                #else
                    System::unmaskSignal(SIGPIPE);
                    Throw(PlatformException, ECONNRESET);
                #endif
                }

            } while ((result < 0) || (bytesRead < size));

    #if defined(_WIN32)
    #else
        }
        System::unmaskSignal(SIGPIPE);
    #endif

        ThrowOn(bytesRead != size, Exception, "The amount of data read from "
                "the connection endpoint does not match the supplied "
                "buffer size.");

        return bytesRead;
    }

    size_t SocketChannel::_receive(std::vector<Handle>& handles,
                                   uint32_t handleCount)
    {
        Throw(NotSupportedException);
    }

    std::unique_ptr<Channel> SocketChannel::factoryMethod(bool isServer,
        const std::string& address)
    {
        return std::unique_ptr<Channel>(new SocketChannel(isServer, address));
    }

    bool SocketChannel::isConnected() const
    {
        return (_endpoint != INVALID_SOCKET);
    }

    ChannelCapabilities SocketChannel::getChannelCaps() const
    {
        return CCapNone;
    }

    const std::string& SocketChannel::getIpAddress() const
    {
        return _ipaddress;
    }

    const std::string& SocketChannel::getPort() const
    {
        return _port;
    }

}