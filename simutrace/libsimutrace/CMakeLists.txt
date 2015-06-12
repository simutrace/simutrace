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

include_directories("../include/simubase"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include/"
                    "../include/simustor")

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

set(HEADER_FILES_API_INTERNAL
    "../include/SimuTraceInternal.h")


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
    "ClientStream.cpp"
    "StaticStream.cpp"
    "DynamicStream.cpp")

set(HEADER_FILES_STREAMS
    "ClientStream.h"
    "StaticStream.h"
    "DynamicStream.h")


# Others

if(WIN32)
    configure_file("libsimutrace.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/libsimutrace.rc"
                   @ONLY)

    set(SOURCE_FILES_RESOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/libsimutrace.rc")

else()
    set(SOURCE_FILES_RESOURCES "")
endif()


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED "")

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_API}
    ${SOURCE_FILES_SESSION}
    ${SOURCE_FILES_STORAGE}
    ${SOURCE_FILES_STREAMS})

set(HEADER_FILES
    ${HEADER_FILES_API}
    ${HEADER_FILES_API_INTERNAL}
    ${HEADER_FILES_PRIVATE}
    ${HEADER_FILES_SESSION}
    ${HEADER_FILES_NETWORK}
    ${HEADER_FILES_STORAGE}
    ${HEADER_FILES_STREAMS})

source_group("Source files" FILES ${PRECOMPILE_SOURCE} ${SOURCE_FILES_API})
source_group("Source files\\Session" FILES ${SOURCE_FILES_SESSION})
source_group("Source files\\Storage" FILES ${SOURCE_FILES_STORAGE})
source_group("Source files\\Streams" FILES ${SOURCE_FILES_STREAMS})

source_group("Header files" FILES ${HEADER_FILES_PRIVATE} ${HEADER_FILES_API_INTERNAL})
source_group("Header files\\Public" FILES ${HEADER_FILES_API})
source_group("Header files\\Network" FILES ${HEADER_FILES_NETWORK})
source_group("Header files\\Session" FILES ${HEADER_FILES_SESSION})
source_group("Header files\\Storage" FILES ${HEADER_FILES_STORAGE})
source_group("Header files\\Streams" FILES ${HEADER_FILES_STREAMS})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

if(BUILD_LIBSIMUTRACE)
    set(libsimutrace_GUID_CMAKE "E8066B21-B511-4D32-8334-3B5C3739CE15" CACHE INTERNAL "libsimutrace GUID")

    add_library(libsimutrace SHARED
                ${PRECOMPILE_SOURCE}
                ${SOURCE_FILES_NOTPRECOMPILED}
                ${SOURCE_FILES_PRECOMPILED}
                ${SOURCE_FILES_RESOURCES}
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

endif()

# Install ---------------------------------------------------------------------

set(HEADER_FILES_API_SIMUTRACE
    ${HEADER_FILES_API}
    ${HEADER_FILES_STOR_API}
    ${HEADER_FILES_BASE_API})

if(UNIX AND BUILD_LIBSIMUTRACE)
    install(TARGETS libsimutrace
            LIBRARY DESTINATION ${INSTALL_LIB_DIR})

    install(FILES ${HEADER_FILES_API_SIMUTRACE} DESTINATION ${INSTALL_INCLUDE_DIR})
endif()