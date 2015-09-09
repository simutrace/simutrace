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

#include "ServerStoreManager.h"

#include "simtrace/SimtraceStoreProvider.h"
#include "ServerSession.h"
#include "StorageServer.h"

namespace SimuTrace
{

    struct ServerStoreManager::StorePrefixDescriptor
    {
        std::string prefix;

        std::unique_ptr<ServerStore> (*createMethod)(StoreId,
                                                     const std::string&,
                                                     bool);
        std::unique_ptr<ServerStore> (*openMethod)(StoreId, const std::string&);
        std::string (*makePath)(const std::string&);
        void (*enumerationMethod)(const std::string&,
                                  std::vector<std::string>&);
    };

    struct ServerStoreManager::ServerStoreReference
    {
        const std::string name; // protected via _lock in manager

        CriticalSection lock;
        Store::Reference store;

        bool dormant;
        Timestamp dormantTime;

        ServerStoreReference(const std::string& name) :
            name(name),
            lock(),
            store(nullptr, NullDeleter<Store>::deleter),
            dormant(false),
            dormantTime(INVALID_TIME_STAMP) { }

        void setStore(std::unique_ptr<ServerStore>& ref)
        {
            // Since we want to output a log message on destruction,
            // we move the Store into a new owner reference with our
            // custom deleter.
            store = Store::makeOwnerReference(ref.release());
        }
    };

    ServerStoreManager::ServerStoreManager() :
        _lock(),
        _persistentCache(Configuration::get<int>("store.persistentCache")),
        _storeIdAllocator(),
        _stores()
    {

    }

    ServerStoreManager::~ServerStoreManager()
    {
        assert(_stores.empty());
    }

    Store::Reference ServerStoreManager::_createOrOpenStore(
        const ServerSession& session, const std::string& specifier,
        bool alwaysCreate, bool open)
    {
        Environment env = {0};
        env.config = session.getEnvironment().config;
        env.log = StorageServer::getInstance().getEnvironment().log;
        SwapEnvironment(&env);

        const StorePrefixDescriptor* desc = nullptr;

        if (!_findPrefixDescriptor(specifier, &desc, false)) {
            // We do not support the prefix that the caller supplied. Throw
            // an exception with a list of supported specifiers.
            uint32_t numPrefixes;
            _getPrefixDescriptors(&desc, &numPrefixes);
            assert(numPrefixes > 0);
            assert(desc != nullptr);

            std::stringstream str;
            for (uint32_t i = 0; i < numPrefixes; ++i, ++desc) {
                str << "'" << desc->prefix << ":'";

                if (i < numPrefixes - 1) {
                    str << ",";
                }
            }

            Throw(Exception, stringFormat("The supplied storage specifier "
                  "is not supported ('%s'). Supported formats are: %s. "
                  "Please see the documentation for more information.",
                  specifier.c_str(), str.str().c_str()));
        }

    retry:
        assert(desc->makePath != nullptr);
        std::string path = desc->makePath(_getPath(*desc, specifier));
        std::shared_ptr<ServerStoreReference> reference;
        StoreId id;

        LockExclusive(_lock); {
            reference = nullptr;
            id = INVALID_STORE_ID;

            // Look for the path in the open list. If the store is already open
            // we just attach the session to the store.
            for (auto it = _stores.begin(); it != _stores.end();) {
                // Reference needed to ensure life of lock after releaseStore()
                std::shared_ptr<ServerStoreReference> ref = it->second;

                // Check if the reference is marked as invalid. This will be
                // a dormant null store. These references must be deleted.
                if ((ref->store == nullptr) && (ref->dormant)) {
                    LockScope(ref->lock);

                    auto rit = it++;
                    _releaseStore(rit);

                    continue;
                } else if (path.compare(ref->name) == 0) {
                    LockScope(ref->lock);

                    // Dormant stores are always read-only. If the caller wants
                    // to open the store for write access, we close the dormant
                    // instance. This is ok, because no other thread can be
                    // attached to the session.
                    // N.B. However, other thread may have already received the
                    // reference and now wait for the lock to attach to the
                    // session. We detect that case (see b), later.
                    if (ref->dormant && !open) {
                        _releaseStore(it);
                    } else {
                        reference = ref;
                    }

                    break;
                }

                ++it;
            }

            if (reference == nullptr) {
                // The requested store is not open. Since we do not want
                // to open/create the store while holding our global lock, we
                // only create an empty reference here and perform the
                // initialization of the store while holding the reference lock
                // only.
                id = _storeIdAllocator.getNextId();
                assert(_stores.find(id) == _stores.end());

                auto it = _stores.insert(std::pair<StoreId,
                            std::shared_ptr<ServerStoreReference>>(id,
                            std::shared_ptr<ServerStoreReference>(
                            new ServerStoreReference(path))));

                reference = it.first->second;

                // We want to open a new store, so lets check if we have to
                // evict an existing one, so we do not exceed the cache limit.
                _evictStore();
            }

        } Unlock();

        // If we get here we have a valid reference. This reference can:
        //  a) be empty (store == nullptr) and we have to create the store
        //  b) be already deleted (store == nullptr, dormant), when a different
        //     thread in the meantime arrived and wants to create a new store
        //     from a dormant one (i.e., overwrite it)
        //  c) be initialized and valid for the requested access
        //  d) be initialized and not valid for the requested access
        //     (not dormant, but read-only and we want to create a new one)
        assert(reference != nullptr);
        Lock(reference->lock); {

            // The store can be uninitialized if this is a new entry. It is not
            // relevant if another thread created the entry and we are now
            // creating the store with our open-parameter. This is just a
            // different outcome of a race between the two threads.
            if (reference->store == nullptr) { // case a) or b)

                // We enter this branch in case b)
                if (reference->dormant) {
                    goto retry;
                }

                try {
                    assert(id != INVALID_STORE_ID);

                    // Let the store's factory method create a new instance
                    // that we accept through a unique_ptr.
                    std::unique_ptr<ServerStore> ustore;

                    if (open) {
                        assert(desc->openMethod != nullptr);

                        ustore = desc->openMethod(id, path);
                    } else {
                        assert(desc->createMethod != nullptr);

                        ustore = desc->createMethod(id, path, alwaysCreate);
                    }

                    assert(ustore != nullptr);
                    assert(ustore->getId() == id);
                    assert(ustore->getName().compare(path) == 0);

                    reference->setStore(ustore);
                } catch (...) {
                    // We could not open/create the store. We therefore
                    // have to remove the reference from the store map and
                    // return the id. However, there might be already other
                    // threads that received the reference (e.g., loading the
                    // same invalid store from multiple threads). Other threads
                    // could however be successful (for example T1 does not
                    // allow overwrite, T2 does). We thus cannot just remove
                    // the reference. Furthermore, we would need to get the
                    // master lock and that could lead to a deadlock (-> lock
                    // ordering).

                    // Mark the store reference as invalid. It will be removed
                    // by the next thread that walks over it in the search loop
                    // at the top of the function
                    reference->dormant = true;

                    throw;
                }

            } else if (reference->dormant) {
                assert(open);

                reference->dormant = false;

                LogDebug("Awaking dormant store '%s'.",
                         reference->store->getName().c_str());
            } else {
                ThrowOn(!open, Exception, stringFormat(
                        "Cannot create store '%s'. The store is already open.",
                        path.c_str()));
            }

            assert(reference->store != nullptr);
            assert(!reference->dormant);

            // Attach the session to the store. This will increase the stores
            // reference count.
            ServerStore* sstore =
                dynamic_cast<ServerStore*>(reference->store.get());

            assert(sstore != nullptr);
            sstore->attach(session.getId());
        } Unlock();

        // To return the store, we create an extra reference with a null
        // deleter, as we keep the unique ownership of the store.
        assert((reference != nullptr) && (reference->store.get() != nullptr));
        return Store::makeUserReference(reference->store);
    }

