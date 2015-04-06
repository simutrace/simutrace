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

#include "Utils.h"
#include "Exceptions.h"

namespace SimuTrace {
namespace System
{

#if (defined(__MACH__) && defined(__APPLE__))
    void _machGetVmStats(vm_statistics_data_t* vmstats)
    {
        kern_return_t result;
        mach_msg_type_number_t info_count = HOST_VM_INFO_COUNT;
        mach_port_t mport = ::mach_host_self();

        result = ::host_statistics(mport, HOST_VM_INFO, (host_info_t)vmstats,
                                   &info_count);

        ThrowOn(result != KERN_SUCCESS, Exception,
                stringFormat("Failed to receive vm stats '%s'.",
                    ::mach_error_string(result)));

        ::mach_port_deallocate(::mach_task_self(), mport);
    }
#endif

    const int getPageSize()
    {
        static int _pageSize = 0;

        if (_pageSize <= 0) {
        #if defined(_WIN32)
            SYSTEM_INFO sysinfo;
            ::GetSystemInfo(&sysinfo);
            _pageSize = sysinfo.dwPageSize;
        #elif (defined(__MACH__) && defined(__APPLE__))
            _pageSize = vm_page_size;
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
        #if defined(_WIN32)
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
    #if defined(_WIN32)
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (!::GlobalMemoryStatusEx(&memstat)) {
            Throw(PlatformException);
        }

        ret = memstat.ullAvailPhys;
    #elif (defined(__MACH__) && defined(__APPLE__))
        vm_statistics_data_t vm_info;
        _machGetVmStats(&vm_info);

        ret = vm_info.free_count * vm_page_size;
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
    #if defined(_WIN32)
        MEMORYSTATUSEX memstat;
        memstat.dwLength = sizeof(memstat);

        if (!::GlobalMemoryStatusEx(&memstat)) {
            Throw(PlatformException);
        }

        ret = memstat.ullTotalPhys;
    #elif (defined(__MACH__) && defined(__APPLE__))
        vm_statistics_data_t vm_info;
        _machGetVmStats(&vm_info);

        ret = (vm_info.active_count + vm_info.inactive_count +
            vm_info.wire_count + vm_info.free_count) * vm_page_size;
    #else
        ret = ::sysconf(_SC_PHYS_PAGES) * getPageSize();
        ThrowOn(ret == -1, PlatformException);
    #endif

        return ret;
    }

}
}