# ViraFindTIFF.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(TIFF CONFIG QUIET)
if(TARGET TIFF::TIFF)
    set(tiff_FOUND TRUE)
    set(tiff_TARGET TIFF::TIFF)
    message(STATUS "Found TIFF::TIFF")
else()
    # Use your existing logic exactly as-is, but only if vcpkg didn't work
    try_config_packages(tiff "TIFF;tiff" "TIFF::TIFF;TIFF::tiff;tiff")
    if(NOT tiff_FOUND)
        try_pkgconfig(tiff "libtiff-4" "TIFF::TIFF")
    endif()
    if(NOT tiff_FOUND)
        try_manual_discovery(tiff "tiff;libtiff;tiff4;libtiff4" "tiff.h;tiffio.h" "TIFF::TIFF")
    endif()
    if(NOT tiff_FOUND)
        message(FATAL_ERROR "Could not find TIFF")
    endif()
endif()

# Verify we have a target
if(NOT tiff_TARGET)
    message(FATAL_ERROR "tiff_TARGET not set")
endif()

mark_as_system_library(${tiff_TARGET})