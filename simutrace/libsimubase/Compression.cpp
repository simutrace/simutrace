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

#include "Compression.h"

#include "Exceptions.h"
#include "Utils.h"

#include "lzma/LzmaLib.h"

namespace SimuTrace {
namespace Compression 
{

    // LZMA ----
    size_t lzmaCompress(const void* source, size_t sourceLength,
                        void* destination, size_t destinationLength,
                        uint32_t level)
    {
        const size_t totalHeaderSize = 5 + sizeof(size_t);
        size_t propSize = 5;

        ThrowOn((sourceLength == 0) ||
                (destinationLength <= totalHeaderSize),
                ArgumentException);

        // We write the header in front of the compressed data. The header
        // is prepended by the uncompressed size, so we can decompress the
        // data later without additional information from the caller.
        size_t* destinationOfSize = static_cast<size_t*>(destination);
        *destinationOfSize = sourceLength;

        unsigned char* destinationOfHeader = reinterpret_cast<unsigned char*>(
            reinterpret_cast<size_t>(destination) + sizeof(size_t));
        unsigned char* destinationAfterHeader = reinterpret_cast<unsigned char*>(
            reinterpret_cast<size_t>(destination) + totalHeaderSize);

        size_t compressedSize = destinationLength - totalHeaderSize;

        int result = LzmaCompress(
            destinationAfterHeader, &compressedSize,
            static_cast<const unsigned char*>(source), sourceLength,
            destinationOfHeader, &propSize,
            level, 0, -1, -1, -1, -1, 1);
        assert(propSize == 5);

        ThrowOn(result != SZ_OK, Exception, stringFormat(
                "LZMA compression failed. The error code is: %d.", result));

        assert(compressedSize <= std::numeric_limits<size_t>::max() - totalHeaderSize);
        return compressedSize + totalHeaderSize;
    }

    size_t lzmaDecompress(const void* source, size_t sourceLength,
                          void* destination, size_t destinationLength)
    {
        const size_t totalHeaderSize = 5 + sizeof(size_t);
        const size_t propSize = 5;

        ThrowOn((sourceLength <= totalHeaderSize) ||
                (destinationLength == 0),
                ArgumentException);

        // We expect the header in front of the compressed data. The header
        // is prepended by the uncompressed size, so we can decompress the
        // data later without additional information from the caller.
        const size_t* sourceOfSize = static_cast<const size_t*>(source);
        size_t uncompressedSize = *sourceOfSize;

        ThrowOn(destinationLength < uncompressedSize, Exception, stringFormat(
                "The destination buffer is too small to decompress the "
                "source. Expected %s, but was given %s.",
                sizeToString(uncompressedSize, SizeUnit::SuBytes).c_str(),
                sizeToString(destinationLength, SizeUnit::SuBytes).c_str()));

        unsigned char* sourceOfHeader = reinterpret_cast<unsigned char*>(
            reinterpret_cast<size_t>(source) + sizeof(size_t));
        unsigned char* sourceAfterHeader = reinterpret_cast<unsigned char*>(
            reinterpret_cast<size_t>(source) + totalHeaderSize);

        size_t compressedSize = sourceLength - totalHeaderSize;

        int result = LzmaUncompress(
            static_cast<unsigned char*>(destination), &uncompressedSize,
            sourceAfterHeader, &compressedSize,
            sourceOfHeader, propSize);

        ThrowOn(result != SZ_OK, Exception, stringFormat(
                "LZMA decompression failed. The error code is: %d.", result));

        return uncompressedSize;
    }

}
}