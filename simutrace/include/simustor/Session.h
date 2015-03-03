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
#pragma once
#ifndef SESSION_H
#define SESSION_H

#include "SimuBase.h"
#include "SimuStorTypes.h"
#include "Store.h"

namespace SimuTrace
{

    class SessionManager;
    class StreamBuffer;
    class Stream;

    class Session
    {
    public:
        typedef ObjectReference<Session> Reference;

        static Reference makeOwnerReference(Session* session);
    private:
        DISABLE_COPY(Session);

        SessionManager& _manager;
        const uint16_t _peerApiVersion;
        const SessionId _localId;

        uint32_t _referenceCount;
        bool _isAlive;

        libconfig::Config _config;
        std::unique_ptr<Configuration::LockList> _configLockList;
        LogCategory _log;
        Environment _environment;

        Store::Reference _store;

        void _initializeConfiguration(libconfig::Config* rootConfig);

        bool _dodetach();
        void _closeStore();
    protected:
        mutable CriticalSection _lock;
        mutable ReaderWriterLock _storeLock;

        Session(SessionManager& manager, uint16_t clientVersion,
                SessionId localId, const Environment& root);

        virtual void _attach(std::unique_ptr<Port>& port) = 0;
        virtual bool _detach(bool whatif) = 0;

        void dropReferences();

        virtual Store::Reference _createStore(const std::string& specifier,
                                              bool alwaysCreate = false) = 0;
        virtual Store::Reference _openStore(const std::string& specifier) = 0;

        virtual void _close() = 0;

        virtual void _enumerateStores(std::vector<std::string>& out) const = 0;

        virtual void _applySetting(const std::string& setting);

        void _setConfigLockList(std::unique_ptr<Configuration::LockList>& lockList);

        Store* _getStore() const;
    public:
        virtual ~Session();

        void attach(std::unique_ptr<Port>& port);
        void detach();

        void close();

        // Store Create Behavior.
        //    Store Exists    AlwaysCreate    Behavior
        //        0                0            New Store
        //        1                0            Fail
        //        0                1            New Store
        //        1                1            New Store / Drop Old
        void createStore(const std::string& specifier,
                         bool alwaysCreate = false);
        void openStore(const std::string& specifier);
        void closeStore();

        BufferId registerStreamBuffer(size_t segmentSize, uint32_t numSegments);
        StreamId registerStream(StreamDescriptor& desc, BufferId buffer);

        void enumerateStreamBuffers(std::vector<BufferId>& out) const;
        void enumerateStreams(std::vector<StreamId>& out, bool includeHidden) const;
        void enumerateStores(std::vector<std::string>& out) const;

        StreamBuffer& getStreamBuffer(BufferId id) const;
        Stream& getStream(StreamId id) const;

        uint16_t getPeerApiVersion() const;
        SessionId getLocalId() const;

        void applySetting(const std::string& setting);

        const Environment& getEnvironment() const;

        bool isAlive() const;
    };

}

#endif