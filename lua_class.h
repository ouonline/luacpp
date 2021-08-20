#ifndef __LUA_CPP_LUA_CLASS_H__
#define __LUA_CPP_LUA_CLASS_H__

#include "lua_ref_object.h"
#include "func_utils.h"
#include <stdint.h>

namespace luacpp {

template <typename T>
class LuaClass final : protected LuaRefObject {
private:
    void PushMetatable() {
        lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_metatable_ref);
    }

    template <typename... FuncArgType>
    static void InitInstance(void* obj, FuncArgType&&... argv) {
        new (obj) T(std::forward<FuncArgType>(argv)...);
    }

    template <typename... FuncArgType>
    static int Constructor(lua_State* l) {
        // creates a new instance as return value
        lua_newuserdata(l, sizeof(T));

        // move the new instance to the first position as the first argument of InitInstance()
        lua_replace(l, 1);

        FunctionCaller<sizeof...(FuncArgType) + 1>::Exec(InitInstance<FuncArgType...>, l, 0);

        // pops arguments so that the new instance is on the top of lua_State
        auto argc = sizeof...(FuncArgType);
        lua_pop(l, argc);

        lua_pushvalue(l, lua_upvalueindex(1)); // metatable for class instances
        lua_setmetatable(l, -2);

        return 1;
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void GenericSetStaticFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();

        PushSelf();
        PushMetatable();
        CreateGenericFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, 1, f);

        lua_pushvalue(l, -1);
        lua_setfield(l, -3, name);

        lua_setfield(l, -3, name);

        lua_pop(l, 2);
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void GenericSetCStyleMemberFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();
        PushMetatable();
        CreateGenericFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, 0, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    template<typename FuncType, typename FuncRetType, typename... FuncArgType>
    void GenericSetMemberFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();
        PushMetatable();
        CreateMemberFunction<T, FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    void SetupClassTable() {
        auto l = m_l.get();

        // retrieve class table
        PushSelf();

        /*
          +-------------+
          | class table |
          +-------------+
          |     ...     |
          +-------------+
        */

        // sets the class table itself as its metatable so that it becomes callable via __call
        lua_pushvalue(l, -1);
        lua_setmetatable(l, -2);

        /*
          +-------------+
          | class table |
          +-------------+
          |     ...     |
          +-------------+
        */

        lua_pop(l, 1);
    }

    void SetupMetatable() {
        auto l = m_l.get();

        // creates metatable for class instances
        lua_newtable(l);

        /*
          +-------------+
          |  metatable  |
          +-------------+
          |     ...     |
          +-------------+
        */

        // sets the metatable itself as its metatable
        lua_pushvalue(l, -1);
        lua_setmetatable(l, -2);

        /*
          +-------------+
          |  metatable  |
          +-------------+
          |     ...     |
          +-------------+
        */

        // sets the __index field to be itself so that userdata can find member functions
        lua_pushvalue(l, -1);
        lua_setfield(l, -2, "__index");

        /*
          +-------------+
          |  metatable  |
          +-------------+
          |     ...     |
          +-------------+
        */

        // destructor for class instances
        lua_pushcfunction(l, GenericDestructor<T>);
        lua_setfield(l, -2, "__gc");

        /*
          +-------------+
          |  metatable  |
          +-------------+
          |     ...     |
          +-------------+
        */

        m_metatable_ref = luaL_ref(l, LUA_REGISTRYINDEX);
    }

public:
    LuaClass(const std::shared_ptr<lua_State>& lp, int index, int gc_table_ref)
        : LuaRefObject(lp, index), m_gc_table_ref(gc_table_ref) {
        SetupClassTable();
        SetupMetatable();
    }

    LuaClass(LuaClass&&) = default;
    LuaClass& operator=(LuaClass&&) = default;
    LuaClass(const LuaClass&) = delete;
    LuaClass& operator=(const LuaClass&) = delete;

    ~LuaClass() {
        if (m_l.get()) { // not moved
            luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_metatable_ref);
        }
    }

    /** constructor */
    template <typename... FuncArgType>
    LuaClass<T>& SetConstructor() {
        auto l = m_l.get();
        PushSelf();
        PushMetatable();
        lua_pushcclosure(l, Constructor<FuncArgType...>, 1);
        lua_setfield(l, -2, "__call");
        lua_pop(l, 1);
        return *this;
    }

    /** member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (T::*f)(FuncArgType...)) {
        GenericSetMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** member function with const qualifier */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (T::*f)(FuncArgType...) const) {
        GenericSetMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** std::function member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetMemberFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = typename std::remove_const<typename std::remove_reference<decltype(f)>::type>::type;
        GenericSetCStyleMemberFunction<FuncType, FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lambda member function */
    template <typename FuncType>
    LuaClass<T>& SetMemberFunction(const char* name, const FuncType& f) {
        SetMemberFunction(name, Lambda2Func(f));
        return *this;
    }

    /** c-style member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (*f)(FuncArgType...)) {
        GenericSetCStyleMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lua-style member function */
    LuaClass<T>& SetMemberFunction(const char* name, int (*f)(lua_State* l)) {
        auto l = m_l.get();
        PushMetatable();
        lua_pushcfunction(l, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
        return *this;
    }

    /** std::function static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetStaticFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = typename std::remove_const<typename std::remove_reference<decltype(f)>::type>::type;
        GenericSetStaticFunction<FuncType, FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** C-style static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass<T>& SetStaticFunction(const char* name, FuncRetType (*f)(FuncArgType...)) {
        GenericSetStaticFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lambda static member function */
    template <typename FuncType>
    LuaClass<T>& SetStaticFunction(const char* name, const FuncType& f) {
        SetStaticFunction(name, Lambda2Func(f));
        return *this;
    }

    /** lua-style static member function */
    LuaClass<T>& SetStaticFunction(const char* name, int (*f)(lua_State* l)) {
        auto l = m_l.get();

        PushSelf();
        PushMetatable();
        lua_pushcfunction(l, f);

        lua_pushvalue(l, -1);
        lua_setfield(l, -3, name);

        lua_setfield(l, -3, name);

        lua_pop(l, 2);
        return *this;
    }

private:
    /** metatable for class instances */
    int m_metatable_ref;

    /** metatable(only contains __gc) for DestructorObject */
    const int m_gc_table_ref;
};

}

#endif
