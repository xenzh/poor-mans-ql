cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++-11")

project(pmql)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fcolor-diagnostics -ferror-limit=3")

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/40dfd4b775a66979ad1bd19321cdfd0feadfea27.zip
)
FetchContent_MakeAvailable(googletest)

find_package(benchmark REQUIRED)

add_subdirectory(pmql)
add_subdirectory(test)
add_subdirectory(bench)
