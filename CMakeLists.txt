cmake_minimum_required (VERSION 2.6)
project (let_test C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall -std=c11")
endif()

add_library(letjson letjson.c)
add_executable(letjson_test test.c)
target_link_libraries(letjson_test letjson)