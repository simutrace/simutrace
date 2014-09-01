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
#include "Exceptions.h"

namespace SimuTrace {
namespace Clock 
{

    const uint64_t getTimerFrequency()
    {
        static uint64_t freq = 0;

        if (freq == 0) {
        #ifdef WIN32
            LARGE_INTEGER li;
            if (!::QueryPerformanceFrequency(&li)) {
                Throw(PlatformException);
            }

            freq = li.QuadPart;
        #else
            freq = 1e+9;
        #endif
        }

        return freq;
    }

    const uint64_t getTicks()
    {
    #ifdef WIN32
        LARGE_INTEGER li;
        if (!::QueryPerformanceCounter(&li)) {
            Throw(PlatformException);
        }

        return li.QuadPart;
    #else
        struct timespec ts;
        if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            Throw(PlatformException);
        }

        return (ts.tv_sec * 1e+9) + ts.tv_nsec;
    #endif
    }

    const TimeType getTime()
    {
        return time(nullptr);
    }

    // Disables the deprecated warnings for time functions
#pragma warning(push)
#pragma warning(disable : 4996)
    const Timestamp getTimestamp()
    {
    #ifdef WIN32
        LARGE_INTEGER tmp2;
        if (!::QueryPerformanceCounter(&tmp2)) {
            Throw(PlatformException);
        }
        return ((tmp2.QuadPart * 10000000) / getTimerFrequency()) +
            116444736000000000;
    #else
        // Higher resolution than time
        struct timespec tspec;
        if (::clock_gettime(CLOCK_MONOTONIC, &tspec) != 0) {
            Throw(PlatformException);
        }

        Timestamp ts = timeToTimestamp(tspec.tv_sec);
        ts += tspec.tv_nsec / 100;
        return ts;
    #endif
    }
#pragma warning(pop)

    const double ticksToSeconds(uint64_t ticks)
    {
        return (double)ticks / (double)getTimerFrequency();
    }

    const Timestamp timeToTimestamp(const TimeType t)
    {
        //TDOO: Add comment: Conversion from UNIX time to FILETIME    
#ifdef WIN32
        return Int32x32To64(t, 10000000) + 116444736000000000;
#else
        return (Timestamp)(t * 10000000) + 116444736000000000;
#endif
    }

    const TimeType timestampToTime(const Timestamp ts)
    {
        TimeType ll = ts - 116444736000000000;
        return ll / 10000000;
    }

    std::string formatTime(const std::string& format, const Timestamp ts)
    {
        // Convert timestamp to struct tm so we can use it with strftime()
        struct tm time;
        TimeType t = timestampToTime(ts);
    #ifdef WIN32
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
            Timestamp milliseconds = (ts / 10000) % 1000;
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