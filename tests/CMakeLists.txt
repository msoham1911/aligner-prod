add_executable(aligner_tests
    test_lexer.cpp
    test_parser.cpp
    test_sema.cpp
    test_e2e.cpp
)

target_link_libraries(aligner_tests
    PRIVATE
        aligner_lib
        GTest::gtest_main
)

# Discover individual test cases
include(GoogleTest)
gtest_discover_tests(aligner_tests)
