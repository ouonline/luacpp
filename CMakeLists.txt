cmake_minimum_required(VERSION 3.10)
project(luacpp)

option(LUACPP_BUILD_TESTS "build tests" ON)

set (CMAKE_CXX_STANDARD 14)

file(GLOB LUACPP_SRC *.cpp)
add_library(luacpp_static STATIC ${LUACPP_SRC})

if(NOT LUA_INCLUDE_DIRS OR NOT LUA_LIBRARIES)
    message(FATAL_ERROR "please set `LUA_INCLUDE_DIRS` and `LUA_LIBRARIES`.")
endif()

target_link_libraries(luacpp_static PUBLIC ${LUA_LIBRARIES})
target_include_directories(luacpp_static PUBLIC ${LUA_INCLUDE_DIRS})

if(LUACPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
