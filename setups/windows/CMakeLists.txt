# Simutrace Windows Setup CMake configuration
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
# Simutrace Windows Setup (winsetup) is part of Simutrace.
#
# Simutrace Windows Setup is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Simutrace Windows Setup is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Simutrace Windows Setup. If not, see <http://www.gnu.org/licenses/>.
#

if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio [^6]") AND (BUILD_WINDOWS_SETUP))

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    find_package(WiX REQUIRED)

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SOURCE_DIR)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" BINARY_DIR)
    file(TO_NATIVE_PATH "${EXECUTABLE_OUTPUT_PATH}" EOUTPUT_PATH)

    if(WIX_FOUND)
        configure_file("config.wxi.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/config.wxi")

        configure_file("winmsi.wixproj.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/winmsi.wixproj")

        include_external_msproject(winmsi
            "${CMAKE_CURRENT_BINARY_DIR}/winmsi.wixproj"
            GUID "97bf4191-db57-4f12-aae6-3aa1b1a21dcb")

        add_dependencies(winmsi
                         simutrace-doc
                         SimuTrace.Net.Interop
                         libsimutrace
                         libsimutraceX
                         libconfig++
                         simustore)

        append_target_property(winmsi FOLDER "Setup")
    endif()

endif()