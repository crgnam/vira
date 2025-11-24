##################################
### OS Specific Compiler Flags ###
##################################
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # If on Windows:
    message("Windows Detected")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_Release ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_Debug OFF)

    add_compile_options(/utf-8) # Required for OpenImageIO

elseif(UNIX AND NOT APPLE)
    # If on Linux:
    message("NIX Detected")

    # Check for the build type:
    if (NOT (CMAKE_BUILD_TYPE STREQUAL "Release"))
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
    
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    	set(CMAKE_CXX_FLAGS "-Wall -mtune=native -march=native")
		set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fno-math-errno -fno-signed-zeros -fno-trapping-math -freciprocal-math -fno-rounding-math -fno-signaling-nans -fexcess-precision=fast -flto=auto")
	endif()

    execute_process(
        COMMAND grep -i microsoft /proc/version
        RESULT_VARIABLE IS_WSL
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(IS_WSL EQUAL 0)
        message("WSL Detected")
        add_definitions(-D__WSL__)
    endif()

elseif(APPLE)
    # If on Apple:
    message("Apple Detected")

    # Check if the linker supports -no_warn_duplicate_libraries
    include(CheckLinkerFlag)
    check_linker_flag(CXX "-Wl,-no_warn_duplicate_libraries" LINKER_SUPPORTS_NO_WARN_DUPLICATE_LIBRARIES)
    
    if(LINKER_SUPPORTS_NO_WARN_DUPLICATE_LIBRARIES)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_warn_duplicate_libraries")
    endif()
endif()