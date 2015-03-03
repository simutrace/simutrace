# SimuTrace.CSharp CMake configuration
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
# Simutrace CSharp Binding (SimuTrace.CSharp) is part of Simutrace.
#
# SimuTrace.CSharp is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SimuTrace.CSharp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with SimuTrace.CSharp. If not, see <http://www.gnu.org/licenses/>.
#

# Base

set(SOURCE_FILES_NATIVE
    "Native.cs")

# Others

set(OTHER_FILES_OTHERS
    "AssemblyInfo.cs"
    "CMakeLists.txt")


# Source Code Grouping --------------------------------------------------------

source_group("" FILES ${SOURCE_FILES_NATIVE} ${OTHER_FILES_OTHERS})


# Build -----------------------------------------------------------------------

if(MSVC)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SOURCE_DIR)

    configure_file("SimuTrace.CSharp.csproj-template"
                   "${CMAKE_CURRENT_BINARY_DIR}/SimuTrace.CSharp.csproj"
                   NEWLINE_STYLE WIN32)

    include_external_msproject(
        SimuTrace.CSharp
        "${CMAKE_CURRENT_BINARY_DIR}/SimuTrace.CSharp.csproj"
        GUID 1984D5BF-4BB4-4AE3-BB4B-34E485CE2C20
        TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC)

    append_target_property(SimuTrace.CSharp FOLDER "Bindings")
endif()
