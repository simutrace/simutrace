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
#pragma once
#ifndef SERVER_STORE_MANAGER_H
#define SERVER_STORE_MANAGER_H

#include "SimuStor.h"

namespace SimuTrace
{

    class ServerSession;
    class ServerStore;

    class ServerStoreManager
    {
    private:
        struct StorePrefixDescriptor;
        struct ServerStoreReference;
        typedef std::map<StoreId, std::shared_ptr<ServerStoreReference>> StoreMap;

    private:
        DISABLE_COPY(ServerStoreManager);

        mutable ReaderWriterLock _lock;

        uint32_t _persistentCache;

        IdAllocator<StoreId> _storeIdAllocator;
        StoreMap _stores;

        Store::Reference _createOrOpenStore(const ServerSession& session,
                                            const std::string& specifier,
                                            bool alwaysCreate,
                                            bool open);

        void _releaseStore(StoreMap::iterator& it);
        void _evictStore();
        void _closeStore(SessionId session, StoreId id);

        static void _getPrefixDescriptors(const StorePrefixDescriptor** out,
                                          uint32_t* outNumPrefixes);

        static bool _findPrefixDescriptor(const std::string& specifier,
                                          const StorePrefixDescriptor** out,
                                          bool perfectMatch);

        static std::string _getPath(const StorePrefixDescriptor& desc,
                                    const std::string& specifier);

        static bool _isValidPrefix(const std::string& prefix);
        static void _splitSpecifier(const std::string& specifier,
                                   std::string* prefix,
                                   std::string* path);

    public:
        ServerStoreManager();
        ~ServerStoreManager();

        Store::Reference createStore(const ServerSession& session,
                                     const std::string& specifier,
                                     bool alwaysCreate);
        Store::Reference openStore(const ServerSession& session,
                                   const std::string& specifier);

        void closeStore(SessionId session, StoreId id);

        static void enumerateStores(const ServerSession& session,
                                    std::vector<std::string>& out);

        ServerStore& getStore(StoreId id) const;

        uint32_t getOpenStoresCount() const;
    };

}

#endif