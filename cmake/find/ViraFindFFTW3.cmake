# ViraFindFFTW3.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(FFTW3f CONFIG QUIET)
if(FFTW3f_FOUND)
    message(STATUS "FFTW3f CONFIG found")
    if(TARGET FFTW3::fftw3f)
        set(FFTW3f_TARGET FFTW3::fftw3f)
        message(STATUS "Found FFTW3::fftw3f")
    else()
        message(STATUS "FFTW3f CONFIG found but no expected targets exist")
        # Reset and try fallback logic
        set(FFTW3f_FOUND FALSE)
    endif()
endif()

if(NOT FFTW3f_FOUND)
    # Use your existing logic exactly as-is
    try_config_packages(FFTW3f "FFTW3f;fftw3f" "FFTW3::fftw3f;fftw3f::fftw3f")
    if(NOT FFTW3f_FOUND)
        try_pkgconfig(FFTW3f "fftw3f" "FFTW3::fftw3f")
    endif()
    if(NOT FFTW3f_FOUND)
        try_manual_discovery(FFTW3f "fftw3f;libfftw3f" "fftw3.h" "FFTW3::fftw3f")
    endif()

    # Additional FFTW3 handling for conda environments (from original CMakeLists.txt)
    if(NOT TARGET FFTW3::fftw3f AND FFTW3f_LIBRARIES)
        # Find the actual library file
        find_library(FFTW3f_LIBRARY_FILE
            NAMES ${FFTW3f_LIBRARIES}
            PATHS ${FFTW3f_LIBRARY_DIRS}
            NO_DEFAULT_PATH
        )
        
        if(FFTW3f_LIBRARY_FILE)
            add_library(FFTW3::fftw3f UNKNOWN IMPORTED)
            set_target_properties(FFTW3::fftw3f PROPERTIES
                IMPORTED_LOCATION ${FFTW3f_LIBRARY_FILE}
                INTERFACE_INCLUDE_DIRECTORIES "${FFTW3f_INCLUDE_DIRS}"
            )
            message(STATUS "Created FFTW3::fftw3f target from conda variables")
            set(FFTW3f_TARGET FFTW3::fftw3f)
        endif()
    endif()

    if(NOT FFTW3f_FOUND)
        message(FATAL_ERROR "Could not find FFTW3f")
    endif()
endif()

# Ensure we have a valid FFTW3f_TARGET
if(NOT FFTW3f_TARGET)
    if(TARGET FFTW3::fftw3f)
        set(FFTW3f_TARGET FFTW3::fftw3f)
    else()
        message(FATAL_ERROR "FFTW3f was found but FFTW3f_TARGET is not set and FFTW3::fftw3f target doesn't exist")
    endif()
endif()

message(STATUS "FFTW3f_TARGET is set to: ${FFTW3f_TARGET}")

mark_as_system_library(${FFTW3f_TARGET})