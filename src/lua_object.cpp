#include "luacpp/lua_object.h"
#include "luacpp/lua_table.h"
#include "luacpp/lua_function.h"
#include "luacpp/lua_user_data.h"
#include <string>
using namespace std;

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

}
