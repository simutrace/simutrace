# libsimustor CMake configuration
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
# Simutrace Store Library (libsimustor) is part of Simutrace.
#
# libsimustor is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libsimustor is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libsimustor. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

set(SIMUSTOR_INCLUDE "../include/simustor")
include_directories("../include/simubase"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include/"
                    "${SIMUSTOR_INCLUDE}")

set(PRECOMPILE_SOURCE "main.cpp")
set(PRECOMPILE_HEADER "SimuBase.h")

# Base

set(HEADER_FILES_STOR_INTERNAL
    "../include/SimuStor.h"
    "${SIMUSTOR_INCLUDE}/SimuStorOptions.h")

set(HEADER_FILES_STOR_API
    "../include/SimuStorTypes.h")

set(HEADER_FILES_STOR_API ${HEADER_FILES_STOR_API} PARENT_SCOPE)


# Network

set(HEADER_FILES_NETWORK
    "${SIMUSTOR_INCLUDE}/RpcProtocol.h")


# Session

set(SOURCE_FILES_SESSION
    "Session.cpp"
    "SessionManager.cpp")

set(HEADER_FILES_SESSION
    "${SIMUSTOR_INCLUDE}/Session.h"
    "${SIMUSTOR_INCLUDE}/SessionManager.h")


# Storage

set(SOURCE_FILES_STORAGE
    "Store.cpp")

set(HEADER_FILES_STORAGE
    "${SIMUSTOR_INCLUDE}/Store.h")


# Stream

set(SOURCE_FILES_STREAMS
    "Stream.cpp"
    "StreamBuffer.cpp")

set(HEADER_FILES_STREAMS
    "${SIMUSTOR_INCLUDE}/Stream.h"
    "${SIMUSTOR_INCLUDE}/StreamBuffer.h")


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES_NOTPRECOMPILED "")

set(SOURCE_FILES_PRECOMPILED
    ${SOURCE_FILES_SESSION}
    ${SOURCE_FILES_STORAGE}
    ${SOURCE_FILES_STREAMS})

set(HEADER_FILES
    ${HEADER_FILES_STOR_API}
    ${HEADER_FILES_STOR_INTERNAL}
    ${HEADER_FILES_NETWORK}
    ${HEADER_FILES_SESSION}
    ${HEADER_FILES_STORAGE}
    ${HEADER_FILES_STREAMS})

source_group("Source files" FILES ${PRECOMPILE_SOURCE})
source_group("Source files\\Session" FILES ${SOURCE_FILES_SESSION})
source_group("Source files\\Storage" FILES ${SOURCE_FILES_STORAGE})
source_group("Source files\\Streams" FILES ${SOURCE_FILES_STREAMS})

source_group("Header files\\Public" FILES ${HEADER_FILES_STOR_API} ${HEADER_FILES_STOR_INTERNAL})
source_group("Header files\\Public\\Network" FILES ${HEADER_FILES_NETWORK})
source_group("Header files\\Public\\Session" FILES ${HEADER_FILES_SESSION})
source_group("Header files\\Public\\Storage" FILES ${HEADER_FILES_STORAGE})
source_group("Header files\\Public\\Streams" FILES ${HEADER_FILES_STREAMS})

set_source_files_properties(${HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

set(libsimustor_GUID_CMAKE "D123C4C4-F561-494E-9E06-BADE186996B5" CACHE INTERNAL "libsimustor GUID")

add_library(libsimustor
            ${PRECOMPILE_SOURCE}
            ${SOURCE_FILES_NOTPRECOMPILED}
            ${SOURCE_FILES_PRECOMPILED}
            ${HEADER_FILES})

set_target_properties(libsimustor PROPERTIES PREFIX "")
target_link_libraries(libsimustor libsimubase)

add_precompiled_header(libsimustor
                       SOURCE_FILES_NOTPRECOMPILED
                       ${PRECOMPILE_HEADER}
                       ${PRECOMPILE_SOURCE})

set_multiprocessor_compilation(libsimustor)
set_sdl_compilation(libsimustor)
