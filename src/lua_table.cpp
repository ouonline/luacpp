#include "luacpp/lua_table.h"
using namespace std;

namespace luacpp {

// ----- getters ----- //

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

const char* LuaTable::GetString(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    const char* str = lua_tostring(m_l, -1);
    lua_pop(m_l, 2);
    return str;
}

const char* LuaTable::GetString(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    const char* str = lua_tostring(m_l, -1);
    lua_pop(m_l, 2);
    return str;
}

LuaStringRef LuaTable::GetStringRef(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    size_t len = 0;
    const char* str = lua_tolstring(m_l, -1, &len);
    lua_pop(m_l, 2);
    return LuaStringRef(str, len);
}

LuaStringRef LuaTable::GetStringRef(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    size_t len = 0;
    const char* str = lua_tolstring(m_l, -1, &len);
    lua_pop(m_l, 2);
    return LuaStringRef(str, len);
}

lua_Number LuaTable::GetNumber(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    lua_Number n = lua_tonumber(m_l, -1);
    lua_pop(m_l, 2);
    return n;
}

lua_Number LuaTable::GetNumber(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    lua_Number n = lua_tonumber(m_l, -1);
    lua_pop(m_l, 2);
    return n;
}

lua_Integer LuaTable::GetInteger(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    lua_Integer n = lua_tointeger(m_l, -1);
    lua_pop(m_l, 2);
    return n;
}

lua_Integer LuaTable::GetInteger(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    lua_Integer n = lua_tointeger(m_l, -1);
    lua_pop(m_l, 2);
    return n;
}

void* LuaTable::GetPointer(int index) const {
    PushSelf();
    lua_rawgeti(m_l, -1, index);
    void* ptr = lua_touserdata(m_l, -1);
    lua_pop(m_l, 2);
    return ptr;
}

void* LuaTable::GetPointer(const char* name) const {
    PushSelf();
    lua_getfield(m_l, -1, name);
    void* ptr = lua_touserdata(m_l, -1);
    lua_pop(m_l, 2);
    return ptr;
}

// ----- setters ----- //

void LuaTable::Set(int index, const LuaObject& lobj) {
    PushSelf();
    PushValue(m_l, lobj);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::Set(const char* name, const LuaObject& lobj) {
    PushSelf();
    PushValue(m_l, lobj);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::SetString(int index, const char* str) {
    PushSelf();
    lua_pushstring(m_l, str);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::SetString(int index, const char* str, uint64_t len) {
    PushSelf();
    lua_pushlstring(m_l, str, len);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::SetString(const char* name, const char* str) {
    PushSelf();
    lua_pushstring(m_l, str);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::SetString(const char* name, const char* str, uint64_t len) {
    PushSelf();
    lua_pushlstring(m_l, str, len);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::SetNumber(int index, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l, value);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::SetNumber(const char* name, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l, value);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::SetInteger(int index, lua_Integer value) {
    PushSelf();
    lua_pushinteger(m_l, value);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::SetInteger(const char* name, lua_Integer value) {
    PushSelf();
    lua_pushinteger(m_l, value);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

void LuaTable::SetPointer(int index, void* ptr) {
    PushSelf();
    lua_pushlightuserdata(m_l, ptr);
    lua_rawseti(m_l, -2, index);
    lua_pop(m_l, 1);
}

void LuaTable::SetPointer(const char* name, void* ptr) {
    PushSelf();
    lua_pushlightuserdata(m_l, ptr);
    lua_setfield(m_l, -2, name);
    lua_pop(m_l, 1);
}

// ----- //

bool LuaTable::ForEach(const function<bool(uint32_t i, const LuaObject& value)>& func) const {
    PushSelf();
    auto len = lua_rawlen(m_l, -1);

    for (uint32_t i = 0; i < len; ++i) {
        lua_rawgeti(m_l, -1, i + 1);
        if (!func(i, LuaObject(m_l, -1))) {
            lua_pop(m_l, 2);
            return false;
        }
        lua_pop(m_l, 1);
    }

    lua_pop(m_l, 1);
    return true;
}

bool LuaTable::ForEach(const function<bool(const LuaObject& key, const LuaObject& value)>& func) const {
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

} // namespace luacpp
