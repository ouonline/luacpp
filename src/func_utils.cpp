#include "luacpp/func_utils.h"
#include "luacpp/lua_table.h"
#include "luacpp/lua_function.h"
#include "luacpp/lua_user_data.h"

namespace luacpp {

ValueConverter::operator LuaObject() const {
    return LuaObject(m_l, m_index);
}

ValueConverter::operator LuaTable() const {
    return LuaTable(m_l, m_index);
}

ValueConverter::operator LuaFunction() const {
    return LuaFunction(m_l, m_index);
}

ValueConverter::operator LuaUserData() const {
    return LuaUserData(m_l, m_index);
}

void PushValue(lua_State* l, const LuaObject& obj) {
    lua_rawgeti(l, LUA_REGISTRYINDEX, obj.GetRefIndex());
}

void PushValue(lua_State* l, const LuaTable& tbl) {
    lua_rawgeti(l, LUA_REGISTRYINDEX, tbl.GetRefIndex());
}

void PushValue(lua_State* l, const LuaFunction& func) {
    lua_rawgeti(l, LUA_REGISTRYINDEX, func.GetRefIndex());
}

void PushValue(lua_State* l, const LuaUserData& ud) {
    lua_rawgeti(l, LUA_REGISTRYINDEX, ud.GetRefIndex());
}

} // namespace luacpp
