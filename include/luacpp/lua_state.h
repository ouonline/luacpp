#ifndef __LUA_CPP_LUA_STATE_H__
#define __LUA_CPP_LUA_STATE_H__

extern "C" {
#include "lua.h"
#include "lualib.h"
}

#include "lua_class.h"
#include "lua_function.h"
#include <functional>

namespace luacpp {

class LuaState final {
private:
    template <typename FuncType, typename... FuncArgType>
    LuaFunction DoCreateFunctionImpl(FuncType&& f, const char* name, const TypeHolder<FuncArgType...>&) {
        using WrapperType = FuncWrapper<FuncType>;

        lua_pushinteger(m_l, 0); // argoffset
        auto wrapper = lua_newuserdatauv(m_l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(std::forward<FuncType>(f));

        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_gc_table_ref);
        lua_setmetatable(m_l, -2);

        lua_pushcclosure(m_l, luacpp_generic_function<FuncType, FuncArgType...>, 2);

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
        using Traits = FunctionTraits<FuncType>;
        using ConvertedFuncType =
            typename If<std::is_pointer<FuncType>::value, FuncType, typename Traits::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        return DoCreateFunctionImpl(std::forward<ConvertedFuncType>(func), name,
                                    typename Traits::argument_type_holder());
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

    LuaObject Get(const char* name) const;
    void Set(const char* name, const LuaObject& lobj);

    void Push(const LuaObject& lobj) {
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
    void PushNil() {
        lua_pushnil(m_l);
    }

    LuaObject CreateString(const char* str, const char* name = nullptr);
    LuaObject CreateString(const char* str, uint64_t len, const char* name = nullptr);
    LuaObject CreateNumber(lua_Number value, const char* name = nullptr);
    LuaObject CreateInteger(lua_Integer value, const char* name = nullptr);

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
    lua_State* m_l;
    void (*m_deleter)(lua_State*);

    /** metatable(only contains __gc) for DestructorObject */
    int m_gc_table_ref;
};

} // namespace luacpp

#endif
