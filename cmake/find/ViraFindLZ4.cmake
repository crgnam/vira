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

# Create alias for the other case variant to ensure compatibility
# This handles the case where viraTargets.cmake was built with one variant
# but the downstream user's system provides the other
if(TARGET lz4::lz4 AND NOT TARGET LZ4::lz4)
    add_library(LZ4::lz4 INTERFACE IMPORTED)
    set_target_properties(LZ4::lz4 PROPERTIES INTERFACE_LINK_LIBRARIES lz4::lz4)
endif()
if(TARGET LZ4::lz4 AND NOT TARGET lz4::lz4)
    add_library(lz4::lz4 INTERFACE IMPORTED)
    set_target_properties(lz4::lz4 PROPERTIES INTERFACE_LINK_LIBRARIES LZ4::lz4)
endif()

mark_as_system_library(${lz4_TARGET})

if(lz4_TARGET AND NOT TARGET ${lz4_TARGET})
    # lz4_TARGET is a string (e.g. LZ4::lz4) but no such target exists.
    # This happens on Windows/conda.
    add_library(${lz4_TARGET} UNKNOWN IMPORTED)

    set_target_properties(${lz4_TARGET} PROPERTIES
        IMPORTED_LOCATION "${LZ4_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
    )
endif()

# ------------------------------------------------------------------
# Normalize LZ4 target name
# Canonical target: lz4::lz4
# ------------------------------------------------------------------
if(TARGET LZ4::lz4 AND NOT TARGET lz4::lz4)
    add_library(lz4::lz4 ALIAS LZ4::lz4)
endif()

if(TARGET lz4::lz4)
    set(lz4_TARGET lz4::lz4)
endif()