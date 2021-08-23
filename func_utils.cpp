#include "func_utils.h"
#include "lua_table.h"
#include "lua_function.h"
#include "lua_user_data.h"

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
    obj.PushSelf();
}

}
