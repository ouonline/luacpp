#ifndef __LUA_CPP_LUA_USER_DATA_H__
#define __LUA_CPP_LUA_USER_DATA_H__

#include "lua_ref_object.h"

namespace luacpp {

class LuaUserData final : public LuaRefObject {
public:
    LuaUserData(const std::shared_ptr<lua_State>& l, int index) : LuaRefObject(l, index) {}

    LuaUserData(LuaUserData&&) = default;
    LuaUserData& operator=(LuaUserData&&) = default;
    LuaUserData(const LuaUserData&) = delete;
    LuaUserData& operator=(const LuaUserData&) = delete;

    template <typename T>
    T* Get() const {
        auto l = m_l.get();
        PushSelf();
        auto ud = (T*)lua_touserdata(l, -1);
        lua_pop(l, 1);
        return ud;
    }
};

}

#endif
