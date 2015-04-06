# storemon makefile
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
# Store Monitor sample (storemon) is part of Simutrace.
#
# storemon is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# storemon is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with storemon. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

include_directories("../../simutrace/include/simubase"
                    "${CMAKE_CURRENT_BINARY_DIR}/../../simutrace/include/")

# Base

set(SOURCE_FILES_BASE
    "main.cpp")

# Others

set(OTHER_FILES_OTHERS
    "CMakeLists.txt")


# Source Code Grouping --------------------------------------------------------

set(SOURCE_FILES
    ${SOURCE_FILES_BASE})

set(OTHER_FILES
    ${OTHER_FILES_OTHERS})

source_group("Source files" FILES ${SOURCE_FILES})

source_group("" FILES ${OTHER_FILES})


# Build -----------------------------------------------------------------------

if(BUILD_SAMPLES)
    set(storemon_GUID_CMAKE "0B401DC9-3F3B-4A2D-B3C4-1DB08D61E50E" CACHE INTERNAL "storemon GUID")

    add_executable(storemon ${SOURCE_FILES})

    target_link_libraries(storemon
                          libsimutrace
                          libsimubase
                          ${RT_LIBRARY}
                          ${CMAKE_THREAD_LIBS_INIT}  # pthreads for Linux
                          )

    append_target_property(storemon FOLDER "Samples")
    set_sdl_compilation(storemon)

endif()