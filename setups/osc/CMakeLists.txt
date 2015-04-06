# Simutrace openSUSE spec file CMake configuration
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

if(BUILD_OPENSUSE_OSC)

    set(SPECFILE "${EXECUTABLE_OUTPUT_PATH}/osc/simutrace.spec")

    configure_file("simutrace.spec.in" ${SPECFILE} @ONLY)

    file(READ "${PROJECT_SOURCE_DIR}/CHANGELOG" CONTENTS)
    string(REGEX REPLACE ";" "\\\\;" CONTENTS "${CONTENTS}")
    string(REGEX REPLACE "\n" ";" CONTENTS "${CONTENTS}")

    foreach(LINE ${CONTENTS})
        if (LINE MATCHES "^Version ([0-9]+\\.[0-9]+\\.[0-9]+) (.+) <(.+@.+)>  (.+), ([0-9]+) ([^0-9]+) ([0-9]+) (.+)$")
            # This is a line introducing a new version
            file(APPEND ${SPECFILE} "\n* ${CMAKE_MATCH_4} ${CMAKE_MATCH_6} ${CMAKE_MATCH_5} ${CMAKE_MATCH_7} ${CMAKE_MATCH_2},${CMAKE_MATCH_3}\n")
        elseif(LINE MATCHES "^- (.*)")
            # This is a line describing a change
            file(APPEND ${SPECFILE} "- ${CMAKE_MATCH_1}\n")
        else()
            file(APPEND ${SPECFILE} "\n")
        endif()
    endforeach()

endif()