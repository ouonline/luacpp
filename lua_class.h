#ifndef __LUA_CPP_LUA_CLASS_H__
#define __LUA_CPP_LUA_CLASS_H__

#include "lua_user_data.h"
#include "func_utils.h"
#include <stdint.h>

namespace luacpp {

static constexpr uint32_t CLASS_PROPERTY = 0;
static constexpr uint32_t CLASS_FUNCTION = 1;

struct LuaClassData final {
    int gc_table_ref = LUA_REFNIL;
    int metatable_ref = LUA_REFNIL;
};

template <typename T>
class LuaClass final : public LuaObject {
private:
    void PushMetatable() const {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_metatable_ref);
    }

    template <typename... FuncArgType>
    static void InitInstance(T* obj, FuncArgType&&... argv) {
        new (obj) T(std::forward<FuncArgType>(argv)...);
    }

    template <typename... FuncArgType>
    static int luacpp_constructor(lua_State* l) {
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

        lua_pushcclosure(l, luacpp_generic_function<FuncType, FuncRetType, FuncArgType...>, 2);
        lua_setfield(l, -2, "func");
    }

    template <typename FuncType, typename FuncRetType, typename... FuncArgType>
    void DoDefStaticFunction(lua_State* l, const char* name, const FuncType& f) {
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
    void DoDefCStyleMemberFunction(lua_State* l, const char* name, const FuncType& f) {
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
    static int luacpp_member_function(lua_State* l) {
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

        lua_pushcclosure(l, luacpp_member_function<FuncType, FuncRetType, FuncArgType...>, 2);
        lua_setfield(l, -2, "func");
    }

    template<typename FuncType, typename FuncRetType, typename... FuncArgType>
    void DoDefMemberFunction(lua_State* l, const char* name, const FuncType& f) {
        PushMetatable();
        CreateMemberFunction<FuncType, FuncRetType, FuncArgType...>(l, m_gc_table_ref, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    /* -------------------------------------------------------------------------- */

    template <typename GetterType>
    static int luacpp_member_property_getter(lua_State* l) {
        using WrapperType = ValueWrapper<GetterType>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, wrapper->value(ud));
        return 1;
    }

    template <typename SetterType>
    static int luacpp_member_property_setter(lua_State* l) {
        using WrapperType = ValueWrapper<SetterType>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        wrapper->value(ud, ValueConverter(l, 2));
        return 0;
    }

    template <typename GetterType, typename SetterType>
    void CreateMemberProperty(lua_State* l, const GetterType& getter,
                              const SetterType& setter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        {
            using WrapperType = ValueWrapper<GetterType>;
            auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
            new (wrapper) WrapperType(getter);
            lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_member_property_getter<GetterType>, 1);
            lua_setfield(l, -2, "getter");
        }
        {
            using WrapperType = ValueWrapper<SetterType>;
            auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
            new (wrapper) WrapperType(setter);
            lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_member_property_setter<SetterType>, 1);
            lua_setfield(l, -2, "setter");
        }
    }

    template <typename Getter>
    void CreateMemberPropertyReadOnly(lua_State* l, const Getter& getter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        using WrapperType = ValueWrapper<Getter>;
        auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(getter);
        lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_getter<Getter>, 1);
        lua_setfield(l, -2, "getter");
    }

