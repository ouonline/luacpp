#ifndef __LUA_CPP_LUA_OBJECT_H__
#define __LUA_CPP_LUA_OBJECT_H__

#include "lua_ref_object.h"
#include "lua_string_ref.h"

namespace luacpp {

class LuaObject final : public LuaRefObject {
public:
    LuaObject(lua_State* l) : LuaRefObject(l) {}
    LuaObject(lua_State* l, int index) : LuaRefObject(l, index) {}
    LuaObject(LuaObject&& rhs) : LuaRefObject(std::move(rhs)) {}
    LuaObject(const LuaObject& rhs) : LuaRefObject(rhs) {}

    LuaObject& operator=(LuaObject&& rhs) = default;
    LuaObject& operator=(const LuaObject& rhs) = default;

    bool ToBool() const;
    lua_Number ToNumber() const;
    lua_Integer ToInteger() const;
    LuaStringRef ToStringRef() const;
    const char* ToString() const;
    void* ToPointer() const;
};

} // namespace luacpp

#endif
