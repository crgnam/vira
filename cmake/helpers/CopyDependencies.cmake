# Cross-platform script to copy shared libraries to virapy directory
# This script is called by CMake during the build process

# Ensure the virapy directory exists
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/virapy")

# Define patterns for shared libraries on different platforms
if(WIN32)
    set(LIB_PATTERNS "*.dll")
elseif(APPLE)
    set(LIB_PATTERNS "*.dylib*")
else()
    set(LIB_PATTERNS "*.so*")
endif()

# Find all shared libraries in the build directory
file(GLOB_RECURSE SHARED_LIBS "${CMAKE_BINARY_DIR}/${LIB_PATTERNS}")

# Copy libraries, excluding those already in virapy directory
foreach(LIB_FILE ${SHARED_LIBS})
    # Get the directory containing this library
    get_filename_component(LIB_DIR ${LIB_FILE} DIRECTORY)
    
    # Skip if this library is already in the virapy directory
    if(NOT LIB_DIR MATCHES ".*/virapy.*")
        get_filename_component(LIB_NAME ${LIB_FILE} NAME)
        
        # Copy the library to virapy directory
        file(COPY ${LIB_FILE} DESTINATION "${CMAKE_BINARY_DIR}/virapy/")
        
        message(STATUS "Copied dependency: ${LIB_NAME}")
    endif()
endforeach()