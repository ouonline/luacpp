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
public:
    LuaState(lua_State* l, bool is_owner);
    LuaState(LuaState&&);
    LuaState(const LuaState&) = delete;
    ~LuaState();

    LuaState& operator=(LuaState&&);
    LuaState& operator=(const LuaState&) = delete;

    lua_State* GetPtr() const {
        return m_l;
    }

    LuaObject Get(const char* name) const;
    void Set(const char* name, const LuaObject& lobj);

    LuaObject CreateObject(const char* str, const char* name = nullptr);
    LuaObject CreateObject(const char* str, uint64_t len, const char* name = nullptr);
    LuaObject CreateObject(lua_Number value, const char* name = nullptr);

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

    template <typename T>
    LuaClass<T> CreateClass(const char* name = nullptr) {
        auto ud = (LuaClassData*)lua_newuserdata(m_l, sizeof(LuaClassData));
        ud->gc_table_ref = m_gc_table_ref;
        ud->metatable_ref = SetupClassInstanceMetatable<T>(m_l);

        SetupClassData(m_l);

        LuaClass<T> ret(m_l, -1);
        if (name) {
            lua_setglobal(m_l, name);
        } else {
            lua_pop(m_l, 1);
        }
        return ret;
    }

    bool DoString(const char* chunk, std::string* errstr = nullptr,
                  const std::function<bool (int, const LuaObject&)>& callback = nullptr);

    bool DoFile(const char* script, std::string* errstr = nullptr,
                const std::function<bool (int, const LuaObject&)>& callback = nullptr);

private:
    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    LuaFunction DoCreateFunction(const FuncType& f, const char* name) {
        using WrapperType = ValueWrapper<FuncType>;

        lua_pushinteger(m_l, 0); // argoffset
        auto wrapper = lua_newuserdata(m_l, sizeof(WrapperType));
        new (wrapper) WrapperType(f);

        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_gc_table_ref);
        lua_setmetatable(m_l, -2);

        lua_pushcclosure(m_l, luacpp_generic_function<FuncType, FuncRetType, FuncArgType...>, 2);

        LuaFunction ret(m_l, -1);
        if (name) {
            lua_setglobal(m_l, name);
        } else {
            lua_pop(m_l, 1);
        }
        return ret;
    }

    /* ----------------------- utils for class ------------------------ */

    static int IndexFunctionForClass(lua_State* l) {
        auto key = lua_tostring(l, 2);
        auto ret_type = luaL_getmetafield(l, 1, key);
        if (ret_type != LUA_TTABLE) {
            lua_pushnil(l);
            return 1;
        }

        lua_getfield(l, -1, "type");
        int value_type = lua_tointeger(l, -1);

        if (value_type == CLASS_FUNCTION) {
            lua_getfield(l, -2, "func");
            return 1;
        } else if (value_type == CLASS_PROPERTY) {
            lua_getfield(l, -2, "getter");
            if (lua_isnil(l, -1)) {
                return 1;
            }
            lua_pushvalue(l, 1); // userdata
            if (lua_pcall(l, 1, 1, 0) == LUA_OK) {
                return 1;
            }
        }

        lua_pushnil(l);
        return 1;
    }

    static int NewIndexFunctionForClass(lua_State* l) {
        auto key = lua_tostring(l, 2);
        auto key_type = luaL_getmetafield(l, 1, key);
        if (key_type != LUA_TTABLE) { // is not a field defined in c++
            return 0;
        }

        lua_getfield(l, -1, "type");
        int value_type = lua_tointeger(l, -1);

        // only values can be modified
        if (value_type == CLASS_PROPERTY) {
            // move userdata to position 2
            lua_pushvalue(l, 1);
            lua_replace(l, 2);
            // move setter to position 1
            lua_getfield(l, -2, "setter");
            if (lua_isnil(l, -1)) {
                return 0;
            }
            lua_replace(l, 1);
            lua_pop(l, 2);
            lua_pcall(l, 2, 0, 0);
        }

        return 0;
    }

    static int DestructorForClass(lua_State* l) {
        // destroying metatable of instances only when this class is destroyed.
        auto ud = (LuaClassData*)lua_touserdata(l, 1);
        luaL_unref(l, LUA_REGISTRYINDEX, ud->metatable_ref);
        return 0;
    }

    void SetupClassData(lua_State* l) {
        // sets a metatable so that it becomes callable via __call
        lua_newtable(l);

        lua_pushvalue(l, -1);
        lua_setmetatable(l, -3);

        lua_pushcfunction(l, NewIndexFunctionForClass);
        lua_setfield(l, -2, "__newindex");

        lua_pushcfunction(l, IndexFunctionForClass);
        lua_setfield(l, -2, "__index");

        lua_pushcfunction(l, DestructorForClass);
        lua_setfield(l, -2, "__gc");

        lua_pop(l, 1);
    }

    // returns ref index of the metatable
    template <typename T>
    int SetupClassInstanceMetatable(lua_State* l) {
        // creates metatable for class instances
        lua_newtable(l);

        // sets the __newindex field so that userdata can modify members
        lua_pushcfunction(l, NewIndexFunctionForClass);
        lua_setfield(l, -2, "__newindex");

        // sets the __index field to be itself so that userdata can find member functions
        lua_pushcfunction(l, IndexFunctionForClass);
        lua_setfield(l, -2, "__index");

        // destructor for class instances
        lua_pushcfunction(l, luacpp_generic_destructor<T>);
        lua_setfield(l, -2, "__gc");

        return luaL_ref(l, LUA_REGISTRYINDEX);
    }

    /* ---------------------------------------------------------------- */

private:
    lua_State* m_l;
    void (*m_deleter)(lua_State*);

    /** metatable(only contains __gc) for DestructorObject */
    int m_gc_table_ref;
};

}

#endif
