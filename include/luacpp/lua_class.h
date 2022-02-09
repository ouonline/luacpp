#ifndef __LUA_CPP_LUA_CLASS_H__
#define __LUA_CPP_LUA_CLASS_H__

#include "lua_52_53.h"
#include "lua_user_data.h"
#include "func_utils.h"
#include <stdint.h>

namespace luacpp {

static constexpr uint32_t CLASS_PARENT_TABLE_IDX = 1;
static constexpr uint32_t CLASS_INSTANCE_METATABLE_IDX = 2;

struct LuaClassData final {
    /** a metatable including only __gc function for various objects(such as ValueWrapper) */
    int gc_table_ref = LUA_REFNIL;
};

template <typename T>
class LuaClass final : public LuaObject {
private:
    void PushInstanceMetatable() const {
        PushSelf();
        lua_getiuservalue(m_l, -1, CLASS_INSTANCE_METATABLE_IDX);
        lua_remove(m_l, -2);
    }
    void PushParentsTable() const {
        PushSelf();
        lua_getiuservalue(m_l, -1, CLASS_PARENT_TABLE_IDX);
        lua_remove(m_l, -2);
    }
    void PushGcTable() const {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_data->gc_table_ref);
    }

    template <typename... FuncArgType>
    static void InitInstance(T* obj, FuncArgType&&... argv) {
        new (obj) T(std::forward<FuncArgType>(argv)...);
    }

    template <typename... FuncArgType>
    static int luacpp_constructor(lua_State* l) {
        // creates a new instance as return value
        lua_newuserdatauv(l, sizeof(T), 1);

        // move the new instance to the first position as the first argument of InitInstance()
        lua_replace(l, 1);

        FunctionCaller<sizeof...(FuncArgType) + 1>::Execute(InitInstance<FuncArgType...>, l, 0);

        // pops arguments so that the new instance is on the top of lua_State
        auto argc = sizeof...(FuncArgType);
        lua_pop(l, argc);

        lua_pushvalue(l, lua_upvalueindex(1)); // this class
        lua_setiuservalue(l, 1, 1);

        lua_getiuservalue(l, lua_upvalueindex(1), CLASS_INSTANCE_METATABLE_IDX); // instance's metatable
        lua_setmetatable(l, 1);

        return 1;
    }

