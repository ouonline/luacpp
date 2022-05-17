#ifndef __LUA_CPP_LUA_TABLE_H__
#define __LUA_CPP_LUA_TABLE_H__

#include "lua_object.h"
#include "lua_function.h"
#include <functional>

namespace luacpp {

class LuaTable final : public LuaRefObject {
public:
    LuaTable(lua_State* l, int index) : LuaRefObject(l, index) {}
    LuaTable(LuaObject&& lobj) : LuaRefObject(std::move(lobj)) {}
    LuaTable(const LuaObject& lobj) : LuaRefObject(lobj) {}
    LuaTable(LuaTable&&) = default;
    LuaTable(const LuaTable&) = default;

    LuaTable& operator=(LuaTable&&) = default;
    LuaTable& operator=(const LuaTable&) = default;

    // ----- getters ----- //

    LuaObject Get(int index) const {
        return GenericGetObject<LuaObject>(index);
    }
    LuaObject Get(const char* name) const {
        return GenericGetObject<LuaObject>(name);
    }

    LuaTable GetTable(int index) const {
        return GenericGetObject<LuaTable>(index);
    }
    LuaTable GetTable(const char* name) const {
        return GenericGetObject<LuaTable>(name);
    }

    LuaFunction GetFunction(int index) const {
        return GenericGetObject<LuaFunction>(index);
    }
    LuaFunction GetFunction(const char* name) const {
        return GenericGetObject<LuaFunction>(name);
    }

    template <typename T>
    LuaClass<T> GetClass(int index) const {
        return GenericGetObject<LuaClass<T>>(index);
    }

    template <typename T>
    LuaClass<T> GetClass(const char* name) const {
        return GenericGetObject<LuaClass<T>>(name);
    }

    LuaStringRef GetStringRef(int index) const;
    LuaStringRef GetStringRef(const char* name) const;

    const char* GetString(int index) const;
    const char* GetString(const char* name) const;

    lua_Number GetNumber(int index) const;
    lua_Number GetNumber(const char* name) const;

    lua_Integer GetInteger(int index) const;
    lua_Integer GetInteger(const char* name) const;

    void* GetPointer(int index) const;
    void* GetPointer(const char* name) const;

    // ----- setters ----- //

    void Set(int index, const LuaRefObject& lobj);
    void Set(const char* name, const LuaRefObject& lobj);

    void SetString(int index, const char* str);
    void SetString(int index, const char* str, uint64_t len);
    void SetString(const char* name, const char* str);
    void SetString(const char* name, const char* str, uint64_t len);

    void SetNumber(int index, lua_Number);
    void SetNumber(const char* name, lua_Number);

    void SetInteger(int index, lua_Integer);
    void SetInteger(const char* name, lua_Integer);

    void SetPointer(int index, void*);
    void SetPointer(const char* name, void*);

    // ----- //

    bool ForEach(const std::function<bool(uint32_t i /* starting from 0 */, const LuaObject& value)>& func) const;
    bool ForEach(const std::function<bool(const LuaObject& key, const LuaObject& value)>& func) const;

private:
    template <typename T>
    T GenericGetObject(int index) const {
        PushSelf();
        lua_rawgeti(m_l, -1, index);
        T ret(m_l, -1);
        lua_pop(m_l, 2);
        return ret;
    }

    template <typename T>
    T GenericGetObject(const char* name) const {
        PushSelf();
        lua_getfield(m_l, -1, name);
        T ret(m_l, -1);
        lua_pop(m_l, 2);
        return ret;
    }
};

} // namespace luacpp

#endif
