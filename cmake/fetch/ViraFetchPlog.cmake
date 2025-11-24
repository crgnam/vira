# ViraFetchPlog.cmake checks for plog and falls back to FetchContent:
include(FetchContent)
include(MarkSystemLibrary)

# Prevent multiple inclusions
if(FETCHPLOG_INCLUDED)
    return()
endif()
set(FETCHPLOG_INCLUDED TRUE)

set(used_fetchcontent FALSE)

# Try vcpkg/standard pattern first
find_package(plog CONFIG QUIET)
if(NOT plog_FOUND)
    # Try without CONFIG (for other package managers)
    find_package(plog QUIET)
endif()

if(NOT plog_FOUND)
    # Fall back to FetchContent
    FetchContent_Declare(
        plog
        GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
        GIT_TAG 1.1.10
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(plog)
    set(used_fetchcontent TRUE)
endif()

# Verify the target exists
if(NOT TARGET plog::plog)
    message(FATAL_ERROR "plog::plog target not found. This indicates a problem with the plog package or FetchContent.")
endif()

# Mark as system
mark_as_system_library(plog::plog)

# Install headers if we used FetchContent
if(used_fetchcontent)
    if(NOT plog_SOURCE_DIR)
        message(FATAL_ERROR "FetchContent was used for plog but plog_SOURCE_DIR is not set - this indicates a FetchContent failure")
    endif()
    
    message(STATUS "PLOG Used FetchContent: ${plog_SOURCE_DIR}")

    install(DIRECTORY "${plog_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
else()
    message(STATUS "PLOG - Used system/package manager")
endif()