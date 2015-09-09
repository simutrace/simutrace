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

#include "LocalChannel.h"

#include "Configuration.h"
#include "Exceptions.h"
#include "Utils.h"

#if defined(_WIN32)
#define PIPE_OUT_BUFFER_SIZE 0x100000
#define PIPE_IN_BUFFER_SIZE 0x100000
#define PIPE_DEFAULT_TIMEOUT 1000
#else
#define UNIX_DOMAIN_SOCKET_MAGIC_ID 0xff42ff42
#endif

namespace SimuTrace
{

    LocalChannel::LocalChannel(bool isServer, const std::string& address) :
        Channel(isServer, address),
        _endpoint()
    {
        if (isServer) {
            _createServerEndpoint();
        } else {
            _connectServerEndpoint();
        }
    }

    LocalChannel::LocalChannel(SafeHandle& endpoint, const std::string& address) :
        Channel(false, address),
        _endpoint()
    {
        assert(endpoint.isValid());

        _endpoint.swap(endpoint);
    }

    LocalChannel::~LocalChannel()
    {
        disconnect();
        assert(!isConnected());
    }

#if defined(_WIN32)
    void LocalChannel::_createServerNamedPipe()
    {
        SECURITY_ATTRIBUTES security;
        security.bInheritHandle       = TRUE;
        security.lpSecurityDescriptor = nullptr;
        security.nLength              = sizeof(security);
        //TODO: Deny NT AUTHORITY\NETWORK to forbid network access to pipe.
        //      This would break the "Handle Transfer" capability.

        std::string tmp = "\\\\.\\pipe\\" + getAddress();

        Handle handle = ::CreateNamedPipeA(tmp.c_str(),
                                           PIPE_ACCESS_DUPLEX,
                                           PIPE_TYPE_BYTE,
                                           PIPE_UNLIMITED_INSTANCES,
                                           PIPE_OUT_BUFFER_SIZE,
                                           PIPE_IN_BUFFER_SIZE,
                                           PIPE_DEFAULT_TIMEOUT,
                                           &security);

        ThrowOn(handle == INVALID_HANDLE_VALUE, PlatformException);

        _endpoint = handle;
    }

    void LocalChannel::_connectServerNamedPipe()
    {
        std::string tmp = "\\\\.\\pipe\\" + getAddress();

        // TODO: read retry properties from configuration. We first need to
        //       establish a working configuration space on the client side!
        const uint32_t maxRetryCount = 10;
        //      Configuration::get<int>("network.local.retryCount");
        const uint32_t sleepTime = 500;
        //      Configuration::get<int>("network.local.retrySleep");

        SafeHandle handle;
        uint32_t retryCount = 0;
        do {
            handle = ::CreateFileA(tmp.c_str(), GENERIC_READ | GENERIC_WRITE,
                                   0, nullptr, OPEN_EXISTING, 0, NULL);
            if (handle.isValid()) {
                break;
            }

            retryCount++;

            ThrowOn(retryCount > maxRetryCount, PlatformException,
                    ERROR_TIMEOUT);
            ThrowOn(System::getLastErrorCode() != ERROR_PIPE_BUSY,
                    PlatformException);

            ::WaitNamedPipe(tmp.c_str(), sleepTime);
        } while (true);

        DWORD mode = PIPE_READMODE_BYTE;
        if (!::SetNamedPipeHandleState(handle, &mode, nullptr, nullptr)) {
            Throw(PlatformException);
        }

        _endpoint.swap(handle);
    }

    void LocalChannel::_duplicateHandles(const std::vector<Handle>& local,
                                         std::vector<Handle>& remote)
    {
        unsigned long remotePid;
        if (!::GetNamedPipeClientProcessId(_endpoint, &remotePid)) {
            Throw(PlatformException);
        }

        SafeHandle remoteProcess;
        remoteProcess = ::OpenProcess(PROCESS_DUP_HANDLE, FALSE, remotePid);
        ThrowOn(!remoteProcess.isValid(), PlatformException);

        // This will be a fake handle, which we do not have to close.
        Handle localProcess = ::GetCurrentProcess();

        remote.clear();
        remote.reserve(local.size());
        for (int i = 0; i < local.size(); i++) {
            Handle remoteHandle;

            BOOL success = ::DuplicateHandle(localProcess, local[i],
                                             remoteProcess, &remoteHandle,
                                             0, TRUE, DUPLICATE_SAME_ACCESS);

            ThrowOn(!success, PlatformException);

            remote.push_back(remoteHandle);
        }
    }

#else