    template <typename SetterType>
    void CreateMemberPropertyWriteOnly(lua_State* l, const SetterType& setter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        using WrapperType = ValueWrapper<SetterType>;
        auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(setter);
        lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename GetterType, typename SetterType>
    void DoDefMemberProperty(lua_State* l, const char* name, const GetterType& getter,
                             const SetterType& setter) {
        PushMetatable();
        CreateMemberProperty(m_l, getter, setter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
    }

    template <typename GetterType>
    void DoDefMemberPropertyReadOnly(lua_State* l, const char* name, const GetterType& getter) {
        PushMetatable();
        CreateMemberPropertyReadOnly(m_l, getter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
    }

    template <typename SetterType>
    void DoDefMemberPropertyWriteOnly(lua_State* l, const char* name, const SetterType& setter) {
        PushMetatable();
        CreateMemberPropertyWriteOnly(m_l, setter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
    }

    template <typename GetterType>
    static int luacpp_static_property_getter(lua_State* l) {
        using WrapperType = ValueWrapper<GetterType>;
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, wrapper->value());
        return 1;
    }

    template <typename SetterType>
    static int luacpp_static_property_setter(lua_State* l) {
        using WrapperType = ValueWrapper<SetterType>;
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        wrapper->value(ValueConverter(l, 2));
        return 0;
    }

    template <typename GetterType, typename SetterType>
    void CreateStaticProperty(lua_State* l, const GetterType& getter, const SetterType& setter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        {
            using WrapperType = ValueWrapper<GetterType>;
            auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
            new (wrapper) WrapperType(getter);
            lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_static_property_getter<GetterType>, 1);
            lua_setfield(l, -2, "getter");
        }
        {
            using WrapperType = ValueWrapper<SetterType>;
            auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
            new (wrapper) WrapperType(setter);
            lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_static_property_setter<SetterType>, 1);
            lua_setfield(l, -2, "setter");
        }
    }

    template <typename GetterType>
    void CreateStaticPropertyReadOnly(lua_State* l, const GetterType& getter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        using WrapperType = ValueWrapper<GetterType>;
        auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(getter);
        lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_getter<GetterType>, 1);
        lua_setfield(l, -2, "getter");
    }

