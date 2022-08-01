cmake_minimum_required (VERSION 3.0)
project(tinyjson C)

if(CMAKE_C_COMPILE_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(tinyjson tinyjson.c)
add_executable(tinyjson_test test.c)
target_link_libraries(tinyjson_test tinyjson)
