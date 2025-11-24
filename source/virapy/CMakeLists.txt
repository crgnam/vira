if(DEFINED Python_EXECUTABLE)
    get_filename_component(Python_ROOT_DIR "${Python_EXECUTABLE}" DIRECTORY)
    get_filename_component(Python_ROOT_DIR "${Python_ROOT_DIR}" DIRECTORY)
endif()

set(PYBIND11_FINDPYTHON ON)

# FetchContent is used because vcpkg will attempt to install python, rather than linking to system version
include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.13.6
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(virapy init_bindings.cpp)
target_link_libraries(virapy PRIVATE vira Python::Python)
target_compile_definitions(virapy PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)

# Create the virapy directory structure in build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/virapy)

# Set output properties for the Python module with platform-specific logic
if(WIN32)
    set_target_properties(virapy PROPERTIES 
        SUFFIX ".pyd"
        OUTPUT_NAME "virapy"
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/virapy"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/virapy"
    )
else()
    set_target_properties(virapy PROPERTIES 
        SUFFIX ".so"
        OUTPUT_NAME "virapy"
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/virapy"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/virapy"
    )
endif()

# Set for all configurations
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG_UPPER)
    set_target_properties(virapy PROPERTIES 
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/virapy"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/virapy"
    )
endforeach()

# Also handle single-config generators
if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
    set_target_properties(virapy PROPERTIES 
        LIBRARY_OUTPUT_DIRECTORY_${BUILD_TYPE_UPPER} "${CMAKE_BINARY_DIR}/virapy"
        RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE_UPPER} "${CMAKE_BINARY_DIR}/virapy"
    )
endif()

# Generate version file from the root project version
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/_version.py.in"
    "${CMAKE_BINARY_DIR}/virapy/_version.py"
    @ONLY
)

# Copy packaging files
configure_file(packaging/pyproject.toml ${CMAKE_BINARY_DIR}/pyproject.toml COPYONLY)
configure_file(packaging/setup.py ${CMAKE_BINARY_DIR}/setup.py COPYONLY)
configure_file(packaging/__init__.py ${CMAKE_BINARY_DIR}/virapy/__init__.py COPYONLY)

# Copy README and License
configure_file(packaging/README.md ${CMAKE_BINARY_DIR}/README.md COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/LICENSE COPYONLY)

# Create _static directory and copy logo
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_static)
configure_file(${CMAKE_SOURCE_DIR}/docs/_static/vira-wide.png ${CMAKE_BINARY_DIR}/_static/vira-wide.png COPYONLY)

# Find all dependency libraries and copy them to the virapy directory (cross-platform)
add_custom_command(TARGET virapy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/helpers/CopyDependencies.cmake"
)