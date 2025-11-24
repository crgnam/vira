# ViraFetchCSPICE.cmake checks for CSPICE and falls back to FetchContent:
include(FetchContent)
include(MarkSystemLibrary)

# Prevent multiple inclusions
if(FETCHCSPICE_INCLUDED)
    return()
endif()
set(FETCHCSPICE_INCLUDED TRUE)

find_library(LIB_CSPICE 
    NAMES cspice
    HINTS 
        ${CSPICE_ROOT}/lib
        $ENV{CSPICE_ROOT}/lib
        $ENV{CONDA_PREFIX}/lib
        /usr/local/lib
        /opt/homebrew/lib
)

find_path(CSPICE_INCLUDE_DIR 
    NAMES SpiceUsr.h
    HINTS 
        ${CSPICE_ROOT}/include
        $ENV{CSPICE_ROOT}/include
        $ENV{CONDA_PREFIX}/include
        /usr/local/include
        /opt/homebrew/include
    PATH_SUFFIXES cspice
)

if (LIB_CSPICE)
    message(STATUS "Found system CSPICE: ${LIB_CSPICE}")
endif()

if (CSPICE_INCLUDE_DIR)
    message(STATUS "Found system CSPICE Include: ${CSPICE_INCLUDE_DIR}")
endif()

if(LIB_CSPICE AND CSPICE_INCLUDE_DIR)
    # Create imported targets for system-installed CSPICE
    add_library(cspice INTERFACE IMPORTED GLOBAL)
    target_link_libraries(cspice INTERFACE ${LIB_CSPICE})
    
    # Check if we need to create the cspice subdirectory structure for compatibility
    if(NOT EXISTS "${CSPICE_INCLUDE_DIR}/cspice/SpiceUsr.h" AND EXISTS "${CSPICE_INCLUDE_DIR}/SpiceUsr.h")
        message(STATUS "Creating cspice subdirectory for include compatibility at ${CSPICE_INCLUDE_DIR}")
        file(MAKE_DIRECTORY "${CSPICE_INCLUDE_DIR}/cspice")
        file(GLOB CSPICE_HEADERS "${CSPICE_INCLUDE_DIR}/*.h")
        foreach(header ${CSPICE_HEADERS})
            get_filename_component(header_name ${header} NAME)
            set(target_header "${CSPICE_INCLUDE_DIR}/cspice/${header_name}")
            if(NOT EXISTS "${target_header}")
                # Use configure_file as a fallback for older CMake versions
                if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
                    file(CREATE_LINK ${header} "${target_header}" COPY_ON_ERROR)
                else()
                    configure_file(${header} "${target_header}" COPYONLY)
                endif()
            endif()
        endforeach()
        message(STATUS "CSPICE headers linked/copied to cspice subdirectory")
    endif()
    
    target_include_directories(cspice INTERFACE ${CSPICE_INCLUDE_DIR})
    
    # Add csupport if found
    if(LIB_CSUPPORT)
        add_library(csupport INTERFACE IMPORTED GLOBAL)
        target_link_libraries(csupport INTERFACE ${LIB_CSUPPORT})
    endif()
    
    # CSPICE requires math library on Unix-like systems
    if(UNIX)
        target_link_libraries(cspice INTERFACE m)
    endif()
    
    set(cspice_FOUND TRUE)
endif()

# Platform-specific CSPICE URL detection
function(_get_cspice_url OUTPUT_VAR)
    if(WIN32)
        set(CSPICE_URL "https://naif.jpl.nasa.gov/pub/naif/toolkit/C/PC_Windows_VisualC_64bit/packages/cspice.zip")
    elseif(APPLE)
        # Detect Apple Silicon vs Intel
        if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
            set(CSPICE_URL "https://naif.jpl.nasa.gov/pub/naif/toolkit/C/MacM1_OSX_clang_64bit/packages/cspice.tar.Z")
        else()
            set(CSPICE_URL "https://naif.jpl.nasa.gov/pub/naif/toolkit/C/MacIntel_OSX_AppleC_64bit/packages/cspice.tar.Z")
        endif()
    else()
        set(CSPICE_URL "https://naif.jpl.nasa.gov/pub/naif/toolkit/C/PC_Linux_GCC_64bit/packages/cspice.tar.Z")
    endif()
    
    set(${OUTPUT_VAR} ${CSPICE_URL} PARENT_SCOPE)
endfunction()

