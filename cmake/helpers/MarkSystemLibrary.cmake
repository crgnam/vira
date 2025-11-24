# MarkSystemLibrary.cmake - Utilities for marking third-party libraries as SYSTEM

# Function to mark third-party targets as SYSTEM to suppress compiler warnings
# This prevents warnings from third-party headers from cluttering your build output
function(mark_as_system_library target)
    if(NOT TARGET ${target})
        message(WARNING "mark_as_system_library: Target '${target}' does not exist")
        return()
    endif()
    
    # Skip alias targets - they can't have properties set directly
    get_target_property(aliased_target ${target} ALIASED_TARGET)
    if(aliased_target)
        # If it's an alias, work with the actual target
        set(actual_target ${aliased_target})
    else()
        set(actual_target ${target})
    endif()
    
    get_target_property(target_type ${actual_target} TYPE)
    if(NOT target_type)
        message(WARNING "mark_as_system_library: Could not determine type of target '${actual_target}'")
        return()
    endif()
    
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        # For regular libraries, mark their interface includes as SYSTEM
        get_target_property(include_dirs ${actual_target} INTERFACE_INCLUDE_DIRECTORIES)
        if(include_dirs)
            set_target_properties(${actual_target} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${include_dirs}"
            )
        endif()
    else()
        # For interface libraries, get and re-set as SYSTEM
        get_target_property(include_dirs ${actual_target} INTERFACE_INCLUDE_DIRECTORIES)
        if(include_dirs)
            set_target_properties(${actual_target} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${include_dirs}"
            )
        endif()
    endif()
endfunction()

# Convenience function to mark multiple targets as SYSTEM at once
function(mark_as_system_libraries)
    foreach(target ${ARGN})
        mark_as_system_library(${target})
    endforeach()
endfunction()