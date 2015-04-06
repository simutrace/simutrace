# Simutrace Source Tarball CMake configuration
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

if(BUILD_SRCTARBALL)

    set(TARBALL_BASE_PATH "${EXECUTABLE_OUTPUT_PATH}/tarball")
    set(TARBALL_CONTENT_PATH "${TARBALL_BASE_PATH}/simutrace-${SIMUTRACE_VERSION}/")
    
    file(MAKE_DIRECTORY "${TARBALL_BASE_PATH}")

    file(GLOB_RECURSE FILES RELATIVE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/*")
    file(GLOB_RECURSE EXCLUDEFILES RELATIVE "${PROJECT_SOURCE_DIR}"
         "${PROJECT_SOURCE_DIR}/.git"
         "${PROJECT_SOURCE_DIR}/.gitmodules"
         "${PROJECT_SOURCE_DIR}/.git/*"
         "${PROJECT_SOURCE_DIR}/simulation/*"
         "${CMAKE_BINARY_DIR}/*")

    list(REMOVE_ITEM FILES ${EXCLUDEFILES})

    add_custom_target(srctarball                      
                      COMMAND ${CMAKE_COMMAND} -E remove_directory
                      "${TARBALL_CONTENT_PATH}")

    foreach(FILE ${FILES})
        add_custom_command(TARGET srctarball
                           COMMAND ${CMAKE_COMMAND} -E copy
                           "${PROJECT_SOURCE_DIR}/${FILE}"
                           "${TARBALL_CONTENT_PATH}/${FILE}")
    endforeach()

    add_custom_command(TARGET srctarball
                      COMMAND ${CMAKE_COMMAND} -E tar "cfvz"
                      "${TARBALL_BASE_PATH}/simutrace-${SIMUTRACE_VERSION}.tar.gz"
                      "${TARBALL_CONTENT_PATH}"
                      WORKING_DIRECTORY "${TARBALL_BASE_PATH}")

    append_target_property(srctarball FOLDER "Setup")

endif()
