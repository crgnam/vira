set(test_private_libs
    ${GTEST_LIBRARIES}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    vira
    vira-vulkan
)
if(UNIX)
  list(APPEND test_private_libs pthread)
endif()

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/kernels")
if(EXISTS ${TEST_DATA_DIR}/successful_download.ind)
    message("All kernels required for tests have previously been downloaded")

else()
    # Download required generic kernels:
    message("Downloading generic kernels required by vulkan tests")
    file(DOWNLOAD 
        https://naif.jpl.nasa.gov/pub/naif/generic_kernels/lsk/naif0012.tls
        ${TEST_DATA_DIR}/generic/lsk/naif0012.tls)
    file(DOWNLOAD 
        https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00010.tpc
        ${TEST_DATA_DIR}/generic/pck/pck00010.tpc)
    
    # Download required OSIRIS-REx Kernels:
    message("Downloading ORX kernels required by vulkan tests")
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/ck/orx_r_190205_190207_v01.bc
        ${TEST_DATA_DIR}/orx/ck/orx_r_190205_190207_v01.bc)
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/fk/orx_v03.tf
        ${TEST_DATA_DIR}/orx/fk/orx_v03.tf)
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/pck/bennu_v17.tpc
        ${TEST_DATA_DIR}/orx/pck/bennu_v17.tpc)
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/sclk/ORX_SCLKSCET.00043.tsc
        ${TEST_DATA_DIR}/orx/sclk/ORX_SCLKSCET.00043.tsc)
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/spk/orx_struct_v03.bsp
        ${TEST_DATA_DIR}/orx/spk/orx_struct_v03.bsp)
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/spk/orx_181231_190305_190215_od099-N-M0D-P-M1D_v1.bsp
        ${TEST_DATA_DIR}/orx/spk/orx_181231_190305_190215_od099-N-M0D-P-M1D_v1.bsp)
    
    # Download Shape model:
    message("Downloading Bennu kernels required by vulkan tests")
    file(DOWNLOAD
        https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/dsk/bennu_g_01680mm_alt_obj_0000n00000_v021.bds
        ${TEST_DATA_DIR}/bennu/dsk/bennu_g_01680mm_alt_obj_0000n00000_v021.bds
    )
    
    file(WRITE ${TEST_DATA_DIR}/successful_download.ind)
endif()

file(COPY meta_kernel.tm DESTINATION ${CMAKE_BINARY_DIR}/vira_test_data/vulkan/)
file(COPY kernels DESTINATION ${CMAKE_BINARY_DIR}/vira_test_data/vulkan/)

set(VULKAN_TESTS
    test_Vulkan.cpp
    test_ViraDevice.cpp
    test_ViraPipeline.cpp
    test_ViraSwapchain.cpp
    test_ViraBuffer.cpp
    test_ViraDescriptors.cpp
)

foreach(test_source ${VULKAN_TESTS})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE ${test_private_libs})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
