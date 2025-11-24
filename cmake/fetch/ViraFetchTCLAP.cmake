# ViraFetchTCLAP.cmake checks for TCLAP and falls back to FetchContent:
include(FetchContent)
include(MarkSystemLibrary)

# Prevent multiple inclusions
if(FETCHTCLAP_INCLUDED)
    return()
endif()
set(FETCHTCLAP_INCLUDED TRUE)

set(used_fetchcontent FALSE)

# Try vcpkg pattern first
find_path(TCLAP_INCLUDE_DIRS "tclap/Arg.h")
if(TCLAP_INCLUDE_DIRS)
    # vcpkg provides include directory but no target, so create one
    add_library(tclap::tclap INTERFACE IMPORTED)
    target_include_directories(tclap::tclap INTERFACE ${TCLAP_INCLUDE_DIRS})
    set(tclap_FOUND TRUE)
endif()

if(NOT tclap_FOUND)
    # Try standard find_package (for other package managers)
    find_package(tclap QUIET)
endif()

if(NOT tclap_FOUND)
    # Fall back to FetchContent
    FetchContent_Declare(
        tclap
        GIT_REPOSITORY https://github.com/mirror/tclap.git
        GIT_TAG v1.2.5
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(tclap)
    
    # Create target for FetchContent version
    add_library(tclap::tclap INTERFACE IMPORTED)
    target_include_directories(tclap::tclap INTERFACE "${tclap_SOURCE_DIR}/include")
    set(used_fetchcontent TRUE)
endif()

# Verify target exists
if(NOT TARGET tclap::tclap)
    message(FATAL_ERROR "tclap::tclap target not found")
endif()

mark_as_system_library(tclap::tclap)

# Install headers if we used FetchContent
if(used_fetchcontent)
    if(NOT tclap_SOURCE_DIR)
        message(FATAL_ERROR "FetchContent was used for tclap but tclap_SOURCE_DIR is not set - this indicates a FetchContent failure")
    endif()

    message(STATUS "TCLAP Used FetchContent: ${tclap_SOURCE_DIR}")

    install(DIRECTORY "${tclap_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )

    set_property(DIRECTORY ${tclap_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL TRUE)

else()
    message(STATUS "TCLAP - Used system/package manager")
endif()