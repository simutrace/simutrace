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

#include "Clock.h"

#include "SimuBaseTypes.h"
#include "Utils.h"
#include "Exceptions.h"

namespace SimuTrace {
namespace Clock
{

    uint64_t getTicks()
    {
    #if defined(_WIN32)
        // Number of ticks in a second
        static uint64_t freq = 0;

        if (freq == 0) {
            LARGE_INTEGER li;
            if (!::QueryPerformanceFrequency(&li)) {
                Throw(PlatformException);
            }

            freq = li.QuadPart;
        }

        LARGE_INTEGER li;
        if (!::QueryPerformanceCounter(&li)) {
            Throw(PlatformException);
        }

        return (li.QuadPart * 1000000000ULL) / freq;
    #elif (defined(__MACH__) && defined(__APPLE__))
        static mach_timebase_info_data_t timebase = {0};

        if (timebase.denom == 0) {
            (void)mach_timebase_info(&timebase);
        }

        return ::mach_absolute_time() * timebase.numer / timebase.denom;
    #else
        struct timespec ts;
        if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            Throw(PlatformException);
        }

        return (ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
    #endif
    }

    Timestamp getTimestamp()
    {
    #if defined(_WIN32)
        FILETIME ft;
        ::GetSystemTimeAsFileTime(&ft);

        Timestamp ts = ft.dwHighDateTime;
        ts = (ts << 32) | ft.dwLowDateTime;

        // Convert to UNIX epoch (1970-01-01 00:00:00) from Windows NT FILETIME
        return (ts - 116444736000000000ULL) * 100;
    #elif (defined(__MACH__) && defined(__APPLE__))
        kern_return_t result;
        clock_serv_t clock;
        mach_timespec_t mts;
        result = ::host_get_clock_service(::mach_host_self(), CALENDAR_CLOCK,
                                          &clock);

        ThrowOn(result != KERN_SUCCESS, Exception,
                stringFormat("Failed to receive calender clock '%s'.",
                    ::mach_error_string(result)));

        result = ::clock_get_time(clock, &mts);

        ThrowOn(result != KERN_SUCCESS, Exception,
                stringFormat("Failed to receive clock value '%s'.",
                    ::mach_error_string(result)));

        ::mach_port_deallocate(::mach_task_self(), clock);

        Timestamp ts = mts.tv_sec;
        ts *= 1000000000ULL;
        ts += mts.tv_nsec;

        return ts;
    #else
        struct timespec tspec;
        if (::clock_gettime(CLOCK_REALTIME, &tspec) != 0) {
            Throw(PlatformException);
        }

        Timestamp ts = tspec.tv_sec;
        ts *= 1000000000ULL;
        ts += tspec.tv_nsec;

        return ts;
    #endif
    }

    std::string formatTime(const std::string& format, const Timestamp ts)
    {
        // Convert timestamp to struct tm so we can use it with strftime()
        struct tm time;
        time_t t = ts / 1000000000ULL;
    #if defined(_WIN32)
        ::localtime_s(&time, &t);
    #else
        ::localtime_r(&t, &time);
    #endif

        // Check the input format for the custom '%~' to print milliseconds
        // If we find it, we build a new format string that already includes
        // the milliseconds as string.
        char str[128];

        auto pos = format.find("%~");
        if (pos != std::string::npos) {
            Timestamp milliseconds = (ts / 1000000ULL) % 1000;
            std::ostringstream formatStream;

            formatStream << format.substr(0, pos)
                         << std::setw(3) << std::setfill('0')
                         << milliseconds
                         << format.substr(pos + 2);

            ::strftime(str, sizeof(str), formatStream.str().c_str(), &time);
        } else {
            ::strftime(str, sizeof(str), format.c_str(), &time);
        }

        return std::string(str);
    }

    std::string formatTimeIso8601(const Timestamp ts)
    {
        return formatTime("%Y-%m-%d %H:%M:%S,%~", ts);
    }

}
}