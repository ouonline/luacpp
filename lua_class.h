#ifndef __LUA_CPP_LUA_CLASS_H__
#define __LUA_CPP_LUA_CLASS_H__

#include "lua_ref_object.h"
#include "func_utils.h"
#include <stdint.h>

namespace luacpp {

static constexpr uint32_t READ = 0x01;
static constexpr uint32_t WRITE = 0x10;
static constexpr uint32_t READWRITE = 0x11;

template <typename T>
class LuaClass final : public LuaRefObject {
private:
    static constexpr uint32_t CLASS_PROPERTY = 0;
    static constexpr uint32_t CLASS_FUNCTION = 1;

    void PushMetatable() {
        lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_metatable_ref);
    }

    template <typename... FuncArgType>
    static void InitInstance(T* obj, FuncArgType&&... argv) {
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

    static int IndexFunction(lua_State* l) {
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

    static int NewIndexFunction(lua_State* l) {
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

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void CreateGenericFunction(lua_State* l, int gc_table_ref, int argoffset, const FuncType& f) {
        using WrapperType = ValueWrapper<FuncType>;

        lua_newtable(l);

        lua_pushinteger(l, CLASS_FUNCTION);
        lua_setfield(l, -2, "type");

        lua_pushinteger(l, argoffset); // argoffset
        auto wrapper = lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(f);

        // destructor
        lua_rawgeti(l, LUA_REGISTRYINDEX, gc_table_ref);
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, GenericFunction<FuncType, FuncRetType, FuncArgType...>, 2);
        lua_setfield(l, -2, "func");
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void DoDefStaticFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();

        PushSelf();
        lua_getmetatable(l, -1);
        PushMetatable();
        CreateGenericFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, 1, f);

        lua_pushvalue(l, -1);
        lua_setfield(l, -3, name);

        lua_setfield(l, -3, name);

        lua_pop(l, 3);
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void DoDefCStyleMemberFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();
        PushMetatable();
        CreateGenericFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, 0, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    void DoDefLuaFunction(lua_State* l, int (*f)(lua_State*)) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_FUNCTION);
        lua_setfield(l, -2, "type");

        lua_pushcfunction(l, f);
        lua_setfield(l, -2, "func");
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    static int MemberFunction(lua_State* l) {
        auto argoffset = lua_tointeger(l, lua_upvalueindex(1));
        auto wrapper = (ValueWrapper<FuncType>*)lua_touserdata(l, lua_upvalueindex(2));
        auto ud = (T*)lua_touserdata(l, 1);
        return FunctionCaller<sizeof...(FuncArgType)>::Exec(ud, wrapper->value, l, argoffset);
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    static void CreateMemberFunction(lua_State* l, int gc_table_ref, const FuncType& f) {
        using WrapperType = ValueWrapper<FuncType>;

        lua_newtable(l);

        lua_pushinteger(l, CLASS_FUNCTION);
        lua_setfield(l, -2, "type");

        lua_pushinteger(l, 1); // argoffset
        auto wrapper = lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(f);

        // destructor
        lua_rawgeti(l, LUA_REGISTRYINDEX, gc_table_ref);
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, MemberFunction<FuncType, FuncRetType, FuncArgType...>, 2);
        lua_setfield(l, -2, "func");
    }

    template<typename FuncType, typename FuncRetType, typename... FuncArgType>
    void DoDefMemberFunction(const char* name, const FuncType& f) {
        auto l = m_l.get();
        PushMetatable();
        CreateMemberFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    template <typename PropertyType>
    static int MemberPropertyGetter(lua_State* l) {
        using WrapperType = ValueWrapper<PropertyType T::*>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, ud->*(wrapper->value));
        return 1;
    }

    template <typename PropertyType>
    static int MemberPropertySetter(lua_State* l) {
        using WrapperType = ValueWrapper<PropertyType T::*>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        ud->*(wrapper->value) = (PropertyType)ValueConverter(l, 2);
        return 0;
    }

    template <typename PropertyType>
    static void DoDefMemberProperty(lua_State* l, int gc_table_ref, PropertyType T::* mptr, uint32_t permission) {
        using WrapperType = ValueWrapper<PropertyType T::*>;

        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        auto wrapper = lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(mptr);

        // destructor
        lua_rawgeti(l, LUA_REGISTRYINDEX, gc_table_ref);
        lua_setmetatable(l, -2);

        if (permission & READ) {
            lua_pushvalue(l, -1); // wrapper
            lua_pushcclosure(l, MemberPropertyGetter<PropertyType>, 1);
            lua_setfield(l, -3, "getter");
        }

        if (permission & WRITE) {
            lua_pushvalue(l, -1); // wrapper
            lua_pushcclosure(l, MemberPropertySetter<PropertyType>, 1);
            lua_setfield(l, -3, "setter");
        }

        lua_pop(l, 1);
    }

    template <typename PropertyType>
    static int StaticPropertyGetter(lua_State* l) {
        auto ptr = (PropertyType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, *ptr);
        return 1;
    }

    template <typename PropertyType>
    static int StaticPropertySetter(lua_State* l) {
        auto ptr = (PropertyType*)lua_touserdata(l, lua_upvalueindex(1));
        *ptr = (PropertyType)ValueConverter(l, 2);
        return 0;
    }

    template <typename PropertyType>
    static void DoDefStaticProperty(lua_State* l, PropertyType* ptr, uint32_t permission) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        if (permission & READ) {
            lua_pushlightuserdata(l, (void*)ptr);
            lua_pushcclosure(l, StaticPropertyGetter<PropertyType>, 1);
            lua_setfield(l, -2, "getter");
        }

        if (permission & WRITE) {
            lua_pushlightuserdata(l, (void*)ptr);
            lua_pushcclosure(l, StaticPropertySetter<PropertyType>, 1);
            lua_setfield(l, -2, "setter");
        }
    }

