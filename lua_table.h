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

    // NOTE: this function needs to push and pop the table
    uint64_t Size() const;

    LuaObject Get(int index) const;
    LuaObject Get(const char* name) const;

    void Set(int index, const char* str);
    void Set(int index, const char* str, uint64_t len);
    void Set(int index, lua_Number);
    void Set(int index, const LuaObject& lobj);

    void Set(const char* name, const char* str);
    void Set(const char* name, const char* str, uint64_t len);
    void Set(const char* name, lua_Number value);
    void Set(const char* name, const LuaObject& lobj);

    bool ForEach(const std::function<bool (int i /* starting from 0 */,
                                           const LuaObject& value)>& func) const;
    bool ForEach(const std::function<bool (const LuaObject& key, const LuaObject& value)>& func) const;
};

}

#endif
