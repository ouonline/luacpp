#include "lua_table.h"
using namespace std;

namespace luacpp {

uint64_t LuaTable::Size() const {
    PushSelf();
    auto len = lua_rawlen(m_l, -1);
    lua_pop(m_l, 1);
    return len;
}

LuaObject LuaTable::Get(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    LuaObject ret(m_l, -1);
    lua_pop(m_l, 2);
    return ret;
}

LuaObject LuaTable::Get(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l, 2);
    return ret;
}

void LuaTable::Set(int index, const char* str) {
    PushSelf();
    lua_pushstring(m_l, str);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::Set(int index, const char* str, uint64_t len) {
    PushSelf();
    lua_pushlstring(m_l, str, len);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::Set(int index, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l, value);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::Set(int index, const LuaObject& lobj) {
    PushSelf();
    lobj.PushSelf();
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::Set(const char* name, const char* str) {
    PushSelf();
    lua_pushstring(m_l, str);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::Set(const char* name, const char* str, uint64_t len) {
    PushSelf();
    lua_pushlstring(m_l, str, len);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::Set(const char* name, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l, value);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::Set(const char* name, const LuaObject& lobj) {
    PushSelf();
    lobj.PushSelf();
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

bool LuaTable::ForEach(const function<bool (const LuaObject& key,
                                            const LuaObject& value)>& func) const {
    PushSelf();
    lua_pushnil(m_l);
    while (lua_next(m_l, -2) != 0) {
        if (!func(LuaObject(m_l, -2), LuaObject(m_l, -1))) {
            lua_pop(m_l, 3);
            return false;
        }

        lua_pop(m_l, 1);
    }
    lua_pop(m_l, 1);

    return true;
}

}