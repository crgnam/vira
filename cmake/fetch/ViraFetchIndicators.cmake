# ViraFetchIndicators.cmake checks for indicators and falls back to FetchContent:
include(FetchContent)
include(MarkSystemLibrary)

# Prevent multiple inclusions
if(FETCHINDICATORS_INCLUDED)
    return()
endif()
set(FETCHINDICATORS_INCLUDED TRUE)

set(used_fetchcontent FALSE)

# Try vcpkg/standard pattern first
find_package(indicators CONFIG QUIET)
if(NOT indicators_FOUND)
    # Try without CONFIG (for other package managers)
    find_package(indicators QUIET)
endif()

if(NOT indicators_FOUND)
    # Fall back to FetchContent
    FetchContent_Declare(
        indicators
        GIT_REPOSITORY https://github.com/p-ranav/indicators.git
        GIT_TAG v2.3
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(indicators)
    set(used_fetchcontent TRUE)
endif()

# Verify the target exists
if(NOT TARGET indicators::indicators)
    message(FATAL_ERROR "indicators::indicators target not found. This indicates a problem with the indicators package or FetchContent.")
endif()

# Mark as system
mark_as_system_library(indicators::indicators)

# Install headers if we used FetchContent
if(used_fetchcontent)
    if(NOT indicators_SOURCE_DIR)
        message(FATAL_ERROR "FetchContent was used for indicators but indicators_SOURCE_DIR is not set - this indicates a FetchContent failure")
    endif()

    message(STATUS "INDICATORS Used FetchContent: ${indicators_SOURCE_DIR}")

    install(DIRECTORY "${indicators_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
else()
    message(STATUS "INDICATORS - Used system/package manager")
endif()