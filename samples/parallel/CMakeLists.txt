# parallel makefile
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
# Parallel Client sample (parallel) is part of Simutrace.
#
# parallel is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# parallel is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with parallel. If not, see <http://www.gnu.org/licenses/>.
#

add_definitions(/DSIMUTRACE)

include_directories("../../simutrace/include/simubase")

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

source_group("" FILES ${OTHER_FILES_OTHERS})

set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)


# Build -----------------------------------------------------------------------

add_executable(parallel ${SOURCE_FILES})

target_link_libraries(parallel
                      libsimutrace
                      libsimubase
                      ${RT_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}  # pthreads for Linux
                      )

append_target_property(parallel FOLDER "Samples")
set_sdl_compilation(parallel)