# appendclose makefile
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
# Append close test (tst_appendclose) is part of Simutrace.
#
# tst_appendclose is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# tst_appendclose is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with tst_appendclose. If not, see <http://www.gnu.org/licenses/>.
#

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

if(BUILD_TESTS)
    set(tst_appendclose_GUID_CMAKE "68295699-5782-419D-A2FB-A57AEFAEE1C4" CACHE INTERNAL "tst_appendclose GUID")

    add_executable(tst_appendclose ${SOURCE_FILES})

    target_link_libraries(tst_appendclose
                          libsimutrace)

    append_target_property(tst_appendclose FOLDER "Tests")
    set_sdl_compilation(tst_appendclose)
endif()