# Custom download and extract function for .tar.Z files
function(_download_and_extract_cspice)
    set(DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/cspice-download")
    set(EXTRACT_DIR "${CMAKE_BINARY_DIR}/_deps/cspice-src")
    
    # Get the URL
    _get_cspice_url(CSPICE_URL)
    
    # Create directories
    file(MAKE_DIRECTORY ${DOWNLOAD_DIR})
    file(MAKE_DIRECTORY ${EXTRACT_DIR})
    
    # Download file
    get_filename_component(FILENAME ${CSPICE_URL} NAME)
    set(DOWNLOAD_FILE "${DOWNLOAD_DIR}/${FILENAME}")
    
    message(STATUS "Downloading CSPICE from ${CSPICE_URL}")
    file(DOWNLOAD ${CSPICE_URL} ${DOWNLOAD_FILE}
        STATUS DOWNLOAD_STATUS)
    
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download CSPICE: ${DOWNLOAD_STATUS}")
    endif()
    
    # Extract based on file extension
    if(FILENAME MATCHES "\\.tar\\.Z$")
        # For .tar.Z files, we need uncompress and tar
        find_program(UNCOMPRESS_PROGRAM uncompress)
        if(NOT UNCOMPRESS_PROGRAM)
            message(STATUS "uncompress not found, trying with zcat and tar")
            execute_process(
                COMMAND zcat ${DOWNLOAD_FILE}
                COMMAND tar -xf - -C ${EXTRACT_DIR} --strip-components=1
                RESULT_VARIABLE EXTRACT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
        else()
            # Use uncompress then tar
            execute_process(
                COMMAND ${UNCOMPRESS_PROGRAM} -c ${DOWNLOAD_FILE}
                COMMAND tar -xf - -C ${EXTRACT_DIR} --strip-components=1
                RESULT_VARIABLE EXTRACT_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
        endif()
    elseif(FILENAME MATCHES "\\.zip$")
        # For ZIP files
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${DOWNLOAD_FILE}
            WORKING_DIRECTORY ${EXTRACT_DIR}
            RESULT_VARIABLE EXTRACT_RESULT
        )
    else()
        message(FATAL_ERROR "Unsupported archive format: ${FILENAME}")
    endif()
    
    if(NOT EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract CSPICE archive")
    endif()
    
    # Set the source directory for the parent scope
    set(cspice_SOURCE_DIR ${EXTRACT_DIR} PARENT_SCOPE)
    message(STATUS "CSPICE extracted to ${EXTRACT_DIR}")
endfunction()

# If CSPICE has not been found, fallback to using custom download
if(NOT cspice_FOUND)
    message(STATUS "CSPICE not found in system, downloading and building...")
    
    # Download and extract CSPICE
    _download_and_extract_cspice()
    
    # Verify extraction worked
    if(NOT EXISTS "${cspice_SOURCE_DIR}/lib")
        message(FATAL_ERROR "CSPICE extraction failed - lib directory not found")
    endif()
    
    # Create cspice subdirectory in include to match expected include path
    file(MAKE_DIRECTORY "${cspice_SOURCE_DIR}/include/cspice")
    file(GLOB CSPICE_HEADERS "${cspice_SOURCE_DIR}/include/*.h")
    foreach(header ${CSPICE_HEADERS})
        get_filename_component(header_name ${header} NAME)
        set(target_header "${cspice_SOURCE_DIR}/include/cspice/${header_name}")
        if(NOT EXISTS "${target_header}")
            # Use configure_file as a fallback for older CMake versions
            if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
                file(CREATE_LINK ${header} "${target_header}" COPY_ON_ERROR)
            else()
                configure_file(${header} "${target_header}" COPYONLY)
            endif()
        endif()
    endforeach()
    
    # Create imported targets for CSPICE libraries with GLOBAL scope
    add_library(cspice STATIC IMPORTED GLOBAL)
    add_library(csupport STATIC IMPORTED GLOBAL)
    
    # Platform-specific library paths and extensions
    if(WIN32)
        set_target_properties(cspice PROPERTIES
            IMPORTED_LOCATION "${cspice_SOURCE_DIR}/lib/cspice.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${cspice_SOURCE_DIR}/include"
        )
        set_target_properties(csupport PROPERTIES
            IMPORTED_LOCATION "${cspice_SOURCE_DIR}/lib/csupport.lib"
        )
    else()
        set_target_properties(cspice PROPERTIES
            IMPORTED_LOCATION "${cspice_SOURCE_DIR}/lib/cspice.a"
            INTERFACE_INCLUDE_DIRECTORIES "${cspice_SOURCE_DIR}/include"
        )
        set_target_properties(csupport PROPERTIES
            IMPORTED_LOCATION "${cspice_SOURCE_DIR}/lib/csupport.a"
        )
        
        # CSPICE requires math library on Unix-like systems
        target_link_libraries(cspice INTERFACE m)
    endif()
    
    # Create namespaced aliases for consistency
    add_library(CSPICE::cspice ALIAS cspice)
    add_library(CSPICE::csupport ALIAS csupport)
    
    message(STATUS "CSPICE configured successfully")
    set(cspice_FOUND TRUE)
endif()

# Verify we have CSPICE available
if(NOT cspice_FOUND)
    message(FATAL_ERROR "Failed to find or fetch CSPICE")
endif()

mark_as_system_library(cspice)