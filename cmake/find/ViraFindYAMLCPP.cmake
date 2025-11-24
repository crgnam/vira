# ViraFindYAMLCPP.cmake
include(MarkSystemLibrary)
include(SimplePackageHelpers)

# Try vcpkg first (standard target)
find_package(yaml-cpp CONFIG QUIET)
if(yaml-cpp_FOUND AND TARGET yaml-cpp::yaml-cpp)
    set(yaml-cpp_TARGET yaml-cpp::yaml-cpp)
    message(STATUS "Found yaml-cpp::yaml-cpp")
endif()

if(NOT yaml-cpp_FOUND)
    # Use your existing logic exactly as-is
    try_config_packages(yaml-cpp "yaml-cpp" "yaml-cpp::yaml-cpp;yaml-cpp")
    if(NOT yaml-cpp_FOUND)
        try_pkgconfig(yaml-cpp "yaml-cpp" "yaml-cpp::yaml-cpp")
    endif()
    if(NOT yaml-cpp_FOUND)
        try_manual_discovery(yaml-cpp "yaml-cpp;libyaml-cpp" "yaml-cpp/yaml.h" "yaml-cpp::yaml-cpp")
    endif()
    if(NOT yaml-cpp_FOUND)
        message(FATAL_ERROR "Could not find yaml-cpp")
    endif()
endif()

# Verify we have a target
if(NOT yaml-cpp_TARGET)
    message(FATAL_ERROR "yaml-cpp_TARGET not set")
endif()

mark_as_system_library(${yaml-cpp_TARGET})