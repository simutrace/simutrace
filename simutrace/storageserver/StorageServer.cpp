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
#include "Version.h"

#include "StorageServer.h"

#include "ServerSessionManager.h"
#include "ServerStoreManager.h"
#include "ServerStreamBuffer.h"
#include "WorkerPool.h"
#include "WorkItem.h"

#include "SimuBaseOptions.h"
#include "SimuStorOptions.h"
#include "StorageServerOptions.h"

using namespace libconfig;

namespace SimuTrace
{

    static const char* _logPattern = "[%d{%Y-%m-%d %H:%M:%S.%~}] %c %s%m%n";

    class StorageServer::Request :
        public WorkItemBase
    {
    private:
        StorageServer& _server;
        std::unique_ptr<Port> _port;
    public:
        Request(StorageServer& server, std::unique_ptr<Port>& port) :
            _server(server),
            _port()
        {
            ThrowOnNull(dynamic_cast<ServerPort*>(port.get()),
                        ArgumentNullException);

            _port = std::move(port);
        }

        virtual void execute() { StorageServer::_processRequest(*this); }

        StorageServer& getStorageServer() const { return _server; }
        std::unique_ptr<Port>& getPort() { return _port; }

        ServerPort& getServerPort()
        {
            return *static_cast<ServerPort*>(_port.get());
        }
    };

    struct StorageServer::Binding
    {
        std::unique_ptr<Thread<Binding*>> thread;
        std::unique_ptr<ServerPort> port;
    };

    const uint32_t StorageServer::_version = SIMUTRACE_VERSION;
    const uint16_t StorageServer::_rpcversion = RPC_VERSION;

    StorageServer* StorageServer::_instance = nullptr;

    std::map<int, StorageServer::RequestHandler> StorageServer::_handlers;

    StorageServer::StorageServer(int argc, const char* argv[]) :
        _bindings(),
        _requestPool(nullptr),
        _workerPool(nullptr),
        _memoryPool(nullptr),
        _sessionManager(nullptr),
        _storeManager(nullptr),
        _shouldStop(true),
        _workspace(),
        _specifier(),
        _config(),
        _logRoot(""),
        _environment()
    {
        _environment.log = &_logRoot;
        _environment.config = &_config;

        _initialize(argc, argv);
    }

    StorageServer::~StorageServer()
    {
        _finalizeServer();
    }

