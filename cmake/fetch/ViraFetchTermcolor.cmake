# ViraFetchTermcolor.cmake checks for termcolor and falls back to FetchContent:
include(FetchContent)
include(MarkSystemLibrary)

# Prevent multiple inclusions
if(FETCHTERMCOLOR_INCLUDED)
    return()
endif()
set(FETCHTERMCOLOR_INCLUDED TRUE)

set(used_fetchcontent FALSE)

# Try standard find_package first (works for some package managers)
find_package(termcolor QUIET)

if(NOT termcolor_FOUND)
    # Try vcpkg pattern
    find_path(TERMCOLOR_INCLUDE_DIRS "termcolor/termcolor.hpp")
    if(TERMCOLOR_INCLUDE_DIRS)
        add_library(termcolor::termcolor INTERFACE IMPORTED)
        target_include_directories(termcolor::termcolor INTERFACE ${TERMCOLOR_INCLUDE_DIRS})
        set(termcolor_FOUND TRUE)
    endif()
endif()

if(NOT termcolor_FOUND)
    # Fall back to FetchContent
    FetchContent_Declare(
        termcolor
        GIT_REPOSITORY https://github.com/ikalnytskyi/termcolor.git
        GIT_TAG v2.1.0
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(termcolor)
    set(used_fetchcontent TRUE)
endif()

# Verify target exists
if(NOT TARGET termcolor::termcolor)
    message(FATAL_ERROR "termcolor::termcolor target not found")
endif()

mark_as_system_library(termcolor::termcolor)

# Install headers if we used FetchContent
if(used_fetchcontent)
    if(NOT termcolor_SOURCE_DIR)
        message(FATAL_ERROR "FetchContent was used for termcolor but termcolor_SOURCE_DIR is not set - this indicates a FetchContent failure")
    endif()

    message(STATUS "TERMCOLOR Used FetchContent: ${termcolor_SOURCE_DIR}")

    install(DIRECTORY "${termcolor_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
else()
    message(STATUS "TERMCOLOR - Used system/package manager")
endif()