# csharp.memreplay CMake configuration
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
    "Program.cs"
    "RamBitmap.cs"
    "RamMap.cs"
    "Replay.cs")

# Gui

set(SOURCE_FILES_GUI
    "MainForm.cs"
    "MainForm.Designer.cs"
    "MainForm.resx"
    "ReplayForm.cs"
    "ReplayForm.Designer.cs"
    "ReplayForm.resx"
    "ScreenForm.cs"
    "ScreenForm.Designer.cs"
    "ScreenForm.resx")

# Others

set(SOURCE_FILES_RESOURCES
    "AssemblyInfo.cs")


# Source Code Grouping --------------------------------------------------------

source_group("" FILES ${SOURCE_FILES_BASE} ${SOURCE_FILES_GUI} ${SOURCE_FILES_RESOURCES})


# Build -----------------------------------------------------------------------

if(("${CMAKE_GENERATOR}" MATCHES "Visual Studio [^6]") AND (BUILD_SAMPLES))

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SOURCE_DIR)

    configure_file("CSharp.MemReplay.csproj.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/CSharp.MemReplay.csproj"
                   NEWLINE_STYLE WIN32)

    include_external_msproject(
        CSharp.MemReplay
        "${CMAKE_CURRENT_BINARY_DIR}/CSharp.MemReplay.csproj"
        GUID "66B8B063-B3B9-441A-ABEE-946F84E3734E")

    append_target_property(CSharp.MemReplay FOLDER "Samples")

    add_dependencies(CSharp.MemReplay
                     SimuTrace.Net.Interop)

endif()