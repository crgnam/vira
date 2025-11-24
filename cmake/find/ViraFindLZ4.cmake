# ViraFindLZ4.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(lz4 CONFIG QUIET)
if(lz4_FOUND)
    # vcpkg might provide different target names, check what's available
    if(TARGET lz4::lz4)
        set(lz4_TARGET lz4::lz4)
        message(STATUS "Found lz4::lz4")
    elseif(TARGET LZ4::lz4)
        set(lz4_TARGET LZ4::lz4)
        message(STATUS "Found LZ4::lz4")
    endif()
endif()

if(NOT lz4_FOUND)
    # LZ4 - try manual discovery first to avoid conda's broken config
    try_manual_discovery(lz4 "lz4;liblz4" "lz4.h" "lz4::lz4")
    if(NOT lz4_FOUND)
        try_config_packages(lz4 "lz4" "lz4::lz4")
    endif()
    if(NOT lz4_FOUND)
        try_config_packages(lz4 "LZ4" "LZ4::lz4")
    endif()
    if(NOT lz4_FOUND)
        try_pkgconfig(lz4 "liblz4" "lz4::lz4")
    endif()
    if(NOT lz4_FOUND)
        message(FATAL_ERROR "Could not find LZ4")
    endif()
endif()

# Verify we have a target
if(NOT lz4_TARGET)
    message(FATAL_ERROR "lz4_TARGET not set")
endif()

mark_as_system_library(${lz4_TARGET})