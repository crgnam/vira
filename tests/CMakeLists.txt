enable_testing()

find_package(GTest CONFIG REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

# add_subdirectory(quipu)

if (VIRA_BUILD_VULKAN)
    add_subdirectory(vulkan)
endif()

# Define the shared main.cpp file for all tests
set(TEST_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

