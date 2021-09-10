#ifndef __LUA_CPP_LUA_STRING_REF_H__
#define __LUA_CPP_LUA_STRING_REF_H__

#include <stdint.h>

namespace luacpp {

struct LuaStringRef final {
    LuaStringRef(const char* b = nullptr, uint64_t l = 0) : base(b), size(l) {}
    const char* base;
    uint64_t size;
};

}

#endif