    template <typename SetterType>
    void CreateStaticPropertyWriteOnly(lua_State* l, const SetterType& setter) {
        lua_newtable(l);

        lua_pushinteger(l, CLASS_PROPERTY);
        lua_setfield(l, -2, "type");

        using WrapperType = ValueWrapper<SetterType>;
        auto wrapper = (WrapperType*)lua_newuserdata(l, sizeof(WrapperType));
        new (wrapper) WrapperType(setter);
        lua_rawgeti(l, LUA_REGISTRYINDEX, m_gc_table_ref); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename GetterType, typename SetterType>
    void DoDefStaticProperty(lua_State* l, const char* name, const GetterType& getter,
                             const SetterType& setter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        PushMetatable();

        CreateStaticProperty(m_l, getter, setter);
        lua_pushvalue(m_l, -1);
        lua_setfield(m_l, -3, name);
        lua_setfield(m_l, -3, name);

        lua_pop(m_l, 3);
    }

    template <typename GetterType>
    void DoDefStaticPropertyReadOnly(lua_State* l, const char* name, const GetterType& getter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        PushMetatable();

        CreateStaticPropertyReadOnly(m_l, getter);
        lua_pushvalue(m_l, -1);
        lua_setfield(m_l, -3, name);
        lua_setfield(m_l, -3, name);

        lua_pop(m_l, 3);
    }

    template <typename SetterType>
    void DoDefStaticPropertyWriteOnly(lua_State* l, const char* name, const SetterType& setter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        PushMetatable();

        CreateStaticPropertyWriteOnly(m_l, setter);
        lua_pushvalue(m_l, -1);
        lua_setfield(m_l, -3, name);
        lua_setfield(m_l, -3, name);

        lua_pop(m_l, 3);
    }

    void Init() {
        PushSelf();
        auto ud = (LuaClassData*)lua_touserdata(m_l, -1);
        m_gc_table_ref = ud->gc_table_ref;
        m_metatable_ref = ud->metatable_ref;
        lua_pop(m_l, 1);
    }

public:
    LuaClass(lua_State* l, int index) : LuaObject(l, index) {
        Init();
    }
    LuaClass(LuaObject&& rhs) : LuaObject(std::move(rhs)) {
        Init();
    }
    LuaClass(const LuaObject& rhs) : LuaObject(rhs) {
        Init();
    }

    LuaClass(LuaClass&&) = default;
    LuaClass(const LuaClass&) = default;

    LuaClass& operator=(LuaClass&&) = default;
    LuaClass& operator=(const LuaClass&) = default;

    /** constructor */
    template <typename... FuncArgType>
    LuaClass& DefConstructor() {
        PushSelf();
        lua_getmetatable(m_l, -1);
        PushMetatable();
        lua_pushcclosure(m_l, luacpp_constructor<FuncArgType...>, 1);
        lua_setfield(m_l, -2, "__call");
        lua_pop(m_l, 2);
        return *this;
    }

    /* ------------------------------- member ----------------------------------- */

    /**
       member property. only lambda functions are supported currently.

       GetterType: (const T*) -> SupportedType
       SetterType: (T*, const SupportedType&) -> void
    */
    template <typename GetterType, typename SetterType>
    LuaClass& DefMember(const char* name, const GetterType& getter, const SetterType& setter) {
        DoDefMemberProperty(m_l, name, Lambda2Func(getter), Lambda2Func(setter));
        return *this;
    }

    template <typename GetterType>
    LuaClass& DefMemberReadOnly(const char* name, const GetterType& getter) {
        DoDefMemberPropertyReadOnly(m_l, name, getter);
        return *this;
    }

    template <typename SetterType>
    LuaClass& DefMemberWriteOnly(const char* name, const SetterType& setter) {
        DoDefMemberPropertyWriteOnly(m_l, name, setter);
        return *this;
    }


    // ----- //

    /** member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...)) {
        DoDefMemberFunction<decltype(f), FuncRetType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** member function with const qualifier */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...) const) {
        DoDefMemberFunction<decltype(f), FuncRetType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** std::function member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        DoDefCStyleMemberFunction<FuncType, FuncRetType, FuncArgType...>(m_l, name, f);
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
        DoDefCStyleMemberFunction<decltype(f), FuncRetType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** lua-style member function */
    LuaClass& DefMember(const char* name, int (*f)(lua_State*)) {
        PushMetatable();
        DoDefLuaFunction(m_l, f);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
        return *this;
    }

    /* ------------------------------- static ----------------------------------- */

    /**
       static property.

       GetterType: () -> SupportedType
       SetterType: (const SupportedType&) -> void
    */
    template <typename GetterType, typename SetterType>
    LuaClass& DefStatic(const char* name, const GetterType& getter, const SetterType& setter) {
        DoDefStaticProperty(m_l, name, getter, setter);
        return *this;
    }

    template <typename GetterType>
    LuaClass& DefStaticReadOnly(const char* name, const GetterType& getter) {
        DoDefStaticPropertyReadOnly(m_l, name, getter);
        return *this;
    }

    template <typename SetterType>
    LuaClass& DefStaticWriteOnly(const char* name, const SetterType& setter) {
        DoDefStaticPropertyWriteOnly(m_l, name, setter);
        return *this;
    }

    // ----- //

    /** std::function static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefStatic(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        DoDefStaticFunction<FuncType, FuncRetType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** C-style static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefStatic(const char* name, FuncRetType (*f)(FuncArgType...)) {
        DoDefStaticFunction<decltype(f), FuncRetType, FuncArgType...>(m_l, name, f);
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
        PushSelf();
        lua_getmetatable(m_l, -1);
        PushMetatable();

        DoDefLuaFunction(m_l, f);

        lua_pushvalue(m_l, -1);
        lua_setfield(m_l, -3, name);

        lua_setfield(m_l, -3, name);

        lua_pop(m_l, 3);
        return *this;
    }

    /* -------------------------------------------------------------------------- */

    template <typename... Argv>
    LuaUserData CreateUserData(Argv&&... argv) const {
        auto ud = (T*)lua_newuserdata(m_l, sizeof(T));
        new (ud) T(std::forward<Argv>(argv)...);

        PushMetatable();
        lua_setmetatable(m_l, -2);

        LuaUserData ret(m_l, -1);
        lua_pop(m_l, 1);
        return ret;
    }

private:
    /** metatable for class instances */
    int m_metatable_ref;

    /** metatable(only containing __gc) for DestructorObject */
    int m_gc_table_ref;
};

}

#endif
