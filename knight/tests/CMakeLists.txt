add_executable(knight_tests
    arbitrage_calculator_test.cpp
    decoder_test.cpp
    test_helpers.h
)

target_link_libraries(knight_tests
    PRIVATE
        knight_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(knight_tests)
