/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Store Library (libsimustor) is part of Simutrace.
 *
 * libsimustor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimustor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuBase.h"
#include "SimuStorTypes.h"

#include "Session.h"

#include "StreamBuffer.h"
#include "Store.h"
#include "Stream.h"
#include "SessionManager.h"

using namespace libconfig;

namespace SimuTrace
{

    Session::Session(SessionManager& manager, uint16_t peerApiVersion,
                     SessionId localId, const Environment& root) :
        _manager(manager),
        _peerApiVersion(peerApiVersion),
        _localId(localId),
        _referenceCount(1),
        _isAlive(true),
        _config(),
        _configLockList(nullptr),
        _log(stringFormat("[Session %d]", localId), root.log),
        _environment(root),
        _store(nullptr, nullptr)
    {
        _environment.log = &_log;
        _environment.config = &_config;

        _initializeConfiguration(root.config);

        LogInfo("Created session %d (RPCv%d.%d).",
                localId,
                RPC_VER_MAJOR(peerApiVersion),
                RPC_VER_MINOR(peerApiVersion));
    }

    Session::~Session()
    {
        assert(_store == nullptr);
        assert(_referenceCount == 0);

        LogInfo("Session %d closed.", _localId);

        // Reset environment
        if (Environment::getConfig() == &_config) {
            Environment::setConfig(nullptr);
        }

        if (Environment::getLog() == &_log) {
            Environment::setLog(_log.getParent());
        }
    }

    Session::Reference Session::makeOwnerReference(Session* session)
    {
        return Reference(session, [](Session* instance){
            SessionId id = instance->getLocalId();
            (void)id; // Make compiler happy in release build

            delete instance;

            LogDebug("Session %d released.", id);
        });
    }

    void Session::_initializeConfiguration(libconfig::Config* rootConfig)
    {
        if (rootConfig != nullptr) {
            SwapEnvironment(&_environment);

            Configuration::apply(rootConfig->getRoot(), nullptr);
        }
    }

    bool Session::_dodetach()
    {
        bool whatif = (_referenceCount == 1);
        bool detached = _detach(whatif);

        if (!whatif && detached) {
            assert(_referenceCount > 1);
            _referenceCount--;
        }

        return detached;
    }

    void Session::_closeStore()
    {
        if (_store == nullptr) {
            return;
        }

        LogInfo("Closing store '%s'.", _store->getName().c_str());
        _store->detach(_localId);

        _store = nullptr;
    }

    void Session::dropReferences()
    {
        if (_referenceCount == 0) {
            return;
        }

        LogWarn("Dropping %d outstanding references to session %d.",
                _referenceCount, _localId);

        _referenceCount = 0;
    }

    void Session::_applySetting(const std::string& setting)
    {
        // Ensure that the session's configuration is active
        SwapEnvironment(&_environment);

        libconfig::Config tconfig;
        tconfig.readString(setting);

        Configuration::apply(tconfig.getRoot(), _configLockList.get());
    }

    void Session::_setConfigLockList(
        std::unique_ptr<Configuration::LockList>& lockList)
    {
        _configLockList = std::move(lockList);
    }

    Store* Session::_getStore() const
    {
        return _store.get();
    }

    void Session::attach(std::unique_ptr<Port>& port)
    {
        LockScope(_lock);
        ThrowOn(!_isAlive, InvalidOperationException);
        ThrowOnNull(port, ArgumentNullException, "port");

        _attach(port);

        _referenceCount++;
    }

