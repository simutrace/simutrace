# libsimutraceX CMake configuration
#
# Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus
#
#             _____ _                 __
#            / ___/(_)___ ___  __  __/ /__________ _________
#            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
#           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
#          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
#                         http://simutrace.org
#
# Simutrace Extensions Library (libsimutraceX) is part of Simutrace.
#
# libsimutraceX is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimutraceX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimutraceX. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

include_directories("../include/simubase"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include/")

set(PRECOMPILE_SOURCE "main.cpp")
set(PRECOMPILE_HEADER "SimuTraceInternal.h")

# Base

set(SOURCE_FILES_APIX
    "SimuTraceX.cpp")

set(HEADER_FILES_APIX
    "../include/SimuTraceX.h"
    "../include/SimuTraceXTypes.h")

set(HEADER_FILES_APIX ${HEADER_FILES_APIX} PARENT_SCOPE)


# Multiplexer

set(SOURCE_FILES_MULTIPLEXER
    "StreamMultiplexer.cpp")

set(HEADER_FILES_MULTIPLEXER
    "StreamMultiplexer.h")

# Others

if(WIN32)
    configure_file("libsimutraceX.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/libsimutraceX.rc"
                   @ONLY)

    set(SOURCE_FILES_RESOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/libsimutraceX.rc")

else()
    set(SOURCE_FILES_RESOURCES "")
endif()


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED "")

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_APIX}
    ${SOURCE_FILES_MULTIPLEXER})

set(HEADER_FILES
    ${HEADER_FILES_APIX}
    ${HEADER_FILES_MULTIPLEXER})

source_group("Source files" FILES ${PRECOMPILE_SOURCE} ${SOURCE_FILES_APIX} ${SOURCE_FILES_MULTIPLEXER})

source_group("Header files" FILES ${HEADER_FILES_MULTIPLEXER})
source_group("Header files\\Public" FILES ${HEADER_FILES_APIX})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

if(BUILD_LIBSIMUTRACEX)
    set(libsimutraceX_GUID_CMAKE "B27D9ADD-8085-4D7A-80CE-A0A6F4B401E4" CACHE INTERNAL "libsimutraceX GUID")

    add_library(libsimutraceX SHARED
                ${PRECOMPILE_SOURCE}
                ${SOURCE_FILES_NOTPRECOMPILED}
                ${SOURCE_FILES_PRECOMPILED}
                ${SOURCE_FILES_RESOURCES}
                ${HEADER_FILES})

    set_target_properties(libsimutraceX PROPERTIES
                          PREFIX ""
                          VERSION ${SIMUTRACE_VERSION}
                          SOVERSION "1")

    target_link_libraries(libsimutraceX
                          libsimutrace
                          libsimubase
                          libconfig++
                          ${RT_LIBRARY}
                          ${CMAKE_THREAD_LIBS_INIT}  # pthreads for Linux
                          )

    add_precompiled_header(libsimutraceX
                           SOURCE_FILES_NOTPRECOMPILED
                           ${PRECOMPILE_HEADER}
                           ${PRECOMPILE_SOURCE})

    set_multiprocessor_compilation(libsimutraceX)
    set_sdl_compilation(libsimutraceX)

endif()

# Install ---------------------------------------------------------------------

set(HEADER_FILES_APIX_SIMUTRACE
    ${HEADER_FILES_APIX})

if(UNIX AND BUILD_LIBSIMUTRACEX)
    install(TARGETS libsimutraceX
            LIBRARY DESTINATION ${INSTALL_LIB_DIR})

    install(FILES ${HEADER_FILES_APIX_SIMUTRACE} DESTINATION ${INSTALL_INCLUDE_DIR})
endif()