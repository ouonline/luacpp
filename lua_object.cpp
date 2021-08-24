#include "lua_object.h"
#include "lua_table.h"
#include "lua_function.h"
#include "lua_user_data.h"
#include <string>
using namespace std;

namespace luacpp {

bool LuaObject::ToBool() const {
    PushSelf();
    auto ret = lua_toboolean(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

LuaBufferRef LuaObject::ToBufferRef() const {
    size_t len = 0;

    PushSelf();
    const char* str = lua_tolstring(m_l, -1, &len);
    LuaBufferRef ret(str, len);
    lua_pop(m_l, 1);

    return ret;
}

lua_Number LuaObject::ToNumber() const {
    PushSelf();
    lua_Number ret = lua_tonumber(m_l, -1);
    lua_pop(m_l, 1);
    return ret;
}

}
