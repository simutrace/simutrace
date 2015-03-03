# libsimubase CMake configuration
#
# Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus
#
#             _____ _                 __
#            / ___/(_)___ ___  __  __/ /__________ _________
#            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
#           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
#          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
#                         http://simutrace.org
#
# Simutrace Base Library (libsimubase) is part of Simutrace.
#
# libsimubase is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimubase is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimubase. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

set(SIMUBASE_INCLUDE "../include/simubase")
include_directories(${SIMUBASE_INCLUDE})

set(PRECOMPILE_SOURCE "main.cpp")
set(PRECOMPILE_HEADER "SimuPlatform.h")

# Base

set(HEADER_FILES_BASE_PRIVATE
    "../include/Version.h")

set(HEADER_FILES_BASE_INTERNAL
    "../include/SimuBase.h"
    "../include/SimuBaseLibraries.h"
    "${SIMUBASE_INCLUDE}/SimuBaseOptions.h")

set(HEADER_FILES_BASE_API
    "../include/SimuPlatform.h"
    "../include/SimuBaseTypes.h")

set(HEADER_FILES_BASE_API ${HEADER_FILES_BASE_API} PARENT_SCOPE)


# Network

set(SOURCE_FILES_NETWORK
    "Channel.cpp"
    "LocalChannel.cpp"
    "SocketChannel.cpp"
    "ChannelProvider.cpp"
    "Port.cpp"
    "ClientPort.cpp"
    "ServerPort.cpp"
    "RpcInterface.cpp")

set(HEADER_FILES_NETWORK_PRIVATE
    "Channel.h"
    "ChannelProvider.h")

set(HEADER_FILES_NETWORK_CHANNELS_PRIVATE
    "LocalChannel.h"
    "SocketChannel.h")

set(HEADER_FILES_NETWORK
    "${SIMUBASE_INCLUDE}/Port.h"
    "${SIMUBASE_INCLUDE}/ClientPort.h"
    "${SIMUBASE_INCLUDE}/ServerPort.h"
    "${SIMUBASE_INCLUDE}/RpcInterface.h")


# Compression

set(SOURCE_FILES_COMPRESSION_BASE
   "Compression.cpp")

set(SOURCE_FILES_COMPRESSION_LZMA
    "lzma/Alloc.c"
    "lzma/LzFind.c"
    "lzma/LzmaDec.c"
    "lzma/LzmaEnc.c"
    "lzma/LzmaLib.c")

set(HEADER_FILES_COMPRESSION_LZMA_PRIVATE
    "lzma/Alloc.h"
    "lzma/LzFind.h"
    "lzma/LzHash.h"
    "lzma/LzmaDec.h"
    "lzma/LzmaEnc.h"
    "lzma/LzmaLib.h"
    "lzma/Types.h")

set(HEADER_FILES_COMPRESSION_BASE
    "${SIMUBASE_INCLUDE}/Compression.h")


# Hashing

set(SOURCE_FILES_HASHING_BASE
    "Hash.cpp")

set(SOURCE_FILES_HASHING_MURMUR
    "murmurhash3/MurmurHash3.cpp")

set(HEADER_FILES_HASHING_MURMUR_PRIVATE
    "murmurhash3/MurmurHash3.h")

set(HEADER_FILES_HASHING_BASE
    "${SIMUBASE_INCLUDE}/Hash.h")


# I/O

set(SOURCE_FILES_IO
    "Directory.cpp"
    "File.cpp")

set(HEADER_FILES_IO
    "${SIMUBASE_INCLUDE}/Directory.h"
    "${SIMUBASE_INCLUDE}/File.h")


# Logging

set(SOURCE_FILES_LOGGING
    "LogCategory.cpp"
    "LogEvent.cpp"
    "LogPriority.cpp"
    "LogAppender.cpp"
    "OstreamLogAppender.cpp"
    "TerminalLogAppender.cpp"
    "FileLogAppender.cpp"
    "SimpleLogLayout.cpp"
    "PatternLogLayout.cpp")

