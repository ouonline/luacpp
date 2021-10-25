file(GLOB LUACPP_TESTS_SRC *.cpp)

add_executable(test_luacpp ${LUACPP_TESTS_SRC})
target_link_libraries(test_luacpp PRIVATE luacpp_static ${LUA_LIBRARIES} dl)
