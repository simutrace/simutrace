/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#include "SimuStor.h"

#include "ServerSession.h"

#include "StorageServer.h"
#include "ServerStoreManager.h"
#include "ServerStore.h"
#include "ServerSessionWorker.h"

namespace SimuTrace
{

    ServerSession::ServerSession(SessionManager& manager,
                                 std::unique_ptr<Port>& port,
                                 uint16_t clientApiVersion, SessionId localId,
                                 const Environment& root) :
        Session(manager, clientApiVersion, localId, root),
        _workerLog(stringFormat("Worker [Session %d]", localId), root.log),
        _workerEnvironment(root)
    {
        _workerEnvironment.log = &_workerLog;

        std::unique_ptr<Configuration::LockList> lockList(
            new Configuration::LockList());
        lockList->push_back(Configuration::ConfigurationLock(
            Configuration::ConfigurationLock::Type::ClkIfExists,
            "client.memmgmt.poolSize"));

        _setConfigLockList(lockList);

        _attach(port);
    }

    ServerSession::~ServerSession()
    {
        assert(_workers.size() == 0);
    }

    void ServerSession::_attach(std::unique_ptr<Port>& port)
    {
        assert(port != nullptr);

        ServerSessionWorker* worker = new ServerSessionWorker(*this, port);

        _workers.push_back(std::unique_ptr<ServerSessionWorker>(worker));

        worker->start();
    }

    bool ServerSession::_detach(bool whatif)
    {
        ThreadBase* thread = ThreadBase::getCurrentThread();
        ServerSessionWorker* worker;

        // Check that the currently running thread is one of this session's
        // workers
        if (thread == nullptr) {
            return false;
        }

        assert(thread->isExecutingThread());

        worker = dynamic_cast<ServerSessionWorker*>(thread);
        if ((worker == nullptr) || (&worker->getSession() != this)) {
            return false;
        } else if (whatif) {
            return true;
        }

        // Ensure that the worker is not in the running state, but calls us
        // from within it's finalize method.
        ThrowOn(thread->isRunning(), InvalidOperationException);

        int index;
        assert(_workers.size() > 0);
        for (index = 0; index < _workers.size(); ++index) {
            if (_workers[index].get() == worker) {
                break;
            }
        }

        assert(index != _workers.size());
        SessionId session = worker->getSession().getId();

        // Release the worker instance. After this point, no member of this
        // class may be accessed!
        _workers.erase(_workers.begin() + index);

        // Reset logging context to the server's log
        if (Environment::getLog() == &_workerLog) {
            Environment::setLog(_workerLog.getParent());
        }

        LogInfo("Detached session worker thread %d from session %d.",
                ThreadBase::getCurrentSystemThreadId(), session);

        return true;
    }

    Store::Reference ServerSession::_createStore(const std::string& specifier,
                                                 bool alwaysCreate)
    {
        ServerStoreManager& mgr = StorageServer::getInstance().getStoreManager();

        return mgr.createStore(*this, specifier, alwaysCreate);
    }

    Store::Reference ServerSession::_openStore(const std::string& specifier)
    {
        ServerStoreManager& mgr = StorageServer::getInstance().getStoreManager();

        return mgr.openStore(*this, specifier);
    }

    void ServerSession::_detachStore()
    {
        ServerStoreManager& mgr = StorageServer::getInstance().getStoreManager();
        Store* store = _getStore();
        assert(store != nullptr);

        mgr.closeStore(getId(), store->getId());
    }

    void ServerSession::_close()
    {
        const int confTimeout =
            Configuration::get<int>("server.session.closeTimeout");

        for (int i = 0; i < _workers.size(); ++i) {
            std::unique_ptr<ServerSessionWorker>& worker = _workers[i];

            worker->close();

            if (worker->isExecutingThread()) {
                continue;
            }

            // N.B. We can still access the workers beyond this point, because
            // the session lock, which we acquired in session.close() prevents
            // the worker threads to be detached (and thus freed), yet.

            int timeout = confTimeout;
            const int sleepTime = 500;
            while (!worker->hasFinished()) {
                ThreadBase::sleep(sleepTime);

                timeout -= sleepTime;
                ThrowOn(timeout <= 0, Exception, stringFormat(
                        "Failed to close session worker thread %d within "
                        "grace period. See also: server.session.closeTimeout.",
                        worker->getCurrentSystemThreadId()));
            }
        }
    }

    void ServerSession::_enumerateStores(std::vector<std::string>& out) const
    {
        return ServerStoreManager::enumerateStores(*this, out);
    }

    void ServerSession::enumerateStreamBuffers(std::vector<StreamBuffer*>& out) const
    {
        LockScopeShared(_storeLock);

        ServerStore* store = static_cast<ServerStore*>(_getStore());
        ThrowOnNull(store, InvalidOperationException);

        assert(isAlive());

        store->enumerateStreamBuffers(out);
    }

    void ServerSession::enumerateStreams(std::vector<Stream*>& out,
                                         StreamEnumFilter filter) const
    {
        LockScopeShared(_storeLock);

        ServerStore* store = static_cast<ServerStore*>(_getStore());
        ThrowOnNull(store, InvalidOperationException);

        assert(isAlive());

        store->enumerateStreams(out, filter);
    }

    const Environment& ServerSession::getWorkerEnvironment() const
    {
        return _workerEnvironment;
    }

}
