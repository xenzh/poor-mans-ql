enable_testing()

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")
add_executable(pmql_test ${sources})

target_link_libraries(pmql_test gtest_main)

include(GoogleTest)
gtest_discover_tests(pmql_test)
