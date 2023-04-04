cmake_minimum_required(VERSION 3.10)
project(luacpp)

option(LUACPP_BUILD_TESTS "build tests" ON)
option(LUACPP_INSTALL "install headers and libs" ON)

file(GLOB LUACPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(luacpp_static STATIC ${LUACPP_SRC})
target_include_directories(luacpp_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(luacpp_static PUBLIC cxx_std_11)
set_target_properties(luacpp_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(luacpp_static PRIVATE /W4)
else()
    target_compile_options(luacpp_static PRIVATE -Wall -Wextra -Werror)
endif()

if(LUA_SRC_DIR AND LUA_INCLUDE_DIR)
    message(FATAL_ERROR "only one of `LUA_SRC_DIR` and `LUA_INCLUDE_DIR` can be set at the same time.")
endif()

if(LUA_SRC_DIR)
    file(GLOB LUA_SRC ${LUA_SRC_DIR}/*.c)
    list(REMOVE_ITEM LUA_SRC ${LUA_SRC_DIR}/lua.c ${LUA_SRC_DIR}/luac.c)
    add_library(lua_static STATIC ${LUA_SRC})
    set_target_properties(lua_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
    endif()

    set(LUA_INCLUDE_DIR ${LUA_SRC_DIR})
    set(LUA_LIBRARIES lua_static)
endif()

if(NOT LUA_INCLUDE_DIR)
    find_package(Lua QUIET)
    if (NOT Lua_FOUND)
        message(FATAL_ERROR "lua dev lib >= 5.2.0 is required. please install lua development files or specify one of `LUA_SRC_DIR` and `LUA_INCLUDE_DIR` manually.")
    endif()
    if(LUA_VERSION_STRING VERSION_LESS "5.2.0")
        message(FATAL_ERROR "lua >= 5.2.0 is required.")
    endif()
endif()

target_include_directories(luacpp_static PUBLIC ${LUA_INCLUDE_DIR})

if(LUA_LIBRARIES)
    target_link_libraries(luacpp_static PUBLIC ${LUA_LIBRARIES})
endif()

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
