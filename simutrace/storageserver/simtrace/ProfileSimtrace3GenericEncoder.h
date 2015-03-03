/*
 * Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
 * Marc Rittinghaus, Thorsten Groeninger
 *
 * Simutrace Storage Server (storageserver) is part of Simutrace.
 *
 * storageserver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * storageserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with storageserver. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef PROFILE_SIMTRACE3_GENERIC_ENCODER_H
#define PROFILE_SIMTRACE3_GENERIC_ENCODER_H

#include "SimuStor.h"

#ifdef SIMUTRACE_PROFILING_SIMTRACE3_GENERIC_COMPRESSION_ENABLE

#include "../StorageServer.h"

namespace SimuTrace
{

    namespace _profile {
        static Profiler* profiler;
        static volatile uint32_t created = 0;
    }

    static void profileCreateProfiler()
    {
        if (Interlocked::interlockedCompareExchange(&_profile::created, 1, 0) == 0) {
            std::string comp = std::string("profile_compression.csv");
            _profile::profiler = new Profiler(
                StorageServer::makePath(nullptr, &comp));
        }
    }

    static void profileCompareCompressionMethods(ServerStream& stream,
                                                 StreamSegmentId segment)
    {
        ProfileContext context;
        ScratchSegment compressionSegment;
        ScratchSegment decompressionSegment;

        ServerStreamBuffer& buffer = dynamic_cast<ServerStreamBuffer&>(
            stream.getStreamBuffer());
        SegmentControlElement* ctrl = buffer.getControlElement(segment);

        const void* sourceBuffer = buffer.getSegment(segment);

        size_t sourceSize = static_cast<size_t>(
            ctrl->rawEntryCount * stream.getType().entrySize);

        void* compressionBuffer   = compressionSegment.getBuffer();
        void* decompressionBuffer = decompressionSegment.getBuffer();

        size_t compressionBufferSize   = compressionSegment.getLength();
        size_t decompressionBufferSize = decompressionSegment.getLength();

        StreamId id = stream.getId();

        context.add("stream", id, false);
        context.add("sqn", ctrl->link.sequenceNumber, false);

        // +++++++++++ zlib +++++++++++
        /*
        // zlib 1
        size_t zlib1CompressSize;
        StopWatch zlib1CompressWatch, zlib1DecompressWatch;
        context.add("si [zlib(1)]", zlib1CompressSize, false);
        context.add("ct [zlib(1)]", zlib1CompressWatch, &StopWatch::getTicks);
        context.add("dt [zlib(1)]", zlib1DecompressWatch, &StopWatch::getTicks);

        zlib1CompressWatch.start();

        zlib1CompressSize = Compression::zlibCompress(sourceBuffer,
                                                      sourceSize,
                                                      compressionBuffer,
                                                      compressionBufferSize,
                                                      1);

        zlib1CompressWatch.stop();
        zlib1DecompressWatch.start();

        Compression::zlibDecompress(compressionBuffer,
                                    zlib1CompressSize,
                                    decompressionBuffer,
                                    decompressionBufferSize);

        zlib1DecompressWatch.stop();
        */
        // zlib6
        size_t zlib6CompressSize;
        StopWatch zlib6CompressWatch, zlib6DecompressWatch;
        context.add("si [zlib(6)]", zlib6CompressSize, false);
        context.add("ct [zlib(6)]", zlib6CompressWatch, &StopWatch::getTicks);
        context.add("dt [zlib(6)]", zlib6DecompressWatch, &StopWatch::getTicks);

        zlib6CompressWatch.start();

        zlib6CompressSize = Compression::zlibCompress(sourceBuffer,
                                                      sourceSize,
                                                      compressionBuffer,
                                                      compressionBufferSize,
                                                      6);

        zlib6CompressWatch.stop();
        zlib6DecompressWatch.start();

        Compression::zlibDecompress(compressionBuffer,
                                    zlib6CompressSize,
                                    decompressionBuffer,
                                    decompressionBufferSize);

        zlib6DecompressWatch.stop();


        // +++++++++++ LZ4 +++++++++++

        size_t lz4CompressSize;
        StopWatch lz4CompressWatch, lz4DecompressWatch;
        context.add("si [lz4]", lz4CompressSize, false);
        context.add("ct [lz4]", lz4CompressWatch, &StopWatch::getTicks);
        context.add("dt [lz4]", lz4DecompressWatch, &StopWatch::getTicks);

        lz4CompressWatch.start();

        lz4CompressSize = Compression::lz4Compress(sourceBuffer,
                                                   sourceSize,
                                                   compressionBuffer,
                                                   compressionBufferSize);

        lz4CompressWatch.stop();
        lz4DecompressWatch.start();

        Compression::lz4Decompress(compressionBuffer,
                                   lz4CompressSize,
                                   decompressionBuffer,
                                   decompressionBufferSize);

        lz4DecompressWatch.stop();


        // +++++++++++ LZMA +++++++++++
        /*
        // LZMA 0
        size_t lzma0CompressSize;
        StopWatch lzma0CompressWatch, lzma0DecompressWatch;
        context.add("si [lzma(0)]", lzma0CompressSize, false);
        context.add("ct [lzma(0)]", lzma0CompressWatch, &StopWatch::getTicks);
        context.add("dt [lzma(0)]", lzma0DecompressWatch, &StopWatch::getTicks);

        lzma0CompressWatch.start();

        lzma0CompressSize = Compression::lzmaCompress(sourceBuffer,
                                                      sourceSize,
                                                      compressionBuffer,
                                                      compressionBufferSize,
                                                      0);

        lzma0CompressWatch.stop();
        lzma0DecompressWatch.start();

        Compression::lzmaDecompress(compressionBuffer,
                                    lzma0CompressSize,
                                    decompressionBuffer,
                                    decompressionBufferSize);

        lzma0DecompressWatch.stop();

        // LZMA 4
        size_t lzma4CompressSize;
        StopWatch lzma4CompressWatch, lzma4DecompressWatch;
        context.add("si [lzma(4)]", lzma4CompressSize, false);
        context.add("ct [lzma(4)]", lzma4CompressWatch, &StopWatch::getTicks);
        context.add("dt [lzma(4)]", lzma4DecompressWatch, &StopWatch::getTicks);

        lzma4CompressWatch.start();

        lzma4CompressSize = Compression::lzmaCompress(sourceBuffer,
                                                      sourceSize,
                                                      compressionBuffer,
                                                      compressionBufferSize,
                                                      4);

        lzma4CompressWatch.stop();
        lzma4DecompressWatch.start();

        Compression::lzmaDecompress(compressionBuffer,
                                    lzma4CompressSize,
                                    decompressionBuffer,
                                    decompressionBufferSize);

        lzma4DecompressWatch.stop();
        */
        // LZMA 5
        /*size_t lzma5CompressSize;
        StopWatch lzma5CompressWatch, lzma5DecompressWatch;
        context.add("si [lzma(5)]", lzma5CompressSize, false);
        context.add("ct [lzma(5)]", lzma5CompressWatch, &StopWatch::getTicks);
        context.add("dt [lzma(5)]", lzma5DecompressWatch, &StopWatch::getTicks);

        lzma5CompressWatch.start();

        lzma5CompressSize = Compression::lzmaCompress(sourceBuffer,
                                                      sourceSize,
                                                      compressionBuffer,
                                                      compressionBufferSize,
                                                      5);

        lzma5CompressWatch.stop();
        lzma5DecompressWatch.start();

        Compression::lzmaDecompress(compressionBuffer,
                                    lzma5CompressSize,
                                    decompressionBuffer,
                                    decompressionBufferSize);

        lzma5DecompressWatch.stop();
        */

        // +++++++++++ LZO +++++++++++
        /*size_t lzoCompressSize;
        StopWatch lzoCompressWatch, lzoDecompressWatch;
        context.add("si [lzo]", lzoCompressSize, false);
        context.add("ct [lzo]", lzoCompressWatch, &StopWatch::getTicks);
        context.add("dt [lzo]", lzoDecompressWatch, &StopWatch::getTicks);

        lzoCompressWatch.start();

        lzoCompressSize = Compression::lzoCompress(sourceBuffer,
                                                   sourceSize,
                                                   compressionBuffer,
                                                   compressionBufferSize);

        lzoCompressWatch.stop();
        lzoDecompressWatch.start();

        Compression::lzoDecompress(compressionBuffer,
                                   lzoCompressSize,
                                   decompressionBuffer,
                                   decompressionBufferSize);

        lzoDecompressWatch.stop();

        // +++++++++++ BZip2 +++++++++++

        //Bzip2 1
        size_t bzip21CompressSize;
        StopWatch bzip21CompressWatch, bzip21DecompressWatch;
        context.add("si [bzip2(1)]", bzip21CompressSize, false);
        context.add("ct [bzip2(1)]", bzip21CompressWatch, &StopWatch::getTicks);
        context.add("dt [bzip2(1)]", bzip21DecompressWatch, &StopWatch::getTicks);

        bzip21CompressWatch.start();

        bzip21CompressSize = Compression::bzip2Compress(sourceBuffer,
                                                        sourceSize,
                                                        compressionBuffer,
                                                        compressionBufferSize,
                                                        1);

        bzip21CompressWatch.stop();
        bzip21DecompressWatch.start();

        Compression::bzip2Decompress(compressionBuffer,
                                     bzip21CompressSize,
                                     decompressionBuffer,
                                     decompressionBufferSize);

        bzip21DecompressWatch.stop();

        //Bzip2 5
        size_t bzip25CompressSize;
        StopWatch bzip25CompressWatch, bzip25DecompressWatch;
        context.add("si [bzip2(5)]", bzip25CompressSize, false);
        context.add("ct [bzip2(5)]", bzip25CompressWatch, &StopWatch::getTicks);
        context.add("dt [bzip2(5)]", bzip25DecompressWatch, &StopWatch::getTicks);

        bzip25CompressWatch.start();

        bzip25CompressSize = Compression::bzip2Compress(sourceBuffer,
                                                        sourceSize,
                                                        compressionBuffer,
                                                        compressionBufferSize,
                                                        5);

        bzip25CompressWatch.stop();
        bzip25DecompressWatch.start();

        Compression::bzip2Decompress(compressionBuffer,
                                     bzip25CompressSize,
                                     decompressionBuffer,
                                     decompressionBufferSize);

        bzip25DecompressWatch.stop();
        */
        //Bzip2 9
        size_t bzip29CompressSize;
        StopWatch bzip29CompressWatch, bzip29DecompressWatch;
        context.add("si [bzip2(9)]", bzip29CompressSize, false);
        context.add("ct [bzip2(9)]", bzip29CompressWatch, &StopWatch::getTicks);
        context.add("dt [bzip2(9)]", bzip29DecompressWatch, &StopWatch::getTicks);

        bzip29CompressWatch.start();

        bzip29CompressSize = Compression::bzip2Compress(sourceBuffer,
                                                        sourceSize,
                                                        compressionBuffer,
                                                        compressionBufferSize,
                                                        9);

        bzip29CompressWatch.stop();
        bzip29DecompressWatch.start();

        Compression::bzip2Decompress(compressionBuffer,
                                     bzip29CompressSize,
                                     decompressionBuffer,
                                     decompressionBufferSize);

        bzip29DecompressWatch.stop();

        // Collect the measurements
        _profile::profiler->collect(context);
    }

}

#else
#define profileCreateProfiler()
#define profileCompareCompressionMethods(buffer, segment)
#endif
#endif
