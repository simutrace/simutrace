# csharp.simple CMake configuration
#
# Copyright 2015 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus
#
# Simple Simutrace CSharp Sample (csharp.simple) is part of Simutrace.
#
# csharp.simple is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# csharp.simple is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with csharp.simple. If not, see <http://www.gnu.org/licenses/>.
#

# Base

set(SOURCE_FILES_BASE
    "Program.cs")

# Others

set(SOURCE_FILES_RESOURCES
    "AssemblyInfo.cs")


# Source Code Grouping --------------------------------------------------------

source_group("" FILES ${SOURCE_FILES_BASE} ${SOURCE_FILES_RESOURCES})


# Build -----------------------------------------------------------------------

if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio [^6]") AND (BUILD_SAMPLES))

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SOURCE_DIR)

    configure_file("CSharp.Simple.csproj.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/CSharp.Simple.csproj"
                   NEWLINE_STYLE WIN32)

    include_external_msproject(
        CSharp.Simple
        "${CMAKE_CURRENT_BINARY_DIR}/CSharp.Simple.csproj"
        GUID "C59E70A8-B4A4-4445-80C2-30CD28895864")

    append_target_property(CSharp.Simple FOLDER "Samples")

    add_dependencies(CSharp.Simple
                     SimuTrace.Net.Interop)

endif()