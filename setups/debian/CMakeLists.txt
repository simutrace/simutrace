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

if(BUILD_DEBIAN)

    set(DEBIAN "${EXECUTABLE_OUTPUT_PATH}/debian/")

    configure_file("control" "${DEBIAN}/control" COPYONLY)
    configure_file("compat" "${DEBIAN}/compat" COPYONLY)
    configure_file("copyright" "${DEBIAN}/copyright" COPYONLY)
    configure_file("libsimutrace.install" "${DEBIAN}/libsimutrace.install" COPYONLY)
    configure_file("libsimutrace-dev.install" "${DEBIAN}/libsimutrace-dev.install" COPYONLY)
    configure_file("simutrace-server.install" "${DEBIAN}/simutrace-server.install" COPYONLY)
    configure_file("rules" "${DEBIAN}/rules" COPYONLY)

    file(MAKE_DIRECTORY "${DEBIAN}/source")
    configure_file("source/format" "${DEBIAN}/source/format" COPYONLY)

    set(CHANGELOG "${DEBIAN}/changelog")

    file(REMOVE ${CHANGELOG})
    file(READ "${PROJECT_SOURCE_DIR}/CHANGELOG" CONTENTS)
    string(REGEX REPLACE ";" "\\\\;" CONTENTS "${CONTENTS}")
    string(REGEX REPLACE "\n" ";" CONTENTS "${CONTENTS}")

    foreach(LINE ${CONTENTS})
        if (LINE MATCHES "^Version ([0-9]+\\.[0-9]+\\.[0-9]+) (.+)$")
            # This is a line introducing a new version
            if(SIG)
                file(APPEND ${CHANGELOG} "${SIG}\n")
            endif()

            file(APPEND ${CHANGELOG} "simutrace (${CMAKE_MATCH_1}-1) STABLE; urgency=medium\n\n")
            set(SIG "\n -- ${CMAKE_MATCH_2}\n")

        elseif(LINE MATCHES "^- (.*)")
            # This is a line describing a change
            file(APPEND ${CHANGELOG} "  * ${CMAKE_MATCH_1}\n")
        else()
            file(APPEND ${CHANGELOG} "\n")
        endif()
    endforeach()

    if(SIG)
        file(APPEND ${CHANGELOG} "${SIG}")
    endif()

endif()