    void ServerStoreManager::_releaseStore(StoreMap::iterator& it)
    {
        SwapEnvironment(&StorageServer::getInstance().getEnvironment());

        StoreId id = it->first;

        // Detach the server session. Attaching a session is not possible then
        ServerStore* store = static_cast<ServerStore*>(it->second->store.get());
        if (store != nullptr) {
            uint32_t refCount = store->detach(SERVER_SESSION_ID);
            assert(refCount == 0);

            it->second->store = nullptr;
        }

        _stores.erase(it);

        _storeIdAllocator.retireId(id);
    }

    void ServerStoreManager::_evictStore()
    {
        // The eviction policy is lru, which is applied when the number of
        // dormant stores exceeds the configured cache limit

        // This is thread safe because we check the use count of the store
        // references. This count can only be increased with the master lock
        // held, which we are currently holding. The state of the selected
        // store thus cannot change, even without locking the store reference
        // itself.

        uint32_t num = 0;
        Timestamp evictTime = INVALID_TIME_STAMP;
        StoreId evictId = INVALID_STORE_ID;
        for (auto it = _stores.begin(); it != _stores.end(); ++it) {
            if ((it->second->dormant) &&
                (it->second->dormantTime < evictTime) &&
                (it->second.use_count() < 2) &&
                (it->second->store != nullptr)) {

                evictTime = it->second->dormantTime;
                evictId = it->first;
                num++;
            }
        }

        if (num > _persistentCache) {
            assert(evictId != INVALID_STORE_ID);
            auto it = _stores.find(evictId);
            assert(it != _stores.end());

            _releaseStore(it);
        }
    }

    void ServerStoreManager::_closeStore(SessionId session, StoreId id)
    {
        assert(session != SERVER_SESSION_ID);

        auto it = _stores.find(id);
        ThrowOn(it == _stores.end(), NotFoundException,
                stringFormat("store with id %d from session %d",
                    id, session));

        // Reference needed to ensure life of lock after releaseStore()
        std::shared_ptr<ServerStoreReference> ref = it->second;
        LockScope(ref->lock);

        ServerStore* store = static_cast<ServerStore*>(ref->store.get());
        if (store->detach(session) == 1) {
            if (_persistentCache == 0) {
                _releaseStore(it);
            } else {
                ref->dormant = true;
                ref->dormantTime = Clock::getTimestamp();
            }
        }
    }

