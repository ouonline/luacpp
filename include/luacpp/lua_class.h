#ifndef __LUA_CPP_LUA_CLASS_H__
#define __LUA_CPP_LUA_CLASS_H__

#include "lua_object.h"
#include "lua_52_53.h"
#include "func_utils.h"
#include <stdint.h>

namespace luacpp {

static constexpr uint32_t CLASS_PARENT_TABLE_IDX = 1;
static constexpr uint32_t CLASS_INSTANCE_METATABLE_IDX = 2;

static constexpr uint32_t MEMBER_GETTER_IDX = 1;
static constexpr uint32_t MEMBER_SETTER_IDX = 2;

struct LuaClassData final {
    /** a metatable including only __gc function for various objects(such as FuncWrapper) */
    int gc_table_ref = LUA_REFNIL;
};

template <typename T>
class LuaClass final : public LuaRefObject {
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

    void Init() {
        PushSelf();
        m_data = (LuaClassData*)lua_touserdata(m_l, -1);
        lua_pop(m_l, 1);
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

        constexpr uint32_t argc = sizeof...(FuncArgType);
        FunctionCaller<argc + 1>::Execute(InitInstance<FuncArgType...>, l, 0);

        // pops arguments so that the new instance is on the top of lua_State
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
        CreateGenericFunction(l, m_data->gc_table_ref, 1, std::forward<FuncType>(f));
        lua_setfield(l, -2, name);
        lua_pop(l, 2);
    }

    /** c-style functions, `std::function`s and lambda functions */
    template <typename FuncType>
    LuaClass& DoDefStatic(const char* name, FuncType&& f) {
        using RealFuncType = typename std::remove_reference<FuncType>::type;
        using ConvertedFuncType =
            typename std::conditional<std::is_pointer<RealFuncType>::value, RealFuncType,
                                      typename FunctionTraits<RealFuncType>::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        DoDefStaticFunction(m_l, name, std::move(func));
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
        CreateGenericFunction(l, m_data->gc_table_ref, argoffset, std::forward<FuncType>(f));
        lua_setfield(l, -2, name);
        lua_pop(l, 1);
    }

    /** class member functions, c-style functions, `std::function`s and lambda functions */
    template <typename FuncType>
    LuaClass& DoDefMember(const char* name, FuncType&& f) {
        constexpr int argoffset = (std::is_member_function_pointer<FuncType>::value ? 1 : 0);
        using ConvertedFuncType =
            typename std::conditional<(std::is_member_function_pointer<FuncType>::value ||
                                       std::is_pointer<FuncType>::value),
                                      FuncType, typename FunctionTraits<FuncType>::std_function_type>::type;
        ConvertedFuncType func(std::forward<FuncType>(f));
        DoDefMemberFunction(m_l, argoffset, name, std::move(func));
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

    template <typename GetterType, typename SetterType>
    void CreateMemberProperty(lua_State* l, GetterType&& getter, SetterType&& setter) {
        lua_createtable(l, 2, 0);
        if (getter) {
            CreateGenericFunction(l, m_data->gc_table_ref, 0, std::forward<GetterType>(getter));
            lua_rawseti(l, -2, MEMBER_GETTER_IDX);
        }
        if (setter) {
            CreateGenericFunction(l, m_data->gc_table_ref, 0, std::forward<SetterType>(setter));
            lua_rawseti(l, -2, MEMBER_SETTER_IDX);
        }
    }

    /* --------------------- static properties ------------------------------------ */

    template <typename GetterType, typename SetterType>
    void CreateStaticProperty(lua_State* l, GetterType&& getter, SetterType&& setter) {
        lua_createtable(l, 2, 0);
        if (getter) {
            CreateGenericFunction(l, m_data->gc_table_ref, 1, std::forward<GetterType>(getter));
            lua_rawseti(l, -2, MEMBER_GETTER_IDX);
        }
        if (setter) {
            CreateGenericFunction(l, m_data->gc_table_ref, 1, std::forward<SetterType>(setter));
            lua_rawseti(l, -2, MEMBER_SETTER_IDX);
        }
    }

    /* -------------------------------------------------------------------------- */

public:
    LuaClass(lua_State* l, int index) : LuaRefObject(l, index) {
        Init();
    }
    LuaClass(LuaObject&& rhs) : LuaRefObject(std::move(rhs)) {
        Init();
    }
    LuaClass(const LuaObject& rhs) : LuaRefObject(rhs) {
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
        return DoDefMember(name, std::forward<FuncType>(f));
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
        return DoDefStatic(name, std::forward<FuncType>(f));
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
    LuaObject CreateInstance(Argv&&... argv) const {
        auto ud = lua_newuserdatauv(m_l, sizeof(T), 1);
        new (ud) T(std::forward<Argv>(argv)...);

        PushSelf();
        lua_setiuservalue(m_l, -2, 1);

        PushInstanceMetatable();
        lua_setmetatable(m_l, -2);

        LuaObject ret(m_l, -1);
        lua_pop(m_l, 1);
        return ret;
    }

private:
    /** pointer to the userdata */
    LuaClassData* m_data;
};

} // namespace luacpp

#endif
