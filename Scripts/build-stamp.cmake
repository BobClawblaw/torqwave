# Writes OUT, a header saying what this build is: run at every build (see CMakeLists.txt).
# A release build is one made at the tag v<VERSION> with no uncommitted changes to tracked files,
# or by a workflow building that tag (RELEASE_TAG); any other is a test build, stamped with when
# it was built and its commit ("+changes" when the tree had uncommitted ones).

# A script run with -P starts with no policies set: this sets them as the project does.
cmake_minimum_required (VERSION 3.22)

execute_process (COMMAND git -C "${SRC}" rev-parse --short=7 HEAD
                 OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE noGit)
execute_process (COMMAND git -C "${SRC}" status --porcelain --untracked-files=no
                 OUTPUT_VARIABLE changes OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process (COMMAND git -C "${SRC}" tag --points-at HEAD
                 OUTPUT_VARIABLE tags OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if (noGit)
    set (commit "")
endif ()

set (release 0)
string (REPLACE "\n" ";" tagList "${tags}")
list (FIND tagList "v${VERSION}" tagAt)   # not IN_LIST: older CMake reads that only under policy CMP0057

if ((NOT tagAt EQUAL -1 AND changes STREQUAL "") OR "$ENV{RELEASE_TAG}" STREQUAL "v${VERSION}")
    set (release 1)
elseif (NOT changes STREQUAL "" AND NOT commit STREQUAL "")
    string (APPEND commit "+changes")
endif ()

string (TIMESTAMP built "%b %d %H:%M")

set (text "// Written by Scripts/build-stamp.cmake at every build: do not edit.\n#pragma once\n\n")
string (APPEND text "#define TORQWAVE_BUILD_RELEASE ${release}\n")
string (APPEND text "#define TORQWAVE_BUILD_TIME \"${built}\"\n")
string (APPEND text "#define TORQWAVE_BUILD_COMMIT \"${commit}\"\n")

set (old "")

if (EXISTS "${OUT}")
    file (READ "${OUT}" old)
endif ()

if (NOT old STREQUAL text)
    file (WRITE "${OUT}" "${text}")
endif ()
