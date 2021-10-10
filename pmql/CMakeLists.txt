add_library(pmql INTERFACE)

target_include_directories(pmql INTERFACE .)

target_link_libraries(
    pmql
    INTERFACE expected
)
