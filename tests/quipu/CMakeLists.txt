

set(QUIPU_TESTS
    test_quipu_io.cpp
)

add_executable(quipu_tests ${QUIPU_TESTS}
)


target_link_libraries(quipu_tests 
    ${GTEST_LIBRARIES}
    pthread
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    vira
)

add_test(NAME QTests COMMAND quipu_tests)