    void LocalChannel::_createDomainSocket(SafeHandle& handle,
                                           struct sockaddr_un& sockaddr)
    {
        handle = ::socket(PF_UNIX, SOCK_STREAM, 0);
        ThrowOn(!handle.isValid(), PlatformException);

        memset(&sockaddr, 0, sizeof(struct sockaddr_un));

        sockaddr.sun_family = AF_UNIX;
        int result = ::snprintf(sockaddr.sun_path,
                                sizeof(sockaddr.sun_path),
                                "%s", getAddress().c_str());

        ThrowOn(result >= sizeof(sockaddr.sun_path), Exception,
                stringFormat("The socket name is too long (max %i).",
                    sizeof(sockaddr.sun_path)));
    }

    void LocalChannel::_createServerDomainSocket()
    {
        ::umask(0);
        ::unlink(getAddress().c_str());

        SafeHandle handle;
        struct sockaddr_un sockaddr;

        _createDomainSocket(handle, sockaddr);

        int result = ::bind(handle, (const struct sockaddr*)&sockaddr,
                            sizeof(struct sockaddr_un));
        ThrowOn(result < 0, PlatformException);

        result = ::listen(handle, 10);
        ThrowOn(result < 0, PlatformException);

        _endpoint.swap(handle);
    }

    void LocalChannel::_connectServerDomainSocket()
    {
        SafeHandle handle;
        struct sockaddr_un sockaddr;

        _createDomainSocket(handle, sockaddr);

        int result = ::connect(handle, (const struct sockaddr*)&sockaddr,
                               sizeof(struct sockaddr_un));
        ThrowOn(result < 0, PlatformException);

        _endpoint.swap(handle);
    }

    struct msghdr* LocalChannel::_allocateHandleTransferMessage(
        uint32_t handleCount)
    {
        struct iovec *iov = new iovec();
        int *iobuf        = new int();
        iov->iov_base = iobuf;
        iov->iov_len  = sizeof(int);

        struct msghdr *msg = new msghdr();
        msg->msg_iov    = iov;
        msg->msg_iovlen = 1;

        size_t size = sizeof(Handle) * handleCount;

        void* buf = new char[CMSG_SPACE(size)];
        memset(buf, 0, CMSG_SPACE(size));

        msg->msg_control    = buf;
        msg->msg_controllen = CMSG_SPACE(size);

        msg->msg_name    = nullptr;
        msg->msg_namelen = 0;
        msg->msg_flags   = 0;

        return msg;
    }

    void LocalChannel::_freeHandleTransferMessage(struct msghdr* msg)
    {
        delete [] static_cast<char*>(msg->msg_control);
        delete static_cast<int*>(msg->msg_iov->iov_base);
        delete msg->msg_iov;
        delete msg;
    }

    uint64_t LocalChannel::_sendHandleTransferMessage(struct msghdr* msg)
    {
        int* iov = static_cast<int*>(msg->msg_iov->iov_base);
        ssize_t result;

        // Set magic id to prevent mismatch
        *iov = UNIX_DOMAIN_SOCKET_MAGIC_ID;

        System::ignoreSignal(SIGPIPE); {

            result = ::sendmsg(_endpoint, msg, 0);
            if (result < 0) {
                int error = System::getLastErrorCode();
                System::restoreSignal(SIGPIPE);

                Throw(PlatformException, error);
            }

        }
        System::restoreSignal(SIGPIPE);

        return result;
    }

    uint64_t LocalChannel::_receiveHandleTransferMessage(struct msghdr* msg,
                                                         uint32_t handleCount)
    {
        ssize_t result;

        System::ignoreSignal(SIGPIPE); {

            result = ::recvmsg(_endpoint, msg, 0);
            if (result < 0) {
                int error = System::getLastErrorCode();
                System::restoreSignal(SIGPIPE);

                Throw(PlatformException, error);
            }

        }
        System::restoreSignal(SIGPIPE);

        int* iov = static_cast<int*>(msg->msg_iov->iov_base);
        ThrowOn(*iov != UNIX_DOMAIN_SOCKET_MAGIC_ID, Exception,
                "Failed to receive handles. Protocol error.");

        return result;
    }
#endif

