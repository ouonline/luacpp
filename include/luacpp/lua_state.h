#ifndef __LUA_CPP_LUA_STATE_H__
#define __LUA_CPP_LUA_STATE_H__

extern "C" {
#include "lua.h"
#include "lualib.h"
}

#include "lua_class.h"
#include "lua_table.h"
#include "lua_function.h"
#include <functional>

namespace luacpp {

class LuaState final {
private:
    template <typename FuncType>
    LuaFunction DoCreateFunctionImpl(FuncType&& f, const char* name) {
        CreateGenericFunction(m_l, m_gc_table_ref, 0, std::forward<FuncType>(f));

        LuaFunction ret(m_l, -1);
        if (name) {
            lua_setglobal(m_l, name);
        } else {
            lua_pop(m_l, 1);
        }
        return ret;
    }

    /** c-style functions, `std::function`s and lambda functions */
    template <typename FuncType>
    LuaFunction DoCreateFunction(FuncType&& f, const char* name = nullptr) {
        using RealFuncType = typename std::remove_reference<FuncType>::type;
        using ConvertedFuncType =
            typename std::conditional<std::is_pointer<RealFuncType>::value, RealFuncType,
                                      typename FunctionTraits<RealFuncType>::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        return DoCreateFunctionImpl(std::move(func), name);
    }

    /** lua-style functions that can be used to implement variadic argument functions */
    LuaFunction DoCreateFunction(int (*f)(lua_State*), const char* name = nullptr) {
        lua_pushcfunction(m_l, f);
        LuaFunction ret(m_l, -1);
        if (name) {
            lua_setglobal(m_l, name);
        } else {
            lua_pop(m_l, 1);
        }
        return ret;
    }

    /* ----------------------- utils for class ------------------------ */

    static int luacpp_index_for_class(lua_State* l);
    static int luacpp_newindex_for_class(lua_State* l);
    static int luacpp_index_for_class_instance(lua_State* l);
    static int luacpp_newindex_for_class_instance(lua_State* l);

    void CreateClassMetatable(lua_State* l);
    void CreateClassInstanceMetatable(lua_State* l, int (*gc)(lua_State*));

public:
    LuaState(lua_State* l, bool is_owner);
    LuaState(LuaState&&);
    LuaState(const LuaState&) = delete;
    ~LuaState();

    LuaState& operator=(LuaState&&);
    LuaState& operator=(const LuaState&) = delete;

    void Set(const char* name, const LuaRefObject& lobj);

    LuaObject Get(const char* name) const {
        return GenericGetObject<LuaObject>(name);
    }
    LuaTable GetTable(const char* name) const {
        return GenericGetObject<LuaTable>(name);
    }
    LuaFunction GetFunction(const char* name) const {
        return GenericGetObject<LuaFunction>(name);
    }

    template <typename T>
    LuaClass<T> GetClass(const char* name) const {
        return GenericGetObject<LuaClass<T>>(name);
    }

    const char* GetString(const char* name) const;
    LuaStringRef GetStringRef(const char* name) const;
    lua_Number GetNumber(const char* name) const;
    lua_Integer GetInteger(const char* name) const;
    void* GetPointer(const char* name) const;

    void Push(const LuaRefObject& lobj) {
        PushValue(m_l, lobj);
    }
    void PushString(const char* str) {
        lua_pushstring(m_l, str);
    }
    void PushString(const char* str, uint64_t len) {
        lua_pushlstring(m_l, str, len);
    }
    void PushNumber(lua_Number value) {
        lua_pushnumber(m_l, value);
    }
    void PushInteger(lua_Integer value) {
        lua_pushinteger(m_l, value);
    }
    void PushPointer(void* ptr) {
        lua_pushlightuserdata(m_l, ptr);
    }
    void PushNil() {
        lua_pushnil(m_l);
    }

    LuaObject CreateString(const char* str, const char* name = nullptr);
    LuaObject CreateString(const char* str, uint64_t len, const char* name = nullptr);
    LuaObject CreateNumber(lua_Number value, const char* name = nullptr);
    LuaObject CreateInteger(lua_Integer value, const char* name = nullptr);
    LuaObject CreatePointer(void* ptr, const char* name = nullptr);

    LuaTable CreateTable(const char* name = nullptr);

    LuaObject CreateNil() {
        return LuaObject(m_l);
    }

    template <typename FuncType>
    LuaFunction CreateFunction(FuncType&& f, const char* name = nullptr) {
        return DoCreateFunction(std::forward<FuncType>(f), name);
    }

    template <typename T>
    LuaClass<T> CreateClass(const char* name = nullptr) {
        auto ud = (LuaClassData*)lua_newuserdatauv(m_l, sizeof(LuaClassData), 2);
        new (ud) LuaClassData();
        ud->gc_table_ref = m_gc_table_ref;

        // metatable for class itself
        CreateClassMetatable(m_l);
        lua_setmetatable(m_l, -2);

        // uservalue 1 is a table of parent classes
        lua_newtable(m_l);
        lua_setiuservalue(m_l, -2, CLASS_PARENT_TABLE_IDX);

        // uservalue 2 is the metatable for class instances
        CreateClassInstanceMetatable(m_l, luacpp_generic_destructor<T>);
        lua_setiuservalue(m_l, -2, CLASS_INSTANCE_METATABLE_IDX);

        LuaClass<T> ret(m_l, -1);
        if (name) {
            lua_setglobal(m_l, name);
        } else {
            lua_pop(m_l, 1);
        }
        return ret;
    }

    bool DoString(const char* chunk, std::string* errstr = nullptr,
                  const std::function<bool(uint32_t, const LuaObject&)>& callback = nullptr);

    bool DoFile(const char* script, std::string* errstr = nullptr,
                const std::function<bool(uint32_t, const LuaObject&)>& callback = nullptr);

private:
    template <typename T>
    T GenericGetObject(const char* name) const {
        lua_getglobal(m_l, name);
        T ret(m_l, -1);
        lua_pop(m_l, 1);
        return ret;
    }

private:
    lua_State* m_l;
    void (*m_deleter)(lua_State*);

    /** metatable(only contains __gc) for DestructorObject */
    int m_gc_table_ref;
};

} // namespace luacpp

#endif
