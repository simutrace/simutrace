# storageserver CMake configuration
#
# Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus, Thorsten Groeninger
#
#             _____ _                 __
#            / ___/(_)___ ___  __  __/ /__________ _________
#            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
#           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
#          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
#                         http://simutrace.org
#
# Simutrace Storage Server (storageserver) is part of Simutrace.
#
# storageserver is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# storageserver is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with storageserver. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

include_directories("../include/simubase")
include_directories("../include/simustor")

set(PRECOMPILE_SOURCE "main.cpp")
set(PRECOMPILE_HEADER "SimuStor.h")

# Base

set(SOURCE_FILES_BASE
    "StorageServer.cpp")

set(HEADER_FILES_BASE
    "StorageServer.h"
    "StorageServerOptions.h")


# Session

set(SOURCE_FILES_SESSION
    "ServerSession.cpp"
    "ServerSessionManager.cpp"
    "ServerSessionWorker.cpp")

set(HEADER_FILES_SESSION
    "ServerSession.h"
    "ServerSessionManager.h"
    "ServerSessionWorker.h")


# Storage

set(SOURCE_FILES_STORAGE_BASE
    "ServerStore.cpp"
    "ServerStoreManager.cpp")

set(SOURCE_FILES_STORAGE_SIMTRACE
    "simtrace/Simtrace3Encoder.cpp"
    "simtrace/Simtrace3GenericEncoder.cpp"
    "simtrace/Simtrace3Frame.cpp"
    "simtrace/Simtrace3Store.cpp"
    "simtrace/SimtraceStoreProvider.cpp")

set(HEADER_FILES_STORAGE_BASE
    "ServerStore.h"
    "ServerStoreManager.h"
    "StreamEncoder.h")

set(HEADER_FILES_STORAGE_SIMTRACE
    "simtrace/Simtrace3Encoder.h"
    "simtrace/Simtrace3GenericEncoder.h"
    "simtrace/ProfileSimtrace3GenericEncoder.h"
    "simtrace/Simtrace3MemoryEncoder.h"
    "simtrace/Simtrace3Frame.h"
    "simtrace/Simtrace3Format.h"
    "simtrace/FileHeader.h"
    "simtrace/Simtrace3Store.h"
    "simtrace/SimtraceStoreProvider.h")

set(HEADER_FILES_STORAGE_SIMTRACE_VPC4
    "simtrace/VPC4/Predictor.h"
    "simtrace/VPC4/FiniteContextMethodHistory.h"
    "simtrace/VPC4/FiniteContextMethodPredictor.h"
    "simtrace/VPC4/KeyedLastNValuePredictor.h"
    "simtrace/VPC4/CompoundPredictor.h"
    "simtrace/VPC4/IpPredictor.h"
    "simtrace/VPC4/ValuePredictor.h"
    "simtrace/VPC4/CyclePredictor.h")


# Stream

set(SOURCE_FILES_STREAMS
    "ServerStream.cpp"
    "ServerStreamBuffer.cpp"
    "ScratchSegment.cpp")

set(HEADER_FILES_STREAMS
    "ServerStream.h"
    "ServerStreamBuffer.h"
    "ScratchSegment.h")


# Workers

set(SOURCE_FILES_WORKERS
    "WorkQueue.cpp"
    "WorkerPool.cpp"
    "WorkerStreamWait.cpp")

set(HEADER_FILES_WORKERS
    "WorkItemBase.h"
    "WorkItem.h"
    "WorkQueue.h"
    "WorkerPool.h"
    "WorkerStreamWait.h")


# Others

set(OTHER_FILES_OTHERS
    "CMakeLists.txt")


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED "")

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_BASE}
    ${SOURCE_FILES_SESSION}
    ${SOURCE_FILES_STORAGE_BASE}
    ${SOURCE_FILES_STORAGE_SIMTRACE}
    ${SOURCE_FILES_STREAMS}
    ${SOURCE_FILES_WORKERS})

set(HEADER_FILES
    ${HEADER_FILES_BASE}
    ${HEADER_FILES_SESSION}
    ${HEADER_FILES_STORAGE_BASE}
    ${HEADER_FILES_STORAGE_SIMTRACE}
    ${HEADER_FILES_STORAGE_SIMTRACE_VPC4}
    ${HEADER_FILES_STREAMS}
    ${HEADER_FILES_WORKERS})

set(OTHER_FILES
    ${OTHER_FILES_OTHERS})

