cmake_minimum_required(VERSION 3.10)
project(luacpp)

option(LUACPP_BUILD_TESTS "build tests" ON)

set (CMAKE_CXX_STANDARD 14)

file(GLOB LUACPP_SRC *.cpp)
add_library(luacpp_static STATIC ${LUACPP_SRC})

if(NOT LUACPP_LUA_INCLUDE_DIRS OR NOT LUACPP_LUA_LIBRARIES)
    message(FATAL_ERROR "please set `LUACPP_LUA_INCLUDE_DIRS` and `LUACPP_LUA_LIBRARIES`.")
endif()

target_link_libraries(luacpp_static PUBLIC ${LUACPP_LUA_LIBRARIES})
target_include_directories(luacpp_static PUBLIC ${LUACPP_LUA_INCLUDE_DIRS})

if(LUACPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
