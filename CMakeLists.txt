cmake_minimum_required(VERSION 3.10)
project(luacpp)

option(LUACPP_BUILD_TESTS "build tests" OFF)

set(CMAKE_CXX_STANDARD 14)

file(GLOB LUACPP_SRC *.cpp)
add_library(luacpp_static STATIC ${LUACPP_SRC})

if(NOT LUA_INCLUDE_DIR)
    find_package(Lua QUIET)
    if (NOT Lua_FOUND)
        message(FATAL_ERROR "lua dev lib >= 5.4.0 is required. please install lua development files or specify `LUA_INCLUDE_DIR` manually.")
    endif()
    if(LUA_VERSION_STRING VERSION_LESS "5.4.0")
        message(FATAL_ERROR "lua >= 5.4.0 is required.")
    endif()
endif()

target_include_directories(luacpp_static PUBLIC ${LUA_INCLUDE_DIR})

if(LUACPP_BUILD_TESTS)
    if(NOT LUA_LIBRARIES)
        message(FATAL_ERROR "lua dev lib >= 5.4.0 is required. please install lua development libs, or specify `LUA_INCLUDE_DIR` and `LUA_LIBRARIES` manually.")
    endif()
    add_subdirectory(tests)
endif()
