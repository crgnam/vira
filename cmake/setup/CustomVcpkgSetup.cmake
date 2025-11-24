########################################################
### Configure VCPKG features and compile definitions ###
########################################################

set(VCPKG_MANIFEST_DIR "${CMAKE_SOURCE_DIR}/dependencies")

set(USING_ALTERNATIVE_PKG_MGR FALSE)

# Only disable vcpkg if explicitly using an alternative toolchain
if(CMAKE_TOOLCHAIN_FILE MATCHES "conda")
    set(USING_ALTERNATIVE_PKG_MGR TRUE)
    message(STATUS "Conda toolchain detected - disabling vcpkg integration")
elseif(CMAKE_TOOLCHAIN_FILE MATCHES "conan")
    set(USING_ALTERNATIVE_PKG_MGR TRUE)
    message(STATUS "Conan toolchain detected - disabling vcpkg integration")
endif()

# Disable vcpkg only if using alternative toolchain
if(USING_ALTERNATIVE_PKG_MGR)
    set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)
    set(VCPKG_TARGET_TRIPLET "")
    unset(ENV{VCPKG_ROOT})
    return()  # Exit early, don't apply vcpkg settings
endif()

# Only apply vcpkg settings if we're actually using vcpkg
if(DEFINED ENV{VCPKG_ROOT} OR CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    
    if(VIRA_BUILD_TESTS)
        list(APPEND VCPKG_MANIFEST_FEATURES "tests")
    endif()

    if(VIRA_BUILD_TOOLS)
        list(APPEND VCPKG_MANIFEST_FEATURES "tools")
    endif()

    if(NOT VIRA_KEEP_VCPKG_BUILDTREES)
        set(VCPKG_INSTALL_OPTIONS "--clean-buildtrees-after-build")
    endif()

    # Suppress developer warnings when using vcpkg
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
    
    message(STATUS "vcpkg integration enabled")
else()
    message(STATUS "vcpkg not detected - skipping vcpkg configuration")
endif()