    void LocalChannel::_createServerEndpoint()
    {
    #if defined(_WIN32)
        _createServerNamedPipe();
    #else
        _createServerDomainSocket();
    #endif
    }

    void LocalChannel::_connectServerEndpoint()
    {
    #if defined(_WIN32)
        _connectServerNamedPipe();
    #else
        _connectServerDomainSocket();
    #endif
    }

    std::unique_ptr<Channel> LocalChannel::_accept()
    {
        assert(_isServerChannel());

        SafeHandle connection;

    #if defined(_WIN32)
        bool clientConnected = false;
        do {
            clientConnected = (::ConnectNamedPipe(_endpoint, nullptr) == TRUE);

            if (!clientConnected) {

                switch (System::getLastErrorCode())
                {
                    case ERROR_PIPE_CONNECTED: {
                        clientConnected = true;

                        break;
                    };

                    case ERROR_NO_DATA: {
                        _endpoint.close();

                        break;
                    }

                    default: {
                        Throw(PlatformException);

                        break;
                    }
                } // end switch

            }

            connection.swap(_endpoint);
            _createServerNamedPipe();

        } while (!clientConnected);

    #else
        connection = ::accept(_endpoint, nullptr, nullptr);
        ThrowOn(!connection.isValid(), PlatformException);
    #endif

        return std::unique_ptr<LocalChannel>(
            new LocalChannel(connection, getAddress()));
    }

    void LocalChannel::_disconnect()
    {
        if (_endpoint.isValid()) {
        #if defined(_WIN32)
            if (_isServerChannel()) {
                ::FlushFileBuffers(_endpoint);
                ::DisconnectNamedPipe(_endpoint);
            }
        #else
            if (_isServerChannel()) {
                ::unlink(getAddress().c_str());
            }
        #endif

            _endpoint.close();
        }
    }

    size_t LocalChannel::_send(const void* data, size_t size)
    {
        assert(data != nullptr && size > 0);

    #if defined(_WIN32)
        DWORD bytesWritten;
        if (!::WriteFile(_endpoint, data, (DWORD) size, &bytesWritten, nullptr)) {
            Throw(PlatformException);
        }
    #else
        ssize_t bytesWritten = 0;
        ssize_t result;

        System::ignoreSignal(SIGPIPE); {

            do {
                void* buf = reinterpret_cast<void*>(
                    reinterpret_cast<size_t>(data) + bytesWritten);

                result = ::send(_endpoint, buf, size - bytesWritten, 0);
                if (result < 0) {
                    int error = System::getLastErrorCode();

                    // If error is interrupted system-call we just continue
                    if (error != EINTR) {
                        System::restoreSignal(SIGPIPE);

                        Throw(PlatformException, error);
                    }
                } else if (result > 0) {
                    bytesWritten += result;
                } else {
                    assert(result == 0);
                    System::restoreSignal(SIGPIPE);

                    Throw(PlatformException, ECONNRESET);
                }

            } while((result < 0) || (bytesWritten < size));

        }
        System::restoreSignal(SIGPIPE);
    #endif

        ThrowOn(bytesWritten != size, Exception, "The amount of data written "
                "to the connection endpoint does not match the supplied "
                "buffer size.");

        return bytesWritten;
    }

