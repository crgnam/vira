find_package(Doxygen REQUIRED)
include(ViraFindSphinx)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/include)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${VIRA_PUBLIC_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs"
                   VERBATIM)

add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_BINARY_DIR}/vira_docs)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Prepare Sphinx command and dependencies
file(GLOB_RECURSE SPHINX_RST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")
set(SPHINX_DEPENDS
    ${SPHINX_RST_FILES}  # Add this line
    ${CMAKE_CURRENT_SOURCE_DIR}/index.rst  # This is now redundant but harmless
    ${DOXYGEN_INDEX_FILE}
)

set(SPHINX_COMMAND
    ${SPHINX_EXECUTABLE} -b html
    # Tell Breathe where to find the Doxygen output
    -Dbreathe_projects.vira=${DOXYGEN_OUTPUT_DIR}/xml
)

# If virapy is being built, add the module directory
if (VIRA_BUILD_VIRAPY)
    set(VIRAPY_MODULE_DIR ${CMAKE_BINARY_DIR}/virapy)
    
    # Add environment variable for the module directory
    set(SPHINX_COMMAND
        ${CMAKE_COMMAND} -E env VIRAPY_BUILD_DIR=${VIRAPY_MODULE_DIR}
        ${SPHINX_COMMAND}
    )
    
    # Add virapy as a dependency
    list(APPEND SPHINX_DEPENDS virapy)
endif()

# Always generate Sphinx documentation
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND ${SPHINX_COMMAND} ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS ${SPHINX_DEPENDS}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")

add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

# Only add virapy dependency if it's being built
if (VIRA_BUILD_VIRAPY)
    add_dependencies(Sphinx virapy)
endif()