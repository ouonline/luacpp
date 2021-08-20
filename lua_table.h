#ifndef __LUA_CPP_LUA_TABLE_H__
#define __LUA_CPP_LUA_TABLE_H__

#include "lua_object.h"
#include <functional>

namespace luacpp {

class LuaTable final : public LuaRefObject {
public:
    LuaTable(const std::shared_ptr<lua_State>& l, int index) : LuaRefObject(l, index) {}

    LuaTable(LuaTable&&) = default;
    LuaTable& operator=(LuaTable&&) = default;
    LuaTable(const LuaTable&) = delete;
    LuaTable& operator=(const LuaTable&) = delete;

    LuaObject Get(int index) const;
    LuaObject Get(const char* name) const;

    void Set(int index, const char* str);
    void Set(int index, const char* str, size_t len);
    void Set(int index, lua_Number);
    void Set(int index, const LuaRefObject& lobj);

    void Set(const char* name, const char* str);
    void Set(const char* name, const char* str, size_t len);
    void Set(const char* name, lua_Number value);
    void Set(const char* name, const LuaRefObject& lobj);

    bool ForEach(const std::function<bool (const LuaObject& key,
                                           const LuaObject& value)>& func) const;
};

}

#endif
