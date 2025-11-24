# ViraFindGDAL.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(GDAL CONFIG QUIET)
if(GDAL_FOUND AND TARGET GDAL::GDAL)
    set(GDAL_TARGET GDAL::GDAL)
    message(STATUS "Found GDAL::GDAL")
    mark_as_system_library(${GDAL_TARGET})
    return() # Add this to exit early when found via CONFIG
endif()

# Your existing fallback logic unchanged
if(NOT GDAL_FOUND)
    try_config_packages(GDAL "GDAL;gdal" "GDAL::GDAL;gdal::gdal")
    if(GDAL_FOUND AND TARGET GDAL::GDAL)  # Add target check here
        set(GDAL_TARGET GDAL::GDAL)
    endif()
endif()

if(NOT GDAL_FOUND)
    try_pkgconfig(GDAL "gdal" "GDAL::GDAL")
    if(GDAL_FOUND AND TARGET GDAL::GDAL)  # Add target check here
        set(GDAL_TARGET GDAL::GDAL)
    endif()
endif()

if(NOT GDAL_FOUND)
    try_manual_discovery(GDAL "gdal;libgdal" "gdal.h" "GDAL::GDAL")
    if(GDAL_FOUND AND TARGET GDAL::GDAL)  # Add target check here
        set(GDAL_TARGET GDAL::GDAL)
    endif()
endif()

if(NOT GDAL_FOUND)
    message(FATAL_ERROR "Could not find GDAL")
endif()

# Verify we have a target
if(NOT GDAL_TARGET)
    message(FATAL_ERROR "GDAL_TARGET not set")
endif()

mark_as_system_library(${GDAL_TARGET})