# Simple helper functions for finding packages across different package managers

# Try to find a package using CONFIG mode with multiple possible names
function(try_config_packages pkg_name config_names target_alternatives)
    set(found FALSE)
    set(final_target "")
    
    foreach(config_name ${config_names})
        if(NOT found)
            find_package(${config_name} QUIET CONFIG)
            if(${config_name}_FOUND)
                message(STATUS "Found ${pkg_name} via CONFIG (${config_name})")
                
                # Check which target actually exists
                foreach(target_candidate ${target_alternatives})
                    if(TARGET ${target_candidate})
                        set(final_target ${target_candidate})
                        message(STATUS "Using target: ${final_target}")
                        set(found TRUE)
                        break()
                    endif()
                endforeach()
                
                # If no target found but package was found, try to create from variables
                if(NOT found)
                    # Try common variable patterns
                    set(lib_vars 
                        ${config_name}_LIBRARIES 
                        ${config_name}_LIBRARY
                        ${pkg_name}_LIBRARIES 
                        ${pkg_name}_LIBRARY
                    )
                    set(inc_vars 
                        ${config_name}_INCLUDE_DIRS 
                        ${config_name}_INCLUDE_DIR
                        ${pkg_name}_INCLUDE_DIRS 
                        ${pkg_name}_INCLUDE_DIR
                    )
                    
                    set(lib_path "")
                    set(inc_path "")
                    
                    foreach(var ${lib_vars})
                        if(DEFINED ${var} AND ${var})
                            set(lib_path ${${var}})
                            break()
                        endif()
                    endforeach()
                    
                    foreach(var ${inc_vars})
                        if(DEFINED ${var} AND ${var})
                            set(inc_path ${${var}})
                            break()
                        endif()
                    endforeach()
                    
                    if(lib_path AND inc_path)
                        # Use the first target alternative as the target name
                        list(GET target_alternatives 0 target_name)
                        
                        # If lib_path is just a library name (not a full path), find the actual file
                        if(NOT IS_ABSOLUTE "${lib_path}")
                            # It's just a library name like "fftw3f", need to find the actual file
                            find_library(${pkg_name}_ACTUAL_LIB 
                                NAMES ${lib_path} lib${lib_path}
                                # Let CMake search all standard locations including conda paths
                            )
                            if(${pkg_name}_ACTUAL_LIB)
                                set(lib_path ${${pkg_name}_ACTUAL_LIB})
                                message(STATUS "Found actual library file: ${lib_path}")
                            else()
                                message(WARNING "Could not find actual library file for ${lib_path}")
                            endif()
                        endif()
                        
                        message(STATUS "Creating ${target_name} from CONFIG variables")
                        add_library(${target_name} UNKNOWN IMPORTED)
                        set_target_properties(${target_name} PROPERTIES
                            IMPORTED_LOCATION "${lib_path}"
                            INTERFACE_INCLUDE_DIRECTORIES "${inc_path}"
                        )
                        set(final_target ${target_name})
                        set(found TRUE)
                    endif()
                endif()
                
                if(found)
                    break()
                endif()
            endif()
        endif()
    endforeach()
    
    # Return results to parent scope
    set(${pkg_name}_FOUND ${found} PARENT_SCOPE)
    if(final_target)
        set(${pkg_name}_TARGET ${final_target} PARENT_SCOPE)
    endif()
endfunction()

# Try to find a package using pkg-config
function(try_pkgconfig pkg_name pc_name target_name)
    find_package(PkgConfig QUIET)
    set(found FALSE)
    
    if(PkgConfig_FOUND)
        pkg_check_modules(${pkg_name}_PC QUIET ${pc_name})
        
        if(${pkg_name}_PC_FOUND)
            message(STATUS "Found ${pkg_name} via pkg-config")
            
            # Handle case where pkg-config doesn't provide LIBRARIES
            if(NOT ${pkg_name}_PC_LIBRARIES)
                set(${pkg_name}_PC_LIBRARIES ${pc_name})
            endif()
            
            # Find the actual library file
            find_library(${pkg_name}_LIB
                NAMES ${${pkg_name}_PC_LIBRARIES}
                PATHS ${${pkg_name}_PC_LIBRARY_DIRS}
                NO_DEFAULT_PATH
            )
            
            if(NOT ${pkg_name}_LIB)
                find_library(${pkg_name}_LIB
                    NAMES ${${pkg_name}_PC_LIBRARIES}
                )
            endif()
            
            if(${pkg_name}_LIB)
                add_library(${target_name} UNKNOWN IMPORTED)
                set_target_properties(${target_name} PROPERTIES
                    IMPORTED_LOCATION ${${pkg_name}_LIB}
                    INTERFACE_INCLUDE_DIRECTORIES "${${pkg_name}_PC_INCLUDE_DIRS}"
                    INTERFACE_COMPILE_OPTIONS "${${pkg_name}_PC_CFLAGS_OTHER}"
                )
                set(found TRUE)
                message(STATUS "Created ${target_name} from pkg-config")
            else()
                message(STATUS "pkg-config found ${pkg_name} but could not locate library file")
            endif()
        endif()
    endif()
    
    # Return results to parent scope
    set(${pkg_name}_FOUND ${found} PARENT_SCOPE)
    if(found)
        set(${pkg_name}_TARGET ${target_name} PARENT_SCOPE)
    endif()
endfunction()

# Try to find a package manually by searching for library and header files
function(try_manual_discovery pkg_name lib_names header_names target_name)
    find_library(${pkg_name}_MANUAL_LIB NAMES ${lib_names})
    find_path(${pkg_name}_MANUAL_INC NAMES ${header_names})
    
    set(found FALSE)
    if(${pkg_name}_MANUAL_LIB AND ${pkg_name}_MANUAL_INC)
        message(STATUS "Found ${pkg_name} manually: ${${pkg_name}_MANUAL_LIB}")
        add_library(${target_name} UNKNOWN IMPORTED)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION "${${pkg_name}_MANUAL_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${${pkg_name}_MANUAL_INC}"
        )
        set(found TRUE)
    endif()
    
    # Return results to parent scope
    set(${pkg_name}_FOUND ${found} PARENT_SCOPE)
    if(found)
        set(${pkg_name}_TARGET ${target_name} PARENT_SCOPE)
    endif()
endfunction()