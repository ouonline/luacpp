#ifndef __LUA_CPP_LUA_STRING_REF_H__
#define __LUA_CPP_LUA_STRING_REF_H__

#include <stdint.h>

namespace luacpp {

struct LuaStringRef final {
    LuaStringRef(const char* b = nullptr, uint64_t l = 0) : base(b), size(l) {}
    LuaStringRef(lua_State* l, int index) {
        base = lua_tolstring(l, index, &size);
    }

    const char* base;
    uint64_t size;
};

} // namespace luacpp

#endif