    template <typename FuncType, typename... FuncArgType>
    void CreateGenericFunction(lua_State* l, int argoffset, const FuncType& f) {
        using WrapperType = ValueWrapper<FuncType>;

        // upvalue 1: argoffset
        lua_pushinteger(l, argoffset);

        // upvalue 2: wrapper
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(f);

        // wrapper's destructor
        PushGcTable();
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, luacpp_generic_function<FuncType, FuncArgType...>, 2);
    }

    template <typename FuncType, typename... FuncArgType>
    void DoDefStaticFunction(lua_State* l, const char* name, const FuncType& f) {
        PushSelf();
        lua_getmetatable(l, -1);
        CreateGenericFunction<FuncType, FuncArgType...>(l, 1, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 2);
    }

    template <typename FuncType, typename... FuncArgType>
    void DoDefCStyleMemberFunction(lua_State* l, const char* name, const FuncType& f) {
        PushInstanceMetatable();
        CreateGenericFunction<FuncType, FuncArgType...>(l, 0, f);
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    template <typename FuncType, typename... FuncArgType>
    static int luacpp_member_function(lua_State* l) {
        auto argoffset = lua_tointeger(l, lua_upvalueindex(1));
        auto wrapper = (ValueWrapper<FuncType>*)lua_touserdata(l, lua_upvalueindex(2));
        auto ud = (T*)lua_touserdata(l, 1);
        return FunctionCaller<sizeof...(FuncArgType)>::Execute(ud, wrapper->value, l, argoffset);
    }

    template <typename FuncType, typename... FuncArgType>
    void CreateMemberFunction(lua_State* l, const FuncType& f) {
        using WrapperType = ValueWrapper<FuncType>;

        // upvalue 1: argoffset
        lua_pushinteger(l, 1);

        // upvalue 2: wrapper
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(f);

        // wrapper's destructor
        PushGcTable();
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, luacpp_member_function<FuncType, FuncArgType...>, 2);
    }

    template<typename FuncType, typename... FuncArgType>
    void DoDefMemberFunction(lua_State* l, const char* name, const FuncType& f) {
        PushInstanceMetatable();
        CreateMemberFunction<FuncType, FuncArgType...>(l, f);
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
        lua_createtable(l, 0, 2);

        using GetterWrapperType = ValueWrapper<GetterType>;
        auto wrapper = lua_newuserdatauv(l, sizeof(GetterWrapperType), 0);
        new (wrapper) GetterWrapperType(getter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_getter<GetterType>, 1);
        lua_setfield(l, -2, "getter");

        using SetterWrapperType = ValueWrapper<SetterType>;
        wrapper = lua_newuserdatauv(l, sizeof(SetterWrapperType), 0);
        new (wrapper) SetterWrapperType(setter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename Getter>
    void CreateMemberPropertyReadOnly(lua_State* l, const Getter& getter) {
        lua_createtable(l, 0, 1);

        using WrapperType = ValueWrapper<Getter>;
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(getter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_getter<Getter>, 1);
        lua_setfield(l, -2, "getter");
    }

    template <typename SetterType>
    void CreateMemberPropertyWriteOnly(lua_State* l, const SetterType& setter) {
        lua_createtable(l, 0, 1);

        using WrapperType = ValueWrapper<SetterType>;
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(setter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_member_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename GetterType, typename SetterType>
    void DoDefMemberProperty(lua_State* l, const char* name, const GetterType& getter,
                             const SetterType& setter) {
        PushInstanceMetatable();
        CreateMemberProperty(m_l, getter, setter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
    }

    template <typename GetterType>
    void DoDefMemberPropertyReadOnly(lua_State* l, const char* name, const GetterType& getter) {
        PushInstanceMetatable();
        CreateMemberPropertyReadOnly(m_l, getter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
    }

    template <typename SetterType>
    void DoDefMemberPropertyWriteOnly(lua_State* l, const char* name, const SetterType& setter) {
        PushInstanceMetatable();
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
        lua_createtable(l, 0, 2);

        using GetterWrapperType = ValueWrapper<GetterType>;
        auto wrapper = lua_newuserdatauv(l, sizeof(GetterWrapperType), 0);
        new (wrapper) GetterWrapperType(getter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_getter<GetterType>, 1);
        lua_setfield(l, -2, "getter");

        using SetterWrapperType = ValueWrapper<SetterType>;
        wrapper = lua_newuserdatauv(l, sizeof(SetterWrapperType), 0);
        new (wrapper) SetterWrapperType(setter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename GetterType>
    void CreateStaticPropertyReadOnly(lua_State* l, const GetterType& getter) {
        lua_createtable(l, 0, 1);

        using WrapperType = ValueWrapper<GetterType>;
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(getter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_getter<GetterType>, 1);
        lua_setfield(l, -2, "getter");
    }

    template <typename SetterType>
    void CreateStaticPropertyWriteOnly(lua_State* l, const SetterType& setter) {
        lua_createtable(l, 0, 1);

        using WrapperType = ValueWrapper<SetterType>;
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(setter);
        PushGcTable(); // wrapper's destructor
        lua_setmetatable(l, -2);
        lua_pushcclosure(l, luacpp_static_property_setter<SetterType>, 1);
        lua_setfield(l, -2, "setter");
    }

    template <typename GetterType, typename SetterType>
    void DoDefStaticProperty(lua_State* l, const char* name, const GetterType& getter,
                             const SetterType& setter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        CreateStaticProperty(m_l, getter, setter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);
    }

    template <typename GetterType>
    void DoDefStaticPropertyReadOnly(lua_State* l, const char* name, const GetterType& getter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        CreateStaticPropertyReadOnly(m_l, getter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);
    }

    template <typename SetterType>
    void DoDefStaticPropertyWriteOnly(lua_State* l, const char* name, const SetterType& setter) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        CreateStaticPropertyWriteOnly(m_l, setter);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);
    }

    void Init() {
        PushSelf();
        m_data = (LuaClassData*)lua_touserdata(m_l, -1);
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

        lua_pushvalue(m_l, -2); // upvalue 1: class itself
        lua_pushcclosure(m_l, luacpp_constructor<FuncArgType...>, 1);

        lua_setfield(m_l, -2, "__call");
        lua_pop(m_l, 2);
        return *this;
    }

    /* ------------------------------- member ----------------------------------- */

    /**
       member property. only lambda functions are supported currently.

       GetterType: (const T*) -> SupportedType
       SetterType: (T*, SupportedType) -> void
    */
    template <typename GetterType, typename SetterType>
    LuaClass& DefMember(const char* name, const GetterType& getter, const SetterType& setter) {
        typename LambdaFunctionTraits<GetterType>::std_function_type getter_func(getter);
        typename LambdaFunctionTraits<SetterType>::std_function_type setter_func(setter);
        DoDefMemberProperty(m_l, name, getter_func, setter_func);
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
        DoDefMemberFunction<decltype(f), FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** member function with const qualifier */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...) const) {
        DoDefMemberFunction<decltype(f), FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** std::function member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, const std::function<FuncRetType (FuncArgType...)>& f) {
        using FuncType = std::function<FuncRetType (FuncArgType...)>;
        DoDefCStyleMemberFunction<FuncType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** lambda member function */
    template <typename FuncType>
    LuaClass& DefMember(const char* name, const FuncType& f) {
        typename LambdaFunctionTraits<FuncType>::std_function_type func(f);
        DefMember(name, func);
        return *this;
    }

    /** c-style member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefMember(const char* name, FuncRetType (*f)(FuncArgType...)) {
        DoDefCStyleMemberFunction<decltype(f), FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** lua-style member function */
    LuaClass& DefMember(const char* name, int (*f)(lua_State*)) {
        PushInstanceMetatable();
        lua_pushcfunction(m_l, f);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
        return *this;
    }

    /* ------------------------------- static ----------------------------------- */

    /**
       static property.

       GetterType: () -> SupportedType
       SetterType: (SupportedType) -> void
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
        DoDefStaticFunction<FuncType, FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** C-style static member function */
    template <typename FuncRetType, typename... FuncArgType>
    LuaClass& DefStatic(const char* name, FuncRetType (*f)(FuncArgType...)) {
        DoDefStaticFunction<decltype(f), FuncArgType...>(m_l, name, f);
        return *this;
    }

    /** lambda static member function */
    template <typename FuncType>
    LuaClass& DefStatic(const char* name, const FuncType& f) {
        typename LambdaFunctionTraits<FuncType>::std_function_type func(f);
        DefStatic(name, func);
        return *this;
    }

    /** lua-style static member function */
    LuaClass& DefStatic(const char* name, int (*f)(lua_State*)) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        lua_pushcfunction(m_l, f);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);
        return *this;
    }

    /* -------------------------------------------------------------------------- */

    template <typename BaseType>
    LuaClass& AddBaseClass(const LuaClass<BaseType>& lclass) {
        PushParentsTable();
        auto len = lua_rawlen(m_l, -1);
        PushValue(m_l, lclass);
        lua_rawseti(m_l, -2, len + 1);
        lua_pop(m_l, 1);
        return *this;
    }

    template <typename... Argv>
    LuaUserData CreateUserData(Argv&&... argv) const {
        auto ud = lua_newuserdatauv(m_l, sizeof(T), 1);
        new (ud) T(std::forward<Argv>(argv)...);

        PushSelf();
        lua_setiuservalue(m_l, -2, 1);

        PushInstanceMetatable();
        lua_setmetatable(m_l, -2);

        LuaUserData ret(m_l, -1);
        lua_pop(m_l, 1);
        return ret;
    }

private:
    /** pointer to the userdata */
    LuaClassData* m_data;
};

}

#endif