    void SetupClass() {
        auto l = m_l.get();

        // retrieve class table
        PushSelf();

        // sets a metatable so that it becomes callable via __call
        lua_newtable(l);

        lua_pushvalue(l, -1);
        lua_setmetatable(l, -3);

        lua_pushcfunction(l, NewIndexFunction);
        lua_setfield(l, -2, "__newindex");

        lua_pushcfunction(l, IndexFunction);
        lua_setfield(l, -2, "__index");

        lua_pop(l, 2);
    }

    void SetupMetatable() {
        auto l = m_l.get();

        // creates metatable for class instances
        lua_newtable(l);

        // sets the __newindex field so that userdata can modify members
        lua_pushcfunction(l, NewIndexFunction);
        lua_setfield(l, -2, "__newindex");

        // destructor for class instances
        lua_pushcfunction(l, GenericDestructor<T>);
        lua_setfield(l, -2, "__gc");

        // sets the __index field to be itself so that userdata can find member functions
        lua_pushcfunction(l, IndexFunction);
        lua_setfield(l, -2, "__index");

        m_metatable_ref = luaL_ref(l, LUA_REGISTRYINDEX);
    }

public:
    LuaClass(const std::shared_ptr<lua_State>& lp, int index, int gc_table_ref)
        : LuaRefObject(lp, index), m_gc_table_ref(gc_table_ref) {
        SetupClass();
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
    LuaClass& DefConstructor() {
        auto l = m_l.get();
        PushSelf();
        lua_getmetatable(l, -1);
        PushMetatable();
        lua_pushcclosure(l, Constructor<FuncArgType...>, 1);
        lua_setfield(l, -2, "__call");
        lua_pop(l, 2);
        return *this;
    }

    /** property */
    template <typename PropertyType>
    LuaClass& DefMember(const char* name, PropertyType T::* mptr, uint32_t permission = READWRITE) {
        auto l = m_l.get();
        PushMetatable();
        DoDefMemberProperty<PropertyType>(l, m_gc_table_ref, mptr, permission);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
        return *this;
    }

    /** member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...)) {
        DoDefMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** member function with const qualifier */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...) const) {
        DoDefMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** std::function member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        DoDefCStyleMemberFunction<FuncType, FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lambda member function */
    template <typename FuncType>
    LuaClass& DefMember(const char* name, const FuncType& f) {
        DefMember(name, Lambda2Func(f));
        return *this;
    }

    /** c-style member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (*f)(FuncArgType...)) {
        DoDefCStyleMemberFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lua-style member function */
    LuaClass& DefMember(const char* name, int (*f)(lua_State*)) {
        auto l = m_l.get();
        PushMetatable();
        DoDefLuaFunction(l, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
        return *this;
    }

    /** static property */
    template <typename PropertyType>
    LuaClass& DefStatic(const char* name, PropertyType* ptr, uint32_t permission = READWRITE) {
        auto l = m_l.get();
        PushSelf();
        lua_getmetatable(l, -1);
        DoDefStaticProperty(l, ptr, permission);
        lua_setfield(l, -2, name);
        lua_pop(l, 2);
        return *this;
    }

    /** std::function static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefStatic(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        DoDefStaticFunction<FuncType, FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** C-style static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefStatic(const char* name, FuncRetType (*f)(FuncArgType...)) {
        DoDefStaticFunction<decltype(f), FuncRetType, FuncArgType...>(name, f);
        return *this;
    }

    /** lambda static member function */
    template <typename FuncType>
    LuaClass& DefStatic(const char* name, const FuncType& f) {
        DefStatic(name, Lambda2Func(f));
        return *this;
    }

    /** lua-style static member function */
    LuaClass& DefStatic(const char* name, int (*f)(lua_State*)) {
        auto l = m_l.get();

        PushSelf();
        lua_getmetatable(l, -1);
        PushMetatable();

        DoDefLuaFunction(l, f);

        lua_pushvalue(l, -1);
        lua_setfield(l, -3, name);

        lua_setfield(l, -3, name);

        lua_pop(l, 3);
        return *this;
    }

private:
    /** metatable for class instances */
    int m_metatable_ref;

    /** metatable(only containing __gc) for DestructorObject */
    const int m_gc_table_ref;
};

}

#endif
