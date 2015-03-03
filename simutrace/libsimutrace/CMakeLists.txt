# libsimutrace CMake configuration
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
# Simutrace Client Library (libsimutrace) is part of Simutrace.
#
# libsimutrace is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimutrace is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimutrace. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

include_directories("../include/simubase")
include_directories("../include/simustor")

set(PRECOMPILE_SOURCE "main.cpp")
set(PRECOMPILE_HEADER "SimuStor.h")

# Base

set(SOURCE_FILES_API
    "SimuTrace.cpp")

set(HEADER_FILES_PRIVATE
    "ClientObject.h")

set(HEADER_FILES_API
    "../include/SimuTrace.h"
    "../include/SimuTraceEntryTypes.h"
    "../include/SimuTraceTypes.h")

set(HEADER_FILES_API ${HEADER_FILES_API} PARENT_SCOPE)


# Session

set(SOURCE_FILES_SESSION
    "ClientSession.cpp"
    "ClientSessionManager.cpp")

set(HEADER_FILES_SESSION
    "ClientSession.h"
    "ClientSessionManager.h")


# Storage

set(SOURCE_FILES_STORAGE
    "ClientStore.cpp")

set(HEADER_FILES_STORAGE
    "ClientStore.h")


# Stream

set(SOURCE_FILES_STREAMS
    "ClientStream.cpp")

set(HEADER_FILES_STREAMS
    "ClientStream.h")


# Others

set(OTHER_FILES_OTHERS
    "CMakeLists.txt")


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED "")

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_API}
    ${SOURCE_FILES_SESSION}
    ${SOURCE_FILES_STORAGE}
    ${SOURCE_FILES_STREAMS})

set(HEADER_FILES
    ${HEADER_FILES_API}
    ${HEADER_FILES_PRIVATE}
    ${HEADER_FILES_SESSION}
    ${HEADER_FILES_NETWORK}
    ${HEADER_FILES_STORAGE}
    ${HEADER_FILES_STREAMS})

set(OTHER_FILES
    ${OTHER_FILES_OTHERS})

source_group("Source files" FILES ${PRECOMPILE_SOURCE} ${SOURCE_FILES_API})
source_group("Source files\\Session" FILES ${SOURCE_FILES_SESSION})
source_group("Source files\\Storage" FILES ${SOURCE_FILES_STORAGE})
source_group("Source files\\Streams" FILES ${SOURCE_FILES_STREAMS})

source_group("Header files" FILES ${HEADER_FILES_API} ${HEADER_FILES_PRIVATE})
source_group("Header files\\Public" FILES ${HEADER_FILES_API})
source_group("Header files\\Network" FILES ${HEADER_FILES_NETWORK})
source_group("Header files\\Session" FILES ${HEADER_FILES_SESSION})
source_group("Header files\\Storage" FILES ${HEADER_FILES_STORAGE})
source_group("Header files\\Streams" FILES ${HEADER_FILES_STREAMS})

source_group("" FILES ${OTHER_FILES_OTHERS})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

add_library(libsimutrace SHARED
            ${PRECOMPILE_SOURCE}
            ${SOURCE_FILES_NOTPRECOMPILED}
            ${SOURCE_FILES_PRECOMPILED}
            ${HEADER_FILES})

set_target_properties(libsimutrace PROPERTIES
                      PREFIX ""
                      VERSION ${SIMUTRACE_VERSION}
                      SOVERSION "3")

target_link_libraries(libsimutrace
                      libsimustor
                      libsimubase
                      libconfig++
                      ${RT_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}  # pthreads for Linux
                      )

add_precompiled_header(libsimutrace
                       SOURCE_FILES_NOTPRECOMPILED
                       ${PRECOMPILE_HEADER}
                       ${PRECOMPILE_SOURCE})

set_multiprocessor_compilation(libsimutrace)
set_sdl_compilation(libsimutrace)

# Install ---------------------------------------------------------------------

set(HEADER_FILES_API_SIMUTRACE
    ${HEADER_FILES_API}
    ${HEADER_FILES_STOR_API}
    ${HEADER_FILES_BASE_API})

if (UNIX)
    install(TARGETS libsimutrace
            LIBRARY DESTINATION "lib")

    install(FILES ${HEADER_FILES_API_SIMUTRACE} DESTINATION "include")
endif()