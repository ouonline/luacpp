#ifndef __LUA_CPP_LUA_STATE_H__
#define __LUA_CPP_LUA_STATE_H__

extern "C" {
#include "lua.h"
#include "lualib.h"
}

#include "lua_class.h"
#include "lua_function.h"
#include <memory>
#include <functional>

namespace luacpp {

class LuaState final {
public:
    LuaState(lua_State* l, bool is_owner);
    LuaState(LuaState&&) = default;
    LuaState& operator=(LuaState&&) = default;
    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;
    ~LuaState();

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
    LuaFunction CreateFunction(FuncRetType (*f)(FuncArgType...), const char* name = nullptr) {
        using FuncType = FuncRetType (*)(FuncArgType...);
        return DoCreateFunction<FuncType, FuncRetType, FuncArgType...>(f, name);
    }

    /** std::function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaFunction CreateFunction(const std::function<FuncRetType (FuncArgType...)>& f, const char* name = nullptr) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        return DoCreateFunction<FuncType, FuncRetType, FuncArgType...>(f, name);
    }

    /** lambda function */
    template <typename FuncType>
    LuaFunction CreateFunction(const FuncType& f, const char* name = nullptr) {
        return CreateFunction(Lambda2Func(f), name);
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
        lua_getmetatable(l, -1);

        /*
          +-------------+
          |  metatable  |
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
          |  metatable  |
          +-------------+
          | class table |
          +-------------+
          |     ...     |
          +-------------+
        */

        LuaUserData ret(m_l, -1);

        if (name) {
            lua_setglobal(l, name);
            lua_pop(l, 2);
        } else {
            lua_pop(l, 3);
        }

        return ret;
    }

    template <typename T>
    LuaClass<T> RegisterClass(const char* name = nullptr) {
        lua_State* l = m_l.get();
        lua_newtable(l);
        LuaClass<T> ret(m_l, -1, m_gc_table_ref);
        if (name) {
            lua_setglobal(l, name);
        } else {
            lua_pop(l, 1);
        }
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

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    LuaFunction DoCreateFunction(const FuncType& f, const char* name) {
        auto l = m_l.get();
        using WrapperType = ValueWrapper<FuncType>;

        lua_pushinteger(l, 0); // argoffset
        auto wrapper = lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(f);

        lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref);
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, GenericFunction<FuncType, FuncRetType, FuncArgType...>, 2);

        LuaFunction ret(m_l, -1);
        if (name) {
            lua_setglobal(l, name);
        } else {
            lua_pop(l, 1);
        }
        return ret;
    }

private:
    std::shared_ptr<lua_State> m_l;

    /** metatable(only contains __gc) for DestructorObject */
    int m_gc_table_ref;
};

}

#endif
