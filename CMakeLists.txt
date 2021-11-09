cmake_minimum_required(VERSION 3.2)

set(CMAKE_C_COMPILER clang-11)
set(CMAKE_CXX_COMPILER clang++-11)

project(
    pmql
    HOMEPAGE_URL https://github.com/xenzh/poor-mans-ql
    DESCRIPTION "Poor man's query language"
    VERSION 0.0.5
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fcolor-diagnostics -ferror-limit=3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        075810f7a20405ea09a93f68847d6e963212fa62
)

FetchContent_Declare(
    tl-expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG        96d547c03d2feab8db64c53c3744a9b4a7c8f2c5
)

FetchContent_Declare(
    taocpp-pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG        2e9440b2c5f57dc4ad762f8304b44f1b7124cbc7
)

set(EXPECTED_BUILD_TESTS OFF)
FetchContent_MakeAvailable(googletest tl-expected taocpp-pegtl)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

add_subdirectory(pmql)
add_subdirectory(docs)
add_subdirectory(test)
add_subdirectory(bench)
