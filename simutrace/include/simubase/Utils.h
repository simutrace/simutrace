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
#ifndef UTILS_H
#define UTILS_H

#include "SimuPlatform.h"
#include "SimuBaseTypes.h"

namespace SimuTrace
{

    //
    // String Formatting
    //

    std::string stringFormat(const std::string fmt, ...);
    std::string stringFormatVa(const std::string fmt, va_list list);
    const std::string& getLineEnding();

    enum SizeUnit {
        SuBytes = 2,
        SuKiB   = 3,
        SuMiB   = 4,
        SuGiB   = 5,
        SuTiB   = 6
    };

    std::string sizeToString(uint64_t size, SizeUnit maxUnit = SuTiB);

    template<typename T, typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits>&
    operator <<(std::basic_ostream<CharT, Traits>& os, const T& in)
    {
        const char s[] = "<unknown-type>";
        os.write(s, sizeof(s));
        return os;
    }

    template<typename T>
    static inline std::string valueToString(T& in)
    {
        std::ostringstream value;
        value << in;
        return value.str();
    }


    //
    // GUID
    //

 #define GUID_STRING_LENGTH 38

    void generateGuid(Guid& guid);
    std::string guidToString(const Guid& guid, bool data1Only = false);


    //
    // System Utility Functions
    //

    namespace System {
        Handle duplicateHandle(Handle handle);
        void closeHandle(Handle handle);
    #if defined(_WIN32)
    #else
        void maskSignal(uint32_t signal);
        void unmaskSignal(uint32_t signal);
    #endif
    }

    //
    // Others
    //

    // Reference to an object with a custom deleter
    template<typename T>
    using ObjectReference = std::unique_ptr<T, void (*)(T*)>;

    template<typename T>
    struct NullDeleter
    {
        void operator()(T*) const { }
        static void deleter(T*) { }
    };

#define IsSet(var, flag) \
    (((var) & (flag)) != 0)

}
#endif // UTILS_H