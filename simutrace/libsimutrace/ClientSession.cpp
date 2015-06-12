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
#include "SimuStor.h"
#include "SimuTrace.h"
#include "SimuTraceTypes.h"

#include "ClientSession.h"

#include "ClientStore.h"

namespace SimuTrace
{

    struct ClientSession::ClientThreadContext {
        CriticalSection lock;
        std::unordered_map<SessionId, std::unique_ptr<ClientPort>> portMap;

        unsigned long threadId;
    };

    ClientSession::ClientSession(SessionManager& manager,
                                 std::unique_ptr<Port>& port,
                                 uint16_t serverApiVersion, SessionId localId,
                                 SessionId serverSideId, const Environment& root) :
        Session(manager, serverApiVersion, localId, root),
        _address(),
        _serverSideId(serverSideId)
    {
        ThrowOnNull(port, ArgumentNullException, "port");

        _address = port->getSpecifier();

        // The attach will ensure that the given port is a client port.
        _attach(port);
    }

    ClientSession::~ClientSession()
    {
        for (int i = 0; i < _clients.size(); ++i) {
            LogWarn("The thread %d attached to session %d, but did not "
                    "detach. Ensure that all threads properly close their "
                    "association to a session before they exit.",
                    _clients[i]->threadId, getLocalId());

            _detachFromContext(_clients[i]);
        }

        _clients.clear();
    }

    __thread ClientSession::ClientThreadContext* ClientSession::_context = nullptr;

    void ClientSession::_detachFromContext(ClientThreadContext* context)
    {
        auto it = context->portMap.find(getLocalId());
        assert(it != context->portMap.end());

    #ifdef _DEBUG
        ClientPort* port = it->second.get();
        assert(port != nullptr);
    #endif

        int index;
        assert(_clients.size() > 0);
        for (index = 0; index < _clients.size(); ++index) {
            if (_clients[index] == context) {
                break;
            }
        }

        assert(index != _clients.size());
        _clients.erase(_clients.begin() + index);

        // Remove the port from the port map. This will also disconnect the
        // channel and destroy the port.
        context->portMap.erase(it);

        // If this was the last port, free the thread's context
        if (context->portMap.empty()) {
            if (_context == context) {
                _context = nullptr;
            }

            delete context;
        }
    }

    void ClientSession::_determineStreamBufferConfiguration(
        uint32_t& numSegments)
    {
        // General note on stream buffer configuration
        // --------------------------------------------------------------------
        // Size: Larger buffers (e.g., 64 MiB) typically compress better and
        //  reduce communication overhead with the server.
        // Number: The more segments are available, the more streams can be
        //  supplied without contention. A high number of segments is also
        //  better at compensating fluctuating producer activity.
        //  WARNING: The number of segments limits the number of streams that
        //           can be simultaneously written to!
        //
        // However, supplying memory beyond a certain point will not improve
        // performance but just eat address space and potentially physical
        // memory. Furthermore, it increases the risk of data loss for
        // copy-based stream buffers.
        // If the producer activity is constantly higher than the processing
        // capacity of the storage server, the stream buffer will eventually
        // fill up and the producer will experience contention no matter how
        // big the stream buffer is. In that case a sufficiently high segment
        // size to enable good compression and a moderate number of segments
        // is optimal.

        const uint64_t segmentSize = SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB;
        const uint64_t recSize = SIMUTRACE_CLIENT_MEMMGMT_RECOMMENDED_POOLSIZE MiB;
        const uint64_t minSize = SIMUTRACE_SERVER_MEMMGMT_MINIMUM_POOLSIZE MiB;

        uint64_t confPoolSize = Configuration::get<int>("client.memmgmt.poolSize") MiB;
        uint64_t poolSize = confPoolSize;

        ThrowOn(poolSize < SIMUTRACE_CLIENT_MEMMGMT_MINIMUM_POOLSIZE,
                ConfigurationException, stringFormat("The configured memory "
                "pool size of %s is below the minimum pool size of %s.",
                sizeToString(poolSize, SizeUnit::SuMiB).c_str(),
                sizeToString(minSize, SizeUnit::SuMiB).c_str()));

        // Do not allocate memory beyond the amount of physical memory
        uint64_t physicalMemory = System::getPhysicalMemory();
        while ((poolSize > minSize) &&
               (poolSize > physicalMemory)) {

            poolSize -= segmentSize;
        }

        if (poolSize < confPoolSize) {
            LogWarn("Configured memory pool size of %s exceeds the host's "
                    "physical memory capacity (%s). Reduced pool size to %s.",
                    sizeToString(confPoolSize, SizeUnit::SuMiB).c_str(),
                    sizeToString(physicalMemory, SizeUnit::SuMiB).c_str(),
                    sizeToString(poolSize, SizeUnit::SuMiB).c_str());
        }

        if (poolSize < recSize) {
            LogWarn("The memory pool size of %s is below the recommended "
                    "configuration of %s.",
                    sizeToString(poolSize, SizeUnit::SuMiB).c_str(),
                    sizeToString(recSize, SizeUnit::SuMiB).c_str());
        }

        uint64_t availPhysicalMemory = System::getAvailablePhysicalMemory();
        if (poolSize > availPhysicalMemory) {
            LogWarn("The memory pool size of %s exceeds the available amount "
                    "of free physical memory (%s).",
                    sizeToString(poolSize, SizeUnit::SuMiB).c_str(),
                    sizeToString(availPhysicalMemory, SizeUnit::SuMiB).c_str());
        }

        numSegments = static_cast<uint32_t>(poolSize / segmentSize);
    }