source_group("Source files" FILES ${SOURCE_FILES_BASE} ${PRECOMPILE_SOURCE})
source_group("Source files\\Session" FILES ${SOURCE_FILES_SESSION})
source_group("Source files\\Storage" FILES ${SOURCE_FILES_STORAGE_BASE})
source_group("Source files\\Storage\\Simtrace" FILES ${SOURCE_FILES_STORAGE_SIMTRACE})
source_group("Source files\\Streams" FILES ${SOURCE_FILES_STREAMS})
source_group("Source files\\Workers" FILES ${SOURCE_FILES_WORKERS})

source_group("Header files" FILES ${HEADER_FILES_BASE})
source_group("Header files\\Session" FILES ${HEADER_FILES_SESSION})
source_group("Header files\\Storage" FILES ${HEADER_FILES_STORAGE_BASE})
source_group("Header files\\Storage\\Simtrace" FILES ${HEADER_FILES_STORAGE_SIMTRACE})
source_group("Header files\\Storage\\Simtrace\\VPC4" FILES ${HEADER_FILES_STORAGE_SIMTRACE_VPC4})
source_group("Header files\\Streams" FILES ${HEADER_FILES_STREAMS})
source_group("Header files\\Workers" FILES ${HEADER_FILES_WORKERS})

source_group("" FILES ${OTHER_FILES_OTHERS})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

add_executable(simustore
               ${PRECOMPILE_SOURCE}
               ${SOURCE_FILES_NOTPRECOMPILED}
               ${SOURCE_FILES_PRECOMPILED}
               ${HEADER_FILES})

target_link_libraries(simustore
                      libsimustor
                      libsimubase
                      libconfig++
                      ${RT_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}  # pthreads for Linux
                      )

add_precompiled_header(simustore
                       SOURCE_FILES_NOTPRECOMPILED
                       ${PRECOMPILE_HEADER}
                       ${PRECOMPILE_SOURCE})

set_multiprocessor_compilation(simustore)
set_sdl_compilation(simustore)

# Build the server config file ------------------------------------------------

# Defaults
set(CFG_SERVER_LOGFILE "" CACHE PATH "server.logfile")
set(CFG_SERVER_LOGLEVEL "5" CACHE STRING "server.loglevel")
set(CFG_SERVER_QUIET OFF CACHE BOOL "server.quiet")
set(CFG_SERVER_WORKSPACE "" CACHE PATH "server.workspace")
set(CFG_SERVER_BINDINGS "local:/tmp/.simutrace" CACHE STRING "server.bindings")

set(CFG_SERVER_MEMMGMT_POOLSIZE "4096" CACHE STRING "server.memmgmt.poolSize")
set(CFG_SERVER_MEMMGMT_DISABLECACHE OFF CACHE BOOL "server.memmgmt.disableCache")
set(CFG_SERVER_MEMMGMT_READAHEAD "4" CACHE STRING "server.memmgmt.readAhead")

set(CFG_CLIENT_MEMMGMT_POOLSIZE "" CACHE STRING "client.memmgmt.poolSize")

if (CFG_SERVER_QUIET)
    set(_CFG_SERVER_QUIET "true")
else()
    set(_CFG_SERVER_QUIET "false")
endif()

if (CFG_SERVER_MEMMGMT_DISABLECACHE)
    set(_CFG_SERVER_MEMMGMT_DISABLECACHE "true")
else()
    set(_CFG_SERVER_MEMMGMT_DISABLECACHE "false")
endif()

if (CFG_CLIENT_MEMMGMT_POOLSIZE)
    set(_CFG_CLIENT_MEMMGMT_POOLSIZE "poolSize = ${CFG_CLIENT_MEMMGMT_POOLSIZE};")
else()
    set(_CFG_CLIENT_MEMMGMT_POOLSIZE "//poolSize = 1024;")
endif()

if(MSVC)
    configure_file("simustore.cfg-template"
                   "${EXECUTABLE_OUTPUT_PATH}/simustore.cfg"
                   NEWLINE_STYLE WIN32)
else()
    configure_file("simustore.cfg-template"
                   "${EXECUTABLE_OUTPUT_PATH}/simustore.cfg"
                   NEWLINE_STYLE UNIX)
endif()

# Install ---------------------------------------------------------------------

if (UNIX)
    install(TARGETS simustore
            RUNTIME DESTINATION "bin")
endif()