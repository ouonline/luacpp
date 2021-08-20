#include "lua_table.h"
using namespace std;

namespace luacpp {

LuaObject LuaTable::Get(int index) const {
    auto l = m_l.get();
    PushSelf();
    lua_rawgeti(l, -1, index);
    LuaObject ret(m_l, -1);
    lua_pop(l, 2);
    return ret;
}

LuaObject LuaTable::Get(const char* name) const {
    auto l = m_l.get();
    PushSelf();
    lua_getfield(l, -1, name);
    LuaObject ret(m_l, -1);
    lua_pop(l, 2);
    return ret;
}

void LuaTable::Set(int index, const char* str) {
    auto l = m_l.get();
    PushSelf();
    lua_pushstring(l, str);
    lua_rawseti(l, -2, index);
    lua_pop(l, 1);
}

void LuaTable::Set(int index, const char* str, size_t len) {
    auto l = m_l.get();
    PushSelf();
    lua_pushlstring(l, str, len);
    lua_rawseti(l, -2, index);
    lua_pop(l, 1);
}

void LuaTable::Set(int index, lua_Number value) {
    auto l = m_l.get();
    PushSelf();
    lua_pushnumber(l, value);
    lua_rawseti(l, -2, index);
    lua_pop(l, 1);
}

void LuaTable::Set(const char* name, const char* str) {
    auto l = m_l.get();
    PushSelf();
    lua_pushstring(l, str);
    lua_setfield(l, -2, name);
    lua_pop(l, 1);
}

void LuaTable::Set(const char* name, const char* str, size_t len) {
    auto l = m_l.get();
    PushSelf();
    lua_pushlstring(l, str, len);
    lua_setfield(l, -2, name);
    lua_pop(l, 1);
}

void LuaTable::Set(const char* name, lua_Number value) {
    auto l = m_l.get();
    PushSelf();
    lua_pushnumber(l, value);
    lua_setfield(l, -2, name);
    lua_pop(l, 1);
}

void LuaTable::Set(int index, const LuaRefObject& lobj) {
    auto l = m_l.get();
    PushSelf();
    lobj.PushSelf();
    lua_rawseti(l, -2, index);
    lua_pop(l, 1);
}

void LuaTable::Set(const char* name, const LuaRefObject& lobj) {
    auto l = m_l.get();
    PushSelf();
    lobj.PushSelf();
    lua_setfield(l, -2, name);
    lua_pop(l, 1);
}

bool LuaTable::ForEach(const function<bool (const LuaObject& key,
                                            const LuaObject& value)>& func) const {
    auto l = m_l.get();

    PushSelf();
    lua_pushnil(l);
    while (lua_next(l, -2) != 0) {
        if (!func(LuaObject(m_l, -2), LuaObject(m_l, -1))) {
            lua_pop(l, 3);
            return false;
        }

        lua_pop(l, 1);
    }
    lua_pop(l, 1);

    return true;
}

}
