file(GLOB sources CONFIGURE_DEPENDS "*.cpp")
add_executable(pmql_bench ${sources})

target_link_libraries(pmql_bench benchmark::benchmark benchmark::benchmark_main)