    void ClientSession::_updateSettings()
    {
        // We determine what stream buffer size we should take for the client
        // and send the setting to the server. It is then up to the server
        // to create a stream buffer for us with the specified size.
        uint32_t numSegments;
        _determineStreamBufferConfiguration(numSegments);
        std::string setting = stringFormat(
            "client: { memmgmt: { poolSize = %d; }; };",
            numSegments * SIMUTRACE_MEMMGMT_SEGMENT_SIZE);

        _applySetting(setting);
    }

    void ClientSession::_attach(std::unique_ptr<Port>& port)
    {
        ClientThreadContext* context = _context;
        if (context != nullptr) {
            assert(context->threadId == ThreadBase::getCurrentThreadId());

            ThrowOn(context->portMap.find(getLocalId()) != context->portMap.end(),
                    Exception, "The thread is already attached to the session.");
        }

        assert(port != nullptr);
        assert(port->isConnected());

        // Ensure we were given a client port
        ClientPort* cp = dynamic_cast<ClientPort*>(port.get());
        ThrowOnNull(cp, ArgumentException, "port");

        if (_clients.size() > 0) {
            cp->call(nullptr, RpcApi::CCV_SessionOpen, RPC_VERSION,
                     getServerSideId());
        }

        if (context == nullptr) {
            context = new ClientThreadContext();
            context->threadId = ThreadBase::getCurrentThreadId();

            _context = context;
        }

        // Transfer ownership of port.
        context->portMap[getLocalId()] =
            std::unique_ptr<ClientPort>(static_cast<ClientPort*>(port.release()));
        _clients.push_back(context);

        Environment::set(&getEnvironment());
        EnvironmentSwapper::overrideSwap();
    }

    bool ClientSession::_detach(bool whatif)
    {
        ClientThreadContext* context = _context;

        if (context == nullptr) {
            return false;
        }

        assert(context->threadId == ThreadBase::getCurrentThreadId());

        auto it = context->portMap.find(getLocalId());
        if (it == context->portMap.end()) {
            return false;
        }

        // The thread is a valid thread of the session.
        if (whatif) {
            return true;
        }

        ClientPort* port = it->second.get();
        port->call(nullptr, RpcApi::CCV_SessionClose);

        _detachFromContext(context);

        Environment::set(nullptr);

        return true;
    }

    Store::Reference ClientSession::_createStore(const std::string& specifier,
                                                 bool alwaysCreate)
    {
        _updateSettings();

        return Store::makeOwnerReference(ClientStore::create(*this, specifier,
                                                             alwaysCreate));
    }

    Store::Reference ClientSession::_openStore(const std::string& specifier)
    {
        _updateSettings();

        return Store::makeOwnerReference(ClientStore::open(*this, specifier));
    }

    void ClientSession::_close()
    {
        // This method should never be called, if the client correctly uses the
        // API! So this is actually error handling here. We do this so that
        // the user won't end up with a broken trace, because a single thread
        // in the client crashed or otherwise misbehaved.
        // To get the session down properly, we have to use some tricks.
        // First, we drop all existing references to the session. This will get
        // us to the store release in the caller method so we can transfer any
        // pending data to the server. For this to work we ensure that
        // the current thread has a connection to the server by potentially
        // hijacking a context. This method thus should only be called, when
        // other threads participating in the session are already stopped so we
        // do not run into synchronization problems. The destructor of the
        // session finally cleans everything up (along with numerous warnings
        // in the log).

        if (_context == nullptr) {
            // The current thread is not a session participating thread. We
            // therefore have to hijack a context. Just take the first one.
            assert(_clients.size() > 0);
            _context = _clients[0];

        #ifdef _DEBUG
            auto it = _context->portMap.find(getLocalId());
            assert(it != _context->portMap.end());
            assert(it->second != nullptr);
            assert(it->second->isConnected());
        #endif
        }

        dropReferences();
    }

    void ClientSession::_enumerateStores(std::vector<std::string>& out) const
    {
        Throw(NotImplementedException);
    }

    void ClientSession::_applySetting(const std::string& setting)
    {
        this->Session::_applySetting(setting);

        getPort().call(nullptr, RpcApi::CCV_SessionSetConfiguration, setting);
    }

    StreamId ClientSession::registerStream(StreamDescriptor& desc)
    {
        ThrowOn(IsSet(desc.flags, StreamFlags::SfDynamic),
                Exception, "Stream descriptor marked as dynamic.");

        return this->Session::registerStream(desc, 0);
    }

    StreamId ClientSession::registerDynamicStream(DynamicStreamDescriptor& desc)
    {
        ThrowOn(!IsSet(desc.base.flags, StreamFlags::SfDynamic),
                Exception, "Stream descriptor not marked as dynamic.");

        // The dynamic stream descriptor starts with a regular stream descriptor
        // and extends it with further fields. We can therefore safely cast
        // the dynamic stream descriptor to a regular descriptor. This
        // operation will eventually surface in the stream creation handler
        // in ClientStore.
        return this->Session::registerStream(
            reinterpret_cast<StreamDescriptor&>(desc), 0);
    }

    const std::string& ClientSession::getAddress() const
    {
        return _address;
    }

    SessionId ClientSession::getServerSideId() const
    {
        return _serverSideId;
    }

    ClientPort& ClientSession::getPort() const
    {
        ThrowOnNull(_context, InvalidOperationException);

        auto it = _context->portMap.find(getLocalId());
        ThrowOn(it == _context->portMap.end(), InvalidOperationException);

        assert(it->second != nullptr);
        return *it->second;
    }

}
