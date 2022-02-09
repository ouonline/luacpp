cmake_minimum_required(VERSION 3.10)
project(luacpp)

option(LUACPP_BUILD_TESTS "build tests" ON)
option(LUACPP_INSTALL "install headers and libs" ON)

file(GLOB LUACPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(luacpp_static STATIC ${LUACPP_SRC})
target_include_directories(luacpp_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(luacpp_static PUBLIC cxx_std_11)

if(NOT LUA_INCLUDE_DIR)
    find_package(Lua QUIET)
    if (NOT Lua_FOUND)
        message(FATAL_ERROR "lua dev lib >= 5.2.0 is required. please install lua development files or specify `LUA_INCLUDE_DIR` manually.")
    endif()
    if(LUA_VERSION_STRING VERSION_LESS "5.2.0")
        message(FATAL_ERROR "lua >= 5.2.0 is required.")
    endif()
endif()

target_include_directories(luacpp_static PUBLIC ${LUA_INCLUDE_DIR})

if(LUACPP_INSTALL)
    install(DIRECTORY include DESTINATION .)
    install(TARGETS luacpp_static LIBRARY DESTINATION lib)
endif()

if(LUACPP_BUILD_TESTS)
    if(NOT LUA_LIBRARIES)
        message(FATAL_ERROR "lua dev lib >= 5.2.0 is required. please install lua development libs, or specify `LUA_INCLUDE_DIR` and `LUA_LIBRARIES` manually.")
    endif()
    add_subdirectory(tests)
endif()