    void ServerStoreManager::_getPrefixDescriptors(
        const StorePrefixDescriptor** out, uint32_t* outNumPrefixes)
    {
        // Define supported store prefixes and corresponding methods
        //
        // !! ADD SUPPORT FOR NEW STORE TYPES HERE !!
        // The prefixes are matched in the order they appear in the array.
        // In the case a prefix is a prefix of another one, place it at a
        // lower index (e.g., [0] = "simtrace2:" and [1] = "simtrace:").

        static const uint32_t numPrefixes = 1;
        static const StorePrefixDescriptor prefix[numPrefixes] = {
            { "simtrace:",
            Simtrace::SimtraceStoreProvider::createMethod,
            Simtrace::SimtraceStoreProvider::openMethod,
            Simtrace::SimtraceStoreProvider::makePath,
            Simtrace::SimtraceStoreProvider::enumerationMethod }
        };

        assert(outNumPrefixes != nullptr);
        assert(out != nullptr);

        *out = &prefix[0];
        *outNumPrefixes = numPrefixes;
    }

    bool ServerStoreManager::_findPrefixDescriptor(const std::string& specifier,
        const StorePrefixDescriptor** out, bool perfectMatch)
    {
        const StorePrefixDescriptor* prefix = nullptr;
        uint32_t numPrefixes = 0;

        _getPrefixDescriptors(&prefix, &numPrefixes);
        assert(prefix != nullptr);

        // Check which type of store the user has requested by finding a
        // matching store prefix in the specifier.
        for (uint32_t i = 0; i < numPrefixes; i++) {

            if ((perfectMatch && (specifier.compare(prefix[i].prefix) == 0)) ||
                (!perfectMatch && (specifier.compare(0, prefix[i].prefix.length(),
                prefix[i].prefix) == 0))) {

                if (out != nullptr) {
                    *out = &prefix[i];
                }

                return true;
            }

        }

        return false;
    }

    std::string ServerStoreManager::_getPath(const StorePrefixDescriptor& desc,
                                             const std::string& specifier)
    {
        return specifier.substr(desc.prefix.length(), std::string::npos);
    }

    bool ServerStoreManager::_isValidPrefix(const std::string& prefix)
    {
        return _findPrefixDescriptor(prefix, nullptr, true);
    }

    void ServerStoreManager::_splitSpecifier(const std::string& specifier,
                                             std::string* prefix,
                                             std::string* path)
    {
        const StorePrefixDescriptor* desc = nullptr;

        if (!_findPrefixDescriptor(specifier, &desc, false)) {
            Throw(NotSupportedException);
        }

        if (prefix != nullptr) {
            *prefix = desc->prefix;
        }

        if (path != nullptr) {
            *path = _getPath(*desc, specifier);
        }
    }

    Store::Reference ServerStoreManager::createStore(const ServerSession& session,
                                                     const std::string& specifier,
                                                     bool alwaysCreate)
    {
        return _createOrOpenStore(session, specifier, alwaysCreate, false);
    }

    Store::Reference ServerStoreManager::openStore(const ServerSession& session,
                                                   const std::string& specifier)
    {
        return _createOrOpenStore(session, specifier, false, true);
    }

    void ServerStoreManager::closeStore(SessionId session, StoreId id)
    {
        LockScopeExclusive(_lock);
        ThrowOn(session == SERVER_SESSION_ID, NotFoundException,
                stringFormat("store with id %d from session %d",
                    id, session));

        _closeStore(session, id);
    }

    void ServerStoreManager::enumerateStores(const ServerSession& session,
                                             std::vector<std::string>& out)
    {
        SwapEnvironment(&session.getEnvironment());

        const StorePrefixDescriptor* prefix = nullptr;
        uint32_t numPrefixes = 0;

        _getPrefixDescriptors(&prefix, &numPrefixes);
        assert(prefix != nullptr);

        out.clear();

        for (uint32_t i = 0; i < numPrefixes; ++i) {
            assert(prefix[i].enumerationMethod != nullptr);
            assert(!prefix[i].prefix.empty());

            // Get the prefix without the trailing ':'
            const std::string& prefixName = prefix[i].prefix;
            std::string prfx = prefixName.substr(0, prefixName.size() - 1);

            // Read the base path from the session's configuration
            std::string base;
            Configuration::tryGet("store." + prfx + ".root", base);

            prefix[i].enumerationMethod(base, out);
        }
    }

    ServerStore& ServerStoreManager::getStore(StoreId id) const
    {
        LockScopeShared(_lock);

        auto it = _stores.find(id);
        ThrowOn(it == _stores.end(), NotFoundException,
                stringFormat("store with id %d", id));

        assert(it->second->store != nullptr);
        return *static_cast<ServerStore*>(it->second->store.get());
    }

    uint32_t ServerStoreManager::getOpenStoresCount() const
    {
        LockScopeShared(_lock);
        return static_cast<uint32_t>(_stores.size());
    }

}