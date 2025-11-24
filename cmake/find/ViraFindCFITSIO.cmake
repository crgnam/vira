# ViraFindCFITSIO.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(cfitsio CONFIG QUIET)
if(cfitsio_FOUND)
    message(STATUS "cfitsio CONFIG found")
    # vcpkg might provide different target names, check what's available
    if(TARGET cfitsio)
        set(cfitsio_TARGET cfitsio)
        message(STATUS "Found cfitsio")
    elseif(TARGET CFITSIO::CFITSIO)
        set(cfitsio_TARGET CFITSIO::CFITSIO)
        message(STATUS "Found CFITSIO::CFITSIO")
    else()
        message(STATUS "cfitsio CONFIG found but no expected targets exist")
        # Reset and try fallback logic
        set(cfitsio_FOUND FALSE)
    endif()
endif()

if(NOT cfitsio_FOUND)
    message(STATUS "Trying fallback cfitsio discovery")
    # Use your existing logic exactly as-is
    try_config_packages(cfitsio "unofficial-cfitsio;cfitsio" "cfitsio;CFITSIO::CFITSIO;cfitsio::cfitsio")
    if(NOT cfitsio_FOUND)
        try_pkgconfig(cfitsio "cfitsio" "cfitsio")
    endif()
    if(NOT cfitsio_FOUND)
        try_manual_discovery(cfitsio "cfitsio;libcfitsio" "fitsio.h" "cfitsio")
    endif()
    if(NOT cfitsio_FOUND)
        message(FATAL_ERROR "Could not find CFITSIO")
    endif()
endif()

# Verify we have a target
if(NOT cfitsio_TARGET)
    message(FATAL_ERROR "cfitsio_TARGET not set")
endif()

mark_as_system_library(${cfitsio_TARGET})