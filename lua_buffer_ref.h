#ifndef __LUA_CPP_LUA_BUFFER_REF_H__
#define __LUA_CPP_LUA_BUFFER_REF_H__

#include <stdint.h>

namespace luacpp {

struct LuaBufferRef final {
    LuaBufferRef(const char* b = nullptr, uint64_t l = 0) : base(b), size(l) {}
    const char* base;
    uint64_t size;
};

}

#endif
