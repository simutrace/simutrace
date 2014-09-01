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
        friend class ServerStore;
    private:
        struct StorePrefixDescriptor;

    private:
        DISABLE_COPY(ServerStoreManager);

        mutable ReaderWriterLock _lock;

        IdAllocator<StoreId> _storeIdAllocator;
        std::map<StoreId, Store::Reference> _stores;

        void _releaseStore(StoreId store); // Called from the store

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

        Store::Reference createStore(ServerSession& session, 
                                     const std::string& specifier,
                                     bool alwaysCreate);

        static void enumerateStores(const ServerSession& session, 
                                    std::vector<std::string>& out);

        ServerStore& getStore(StoreId id) const;

        uint32_t getOpenStoresCount() const;
    };

}

#endif