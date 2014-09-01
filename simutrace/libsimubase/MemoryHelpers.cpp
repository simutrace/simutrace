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
#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

#include "MemoryHelpers.h"

#include "Exceptions.h"

namespace SimuTrace {
namespace System
{

    const int getPageSize()
    {
        static int _pageSize = 0;

        if (_pageSize <= 0) {
        #ifdef WIN32
            SYSTEM_INFO sysinfo;
            ::GetSystemInfo(&sysinfo);
            _pageSize = sysinfo.dwPageSize;
        #else
            _pageSize = ::sysconf(_SC_PAGE_SIZE);
            ThrowOn(_pageSize == -1, PlatformException);
        #endif
        }

        return _pageSize;
    }

    const int getMemoryAllocationGranularity()
    {
        static int _allocationGranularity = 0;

        if (_allocationGranularity <= 0) {
        #ifdef WIN32
            SYSTEM_INFO sysinfo;
            ::GetSystemInfo(&sysinfo);
            _allocationGranularity = sysinfo.dwAllocationGranularity;
        #else
            _allocationGranularity = getPageSize();
        #endif
        }

        return _allocationGranularity;
    }

    const uint64_t getAvailablePhysicalMemory()
    {
        uint64_t ret = 0;
    #ifdef WIN32
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (!::GlobalMemoryStatusEx(&memstat)) {
            Throw(PlatformException);
        }

        ret = memstat.ullAvailPhys;
    #else
        // In Linux we have to read /proc/meminfo to get the available amount
        // of physical memory including the caches
        std::string field;
        std::string unit;
        std::string line;
        uint64_t data;

        std::ifstream file("/proc/meminfo", std::ifstream::in);
        ThrowOn(!file, PlatformException, EACCES);

        while (std::getline(file, line)) {
            std::istringstream(line) >> field >> data >> unit;

            if ((field.compare("MemFree:") == 0) ||
                (field.compare("Buffers:") == 0) ||
                (field.compare("Cached:") == 0)) {

                assert(!unit.empty() &&
                       ((unit[0] == 'k') || (unit[0] == 'K')));

                ret += data;
            }
        }

        ret *= 1024;
    #endif

        return ret;
    }

    const uint64_t getPhysicalMemory()
    {
        uint64_t ret;
    #ifdef WIN32
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (!::GlobalMemoryStatusEx(&memstat)) {
            Throw(PlatformException);
        }

        ret = memstat.ullTotalPhys;
    #else
        ret = ::sysconf(_SC_PHYS_PAGES) * getPageSize();
        ThrowOn(ret == -1, PlatformException);
    #endif

        return ret;
    }

}
}