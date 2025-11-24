include(GNUInstallDirs)

include(ViraFetchTCLAP)

# Function to create Vira applications
function(add_vira_app APP_NAME)
    # Parse additional arguments (for extra dependencies)
    set(EXTRA_LIBS ${ARGN})
    
    # Automatically create source file name by appending .cpp
    set(SOURCE_FILE "${APP_NAME}.cpp")
    
    add_executable(${APP_NAME} ${SOURCE_FILE})
    
    # Start with common libraries (now includes tclap::tclap target)
    set(LINK_LIBS vira tclap::tclap ${EXTRA_LIBS})
    
    target_link_libraries(${APP_NAME} PRIVATE ${LINK_LIBS})

    # Set RPATH for Linux to look for libraries in ../lib/vira relative to executable
    if(NOT WIN32)
        set_target_properties(${APP_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/vira"
            INSTALL_RPATH_USE_LINK_PATH FALSE)
    endif()

    # Install the executable
    install(TARGETS ${APP_NAME} 
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            EXPORT ViraToolsTargets)
endfunction()

# Add all applications:
#add_vira_app(vira_dem2obj) #TODO Reimplement model exporting
add_vira_app(vira_spcmap2qld)
add_vira_app(vira_dem2qld)
add_vira_app(vira_tycho2)

# Export the tools targets
install(EXPORT ViraToolsTargets
    FILE ViraToolsTargets.cmake
    NAMESPACE vira::
    DESTINATION ${VIRA_INSTALL_CMAKEDIR}
)