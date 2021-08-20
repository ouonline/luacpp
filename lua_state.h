#ifndef __LUA_CPP_LUA_STATE_H__
#define __LUA_CPP_LUA_STATE_H__

#include "lua.hpp"
#include "lua_class.h"
#include "lua_function.h"
#include <memory>
#include <functional>

namespace luacpp {

class LuaState final {
public:
    LuaState() : m_l(luaL_newstate(), lua_close) {
        luaL_openlibs(m_l.get());
        SetupGcTable();
    }

    ~LuaState() {
        if (m_l.get()) { // not moved
            luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_gc_table_ref);
        }
    }

    LuaState(LuaState&&) = default;
    LuaState& operator=(LuaState&&) = default;
    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    lua_State* GetRawPtr() const {
        return m_l.get();
    }

    LuaObject Get(const char* name) const;

    void Set(const char* name, const char* str);
    void Set(const char* name, const char* str, size_t len);
    void Set(const char* name, lua_Number value);
    void Set(const char* name, const LuaRefObject& lobj);

    LuaTable CreateTable(const char* name = nullptr);

    /** c-style function */
    template <typename FuncRetType, typename... FuncArgType>
    void CreateFunction(const char* name, FuncRetType (*f)(FuncArgType...)) {
        auto l = m_l.get();
        CreateGenericFunction<decltype(f), FuncRetType, FuncArgType...>(l, m_gc_table_ref, 0, f);
        lua_setglobal(l, name);
    }

    /** std::function */
    template <typename FuncRetType, typename... FuncArgType>
    void CreateFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        auto l = m_l.get();
        using FuncType = typename std::remove_const<typename std::remove_reference<decltype(f)>::type>::type;
        CreateGenericFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, 0, f);
        lua_setglobal(l, name);
    }

    /** lambda function */
    template <typename FuncType>
    void CreateFunction(const char* name, const FuncType& f) {
        CreateFunction(name, Lambda2Func(f));
    }

    template <typename... Argv>
    LuaUserData CreateUserData(const char* classname, const char* name = nullptr, Argv&&... argv) {
        auto l = m_l.get();
        auto top_idx = lua_gettop(l);

        // class table not found
        lua_getglobal(l, classname);
        if (!lua_istable(l, -1)) {
            lua_pushnil(l);
            LuaUserData ret(m_l, -1);
            lua_pop(l, lua_gettop(l) - top_idx);
            return ret;
        }

        /*
          +-------------+
          | class table |
          +-------------+
          |     ...     |
          +-------------+
        */

        lua_getfield(l, -1, "__call"); // get constructor function
        lua_pushvalue(l, -2); // first argument is the class table itself
        PushValues(l, std::forward<Argv>(argv)...);
        if (lua_pcall(l, sizeof...(Argv) + 1, 1, 0) != LUA_OK) {
            lua_pushnil(l);
            LuaUserData ret(m_l, -1);
            lua_pop(l, lua_gettop(l) - top_idx);
            return ret;
        }

        /*
          +-------------+
          |  userdata   |
          +-------------+
          | class table |
          +-------------+
          |     ...     |
          +-------------+
        */

        LuaUserData ret(m_l, -1);

        if (name) {
            lua_setglobal(l, name);
            lua_pop(l, 1);
        } else {
            lua_pop(l, 2);
        }

        return ret;
    }

    template <typename T>
    LuaClass<T> RegisterClass(const char* name) {
        lua_State* l = m_l.get();
        lua_newtable(l);
        LuaClass<T> ret(m_l, -1, m_gc_table_ref);
        lua_setglobal(l, name);
        return ret;
    }

    bool DoString(const char* chunk, std::string* errstr = nullptr,
                  const std::function<bool (int, const LuaObject&)>& callback = nullptr);

    bool DoFile(const char* script, std::string* errstr = nullptr,
                const std::function<bool (int, const LuaObject&)>& callback = nullptr);

private:
    void SetupGcTable() {
        auto l = m_l.get();
        lua_newtable(l);
        lua_pushcfunction(l, GenericDestructor<DestructorObject>);
        lua_setfield(l, -2, "__gc");
        m_gc_table_ref = luaL_ref(l, LUA_REGISTRYINDEX);
    }

private:
    std::shared_ptr<lua_State> m_l;

    /** metatable(only contains __gc) for DestructorObject */
    int m_gc_table_ref;
};

}

#endif
