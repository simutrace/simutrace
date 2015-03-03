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

#include "Hash.h"

#include "Exceptions.h"

#include "murmurhash3/MurmurHash3.h"

namespace SimuTrace {
namespace Hash
{

    void murmur3_32(const void* buffer, size_t length, void* hashBuffer,
                    size_t hashBufferLength, uint32_t seed)
    {
        ThrowOn((hashBufferLength < 4) ||
                (length > std::numeric_limits<int>::max()),
                ArgumentException);

        MurmurHash3_x86_32(buffer, static_cast<int>(length), seed, hashBuffer);
    }

    void murmur3_128(const void* buffer, size_t length, void* hashBuffer,
                     size_t hashBufferLength, uint32_t seed)
    {
        ThrowOn((hashBufferLength < 16) ||
                (length > std::numeric_limits<int>::max()),
                ArgumentException);

        MurmurHash3_x64_128(buffer, static_cast<int>(length), seed, hashBuffer);
    }


}
}