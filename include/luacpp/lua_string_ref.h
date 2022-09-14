#ifndef __LUA_CPP_LUA_STRING_REF_H__
#define __LUA_CPP_LUA_STRING_REF_H__

namespace luacpp {

struct LuaStringRef final {
    LuaStringRef(const char* b = nullptr, size_t l = 0) : base(b), size(l) {}
    LuaStringRef(lua_State* l, int index) {
        base = lua_tolstring(l, index, &size);
    }

    const char* base;
    size_t size;
};

} // namespace luacpp

#endif
