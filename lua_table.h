#ifndef __LUA_CPP_LUA_TABLE_H__
#define __LUA_CPP_LUA_TABLE_H__

#include "lua_object.h"
#include <functional>

namespace luacpp {

class LuaTable final : public LuaObject {
public:
    LuaTable(lua_State* l, int index) : LuaObject(l, index) {}
    LuaTable(LuaObject&& lobj) : LuaObject(std::move(lobj)) {}
    LuaTable(const LuaObject& lobj) : LuaObject(lobj) {}
    LuaTable(LuaTable&&) = default;
    LuaTable(const LuaTable&) = default;

    LuaTable& operator=(LuaTable&&) = default;
    LuaTable& operator=(const LuaTable&) = default;

    LuaObject Get(int index) const;
    LuaObject Get(const char* name) const;

    void Set(int index, const LuaObject& lobj);
    void Set(const char* name, const LuaObject& lobj);

    void SetString(int index, const char* str);
    void SetString(int index, const char* str, uint64_t len);
    void SetString(const char* name, const char* str);
    void SetString(const char* name, const char* str, uint64_t len);

    void SetNumber(int index, lua_Number);
    void SetNumber(const char* name, lua_Number);

    void SetInteger(int index, lua_Integer);
    void SetInteger(const char* name, lua_Integer);

    bool ForEach(const std::function<bool (uint32_t i /* starting from 0 */,
                                           const LuaObject& value)>& func) const;
    bool ForEach(const std::function<bool (const LuaObject& key, const LuaObject& value)>& func) const;
};

}

#endif
