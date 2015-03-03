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

    struct ServerStoreManager::StorePrefixDescriptor {
        std::string prefix;

        std::unique_ptr<ServerStore> (*createMethod)(StoreId,
                                                     const std::string&,
                                                     bool);
        std::unique_ptr<ServerStore> (*openMethod)(StoreId, const std::string&);
        std::string (*makePath)(const std::string&);
        void (*enumerationMethod)(const std::string&,
                                  std::vector<std::string>&);
    };

    ServerStoreManager::ServerStoreManager()
    {

    }

    ServerStoreManager::~ServerStoreManager()
    {
        assert(_stores.empty());
    }

    Store::Reference ServerStoreManager::_createOrOpenStore(
        ServerSession& session, const std::string& specifier,
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

        assert(desc->makePath != nullptr);
        std::string path = desc->makePath(_getPath(*desc, specifier));
        Store::Reference* store = nullptr;

        LockExclusive(_lock); {

            // Look for the path in the open list. If the store is already open
            // we just attach the session to the store.
            for (auto& ostore : _stores) {
                if (path.compare(ostore.second->getName()) == 0) {
                    store = &ostore.second;
                    break;
                }
            }

            if (store == nullptr) {
                // The requested store is not open. Try to open it.
                StoreId id = _storeIdAllocator.getNextId();
                assert(_stores.find(id) == _stores.end());

                try {
                     // Let the store's factory method create a new instance
                     // that we accept through a unique_ptr.
                     std::unique_ptr<ServerStore> ustore;

                     if (open) {
                        assert(desc->openMethod != nullptr);

                        ustore = desc->openMethod(id, path);
                     } else {
                        ustore = desc->createMethod(id, path, alwaysCreate);
                     }

                     assert(ustore != nullptr);
                     assert(ustore->getId() == id);

                     // Since we want to output a log message on destruction,
                     // we move the Store into a new owner reference with our
                     // custom deleter. ustore should be empty afterwards.
                     _stores.insert(std::pair<StoreId, Store::Reference>(
                         id, Store::makeOwnerReference(ustore.release())));

                     auto it = _stores.find(id);
                     assert(it != _stores.end());
                     store = &it->second;
                } catch (...) {
                    _storeIdAllocator.retireId(id);

                    throw;
                }

            } else {
                ThrowOn(!open, Exception, stringFormat(
                        "Cannot create store '%s'. The store is already open.",
                        path.c_str()));
            }

            // Attach the session to the store. This will increase the stores
            // reference count.
            ServerStore* sstore = dynamic_cast<ServerStore*>(store->get());

            assert(sstore != nullptr);
            sstore->attach(session.getLocalId());

        } Unlock();

        // To return the store, we create an extra reference with a null
        // deleter, as we keep the unique ownership of the store.
        assert((store != nullptr) && (store->get() != nullptr));
        return Store::makeUserReference(*store);
    }

    void ServerStoreManager::_releaseStore(StoreId store)
    {
        LockScopeExclusive(_lock);
        SwapEnvironment(&StorageServer::getInstance().getEnvironment());

        auto it = _stores.find(store);
        assert(it != _stores.end());
        assert(it->second != nullptr);

        _stores.erase(it);
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

    Store::Reference ServerStoreManager::createStore(ServerSession& session,
                                                     const std::string& specifier,
                                                     bool alwaysCreate)
    {
        return _createOrOpenStore(session, specifier, alwaysCreate, false);
    }

    Store::Reference ServerStoreManager::openStore(ServerSession& session,
                                                   const std::string& specifier)
    {
        return _createOrOpenStore(session, specifier, false, true);
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
        ThrowOn(it == _stores.end(), NotFoundException);

        assert(it->second != nullptr);
        return *static_cast<ServerStore*>(it->second.get());
    }

    uint32_t ServerStoreManager::getOpenStoresCount() const
    {
        LockScopeShared(_lock);
        return static_cast<uint32_t>(_stores.size());
    }

}