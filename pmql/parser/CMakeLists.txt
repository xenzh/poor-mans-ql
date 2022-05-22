add_library(pmql-parser INTERFACE)

target_include_directories(pmql-parser INTERFACE .)

target_link_libraries(
    pmql-parser
    INTERFACE expected
    INTERFACE pmql
    INTERFACE taocpp::pegtl
)