set(HEADER_FILES_LOGGING
    "${SIMUBASE_INCLUDE}/Logging.h"
    "${SIMUBASE_INCLUDE}/LogCategory.h"
    "${SIMUBASE_INCLUDE}/LogEvent.h"
    "${SIMUBASE_INCLUDE}/LogPriority.h"
    "${SIMUBASE_INCLUDE}/LogAppender.h"
    "${SIMUBASE_INCLUDE}/OstreamLogAppender.h"
    "${SIMUBASE_INCLUDE}/TerminalLogAppender.h"
    "${SIMUBASE_INCLUDE}/FileLogAppender.h"
    "${SIMUBASE_INCLUDE}/LogLayout.h"
    "${SIMUBASE_INCLUDE}/SimpleLogLayout.h"
    "${SIMUBASE_INCLUDE}/PatternLogLayout.h")


# Memory

set(SOURCE_FILES_MEMORY
    "MemoryHelpers.cpp"
    "MemorySegment.cpp"
    "PrivateMemorySegment.cpp"
    "FileBackedMemorySegment.cpp"
    "SharedMemorySegment.cpp")

set(HEADER_FILES_MEMORY
    "${SIMUBASE_INCLUDE}/MemoryHelpers.h"
    "${SIMUBASE_INCLUDE}/MemorySegment.h"
    "${SIMUBASE_INCLUDE}/PrivateMemorySegment.h"
    "${SIMUBASE_INCLUDE}/FileBackedMemorySegment.h"
    "${SIMUBASE_INCLUDE}/SharedMemorySegment.h")


# Threading

set(SOURCE_FILES_THREADING
    "ThreadBase.cpp"
    "Interlocked.cpp"
    "Event.cpp"
    "CriticalSection.cpp"
    "ReaderWriterLock.cpp"
    "ConditionVariable.cpp")

set(HEADER_FILES_THREADING
    "${SIMUBASE_INCLUDE}/ThreadBase.h"
    "${SIMUBASE_INCLUDE}/Thread.h"
    "${SIMUBASE_INCLUDE}/Interlocked.h"
    "${SIMUBASE_INCLUDE}/Event.h"
    "${SIMUBASE_INCLUDE}/CriticalSection.h"
    "${SIMUBASE_INCLUDE}/LockRegion.h"
    "${SIMUBASE_INCLUDE}/ReaderWriterLock.h"
    "${SIMUBASE_INCLUDE}/ReaderWriterLockRegion.h"
    "${SIMUBASE_INCLUDE}/ConditionVariable.h"
    "${SIMUBASE_INCLUDE}/WaitContext.h")


# Time

set(SOURCE_FILES_TIME
    "Clock.cpp"
    "StopWatch.cpp")

set(HEADER_FILES_TIME
    "${SIMUBASE_INCLUDE}/Clock.h"
    "${SIMUBASE_INCLUDE}/StopWatch.h")


# Others

set(SOURCE_FILES_OTHERS
    "Configuration.cpp"
    "Environment.cpp"
    "Exceptions.cpp"
    "SafeHandle.cpp"
    "Utils.cpp"
    "Profiler.cpp")

set(HEADER_FILES_OTHERS
    "${SIMUBASE_INCLUDE}/Configuration.h"
    "${SIMUBASE_INCLUDE}/Environment.h"
    "${SIMUBASE_INCLUDE}/Exceptions.h"
    "${SIMUBASE_INCLUDE}/IdAllocator.h"
    "${SIMUBASE_INCLUDE}/SafeHandle.h"
    "${SIMUBASE_INCLUDE}/Utils.h"
    "${SIMUBASE_INCLUDE}/ProfileEntry.h"
    "${SIMUBASE_INCLUDE}/Profiler.h"
    "${SIMUBASE_INCLUDE}/FastDelegate.h")

set(OTHER_FILES_OTHERS
    "CMakeLists.txt")


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED
    ${SOURCE_FILES_COMPRESSION_LZMA}
    ${SOURCE_FILES_HASHING_MURMUR})

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_OTHERS}
    ${SOURCE_FILES_NETWORK}
    ${SOURCE_FILES_COMPRESSION_BASE}
    ${SOURCE_FILES_HASHING_BASE}
    ${SOURCE_FILES_IO}
    ${SOURCE_FILES_LOGGING}
    ${SOURCE_FILES_MEMORY}
    ${SOURCE_FILES_THREADING}
    ${SOURCE_FILES_TIME})