    void StorageServer::_determineMemoryPoolConfiguration(uint32_t& numSegments)
    {
        const uint64_t segmentSize = SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB;
        const uint64_t recSize = SIMUTRACE_SERVER_MEMMGMT_RECOMMENDED_POOLSIZE MiB;
        const uint64_t minSize = SIMUTRACE_SERVER_MEMMGMT_MINIMUM_POOLSIZE MiB;

        uint64_t confPoolSize = Configuration::get<int>("server.memmgmt.poolSize") MiB;
        uint64_t poolSize = confPoolSize;

        ThrowOn(poolSize < SIMUTRACE_SERVER_MEMMGMT_MINIMUM_POOLSIZE,
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

    void StorageServer::_initialize(int argc, const char* argv[])
    {

        try {
            //
            // Step 1: Initialize logging so we can report errors.
            //

            _initializeLogging(0);


            //
            // Step 2: Initialize configuration so we can apply user settings.
            //

            if (!_initializeConfiguration(argc, argv)) {
                // The user specified a parameter that demands an early stop
                // such as "help" or an error occurred for which we handled
                // the exception but want to stop.

                return;
            }

            _initializeLogging(1);


            // Apply server configuration
            _applyConfiguration();


            //
            // Step 3: Initialize worker pool and control port.
            //

            _initializeServer();

        } catch (const std::exception& e) {
            LogFatal("Failed to initialize server. The exception is '%s'. "
                     "Aborting.", e.what());

            _finalizeServer();

            throw;
        }

    }

    void StorageServer::_initializeLogging(int phase)
    {
        if (phase == 0) {
            //
            // Step 1: Initialize logging so we can report any errors to the
            //         user. Until the configuration is successfully loaded,
            //         we are only logging to stdout.
            //

            std::unique_ptr<LogLayout> layout(new PatternLogLayout(_logPattern));
            std::shared_ptr<LogAppender> consoleLog =
                std::make_shared<TerminalLogAppender>("terminal", layout, true);

            _logRoot.registerAppender(consoleLog);

            // Activate logging to the new log
            Environment::setLog(&_logRoot);

        } else { // Adapt logging according to configuration
            assert(phase == 1);

            std::string logfile;
            Configuration::get("server.logfile", logfile);
            if (!logfile.empty()) {
                std::unique_ptr<LogLayout> layout(
                    new PatternLogLayout(_logPattern));
                std::shared_ptr<LogAppender> fileLog =
                    std::make_shared<FileLogAppender>(logfile, layout, true);

                _logRoot.registerAppender(fileLog);
            }

            // Apply --server.quiet setting so we can determine if we should
            // print the banner or not.
            if (Configuration::get<bool>("server.quiet")) {
                _logRoot.enableAppender("terminal", false);
            }

            // Apply --server.logNoColor
            if (!Configuration::get<bool>("server.logNoColor")) {
                std::vector<std::shared_ptr<LogAppender>> appenders;

                _logRoot.enumerateAppenders(appenders);

                for (auto appender : appenders) {
                    TerminalLogAppender* terminalLog =
                        dynamic_cast<TerminalLogAppender*>(appender.get());

                    if (terminalLog != nullptr) {
                        terminalLog->setEnableColor(true);
                    }
                }
            }

            _logBanner();
        }
    }

    bool StorageServer::_initializeConfiguration(int argc, const char* argv[])
    {
        Environment::setConfig(&_config);

        //
        // Step 2: Initialize configuration. We first add options from all
        //         components to the option parser. We then can apply the
        //         detected options to the configuration. Options set via
        //         the command line always supersede options set in the
        //         configuration file.
        //

        Options::SettingsTypeMap typeMap;
        ez::ezOptionParser options;
        ez::OptionGroup* group;

        Options::addSimuBaseOptions(options, typeMap);
        Options::addSimuStorOptions(options, typeMap);
        Options::addStorageServerOptions(options, typeMap);

        options.parse(argc, argv);

        std::vector<std::string> badOptions;
        if (!options.gotRequired(badOptions)) {
            for (auto i = 0; i < badOptions.size(); ++i) {
                LogFatal("Missing required option '%s'.", badOptions[i].c_str());
            }

            Throw(OptionException, "One or more required options are missing.");
        }

        if (!options.gotExpected(badOptions)) {
            for (auto i = 0; i < badOptions.size(); ++i) {
                ez::OptionGroup* group = options.get(badOptions[i].c_str());
                assert(group != nullptr);

                LogFatal("Invalid number of arguments for option '%s'. "
                         "Expected %d arguments.", badOptions[i].c_str(),
                         group->expectArgs);
            }

            Throw(OptionException, "One or more invalid options specified.");
        }

        if (options.isSet(OPT_LONG_PREFIX "help")) {
            // TODO: print help

            return false;
        }

        // Check if the user supplied the quiet parameter. In that case we
        // need to disable logging already at this point, to prevent
        // the printing of debug messages when updating settings.
        if (options.isSet(OPT_LONG_PREFIX "server.quiet")) {
            _logRoot.enableAppender("terminal", false);
        }

        // Print a warning for unrecognized args. We do not use any positional
        // arguments.
        for (int i = 1; i < options.firstArgs.size(); ++i) {
            LogWarn("Ignoring unrecognized option: %s.",
                    options.firstArgs[i]->c_str());
        }
        for (int i = 0; i < options.unknownArgs.size(); ++i) {
            LogWarn("Ignoring unrecognized option: %s.",
                    options.unknownArgs[i]->c_str());
        }
        for (int i = 0; i < options.lastArgs.size(); ++i) {
            LogWarn("Ignoring unrecognized option: %s.",
                    options.lastArgs[i]->c_str());
        }

        // Get the workspace path. We will use this path as the base for all
        // further file operations (load configuration file, store trace etc.).
        group = options.get(OPT_LONG_PREFIX "server.workspace");
        assert(group != nullptr);

        if (group->isSet) {
            std::string workspace;
            group->getString(workspace);

            _setWorkspace(workspace);
        }

        // Check if the user specified a custom configuration file.
        std::string configFile;
        group = options.get(OPT_LONG_PREFIX "server.configuration");
        assert(group != nullptr);

        group->getString(configFile);

        if (!configFile.empty()) {
            // We use the directory containing the config file as include
            // path for referenced config files.

            try {
                std::string path = Directory::getAbsolutePath(configFile);
                path = Directory::getDirectory(path);

                _config.setIncludeDir(path.c_str());
                _config.readFile(configFile.c_str());
            } catch (const std::exception& e) {
                LogError("Cannot read configuration file '%s'. "
                         "The exception is '%s'.",
                         configFile.c_str(), e.what());

                throw;
            }
        }

        // Add or overwrite all settings in the configuration file with
        // command line settings.
        std::string prefix(OPT_LONG_PREFIX);
        auto prefixLength = prefix.length();

        for (auto it = options.groups.begin(); it != options.groups.end(); ++it) {
            ez::OptionGroup* group = (*it);
            assert(group->flags.size() > 0);

            // Get the long option name (by convention the last flag) and
            // remove the prefix string (e.g., "--" or "/"). We can then use
            // the name to get or create the setting.
            std::string* flag = group->flags[group->flags.size() - 1];
            std::string option = flag->substr(prefixLength);
            assert(!option.empty());

            auto typeIt = typeMap.find(option);
            if (typeIt == typeMap.end()) {
                // We ignore settings that are not in the type map. That
                // filters command line options such as "help".
                continue;
            }

            if (_config.exists(option)) {
                Setting& setting = _config.lookup(option);
                if (setting.getType() == typeIt->second) {
                    if (!group->isSet) {
                        // The setting is configured in the file, but not set
                        // on the command line. Proceed with the next option,
                        // so we do not overwrite the setting's value with the
                        // default one.
                        LogDebug("Setting: %s = %s", option.c_str(),
                            Configuration::settingToString(setting).c_str());

                        continue;
                    }
                } else {
                    LogWarn("Type mismatch for setting '%s'. "
                            "Expected '%s', but found '%s'.",
                            option.c_str(),
                            Configuration::settingTypeToString(typeIt->second),
                            Configuration::settingTypeToString(setting.getType()));
                }
            }

            try {
                // Set the setting's value. Unfortunately, ezOptionParser does
                // not provide any means to check the type of the supplied
                // option value before accessing it; potentially leading to
                // undefined behavior (e.g., through atoi())!

                //TODO: Improve exception handling

                switch (typeIt->second)
                {
                    case Setting::Type::TypeString: {
                        std::string out;
                        group->getString(out);

                        Configuration::set(option, out, true);
                        break;
                    }

                    case Setting::Type::TypeInt: {
                        int out;
                        group->getInt(out);

                        Configuration::set(option, out, true);
                        break;
                    }

                    case Setting::Type::TypeInt64: {
                        long long out;
                        group->getLongLong(out);

                        Configuration::set(option, out, true);
                        break;
                    }

                    case Setting::Type::TypeBoolean: {
                        Configuration::set(option, group->isSet, true);
                        break;
                    }

                    default: {
                        assert(false);
                        break;
                    }
                }
            } catch (...) {
                LogWarn("Failed to apply setting '%s'. Expected type is '%s'.",
                        option.c_str(),
                        Configuration::settingTypeToString(typeIt->second));
            }
        }

        return true;
    }

    void StorageServer::_initializeServer()
    {
        //
        // Step 3: Initialize server components. After this step we are ready
        //         to accept client connections in the main loop.
        //

        _sessionManager = std::unique_ptr<ServerSessionManager>(
            new ServerSessionManager());
        _storeManager = std::unique_ptr<ServerStoreManager>(
            new ServerStoreManager());

        int rworkers = Configuration::get<int>("server.requestworkerpool.size");
        _requestPool = std::unique_ptr<WorkerPool>(
            new WorkerPool(rworkers, _environment));

        LogInfo("Created %d server request worker threads.",
                _requestPool->getWorkerCount());

        // Create the worker pool that will process trace data. Since we do not
        // want worker threads to slow down session threads or worker threads
        // in the request pool, we reduce the worker threads' priority.
       int workers = Configuration::get<int>("server.workerpool.size");
        _workerPool = std::unique_ptr<WorkerPool>(
            new WorkerPool(workers, _environment));

        LogInfo("Created %d server processing worker threads.",
                _workerPool->getWorkerCount());

    #if defined(_WIN32)
        _workerPool->setPoolPriority(THREAD_PRIORITY_BELOW_NORMAL);
    #else
        // On Linux, higher priority values mean lower scheduling
        // priority.

        //TODO: Invalid argument exception
        //_workerPool->setPoolPriority(1);
    #endif

        // Create memory pool
        uint32_t nseg;
        _determineMemoryPoolConfiguration(nseg);

        _memoryPool = std::unique_ptr<ServerStreamBuffer>(
            new ServerStreamBuffer(SERVER_BUFFER_ID,
                                   SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB,
                                   nseg, false));

        LogInfo("Created server memory pool with %s (%s per segment).",
                sizeToString(_memoryPool->getBufferSize()).c_str(),
                sizeToString(SIMUTRACE_MEMMGMT_SEGMENT_SIZE MiB).c_str());

        // Create the specified server bindings
        std::string specifier(_specifier);
        while (!specifier.empty()) {
            std::string addr;

            auto pos = specifier.find(',');
            if (pos != std::string::npos) {
                addr = specifier.substr(0, pos);
                specifier = specifier.substr(pos + 1);
            } else {
                addr.swap(specifier);
            }

            std::unique_ptr<Binding> binding(new Binding());
            Binding* pbinding = binding.get();

            pbinding->port = std::unique_ptr<ServerPort>(
                new ServerPort(addr));
            pbinding->thread = std::unique_ptr<Thread<Binding*>>(
                new Thread<Binding*>(_bindingThreadMain, pbinding));

            _bindings.push_back(std::move(binding));

            LogInfo("Created server binding '%s'.", addr.c_str());
        }
        assert(!_bindings.empty());

        // Everything set up. Allow to call run().
        _shouldStop = false;
    }

    void StorageServer::_applyConfiguration()
    {
        // --server.loglevel
        LogPriority::Value loglevel;
        Configuration::get("server.loglevel", loglevel);

        LogDebug("Setting log level to %s (%d).",
            LogPriority::getPriorityName(loglevel).c_str(), loglevel);

        _logRoot.setPriorityThreshold(loglevel);

        // --server.workspace
        // If the workspace is still empty, we were not given a workspace via
        // command line. Read it from the configuration.
        if (_workspace.empty()) {
            std::string workspace;
            Configuration::get("server.workspace", workspace);

            _setWorkspace(workspace);
        }

        // --server.bindings
        _specifier = Configuration::get<std::string>("server.bindings");
        ThrowOn(_specifier.empty(), Exception, "No server bindings defined. "
                "See 'server.bindings' setting for more information.");
    }

    void StorageServer::_finalizeServer()
    {
        assert(_shouldStop);

        // Close the server's bindings. That will prevent new requests from
        // being taken.
        for (auto& binding : _bindings) {
            Thread<Binding*>* thread = binding->thread.get();

            if (thread != nullptr) {
                if (thread->isRunning()) {
                    thread->stop();
                    thread->waitForThread();
                }

                assert(!thread->isRunning());
            }

            assert(binding->port != nullptr);
        }

        _bindings.clear();

        // Close the request pool and wait for any outstanding requests to
        // be completely processed.
        if (_requestPool != nullptr) {
            _requestPool->close();

            _requestPool = nullptr;
        }

        // Instruct the session manager to close all sessions. This will close
        // the session worker threads, rundown the stores and wait for
        // pending segments to be submitted and written to disk.
        if (_sessionManager != nullptr) {
            assert(_sessionManager->getOpenSessionsCount() == 0);
            _sessionManager->close();
        }

        // Close the worker pool and end all worker threads. Since all sessions
        // should be closed by now, the worker queue must be empty.
        if (_workerPool != nullptr) {
            assert(_workerPool->getQueueLength() == 0);
            _workerPool->close();

            _workerPool = nullptr;
        }

        // All stores should be closed by now, because all sessions are closed.
        // Hence, we can free the store manager.
        if (_storeManager != nullptr) {
            assert(_storeManager->getOpenStoresCount() == 0);

            _storeManager = nullptr;
        }

        // Free the global memory pool
        _memoryPool = nullptr;

        // At this point, only the main thread should be left. It is now safe
        // to delete the session manager.
        _sessionManager = nullptr;

        // Reset the environment. Do not use Read-/WriteConfig from this point
        // on. This deactivates logging to our log category. The server's
        // destructor takes care of freeing log appenders by releasing the log
        // root. After this point, errors are only reported on stderr through
        // the PreLog environment.
        Environment::set(nullptr);
    }

    void StorageServer::_stop()
    {
        _shouldStop = true;

        // TODO: Abort all accept() calls on the control ports to wake up the
        // binding threads.
    }

    void StorageServer::_run()
    {
        if (_shouldStop) {
            return;
        }

        try {
            assert(!_bindings.empty());
            for (int i = 0; i < _bindings.size() - 1; ++i) {
                _bindings[i]->thread->start();
            }
        } catch(const std::exception& e) {
            LogFatal("Failed to start binding thread. The exception is '%s'.",
                     e.what());

            throw;
        }

        // The last binding is managed by the main thread. We let the main
        // thread adopt the thread object and run the main routine.
        _bindings[_bindings.size() - 1]->thread->adopt();
    }

    void StorageServer::_setWorkspace(const std::string& workspace)
    {
        LogInfo("Setting workspace to '%s'.", workspace.c_str());

        if (!workspace.empty()) {
            try {
                Directory::setWorkingDirectory(workspace.c_str());
            } catch (const std::exception& e) {
                LogError("Could not set workspace to '%s'. "
                         "The exception is '%s'.",
                         workspace.c_str(),
                         e.what());

                throw;
            }

            _workspace = workspace;
        } else {
            _workspace = Directory::getWorkingDirectory();
        }
    }

    void StorageServer::_initializeHandlerMap()
    {
        std::map<int, RequestHandler>& m = _handlers; // short alias

        if (m.size() > 0) {
            return;
        }

        /* =======  Main Server API  ======= */
        m[RpcApi::CCV32_Null]              = _handlePeekVersion;
        m[RpcApi::CCV32_EnumerateSessions] = _handleEnumerateSessions;

        /* =======  Session API  ======= */
        m[RpcApi::CCV32_SessionCreate]     = _handleSessionCreate;
        m[RpcApi::CCV32_SessionOpen]       = _handleSessionOpen;
        m[RpcApi::CCV32_SessionQuery]      = _handleSessionQuery;
    }

    void StorageServer::_init(int argc, const char* argv[])
    {
        assert(_instance == nullptr);

        _initializeHandlerMap();
        _instance = new StorageServer(argc, argv);
    }

    void StorageServer::_free()
    {
        // Instance might be nullptr, when we encounter a fatal exception during
        // initialization. In that case, we hit the exception handler (which
        // calls us) before the instance has been created.

        if (_instance != nullptr) {
            delete _instance;
            _instance = nullptr;
        }
    }

    void StorageServer::_logBanner()
    {
        static const char* bannerText =
            "Simutrace Storage Server v%d.%d.%d%s\n"
            "Karlsruhe Institute of Technology (KIT)\n"
            "http://simutrace.org\n";
        static const char* buildText =
    #ifdef _DEBUG
            " - Debug Build";
    #else
            "";
    #endif

        LogInfo(bannerText,
                SIMUTRACE_VER_MAJOR(_version),
                SIMUTRACE_VER_MINOR(_version),
                SIMUTRACE_VER_REVISION(_version),
                buildText);
    }

    void StorageServer::_handlePeekVersion(Request& request, Message& msg)
    {
        TEST_REQUEST_V32(Null, msg);
        ServerPort& port = request.getServerPort();

        uint16_t ver = static_cast<uint16_t>(msg.parameter0);
        (void)ver;

        LogDebug("<client: %s> Version information %d.%d",
                 port.getAddress().c_str(),
                 RPC_VER_MAJOR(ver), RPC_VER_MINOR(ver));

        port.ret(msg, RpcApi::SC_Success, _rpcversion);
    }

    void StorageServer::_handleEnumerateSessions(Request& request, Message& msg)
    {
        TEST_REQUEST_V32(EnumerateSessions, msg);
        ServerPort& port = request.getServerPort();
        StorageServer& server = request.getStorageServer();

        LogDebug("<client: %s> Session enumeration requested.",
                 port.getAddress().c_str());

        std::vector<SessionId> ids;
        server._sessionManager->enumerateSessions(ids);

        port.ret(msg, RpcApi::SC_Success, ids.data(),
                 static_cast<uint32_t>(ids.size()) * sizeof(SessionId),
                 _rpcversion, static_cast<uint32_t>(ids.size()));
    }

    void StorageServer::_handleSessionCreate(Request& request, Message& msg)
    {
        TEST_REQUEST_V32(SessionCreate, msg);
        StorageServer& server = request.getStorageServer();

        uint16_t ver = static_cast<uint16_t>(msg.parameter0);

        server._sessionManager->createSession(request.getPort(), ver);
    }

    void StorageServer::_handleSessionOpen(Request& request, Message& msg)
    {
        TEST_REQUEST_V32(SessionOpen, msg);
        StorageServer& server = request.getStorageServer();

        uint16_t ver = static_cast<uint16_t>(msg.parameter0);
        SessionId localId = static_cast<SessionId>(msg.embedded.parameter1);

        server._sessionManager->openLocalSession(localId, request.getPort(), ver);
    }

    void StorageServer::_handleSessionQuery(Request& request, Message& msg)
    {
        TEST_REQUEST_V32(SessionQuery, msg);

        Throw(NotImplementedException);
    }

    void StorageServer::_processRequest(Request& request)
    {
         // -- This method is called in the context of a worker thread --

        ServerPort& port = request.getServerPort();
        Message msg = {0};

        try {
            // Wait for the client to send a request.
            // TODO: Include timeout!
            port.wait(msg);

            // Find the handler for the given request and call it.
            uint16_t ver = static_cast<uint16_t>(msg.parameter0);
            uint32_t code = RPC_VER_AND_CODE(ver, msg.request.controlCode);

            auto it = _handlers.find(code);
            ThrowOn(it == _handlers.end(), NotSupportedException);

            RequestHandler handler = it->second;
            assert(handler != nullptr);

            handler(request, msg);

        } catch (const SocketException& e) {

            // We encountered a socket exception. This is most probably caused
            // by a failed transmission of the response. We simply drop the
            // connection without returning any further information.

            assert(request.getPort() != nullptr);

            LogError("<client: %s, code: 0x%x> SocketException: '%s'. "
                     "Dropping connection.", port.getAddress().c_str(),
                     msg.request.controlCode, e.what());

        } catch (const Exception& e) {

            // We catch all other Simutrace related exceptions and return these
            // to the caller. In any case, we drop the connection afterwards.

            // Any handler should not move the port and thus invalidate our
            // reference if an exception can occur afterwards that is caught
            // here!
            assert(request.getPort() != nullptr);

            try {
                LogWarn("<client: %s, code: 0x%x> Exception: '%s'",
                        port.getAddress().c_str(), msg.request.controlCode,
                        e.what());

                port.ret(msg, RpcApi::SC_Failed, e.what(),
                         static_cast<uint32_t>(e.whatLength()),
                         e.getErrorClass(), e.getErrorCode());

            } catch (const std::exception& e) {
                LogError("<client: %s, code: 0x%x> Failed to transmit "
                         "exception: '%s'. Dropping connection.",
                         port.getAddress().c_str(), msg.request.controlCode,
                         e.what());
            }

        } catch (const std::exception& e) {

            // We expect std::exceptions to denote internal server errors. In
            // that case, we better terminate.

            assert(request.getPort() != nullptr);

            LogFatal("Encountered fatal exception during processing of "
                     "request for client %s. The exception is '%s'. "
                     "Stopping server.", port.getAddress().c_str(),
                     e.what());

            StorageServer::getInstance()._stop();
        }
    }

    int StorageServer::_bindingThreadMain(Thread<Binding*>& thread)
    {
        StorageServer& server = StorageServer::getInstance();
        Environment::set(&server.getEnvironment());

        Binding& binding = *thread.getArgument();
        assert(binding.port != nullptr);

        LogInfo("Waiting for connections on binding %s...",
                binding.port->getAddress().c_str());

        //
        // Main request processing loop
        //

        try {

            while (!server._shouldStop) {
                // TODO: Add way to cancel accept and wake up if we should stop!
                auto port = binding.port->accept();
                assert(port != nullptr);

                std::unique_ptr<WorkItemBase> request(new Request(server, port));
                server._requestPool->submitWork(request);
            }

        } catch (const std::exception& e) {
            // We expect exceptions at this point to denote internal server
            // errors. Better terminate.

            LogFatal("Encountered fatal exception for binding %s. "
                     "The exception is '%s'. Stopping server.",
                     binding.port->getAddress().c_str(), e.what());

            server._stop();

            return -1;
        }

        return 0;
    }

    WorkerPool& StorageServer::getWorkerPool()
    {
        return *_workerPool;
    }

    ServerStreamBuffer& StorageServer::getMemoryPool()
    {
        return *_memoryPool;
    }

    ServerSessionManager& StorageServer::getSessionManager()
    {
        return *_sessionManager;
    }

    ServerStoreManager& StorageServer::getStoreManager()
    {
        return *_storeManager;
    }

    int StorageServer::run(int argc, const char* argv[])
    {
        int ret = -1;

        try
        {
            _init(argc, argv);
            _instance->_run();
            _free();

            ret = 0;
        } catch (...) {
            _free();
        }

        return ret;
    }

    const uint32_t StorageServer::getVersion()
    {
        return _version;
    }

    StorageServer& StorageServer::getInstance()
    {
        assert(_instance != nullptr);
        return *_instance;
    }

    std::string StorageServer::makePath(const std::string* directory,
                                        const std::string* fileName)
    {
        std::string path;
        const std::string* dir = nullptr;

        if ((directory == nullptr) || (directory->empty())) {
            if (Configuration::tryGet("server.workspace", path)) {
                dir = &path;
            }
        } else {
            dir = directory;
        }

        return Directory::makePath(dir, fileName);
    }

    const Environment& StorageServer::getEnvironment() const
    {
        return _environment;
    }

    bool StorageServer::hasLocalBindings() const
    {
        for (auto const &binding : _bindings) {
              ChannelCapabilities caps = binding->port->getChannelCaps();
              if (IsSet(caps, ChannelCapabilities::CCapHandleTransfer)) {
                return true;
              }
        }

        return false;
    }
}