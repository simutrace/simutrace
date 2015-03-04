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
cmake_minimum_required(VERSION 2.8.9)

# General Project Settings and Dependencies -----------------------------------

set(CMAKE_CONFIGURATION_TYPES Release Debug)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
message("Path: ${CMAKE_MODULE_PATH}")

add_subdirectory("3rdparty/libconfig-1.4.9")

project(Simutrace C CXX)
set(SIMUTRACE_VERSION_MAJOR 3)
set(SIMUTRACE_VERSION_MINOR 1)
set(SIMUTRACE_VERSION_REVISION 1)
set(SIMUTRACE_VERSION ${SIMUTRACE_VERSION_MAJOR}.${SIMUTRACE_VERSION_MINOR}.${SIMUTRACE_VERSION_REVISION})

find_package(Threads REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
    set(RT_LIBRARY "rt")
endif()

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/libconfig-1.4.9")
include_directories("${PROJECT_SOURCE_DIR}/simutrace/include")


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

add_subdirectory(simutrace/libsimubase)
add_subdirectory(simutrace/libsimustor)
add_subdirectory(simutrace/libsimutrace)
add_subdirectory(simutrace/storageserver)

add_subdirectory(simutrace/bindings/csharp)

add_subdirectory(samples/simple)
add_subdirectory(samples/parallel)
add_subdirectory(samples/storemon)
add_subdirectory(samples/csharp.simple)
