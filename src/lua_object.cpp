#include "luacpp/lua_object.h"

namespace luacpp {

bool LuaObject::ToBool() const {
    PushSelf();
    auto ret = lua_toboolean(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

LuaStringRef LuaObject::ToStringRef() const {
    size_t len = 0;

    PushSelf();
    const char* str = lua_tolstring(m_l, -1, &len);
    LuaStringRef ret(str, len);
    lua_pop(m_l, 1);

    return ret;
}

const char* LuaObject::ToString() const {
    PushSelf();
    const char* ret = lua_tostring(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

lua_Integer LuaObject::ToInteger() const {
    PushSelf();
    auto ret = lua_tointeger(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

lua_Number LuaObject::ToNumber() const {
    PushSelf();
    auto ret = lua_tonumber(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

void* LuaObject::ToPointer() const {
    PushSelf();
    auto ret = lua_touserdata(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

} // namespace luacpp
