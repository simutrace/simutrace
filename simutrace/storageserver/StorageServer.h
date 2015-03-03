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
#ifndef STORAGE_SERVER_H
#define STORAGE_SERVER_H

#include "SimuStor.h"

#pragma warning(push)
#pragma warning(disable : 4290)
#include <libconfig.h++>
#pragma warning(pop)

namespace SimuTrace
{

    class WorkerPool;
    class ServerStreamBuffer;
    class ServerSessionManager;
    class ServerStoreManager;

    class StorageServer
    {
    private:
        class Request;
        struct Binding;

        typedef void(*RequestHandler)(Request&, Message&);

    private:
        DISABLE_COPY(StorageServer);

        std::vector<std::unique_ptr<Binding>> _bindings;
        std::unique_ptr<WorkerPool> _requestPool;

        std::unique_ptr<WorkerPool> _workerPool;
        std::unique_ptr<ServerStreamBuffer> _memoryPool;

        std::unique_ptr<ServerSessionManager> _sessionManager;
        std::unique_ptr<ServerStoreManager> _storeManager;

        volatile bool _shouldStop;

        std::string _workspace;
        std::string _specifier;

        libconfig::Config _config;
        LogCategory _logRoot;
        Environment _environment;
    private:
        StorageServer(int argc, const char* argv[]);
        ~StorageServer();

        void _determineMemoryPoolConfiguration(uint32_t& numSegments);

        void _initialize(int argc, const char* argv[]);
        void _initializeLogging(int phase);
        bool _initializeConfiguration(int argc, const char* argv[]);
        void _initializeServer();
        void _applyConfiguration();
        void _finalizeServer();

        void _stop();
        void _run();

        void _setWorkspace(const std::string& workspace);
    private:
        static const uint32_t _version;
        static const uint16_t _rpcversion;

        static StorageServer* _instance;

        static std::map<int, RequestHandler> _handlers;

        static void _initializeHandlerMap();
        static void _init(int argc, const char* argv[]);
        static void _free();

        static void _logBanner();

        static void _handlePeekVersion(Request& request, Message& msg);
        static void _handleEnumerateSessions(Request& request, Message& msg);

        static void _handleSessionCreate(Request& request, Message& msg);
        static void _handleSessionOpen(Request& request, Message& msg);
        static void _handleSessionQuery(Request& request, Message& msg);

        static void _processRequest(Request& request);

        static int _bindingThreadMain(Thread<Binding*>& thread);
    public:
        WorkerPool& getWorkerPool();
        ServerStreamBuffer& getMemoryPool();
        ServerSessionManager& getSessionManager();
        ServerStoreManager& getStoreManager();

        static int run(int argc, const char* argv[]);

        static const uint32_t getVersion();
        static StorageServer& getInstance();

        static std::string makePath(const std::string* directory,
                                    const std::string* fileName);

        const Environment& getEnvironment() const;
        bool hasLocalBindings() const;
    };

}

#endif // STORAGE_SERVER_H