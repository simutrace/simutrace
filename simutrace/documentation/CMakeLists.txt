# Simutrace Documentation CMake configuration
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
# Simutrace Documentation (doc) is part of Simutrace.
#
# Simutrace is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Simutrace is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Simutrace. If not, see <http://www.gnu.org/licenses/>.
#

# Build -----------------------------------------------------------------------

find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(simutrace-doc_GUID_CMAKE "F35CE8E7-117D-443B-924B-AEE73B6C97E5" CACHE INTERNAL "simutrace-doc GUID")

    configure_file("PublicApi.doxyfile.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/PublicApi.doxyfile"
                   @ONLY)

    add_custom_target(simutrace-doc
                      "${DOXYGEN_EXECUTABLE}"
                      "${CMAKE_CURRENT_BINARY_DIR}/PublicApi.doxyfile"
                      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                      COMMENT "Generating API documentation" VERBATIM
                      SOURCES "${CMAKE_CURRENT_BINARY_DIR}/PublicApi.doxyfile")

    add_custom_command(TARGET simutrace-doc
                       POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                       "${CMAKE_CURRENT_SOURCE_DIR}/theme"
                       "${EXECUTABLE_OUTPUT_PATH}/doc/html/")

    append_target_property(simutrace-doc FOLDER "Documentation")

endif()