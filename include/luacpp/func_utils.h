#ifndef __LUA_CPP_FUNC_UTILS_H__
#define __LUA_CPP_FUNC_UTILS_H__

extern "C" {
#include "lua.h"
}

#include "lua_52_53.h"
#include "lua_string_ref.h"
#include <stdint.h>
#include <functional>

namespace luacpp {

class LuaRefObject;
class LuaObject;
class LuaTable;
class LuaFunction;

template <typename T>
class LuaClass;

/* -------------------------------------------------------------------------- */

class ValueConverter final {
private:
    template <typename T>
    struct IntegerConverter final {
        T Convert(lua_State* l, int idx) const {
            return lua_tointeger(l, idx);
        }
    };

    template <typename T>
    struct FloatConverter final {
        T Convert(lua_State* l, int idx) const {
            return lua_tonumber(l, idx);
        }
    };

    template <typename T>
    struct PointerConverter final {
        T Convert(lua_State* l, int idx) const {
            return (T)lua_touserdata(l, idx);
        }
    };

public:
    ValueConverter(lua_State* l, int index) : m_l(l), m_index(index) {}

    operator const char*() const {
        return lua_tostring(m_l, m_index);
    }

    operator LuaStringRef() const {
        size_t len = 0;
        auto addr = lua_tolstring(m_l, m_index, &len);
        return LuaStringRef(addr, len);
    }

    operator LuaObject() const;
    operator LuaTable() const;
    operator LuaFunction() const;