    void Session::detach()
    {
        /* This method should only be called from a thread that is attached
           to the session. Otherwise, we throw an exception.                 */

        ThrowOn(!_isAlive, InvalidOperationException);

        Lock(_lock); {
            uint32_t refCount = _referenceCount;

            // Detach the calling thread if it is part of the session and
            // not the last one in the session.
            if (!_dodetach()) {
                Throw(InvalidOperationException);
            }

            if (refCount > 1) {
                return;
            }

            // This is the last thread, so we have to close the session. We
            // need to acquire the store lock to prevent that someone opens a
            // new store after we released the current one (if any) here.
            LockExclusive(_storeLock); {
                _closeStore();

                // This will detach the current (i.e., the last) thread.
                // After the call the worker thread object has been destroyed.
                _detach(false);

                // After switching the alive flag practically no operations are
                // any longer allowed. This allows us releasing all locks
                // without having to worry about asynchronous calls.
                _isAlive = false;
                _referenceCount = 0;
            } Unlock();
        } Unlock();

        // The manager will free this session object. Do not access any members
        // after this point!
        _manager._releaseSession(getLocalId());
    }

    void Session::close()
    {
        ThrowOn(!_isAlive, InvalidOperationException);
        SwapEnvironment(&_environment);

        Lock(_lock); {

            // Give any derived classes a chance to close. This should inform
            // all threads attached to the session about the intended shutdown.
            _close();

            // The server should never cross this check
            if (_referenceCount > 0) {
                return;
            }

            // Close the session. We need to acquire the store lock to prevent
            // that someone opens a new store after we released the current
            // one (if any) here.
            LockExclusive(_storeLock); {
                _closeStore();

                // After switching the alive flag practically no operations are
                // any longer allowed. This allows us releasing all locks
                // without having to worry about asynchronous calls.
                _isAlive = false;
            } Unlock();
        } Unlock();

        // The manager will free this session object. Do not access any members
        // after this point!
        _manager._releaseSession(getLocalId());
    }

    void Session::createStore(const std::string& specifier, bool alwaysCreate)
    {
        LockScopeExclusive(_storeLock);
        ThrowOn((_store != nullptr) || !_isAlive, InvalidOperationException);

        LogInfo("Creating store '%s'.%s", specifier.c_str(),
                (alwaysCreate) ? " Overwriting if existing." : "");

        _store = _createStore(specifier, alwaysCreate);
        assert(_store != nullptr);
    }

    void Session::openStore(const std::string& specifier)
    {
        LockScopeExclusive(_storeLock);
        ThrowOn((_store != nullptr) || !_isAlive, InvalidOperationException);

        LogInfo("Opening store '%s'.", specifier.c_str());

        _store = _openStore(specifier);
        assert(_store != nullptr);
    }

    void Session::closeStore()
    {
        LockScopeExclusive(_storeLock);
        ThrowOn(!_isAlive, InvalidOperationException);

        _closeStore();
    }

    BufferId Session::registerStreamBuffer(size_t segmentSize,
                                           uint32_t numSegments)
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        return _store->registerStreamBuffer(segmentSize, numSegments);
    }

    StreamId Session::registerStream(StreamDescriptor& desc,
                                     BufferId buffer)
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        return _store->registerStream(desc, buffer);
    }

    void Session::enumerateStreamBuffers(std::vector<BufferId>& out) const
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        _store->enumerateStreamBuffers(out);
    }

    void Session::enumerateStreams(std::vector<StreamId>& out,
                                   StreamEnumFilter filter) const
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        _store->enumerateStreams(out, filter);
    }

    void Session::enumerateStores(std::vector<std::string>& out) const
    {
        _enumerateStores(out);
    }

    StreamBuffer& Session::getStreamBuffer(BufferId id) const
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        return _store->getStreamBuffer(id);
    }

    Stream& Session::getStream(StreamId id) const
    {
        LockScopeShared(_storeLock);
        ThrowOnNull(_store, InvalidOperationException);

        assert(_isAlive);

        return _store->getStream(id);
    }

    uint16_t Session::getPeerApiVersion() const
    {
        return _peerApiVersion;
    }

    SessionId Session::getLocalId() const
    {
        return _localId;
    }

    void Session::applySetting(const std::string& setting)
    {
        LockScope(_lock);
        ThrowOn(!_isAlive, InvalidOperationException);

        _applySetting(setting);
    }

    const Environment& Session::getEnvironment() const
    {
        return _environment;
    }

    bool Session::isAlive() const
    {
        return _isAlive;
    }

}
