#ifndef __LUA_CPP_LUA_USER_DATA_H__
#define __LUA_CPP_LUA_USER_DATA_H__

#include "lua_object.h"

namespace luacpp {

class LuaUserData final : public LuaObject {
public:
    LuaUserData(lua_State* l, int index) : LuaObject(l, index) {}
    LuaUserData(LuaObject&& lobj) : LuaObject(std::move(lobj)) {}
    LuaUserData(LuaUserData&&) = default;
    LuaUserData(const LuaUserData&) = delete;

    LuaUserData& operator=(LuaUserData&&) = default;
    LuaUserData& operator=(const LuaUserData&) = delete;

    template <typename T>
    T* Get() const {
        PushSelf();
        auto ud = (T*)lua_touserdata(m_l, -1);
        lua_pop(m_l, 1);
        return ud;
    }
};

}

#endif
