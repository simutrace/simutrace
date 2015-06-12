# SimuTrace.Net.Interop CMake configuration
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
# Simutrace .Net Binding (SimuTrace.Net.Interop) is part of Simutrace.
#
# SimuTrace.Net.Interop is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SimuTrace.Net.Interop is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with SimuTrace.Net.Interop. If not, see <http://www.gnu.org/licenses/>.
#

# Base

set(SOURCE_FILES_NATIVE
    "Native.cs"
    "NativeMethods.cs"
    "NativeX.cs"
    "NativeMethodsX.cs")

# Others

set(SOURCE_FILES_RESOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/AssemblyInfo.cs")


# Source Code Grouping --------------------------------------------------------

source_group("" FILES ${SOURCE_FILES_NATIVE} ${SOURCE_FILES_RESOURCES})


# Build -----------------------------------------------------------------------

if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio [^6]") AND (BUILD_DOTNET_INTEROP))

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SOURCE_DIR)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" BINARY_DIR)
    file(TO_NATIVE_PATH "${EXECUTABLE_OUTPUT_PATH}" EOUTPUT_PATH)

    configure_file("AssemblyInfo.cs.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/AssemblyInfo.cs"
                   @ONLY)

    configure_file("SimuTrace.Net.Interop.csproj.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/SimuTrace.Net.Interop.csproj"
                   NEWLINE_STYLE WIN32)

    include_external_msproject(
        SimuTrace.Net.Interop
        "${CMAKE_CURRENT_BINARY_DIR}/SimuTrace.Net.Interop.csproj"
        GUID "5316CC10-F5C3-4606-A269-72A3060E6061")

    append_target_property(SimuTrace.Net.Interop FOLDER "Bindings")

    add_dependencies(SimuTrace.Net.Interop
                     libsimutrace
                     libsimutraceX
                     libconfig++)

endif()