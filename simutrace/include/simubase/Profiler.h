/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Base Library (libsimubase) is part of Simutrace.
 *
 * libsimubase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libsimubase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef PROFILER_H
#define PROFILER_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "Thread.h"
#include "File.h"
#include "CriticalSection.h"

#include "ProfileEntry.h"

namespace SimuTrace 
{

    template<typename T> class ProfileVariableReference;
    template<typename T, typename C> class ProfileMemberDelegate0;

    class ProfileContext 
    {
    private:

        struct PEntry {
            ProfileEntry* entry;
            bool isOwner;
        };

        std::vector<PEntry> _entries;
    public:
        ~ProfileContext()
        {
            for (int i = 0; i < _entries.size(); ++i) {
                if (_entries[i].isOwner) {
                    assert(_entries[i].entry != nullptr);
                    delete _entries[i].entry;
                }
            }

            _entries.clear();
        }

        void add(ProfileEntry& entry)
        {
            PEntry pentry;

            pentry.entry = &entry;
            pentry.isOwner = false;

            _entries.push_back(pentry);
        }

        template<typename T>
        void add(const char* name, T& variable, bool delta)
        {
            PEntry entry;

            entry.entry = new ProfileVariableReference<T>(name, variable, delta);
            entry.isOwner = true;

            _entries.push_back(entry);
        }

        template<typename T, typename C>
        void add(const char* name, C& instance, T (C::*func)() const) 
        {
            PEntry entry;

            entry.entry = new ProfileMemberDelegate0<T, C>(name, instance, func);
            entry.isOwner = true;

            _entries.push_back(entry);
        }

        template<typename T, typename C>
        void add(const char* name, C& instance, T (C::*func)()) 
        {
            PEntry entry;

            entry.entry = new ProfileMemberDelegate0<T, C>(name, instance, func);
            entry.isOwner = true;

            _entries.push_back(entry);
        }

        ProfileEntry& get(uint32_t index) const
        {
            assert(index < _entries.size());
            return *_entries[index].entry;
        }

        std::vector<PEntry>::size_type getCount() const
        {
            return _entries.size();
        }
    
    };

    class Profiler
    {
    private:
        enum ProfileState {
            PsRun,
            PsPause,
            PsStop
        };
    private:
        DISABLE_COPY(Profiler);

        CriticalSection _lock;

        File _file;
        
        Thread<Profiler&> _writeThread;
        uint32_t _writeInterval;
        volatile ProfileState _state;

        bool _headerWritten;
        uint32_t _pentryCount;

        ProfileContext* _context;
        bool _contextOwner;

        static int _writeThreadMain(Thread<Profiler&>& thread);
    public:
        Profiler(const std::string& csvFileName);
        Profiler(const std::string& csvFileName, ProfileContext& context, 
                 uint32_t writeInterval);
        ~Profiler();

        void start();
        void pause();
        void stop();

        void getContext(ProfileContext& context);

        void collect(ProfileContext& context);
    };

}
#endif