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

#include "Utils.h"

#include "Exceptions.h"
#include "Clock.h"

namespace SimuTrace
{

// Disable deprecated warning for vsnprintf
#pragma warning(push)
#pragma warning(disable : 4996)

    std::string stringFormat(const std::string fmt, ...)
    {
        va_list va;
        va_start(va, fmt);

        std::string result = stringFormatVa(fmt, va);

        va_end(va);

        return result;
    }

    std::string stringFormatVa(const std::string fmt, va_list list)
    {
        int size = 128;
        char* buffer = new char[size];

        while (1) {
            va_list list_copy;
            va_copy(list_copy, list);

            int n = ::vsnprintf(buffer, size, fmt.c_str(), list_copy);

            va_end(list_copy);

            if ((n > -1) && (n < size)) {
                std::string str(buffer);
                delete[] buffer;

                return str;
            }

            // Resize buffer
            size = (n > -1) ? n + 1 : size * 2;

            delete[] buffer;
            buffer = new char[size];
        }
    }

    const std::string& getLineEnding()
    {
        static std::string lineEnding;

        if (lineEnding.empty()) {
            std::ostringstream endline;
            endline << std::endl;

            lineEnding = endline.str();
        }

        return lineEnding;
    }

    std::string sizeToString(uint64_t size, SizeUnit maxUnit)
    {
        static const char* prefix[7] {
            "bytes", // 0 bytes
            "byte",  // 1 byte
            "bytes", // n bytes
            "KiB",
            "MiB",
            "GiB",
            "TiB"
        };

        uint32_t idx;
        if (size > 1) {
            idx = 2;
            while ((idx < static_cast<uint32_t>(maxUnit)) && (size >= 1024)) {
                size = size >> 10; // divide by 1024
                idx++;
            }
        } else {
            idx = static_cast<uint32_t>(size); // size is either 0 or 1
        }

        return stringFormat("%llu %s", size, prefix[idx]);
    }

#pragma warning(pop)

    void generateGuid(Guid& guid)
    {
        int i;

    #if defined(_WIN32)
        // In Win32 the seed is thread-local. To improve the chance that
        // the seeds among threads differ, we simply use the current high
        // resolution tick count as seed
        srand(static_cast<uint32_t>(Clock::getTicks()));
    #endif

        guid.data1 = (uint32_t)rand();
        guid.data2 = (uint16_t)rand();
        guid.data3 = (uint16_t)rand();

        for (i = 0; i < 8; ++i) {
            guid.data4[i] = (uint8_t)rand();
        }
    }

    std::string guidToString(const Guid& guid, bool data1Only)
    {
        // Example Output: {27158148-9814-47DB-8E50-E39573EA40AE}
        static const std::string& format =
            "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";
        static const std::string& shortFormat =
            "{%08X}";

        return stringFormat((data1Only) ? shortFormat : format,
            guid.data1, guid.data2, guid.data3,
            guid.data4[0], guid.data4[1], guid.data4[2], guid.data4[3],
            guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7]);
    }

namespace System {

    Handle duplicateHandle(Handle handle)
    {
        if (handle == INVALID_HANDLE_VALUE) {
            return INVALID_HANDLE_VALUE;
        }

        Handle dupHandle;
    #if defined(_WIN32)
        if (!::DuplicateHandle(::GetCurrentProcess(), handle,
                               ::GetCurrentProcess(), &dupHandle, 0, FALSE,
                               DUPLICATE_SAME_ACCESS)) {
            Throw(PlatformException);
        }
    #else
        dupHandle = ::dup(handle);
        ThrowOn(dupHandle == INVALID_HANDLE_VALUE, PlatformException);
    #endif

        return dupHandle;
    }

    void closeHandle(Handle handle)
    {
        if (handle == INVALID_HANDLE_VALUE) {
            return;
        }

    #if defined(_WIN32)
        ::CloseHandle(handle);
    #else
        ::close(handle);
    #endif
    }

#if defined(_WIN32)
#else
    void maskSignal(uint32_t signal)
    {
        sigset_t set;
        memset(&set, 0, sizeof(sigset_t));

        sigaddset(&set, signal);

        int result = ::pthread_sigmask(SIG_BLOCK, &set, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    }

    void unmaskSignal(uint32_t signal)
    {
        sigset_t set;
        memset(&set, 0, sizeof(sigset_t));

        sigaddset(&set, signal);

        // Consume any pending signals and unmask the signal
        // TODO: Do we need this?
        //timespec timeout = {0};
        //sigtimedwait(&set, nullptr, &timeout);

        int result = ::pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    }

    void ignoreSignal(uint32_t signal)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));

        act.sa_handler = SIG_IGN;

        int result = ::sigaction(signal, &act, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    }

    void restoreSignal(uint32_t signal)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));

        act.sa_handler = SIG_DFL;

        int result = ::sigaction(signal, &act, nullptr);
        ThrowOn(result != 0, PlatformException, result);
    }
#endif

}
}