    size_t LocalChannel::_send(const std::vector<Handle>& handles)
    {
        if (handles.size() == 0) {
            return 0;
        }

        size_t size = sizeof(Handle) * handles.size();
        size_t bytesSent;

    #if defined(_WIN32)
        // In Windows, duplication of handles does not use the established
        // connection, but instead works by calling the respective API. After
        // the call to duplicateHandles, the handles are valid in the target
        // process.

        std::vector<Handle> remote;
        _duplicateHandles(handles, remote);

        // Send the list of handles to the target process to inform the target
        // about which handles have been to be transferred.

        bytesSent = _send(remote.data(), size);
    #else

        // In Linux, we make the handles valid in the target process by sending
        // a specially crafted message via a domain socket. The handle values that
        // are received by the target process are modified by the system to
        // reflect valid handles.

        struct msghdr* msg = _allocateHandleTransferMessage(handles.size());
        msg->msg_name    = nullptr;
        msg->msg_namelen = 0;
        msg->msg_flags   = 0;

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type  = SCM_RIGHTS; // Transfer file descriptors.
        cmsg->cmsg_len   = CMSG_LEN(size);

        memcpy(CMSG_DATA(cmsg), handles.data(), size);

        try {
            bytesSent = _sendHandleTransferMessage(msg);

        } catch (...) {
            _freeHandleTransferMessage(msg);

            throw;
        }

        _freeHandleTransferMessage(msg);
    #endif

        return bytesSent;
    }

    size_t LocalChannel::_receive(void* data, size_t size)
    {
        assert(data != nullptr && size > 0);

    #if defined(_WIN32)
        DWORD bytesRead;
        if (!::ReadFile(_endpoint, data, (DWORD) size, &bytesRead, nullptr)) {
            Throw(PlatformException);
        }
    #else
        ssize_t bytesRead = 0;
        ssize_t result;

        System::ignoreSignal(SIGPIPE); {

            do {
                void* buf = reinterpret_cast<void*>(
                    reinterpret_cast<size_t>(data) + bytesRead);

                result = ::recv(_endpoint, buf, size - bytesRead, 0);
                if (result < 0) {
                    int error = System::getLastErrorCode();

                    // If error is interrupted system-call we just continue
                    if (error != EINTR) {
                        System::restoreSignal(SIGPIPE);

                        Throw(PlatformException, error);
                    }
                } else if (result > 0) {
                    bytesRead += result;
                } else {
                    assert(result == 0);
                    System::restoreSignal(SIGPIPE);

                    Throw(PlatformException, ECONNRESET);
                }

            } while ((result < 0) || (bytesRead < size));

        }
        System::restoreSignal(SIGPIPE);
    #endif

        return bytesRead;
    }

    size_t LocalChannel::_receive(std::vector<Handle>& handles,
                                  uint32_t handleCount)
    {
        if (handleCount == 0) {
            return 0;
        }

        size_t size = sizeof(Handle) * handleCount;
        size_t bytesReceived;
        Handle* handlelist = nullptr;

        // Receive the list of handles from the communication peer.

    #if defined(_WIN32)
        handlelist = new Handle[handleCount];

        try {
            bytesReceived = _receive(handlelist, size);

        } catch (...) {
            delete [] handlelist;

            throw;
        }

    #else
        struct msghdr* msgh = _allocateHandleTransferMessage(handleCount);
        struct cmsghdr* cmsg;

        try {
            bytesReceived = _receiveHandleTransferMessage(msgh, size);

            for(cmsg = CMSG_FIRSTHDR(msgh); cmsg != nullptr;
                cmsg = CMSG_NXTHDR(msgh, cmsg)) {

                if( (cmsg->cmsg_level == SOL_SOCKET) &&
                    (cmsg->cmsg_type == SCM_RIGHTS) ) {

                    handlelist = reinterpret_cast<Handle*>(CMSG_DATA(cmsg));
                    break;
                }
            }

        } catch (...) {
            _freeHandleTransferMessage(msgh);

            throw;
        }
    #endif

        //
        // Copy the list of new handles into the supplied vector and cleanup.
        //

        handles.clear();

        if (handlelist != nullptr) {
            for (uint32_t i = 0; i < handleCount; i++) {
                handles.push_back(handlelist[i]);
            }
        }

    #if defined(_WIN32)
        delete [] handlelist;
    #else
        _freeHandleTransferMessage(msgh);
    #endif

        return bytesReceived;
    }

    std::unique_ptr<Channel> LocalChannel::factoryMethod(bool isServer,
        const std::string& address)
    {
        return std::unique_ptr<Channel>(new LocalChannel(isServer, address));
    }

    bool LocalChannel::isConnected() const
    {
        return (_endpoint.isValid());
    }

    ChannelCapabilities LocalChannel::getChannelCaps() const
    {
        return CCapHandleTransfer;
    }

}
