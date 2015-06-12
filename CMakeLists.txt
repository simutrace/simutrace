# Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)
# Marc Rittinghaus
#
#             _____ _                 __
#            / ___/(_)___ ___  __  __/ /__________ _________
#            \__ \/ / __ `__ \/ / / / __/ ___/ __ `/ ___/ _ \
#           ___/ / / / / / / / /_/ / /_/ /  / /_/ / /__/  __/
#          /____/_/_/ /_/ /_/\__,_/\__/_/   \__,_/\___/\___/
#                         http://simutrace.org
#
#
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

cmake_policy(SET CMP0009 NEW) # Do not follow symlinks in file glob recurse

# General Project Settings and Dependencies -----------------------------------

set(CMAKE_CONFIGURATION_TYPES Release Debug)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory("3rdparty/libconfig-1.4.9")

project(Simutrace C CXX)

file(READ "${PROJECT_SOURCE_DIR}/VERSION" _VERSION)
if (NOT _VERSION MATCHES "([0-9]+).([0-9]+).([0-9]+)")
    message(FATAL_ERROR "Version information in VERSION file not compatible. Must be 'major.minor.revision'")
endif()

set(SIMUTRACE_VERSION_MAJOR ${CMAKE_MATCH_1})
set(SIMUTRACE_VERSION_MINOR ${CMAKE_MATCH_2})
set(SIMUTRACE_VERSION_REVISION ${CMAKE_MATCH_3})
set(SIMUTRACE_VERSION ${SIMUTRACE_VERSION_MAJOR}.${SIMUTRACE_VERSION_MINOR}.${SIMUTRACE_VERSION_REVISION})
message(STATUS "Detected version: ${SIMUTRACE_VERSION}")

find_package(Threads REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

    if(NOT APPLE)
        set(RT_LIBRARY "rt")
    endif()
endif()

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/libconfig-1.4.9")
include_directories("${PROJECT_SOURCE_DIR}/simutrace/include")

# Build settings --------------------------------------------------------------

set(BUILD_SAMPLES OFF CACHE BOOL "Build Samples")
set(BUILD_LIBSIMUTRACE ON CACHE BOOL "Build Client Library")
set(BUILD_LIBSIMUTRACEX ON CACHE BOOL "Build Extensions Library")
set(BUILD_SIMUTRACE_SERVER ON CACHE BOOL "Build Storage Server")

if(UNIX)
    set(INSTALL_LIB_DIR "lib" CACHE STRING "Library Installation Directory")
    set(INSTALL_BIN_DIR "bin" CACHE STRING "Binary Installation Directory")
    set(INSTALL_INCLUDE_DIR "include" CACHE STRING "Header Installation Directory")
else()
    if("${CMAKE_GENERATOR}" MATCHES "Visual Studio [^6]")
        set(BUILD_DOTNET_INTEROP ON CACHE BOOL "Build .Net Interop Library")
        set(BUILD_WINDOWS_SETUP OFF CACHE BOOL "Build Windows Setup")
    endif()
endif()

set(BUILD_SRCTARBALL ON CACHE BOOL "Build source tarball")
set(BUILD_OPENSUSE_OSC ON CACHE BOOL "Build package data for openSUSE build service")
set(BUILD_DEBIAN ON CACHE BOOL "Build package data for debian")

# Macros ----------------------------------------------------------------------

macro(append_target_property target
                             property
                             value)

    get_target_property(current_property ${target} ${property})
    if(NOT current_property)
        set_target_properties(${target} PROPERTIES ${property} ${value})
    else()
        set_target_properties(${target} PROPERTIES ${property}
                              "${current_property} ${value}")
    endif()

endmacro()

macro(append_source_property source
                             property
                             value)

    get_source_file_property(current_property ${source} ${property})
    if(NOT current_property)
        set_source_files_properties(${source} PROPERTIES ${property} ${value})
    else()
        set_source_files_properties(${source} PROPERTIES ${property}
                                    "${current_property} ${value}")
    endif()

endmacro()

macro(add_precompiled_header target
                             excludedSourcesVar
                             precompiledHeader
                             precompiledSource)

    if(MSVC)
        get_filename_component(precompiledBasename ${precompiledHeader} NAME_WE)
        set(precompiledBinary "$(IntDir)/${precompiledBasename}.pch")

        # Set the precompiled source file to create the pch
        append_source_property(${precompiledSource} COMPILE_FLAGS
                               "/Yc\"${precompiledHeader}\" "
                               "/Fp\"${precompiledBinary}\"")
        append_source_property(${precompiledSource} OBJECT_OUTPUTS
                               "${precompiledBinary}")

        # Activate precompiled headers for the target project
        append_target_property(${target} COMPILE_FLAGS
                               "/Yu\"${precompiledHeader}\" "
                               "/FI\"${precompiledHeader}\" "
                               "/Fp\"${precompiledBinary}\"")
        append_target_property(${target} OBJECT_DEPENDS
                               "${precompiledBinary}")

        # If there are files that should not use precompiled headers, exclude
        # them here with the /Y- flag
        foreach(source ${${excludedSourcesVar}})
            append_source_property(${source} COMPILE_FLAGS "/Y-")
        endforeach()
    endif()

endmacro()

macro(set_multiprocessor_compilation target)
    if(MSVC)
        append_target_property(${target} COMPILE_FLAGS "/MP")
    endif()
endmacro()

macro(set_sdl_compilation target)
    if(MSVC)
        append_target_property(${target} COMPILE_FLAGS "/sdl")
    endif()
endmacro()

# Projects --------------------------------------------------------------------

# Simutrace
add_subdirectory(simutrace/libsimubase)
add_subdirectory(simutrace/libsimustor)
add_subdirectory(simutrace/libsimutrace)
add_subdirectory(simutrace/libsimutraceX)
add_subdirectory(simutrace/storageserver)

# Bindings
add_subdirectory(simutrace/bindings/dotnet)

# Samples
add_subdirectory(samples/simple)
add_subdirectory(samples/parallel)
add_subdirectory(samples/storemon)
add_subdirectory(samples/csharp.simple)
add_subdirectory(samples/csharp.memreplay)

# Documentation
add_subdirectory(simutrace/documentation)

# Setup
add_subdirectory(setups/windows)
add_subdirectory(setups/tarball)
add_subdirectory(setups/osc)
add_subdirectory(setups/debian)