set(HEADER_FILES
    ${HEADER_FILES_BASE_PRIVATE}
    ${HEADER_FILES_NETWORK_PRIVATE}
    ${HEADER_FILES_NETWORK_CHANNELS_PRIVATE}
    ${HEADER_FILES_COMPRESSION_LZMA_PRIVATE}
    ${HEADER_FILES_BASE_API}
    ${HEADER_FILES_BASE_INTERNAL}
    ${HEADER_FILES_OTHERS}
    ${HEADER_FILES_NETWORK}
    ${HEADER_FILES_COMPRESSION_BASE}
    ${HEADER_FILES_HASHING_BASE}
    ${HEADER_FILES_IO}
    ${HEADER_FILES_LOGGING}
    ${HEADER_FILES_MEMORY}
    ${HEADER_FILES_THREADING}
    ${HEADER_FILES_TIME})

set(OTHER_FILES
    ${OTHER_FILES_OTHERS})

source_group("Source files" FILES ${PRECOMPILE_SOURCE} ${SOURCE_FILES_BASE} ${SOURCE_FILES_OTHERS})
source_group("Source files\\Network" FILES ${SOURCE_FILES_NETWORK})
source_group("Source files\\Compression" FILES ${SOURCE_FILES_COMPRESSION_BASE})
source_group("Source files\\Compression\\lzma" FILES ${SOURCE_FILES_COMPRESSION_LZMA})
source_group("Source files\\Hashing" FILES ${SOURCE_FILES_HASHING_BASE})
source_group("Source files\\Hashing\\murmur3" FILES ${SOURCE_FILES_HASHING_MURMUR})
source_group("Source files\\IO" FILES ${SOURCE_FILES_IO})
source_group("Source files\\Logging" FILES ${SOURCE_FILES_LOGGING})
source_group("Source files\\Memory" FILES ${SOURCE_FILES_MEMORY})
source_group("Source files\\Threading" FILES ${SOURCE_FILES_THREADING})
source_group("Source files\\Time" FILES ${SOURCE_FILES_TIME})

source_group("Header files" FILES ${HEADER_FILES_BASE_PRIVATE})
source_group("Header files\\Network" FILES ${HEADER_FILES_NETWORK_PRIVATE})
source_group("Header files\\Network\\Channels" FILES ${HEADER_FILES_NETWORK_CHANNELS_PRIVATE})
source_group("Header files\\Compression\\lzma" FILES ${HEADER_FILES_COMPRESSION_LZMA_PRIVATE})
source_group("Header files\\Hashing\\murmur3" FILES ${HEADER_FILES_HASHING_MURMUR_PRIVATE})
source_group("Header files\\Public" FILES ${HEADER_FILES_BASE_API} ${HEADER_FILES_BASE_INTERNAL} ${HEADER_FILES_OTHERS})
source_group("Header files\\Public\\Network" FILES ${HEADER_FILES_NETWORK})
source_group("Header files\\Public\\Compression" FILES ${HEADER_FILES_COMPRESSION_BASE})
source_group("Header files\\Public\\Hashing" FILES ${HEADER_FILES_HASHING_BASE})
source_group("Header files\\Public\\IO" FILES ${HEADER_FILES_IO})
source_group("Header files\\Public\\Logging" FILES ${HEADER_FILES_LOGGING})
source_group("Header files\\Public\\Memory" FILES ${HEADER_FILES_MEMORY})
source_group("Header files\\Public\\Threading" FILES ${HEADER_FILES_THREADING})
source_group("Header files\\Public\\Time" FILES ${HEADER_FILES_TIME})

source_group("" FILES ${OTHER_FILES_OTHERS})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

add_library(libsimubase
            ${PRECOMPILE_SOURCE}
            ${SOURCE_FILES_NOTPRECOMPILED}
            ${SOURCE_FILES_PRECOMPILED}
            ${HEADER_FILES})

set_target_properties(libsimubase PROPERTIES PREFIX "")

add_precompiled_header(libsimubase
                       SOURCE_FILES_NOTPRECOMPILED
                       ${PRECOMPILE_HEADER}
                       ${PRECOMPILE_SOURCE})

set_multiprocessor_compilation(libsimubase)
set_sdl_compilation(libsimubase)