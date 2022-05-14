#ifndef __LUA_CPP_LUA_52_53_H__
#define __LUA_CPP_LUA_52_53_H__

// some Lua 5.4 APIs for 5.2 and 5.3

#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 504

namespace luacpp {

inline void* lua_newuserdatauv(lua_State* l, size_t sz, int nuvalue) {
    auto ret = lua_newuserdata(l, sz);
    if (nuvalue > 0) {
        lua_createtable(l, nuvalue, 0);
        lua_setuservalue(l, -2);
    }
    return ret;
}

inline int lua_getiuservalue(lua_State* l, int idx, int n) {
    lua_getuservalue(l, idx);
    if (!lua_istable(l, -1)) {
        lua_pop(l, 1);
        lua_pushnil(l);
        return LUA_TNIL;
    }

    lua_rawgeti(l, -1, n);
    lua_remove(l, -2);
    return lua_type(l, -1);
}

inline int lua_setiuservalue(lua_State* l, int idx, int n) {
    lua_getuservalue(l, idx);
    if (!lua_istable(l, -1)) {
        lua_pop(l, 2);
        return 0;
    }

    lua_pushvalue(l, -2);
    lua_rawseti(l, -2, n);
    lua_pop(l, 2);
    return 1;
}

} // namespace luacpp

#endif

#endif
