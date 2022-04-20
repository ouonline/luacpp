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
    /** a metatable including only __gc function for various objects(such as FuncWrapper) */
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

    void Init() {
        PushSelf();
        m_data = (LuaClassData*)lua_touserdata(m_l, -1);
        lua_pop(m_l, 1);
    }

    template <typename FuncType, typename... FuncArgType>
    void CreateGenericFunction(lua_State* l, int argoffset, FuncType&& f, const TypeHolder<FuncArgType...>&) {
        using WrapperType = FuncWrapper<FuncType>;

        // upvalue 1: argoffset
        lua_pushinteger(l, argoffset);

        // upvalue 2: wrapper
        auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
        new (wrapper) WrapperType(std::forward<FuncType>(f));

        // wrapper's destructor
        PushGcTable();
        lua_setmetatable(l, -2);

        lua_pushcclosure(l, luacpp_generic_function<FuncType, FuncArgType...>, 2);
    }

    /* ---------------------------- constructor --------------------------------- */

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

    /* --------------------- static member functions ----------------------------- */

    template <typename FuncType>
    void DoDefStaticFunction(lua_State* l, const char* name, FuncType&& f) {
        PushSelf();
        lua_getmetatable(l, -1);
        CreateGenericFunction<FuncType>(l, 1, std::forward<FuncType>(f),
                                        typename FunctionTraits<FuncType>::argument_type_holder());
        lua_setfield(l, -2, name);
        lua_pop(l, 2);
    }

    /** c-style functions, `std::function`s and lambda functions */
    template <typename FuncType>
    LuaClass& DoDefStatic(const char* name, FuncType&& f) {
        using ConvertedFuncType = typename If<std::is_pointer<FuncType>::value, FuncType,
                                              typename FunctionTraits<FuncType>::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        DoDefStaticFunction(m_l, name, std::forward<ConvertedFuncType>(func));
        return *this;
    }

    /** lua-style functions that can be used to implement variadic argument functions */
    LuaClass& DoDefStatic(const char* name, int (*f)(lua_State*)) {
        PushSelf();
        lua_getmetatable(m_l, -1);
        lua_pushcfunction(m_l, f);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);
        return *this;
    }

    /* ------------------------ member functions --------------------------------- */

    template <typename FuncType>
    void DoDefMemberFunction(lua_State* l, int argoffset, const char* name, FuncType&& f) {
        PushInstanceMetatable();
        CreateGenericFunction<FuncType>(l, argoffset, std::forward<FuncType>(f),
                                        typename FunctionTraits<FuncType>::argument_type_holder());
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    /** class member functions, c-style functions, `std::function`s and lambda functions */
    template <typename FuncType>
    LuaClass& DoDefMember(const char* name, FuncType&& f) {
        constexpr int argoffset = (std::is_member_function_pointer<FuncType>::value ? 1 : 0);
        using ConvertedFuncType =
            typename If<(std::is_member_function_pointer<FuncType>::value || std::is_pointer<FuncType>::value),
                        FuncType, typename FunctionTraits<FuncType>::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        DoDefMemberFunction(m_l, argoffset, name, std::forward<ConvertedFuncType>(func));
        return *this;
    }

    /** lua-style functions that can be used to implement variadic argument functions */
    LuaClass& DoDefMember(const char* name, int (*f)(lua_State*)) {
        PushInstanceMetatable();
        lua_pushcfunction(m_l, f);
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);
        return *this;
    }

    /* -------------------- member properties ------------------------------------- */

    template <typename GetterType>
    static int luacpp_member_property_getter(lua_State* l) {
        using WrapperType = FuncWrapper<GetterType>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, wrapper->f(ud));
        return 1;
    }

    template <typename SetterType>
    static int luacpp_member_property_setter(lua_State* l) {
        using WrapperType = FuncWrapper<SetterType>;
        auto ud = (T*)lua_touserdata(l, 1);
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        wrapper->f(ud, ValueConverter(l, 2));
        return 0;
    }

    template <typename GetterType, typename SetterType>
    void CreateMemberProperty(lua_State* l, GetterType&& getter, SetterType&& setter) {
        lua_createtable(l, 0, 2);

        if (getter) {
            using GetterWrapperType = FuncWrapper<GetterType>;
            auto wrapper = lua_newuserdatauv(l, sizeof(GetterWrapperType), 0);
            new (wrapper) GetterWrapperType(std::forward<GetterType>(getter));
            PushGcTable(); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_member_property_getter<GetterType>, 1);
            lua_setfield(l, -2, "getter");
        }

        if (setter) {
            using SetterWrapperType = FuncWrapper<SetterType>;
            auto wrapper = lua_newuserdatauv(l, sizeof(SetterWrapperType), 0);
            new (wrapper) SetterWrapperType(std::forward<SetterType>(setter));
            PushGcTable(); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_member_property_setter<SetterType>, 1);
            lua_setfield(l, -2, "setter");
        }
    }

    /* --------------------- static properties ------------------------------------ */

    template <typename GetterType>
    static int luacpp_static_property_getter(lua_State* l) {
        using WrapperType = FuncWrapper<GetterType>;
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        PushValue(l, wrapper->f());
        return 1;
    }

    template <typename SetterType>
    static int luacpp_static_property_setter(lua_State* l) {
        using WrapperType = FuncWrapper<SetterType>;
        auto wrapper = (WrapperType*)lua_touserdata(l, lua_upvalueindex(1));
        wrapper->f(ValueConverter(l, 2));
        return 0;
    }

    template <typename GetterType, typename SetterType>
    void CreateStaticProperty(lua_State* l, GetterType&& getter, SetterType&& setter) {
        lua_createtable(l, 0, 2);

        if (getter) {
            using GetterWrapperType = FuncWrapper<GetterType>;
            auto wrapper = lua_newuserdatauv(l, sizeof(GetterWrapperType), 0);
            new (wrapper) GetterWrapperType(std::forward<GetterType>(getter));
            PushGcTable(); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_static_property_getter<GetterType>, 1);
            lua_setfield(l, -2, "getter");
        }

        if (setter) {
            using SetterWrapperType = FuncWrapper<SetterType>;
            auto wrapper = lua_newuserdatauv(l, sizeof(SetterWrapperType), 0);
            new (wrapper) SetterWrapperType(std::forward<SetterType>(setter));
            PushGcTable(); // wrapper's destructor
            lua_setmetatable(l, -2);
            lua_pushcclosure(l, luacpp_static_property_setter<SetterType>, 1);
            lua_setfield(l, -2, "setter");
        }
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
       member property
         - GetterType: (const T*) -> PropertyType
         - SetterType: (T*, PropertyType) -> void
    */
    template <typename PropertyType, typename GetterType, typename SetterType>
    LuaClass& DefMember(const char* name, GetterType&& getter, SetterType&& setter) {
        std::function<PropertyType(const T*)> getter_func(std::forward<GetterType>(getter));
        std::function<void(T*, PropertyType)> setter_func(std::forward<SetterType>(setter));

        PushInstanceMetatable();
        CreateMemberProperty(m_l, std::move(getter_func), std::move(setter_func));
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 1);

        return *this;
    }

    template <typename FuncType>
    LuaClass& DefMember(const char* name, FuncType&& f) {
        DoDefMember(name, std::forward<FuncType>(f));
        return *this;
    }

    /* ------------------------------- static ----------------------------------- */

    /**
       static property.
         - GetterType: () -> PropertyType
         - SetterType: (PropertyType) -> void
    */
    template <typename PropertyType, typename GetterType, typename SetterType>
    LuaClass& DefStatic(const char* name, GetterType&& getter, SetterType&& setter) {
        std::function<PropertyType(void)> getter_func(std::forward<GetterType>(getter));
        std::function<void(PropertyType)> setter_func(std::forward<SetterType>(setter));

        PushSelf();
        lua_getmetatable(m_l, -1);
        CreateStaticProperty(m_l, std::move(getter_func), std::move(setter_func));
        lua_setfield(m_l, -2, name);
        lua_pop(m_l, 2);

        return *this;
    }

    template <typename FuncType>
    LuaClass& DefStatic(const char* name, FuncType&& f) {
        DoDefStatic(name, std::forward<FuncType>(f));
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

} // namespace luacpp

#endif