    template <typename T>
    operator T() const {
        typename std::conditional<
            std::is_integral<T>::value, IntegerConverter<T>,
            typename std::conditional<std::is_floating_point<T>::value, FloatConverter<T>,
                                      typename std::conditional<std::is_pointer<T>::value, PointerConverter<T>,
                                                                void>::type>::type>::type converter;
        return converter.Convert(m_l, m_index);
    }

private:
    lua_State* m_l;
    int m_index;
};

/* -------------------------------------------------------------------------- */

inline void PushValue(lua_State* l, const char* arg) {
    lua_pushstring(l, arg);
}

inline void PushValue(lua_State* l, const LuaStringRef& arg) {
    lua_pushlstring(l, (const char*)arg.base, arg.size);
}

void PushValue(lua_State* l, const LuaRefObject&);
void PushValue(lua_State* l, const LuaObject&);
void PushValue(lua_State* l, const LuaTable&);
void PushValue(lua_State* l, const LuaFunction&);

template <typename T>
void PushValue(lua_State* l, const LuaClass<T>& cls) {
    lua_rawgeti(l, LUA_REGISTRYINDEX, cls.GetRefIndex());
}

template <typename T>
struct IntegerPusher final {
    IntegerPusher(lua_State* l, T value) {
        lua_pushinteger(l, value);
    }
};

template <typename T>
struct FloatPusher final {
    FloatPusher(lua_State* l, T value) {
        lua_pushnumber(l, value);
    }
};

template <typename T>
struct PointerPusher final {
    PointerPusher(lua_State* l, T value) {
        lua_pushlightuserdata(l, (void*)value);
    }
};

template <typename T>
void PushValue(lua_State* l, T arg) {
    typename std::conditional<
        std::is_integral<T>::value, IntegerPusher<T>,
        typename std::conditional<std::is_floating_point<T>::value, FloatPusher<T>,
                                  typename std::conditional<std::is_pointer<T>::value, PointerPusher<T>,
                                                            void>::type>::type>::type pusher(l, arg);
}

inline void PushValues(lua_State*) {}

template <typename First, typename... Rest>
void PushValues(lua_State* l, First&& first, Rest&&... rest) {
    PushValue(l, std::forward<First>(first));
    PushValues(l, std::forward<Rest>(rest)...);
}

/* -------------------------------------------------------------------------- */

template <typename... Ts>
struct TypeHolder final {};

template <typename T>
struct FunctionTraits;

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType(FuncArgType...)> {
    using return_type = FuncRetType;
    using argument_type_holder = TypeHolder<FuncArgType...>;
    using std_function_type = std::function<FuncRetType(FuncArgType...)>;
    static constexpr uint32_t argc = sizeof...(FuncArgType);
};

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (*)(FuncArgType...)> final : public FunctionTraits<FuncRetType(FuncArgType...)> {};

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<std::function<FuncRetType(FuncArgType...)>> final
    : public FunctionTraits<FuncRetType(FuncArgType...)> {};

template <typename ClassType, typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (ClassType::*)(FuncArgType...)> final
    : public FunctionTraits<FuncRetType(FuncArgType...)> {
    using class_type = ClassType;
};

template <typename ClassType, typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (ClassType::*)(FuncArgType...) const>
    : public FunctionTraits<FuncRetType(FuncArgType...)> {
    using class_type = ClassType;
};

// for lambda functions that will be deduced to `FuncRetType (ClassType::*)(FuncArgType...) const`
template <typename ClassType>
struct FunctionTraits final : public FunctionTraits<decltype(&ClassType::operator())> {};

/* -------------------------------------------------------------------------- */

template <typename FuncType, typename... Argv>
struct FuncWithReturnValue final {
    FuncWithReturnValue(lua_State* l, const FuncType& f, Argv&&... argv) {
        PushValue(l, f(std::forward<Argv>(argv)...));
    }
    static constexpr int returned_value_num = 1;
};

template <typename FuncType, typename... Argv>
struct FuncWithoutReturnValue final {
    FuncWithoutReturnValue(lua_State*, const FuncType& f, Argv&&... argv) {
        f(std::forward<Argv>(argv)...);
    }
    static constexpr int returned_value_num = 0;
};

template <typename FuncType, typename... Argv>
struct ClassMemberFuncWithReturnValue final {
    ClassMemberFuncWithReturnValue(lua_State* l, const FuncType& f, Argv&&... argv) {
        auto obj = (typename FunctionTraits<FuncType>::class_type*)lua_touserdata(l, 1);
        PushValue(l, (obj->*f)(std::forward<Argv>(argv)...));
    }
    static constexpr int returned_value_num = 1;
};

template <typename FuncType, typename... Argv>
struct ClassMemberFuncWithoutReturnValue final {
    ClassMemberFuncWithoutReturnValue(lua_State* l, const FuncType& f, Argv&&... argv) {
        auto obj = (typename FunctionTraits<FuncType>::class_type*)lua_touserdata(l, 1);
        (obj->*f)(std::forward<Argv>(argv)...);
    }
    static constexpr int returned_value_num = 0;
};

template <uint32_t N>
struct FunctionCaller final {
    template <typename FuncType, typename... Argv>
    static int Execute(const FuncType& f, lua_State* l, int argoffset, Argv&&... argv) {
        return FunctionCaller<N - 1>::Execute(f, l, argoffset, ValueConverter(l, N + argoffset),
                                              std::forward<Argv>(argv)...);
    }
};

template <>
struct FunctionCaller<0> final {
    template <typename FuncType, typename... Argv>
    static int Execute(const FuncType& f, lua_State* l, int, Argv&&... argv) {
        constexpr bool is_void_ret = std::is_void<typename FunctionTraits<FuncType>::return_type>::value;
        typename std::conditional<
            std::is_member_function_pointer<FuncType>::value,
            typename std::conditional<is_void_ret, ClassMemberFuncWithoutReturnValue<FuncType, Argv...>,
                                      ClassMemberFuncWithReturnValue<FuncType, Argv...>>::type,
            typename std::conditional<is_void_ret, FuncWithoutReturnValue<FuncType, Argv...>,
                                      FuncWithReturnValue<FuncType, Argv...>>::type>::type
            func_executor(l, f, std::forward<Argv>(argv)...);
        return func_executor.returned_value_num;
    }
};

struct DestructorObject {
    virtual ~DestructorObject() {}
};

/*
  All `FuncWrapper` instances share the same metatable.
  According to the c++ standard, converting a void*, which is converted from a pointer to derived class, to its base
  class pointer, is not save. But `DestructorObject` is an empty class so that the address of `DestructorObject` and
  `FuncWrapper` are the same in our case.
*/
template <typename FuncType>
struct FuncWrapper final : public DestructorObject {
    FuncWrapper(const FuncType& v) : f(v) {}
    FuncWrapper(FuncType&& v) : f(std::move(v)) {}
    FuncType f;
};

template <typename T>
int luacpp_generic_destructor(lua_State* l) {
    auto ud = (T*)lua_touserdata(l, 1);
    ud->~T();
    return 0;
}

/** FuncType may be a c-style function or a std::function or a callable object */
template <typename FuncType>
int luacpp_generic_function(lua_State* l) {
    auto argoffset = lua_tointeger(l, lua_upvalueindex(1));
    auto wrapper = (FuncWrapper<FuncType>*)lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<FunctionTraits<FuncType>::argc>::Execute(wrapper->f, l, argoffset);
}

// pushes an instance of `luacpp_generic_function`
template <typename FuncType>
void CreateGenericFunction(lua_State* l, int gc_table_ref, int argoffset, FuncType&& f) {
    using WrapperType = FuncWrapper<FuncType>;

    // upvalue 1: argoffset
    lua_pushinteger(l, argoffset);

    // upvalue 2: wrapper
    auto wrapper = lua_newuserdatauv(l, sizeof(WrapperType), 0);
    new (wrapper) WrapperType(std::forward<FuncType>(f));

    // wrapper's destructor
    lua_rawgeti(l, LUA_REGISTRYINDEX, gc_table_ref);
    lua_setmetatable(l, -2);

    lua_pushcclosure(l, luacpp_generic_function<FuncType>, 2);
}

template <typename T>
void BuiltInTypeAssert() {
    static_assert(std::is_same<T, LuaRefObject>::value || std::is_same<T, LuaObject>::value ||
                  std::is_same<T, LuaTable>::value || std::is_same<T, LuaFunction>::value ||
                  std::is_same<T, LuaStringRef>::value, "is not a luacpp builtin type");
}

} // namespace luacpp

#endif
