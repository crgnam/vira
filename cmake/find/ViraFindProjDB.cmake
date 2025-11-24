# Function to find proj.db, now with Conan support but backward compatible
function(find_proj_db_from_gdal result_var)
    set(proj_data_dir "")
    set(search_roots "")
    
    # Method 1: Check if PROJ is available as a separate target (Conan case)
    # This only runs if PROJ targets exist, so it won't break existing systems
    set(found_proj_target FALSE)
    foreach(proj_target PROJ::proj proj::proj PROJ4::proj proj)
        if(TARGET ${proj_target})
            message(STATUS "Found PROJ target: ${proj_target} (likely from Conan)")
            set(found_proj_target TRUE)
            
            # Try direct Conan cache search first
            message(STATUS "PROJ target found, checking for Conan cache...")
            
            # Use CMAKE_BINARY_DIR to find conan_toolchain.cmake automatically
            file(GLOB_RECURSE conan_toolchain_files "${CMAKE_BINARY_DIR}/**/conan_toolchain.cmake")
            
            if(conan_toolchain_files)
                list(GET conan_toolchain_files 0 conan_toolchain_path)
                message(STATUS "Confirmed Conan environment: ${conan_toolchain_path}")
                
                # Get user home directory  
                if(WIN32)
                    set(conan_cache "$ENV{USERPROFILE}\\.conan2")
                else()
                    set(conan_cache "$ENV{HOME}/.conan2")
                endif()
                
                if(EXISTS "${conan_cache}")
                    message(STATUS "Searching Conan cache for proj.db: ${conan_cache}")
                    file(GLOB_RECURSE found_proj_dbs "${conan_cache}/p/*/proj.db")
                    
                    if(found_proj_dbs)
                        message(STATUS "Found proj.db files in Conan cache: ${found_proj_dbs}")
                        # Prefer the one in a "res" directory, otherwise take the first one
                        set(best_proj_db "")
                        foreach(proj_db_path ${found_proj_dbs})
                            get_filename_component(proj_db_dir "${proj_db_path}" DIRECTORY)
                            get_filename_component(dir_name "${proj_db_dir}" NAME)
                            if(dir_name STREQUAL "res")
                                set(best_proj_db "${proj_db_path}")
                                message(STATUS "Found proj.db in 'res' directory: ${best_proj_db}")
                                break()
                            elseif(NOT best_proj_db)
                                set(best_proj_db "${proj_db_path}")
                            endif()
                        endforeach()
                        
                        if(best_proj_db)
                            get_filename_component(proj_data_dir "${best_proj_db}" DIRECTORY)
                            message(STATUS "SUCCESS: Using proj.db from Conan cache: ${proj_data_dir}")
                            # Found it - set result and return
                            set(${result_var} "${proj_data_dir}" PARENT_SCOPE)
                            return()
                        endif()
                    else()
                        message(STATUS "No proj.db found in Conan cache")
                    endif()
                else()
                    message(STATUS "Conan cache directory not found: ${conan_cache}")
                endif()
            else()
                message(STATUS "No conan_toolchain.cmake found, not a Conan environment")
            endif()
            
            # If Conan search failed, try to get PROJ target properties as fallback
            get_target_property(PROJ_INCLUDE_DIRS ${proj_target} INTERFACE_INCLUDE_DIRECTORIES)
            get_target_property(PROJ_IMPORTED_LOCATION ${proj_target} IMPORTED_LOCATION)
            if(NOT PROJ_IMPORTED_LOCATION)
                get_target_property(PROJ_IMPORTED_LOCATION ${proj_target} IMPORTED_LOCATION_RELEASE)
            endif()
            if(NOT PROJ_IMPORTED_LOCATION)
                get_target_property(PROJ_IMPORTED_LOCATION ${proj_target} IMPORTED_LOCATION_DEBUG)
            endif()
            
            # Handle generator expressions and filter out empty values
            set(processed_include_dirs "")
            if(PROJ_INCLUDE_DIRS)
                foreach(include_dir ${PROJ_INCLUDE_DIRS})
                    # Skip generator expressions that we can't evaluate at configure time
                    if(NOT include_dir MATCHES "^\\$<")
                        list(APPEND processed_include_dirs "${include_dir}")
                    endif()
                endforeach()
            endif()
            
            message(STATUS "PROJ Include dirs (processed): ${processed_include_dirs}")
            message(STATUS "PROJ Imported location: ${PROJ_IMPORTED_LOCATION}")
            
            # Add PROJ-specific search roots
            if(processed_include_dirs)
                foreach(include_dir ${processed_include_dirs})
                    get_filename_component(potential_root "${include_dir}" DIRECTORY)
                    list(APPEND search_roots "${potential_root}")
                    message(STATUS "Added search root from include: ${potential_root}")
                endforeach()
            endif()
            
            if(PROJ_IMPORTED_LOCATION AND NOT PROJ_IMPORTED_LOCATION STREQUAL "PROJ_IMPORTED_LOCATION-NOTFOUND")
                get_filename_component(proj_lib_dir "${PROJ_IMPORTED_LOCATION}" DIRECTORY)
                get_filename_component(potential_root "${proj_lib_dir}" DIRECTORY)
                list(APPEND search_roots "${potential_root}")
                message(STATUS "Added search root from lib: ${potential_root}")
            endif()
            
            break()
        endif()
    endforeach()
    
    if(NOT found_proj_target)
        message(STATUS "No separate PROJ target found, using GDAL-based search (normal for non-Conan builds)")
    endif()
    
    # Method 2: Get all possible root directories from GDAL target properties (fallback)
    if(TARGET GDAL::GDAL)
        # Try to get include directories
        get_target_property(GDAL_INCLUDE_DIRS GDAL::GDAL INTERFACE_INCLUDE_DIRECTORIES)
        if(GDAL_INCLUDE_DIRS)
            foreach(include_dir ${GDAL_INCLUDE_DIRS})
                get_filename_component(potential_root "${include_dir}" DIRECTORY)
                list(APPEND search_roots "${potential_root}")
            endforeach()
        endif()
        
        # Try to get the imported location (library file)
        get_target_property(GDAL_IMPORTED_LOCATION GDAL::GDAL IMPORTED_LOCATION)
        if(NOT GDAL_IMPORTED_LOCATION)
            # Try different configuration types
            get_target_property(GDAL_IMPORTED_LOCATION GDAL::GDAL IMPORTED_LOCATION_RELEASE)
        endif()
        if(NOT GDAL_IMPORTED_LOCATION)
            get_target_property(GDAL_IMPORTED_LOCATION GDAL::GDAL IMPORTED_LOCATION_DEBUG)
        endif()
        if(GDAL_IMPORTED_LOCATION)
            get_filename_component(gdal_lib_dir "${GDAL_IMPORTED_LOCATION}" DIRECTORY)
            get_filename_component(potential_root "${gdal_lib_dir}" DIRECTORY)
            list(APPEND search_roots "${potential_root}")
        endif()
        
        # Try to get link directories
        get_target_property(GDAL_LINK_DIRS GDAL::GDAL INTERFACE_LINK_DIRECTORIES)
        if(GDAL_LINK_DIRS)
            foreach(link_dir ${GDAL_LINK_DIRS})
                get_filename_component(potential_root "${link_dir}" DIRECTORY)
                list(APPEND search_roots "${potential_root}")
            endforeach()
        endif()
    endif()
    
    # Method 3: Use CMAKE_PREFIX_PATH as additional search roots
    list(APPEND search_roots ${CMAKE_PREFIX_PATH})
    
    # Method 4: Use standard CMake package search paths
    list(APPEND search_roots 
        "${CMAKE_INSTALL_PREFIX}"
        "/usr/local"
        "/usr"
    )
    
    # Remove duplicates and empty entries, and filter out generator expressions
    set(clean_search_roots "")
    if(search_roots)
        foreach(root ${search_roots})
            # Skip generator expressions that we can't evaluate at configure time
            if(NOT root MATCHES "^\\$<" AND NOT root STREQUAL "")
                list(APPEND clean_search_roots "${root}")
            endif()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES clean_search_roots)
    set(search_roots ${clean_search_roots})
    
    message(STATUS "Clean search roots: ${search_roots}")
    
    # Search for proj.db in common relative locations from each root
    set(relative_paths
        "res"                    # Conan specific path
        "share/proj"
        "data/proj" 
        "lib/proj"
        "proj/data"
        "proj"
        "../share/proj"
        "../../share/proj"
        "../../../share/proj"
    )
    
    message(STATUS "Searching for proj.db in relative paths: ${relative_paths}")
    
    foreach(root ${search_roots})
        foreach(rel_path ${relative_paths})
            set(candidate_path "${root}/${rel_path}")
            # Skip generator expression paths that we can't check
            if(NOT candidate_path MATCHES "^\\$<")
                message(STATUS "Checking: ${candidate_path}")
                if(EXISTS "${candidate_path}/proj.db")
                    get_filename_component(proj_data_dir "${candidate_path}" ABSOLUTE)
                    message(STATUS "Found proj.db in: ${proj_data_dir}")
                    break()
                endif()
            endif()
        endforeach()
        if(proj_data_dir)
            break()
        endif()
    endforeach()
    
    # If still not found, do a broader search under the search roots
    if(NOT proj_data_dir)
        message(STATUS "proj.db not found in standard locations, doing recursive search...")
        foreach(root ${search_roots})
            file(GLOB_RECURSE found_proj_db "${root}/*/proj.db")
            if(found_proj_db)
                list(GET found_proj_db 0 first_found)  # Take the first one
                get_filename_component(proj_data_dir "${first_found}" DIRECTORY)
                message(STATUS "Found proj.db via recursive search in: ${proj_data_dir}")
                break()
            endif()
        endforeach()
    endif()
    
    set(${result_var} "${proj_data_dir}" PARENT_SCOPE)
endfunction()

# Find the PROJ data directory
find_proj_db_from_gdal(PROJ_DATA_DIR)

if(PROJ_DATA_DIR)
    message(STATUS "Found PROJ data directory: ${PROJ_DATA_DIR}")
    target_compile_definitions(vira INTERFACE MY_PROJ_DIR="${PROJ_DATA_DIR}")
else()
    message(WARNING "Could not find proj.db - PROJ data path will need to be set at runtime")
    message(STATUS "Consider setting PROJ_DATA environment variable or using runtime discovery")
endif()