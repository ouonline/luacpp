#ifndef __LUA_CPP_FUNC_UTILS_H__
#define __LUA_CPP_FUNC_UTILS_H__

extern "C" {
#include "lua.h"
}

#include "lua_buffer_ref.h"
#include <functional>

namespace luacpp {

class LuaObject;
class LuaTable;
class LuaFunction;
class LuaUserData;

/* -------------------------------------------------------------------------- */

class ValueConverter final {
public:
    ValueConverter(lua_State* l, int index) : m_l(l), m_index(index) {}

    // LUA_TBOOLEAN
    operator bool () const {
        return lua_toboolean(m_l, m_index);
    }

    // LUA_TNUMBER
    operator int8_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint8_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator int16_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint16_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator int32_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint32_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator int64_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint64_t () const {
        return lua_tointeger(m_l, m_index);
    }
    operator float () const {
        return lua_tonumber(m_l, m_index);
    }
    operator double () const {
        return lua_tonumber(m_l, m_index);
    }

    // LUA_TUSERDATA and LUA_TLIGHTUSERDATA
    template <typename T>
    operator T* () const {
        return (T*)lua_touserdata(m_l, m_index);
    }

    // LUA_TSTRING
    operator const char* () const {
        return lua_tostring(m_l, m_index);
    }
    operator LuaBufferRef () const {
        size_t len = 0;
        auto addr = lua_tolstring(m_l, m_index, &len);
        return LuaBufferRef(addr, len);
    }

    operator LuaObject () const;
    operator LuaTable () const;
    operator LuaFunction () const;
    operator LuaUserData () const;

private:
    lua_State* m_l;
    int m_index;
};

/* -------------------------------------------------------------------------- */

// LUA_TBOOLEAN
static inline void PushValue(lua_State* l, bool arg) {
    lua_pushboolean(l, arg);
}

// LUA_TNUMBER
static inline void PushValue(lua_State* l, int8_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, uint8_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, int16_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, uint16_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, int32_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, uint32_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, int64_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, uint64_t arg) {
    lua_pushinteger(l, arg);
}
static inline void PushValue(lua_State* l, float arg) {
    lua_pushnumber(l, arg);
}
static inline void PushValue(lua_State* l, double arg) {
    lua_pushnumber(l, arg);
}

// LUA_TSTRING
static inline void PushValue(lua_State* l, const char* arg) {
    lua_pushstring(l, arg);
}
static inline void PushValue(lua_State* l, const LuaBufferRef& arg) {
    lua_pushlstring(l, (const char*)arg.base, arg.size);
}

// LUA_TLIGHTUSERDATA
template <typename T>
static inline void PushValue(lua_State* l, T* arg) {
    lua_pushlightuserdata(l, (void*)arg);
}

void PushValue(lua_State* l, const LuaObject& obj);

static inline void PushValues(lua_State*) {}

template <typename First, typename... Rest>
static void PushValues(lua_State* l, First&& first, Rest&&... rest) {
    PushValue(l, std::forward<First>(first));
    PushValues(l, std::forward<Rest>(rest)...);
}

/* -------------------------------------------------------------------------- */

// function traits for extracting typeinfo from lambda functions

template <typename FuncType>
struct FunctionTraits final : public FunctionTraits<decltype(&FuncType::operator())> {};

template <typename ClassType, typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType(ClassType::*)(FuncArgType...) const> {
    using ret_type = FuncRetType;
    using arg_tuple = std::tuple<FuncArgType...>;
    static constexpr auto argc = sizeof...(FuncArgType);
};

template <typename FuncType, std::size_t... Is, typename TraitsType>
static auto LambdaToFuncImpl(const FuncType& f, const std::index_sequence<Is...>&, const TraitsType&) {
    return std::function<typename TraitsType::ret_type (std::tuple_element_t<Is, typename TraitsType::arg_tuple>...)>(f);
}

template <typename FuncType>
static auto Lambda2Func(const FuncType& f) {
    using TraitsType = FunctionTraits<FuncType>;
    return LambdaToFuncImpl(f, std::make_index_sequence<TraitsType::argc>(), TraitsType());
}

/* -------------------------------------------------------------------------- */

template <uint32_t N>
struct FunctionCaller final {
    template <typename FuncType, typename... Argv>
    static int Exec(const FuncType& f, lua_State* l, int argoffset, Argv&&... argv) {
        return FunctionCaller<N - 1>::Exec(f, l, argoffset,
                                           ValueConverter(l, N + argoffset),
                                           std::forward<Argv>(argv)...);
    }

    template <typename T, typename FuncType, typename... Argv>
    static int Exec(T* obj, const FuncType& f, lua_State* l, int argoffset, Argv&&... argv) {
        return FunctionCaller<N - 1>::Exec(obj, f, l, argoffset,
                                           ValueConverter(l, N + argoffset),
                                           std::forward<Argv>(argv)...);
    }
};

template <>
struct FunctionCaller<0> final {
    template <typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Exec(FuncRetType (*f)(FuncArgType...), lua_State* l, int, Argv&&... argv) {
        PushValue(l, f(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename... FuncArgType, typename... Argv>
    static int Exec(void (*f)(FuncArgType...), lua_State*, int, Argv&&... argv) {
        f(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Exec(const std::function<FuncRetType (FuncArgType...)>& f,
                    lua_State* l, int, Argv&&... argv) {
        PushValue(l, f(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename... FuncArgType, typename... Argv>
    static int Exec(const std::function<void (FuncArgType...)>& f,
                    lua_State*, int, Argv&&... argv) {
        f(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Exec(T* obj, FuncRetType (T::*f)(FuncArgType...), lua_State* l, int,
                    Argv&&... argv) {
        PushValue(l, (obj->*f)(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename T, typename... FuncArgType, typename... Argv>
    static int Exec(T* obj, void (T::*f)(FuncArgType...), lua_State*, int,
                    Argv&&... argv) {
        (obj->*f)(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Exec(T* obj, FuncRetType (T::*f)(FuncArgType...) const, lua_State* l, int,
                    Argv&&... argv) {
        PushValue(l, (obj->*f)(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename T, typename... FuncArgType, typename... Argv>
    static int Exec(T* obj, void (T::*f)(FuncArgType...) const, lua_State*, int,
                    Argv&&... argv) {
        (obj->*f)(std::forward<Argv>(argv)...);
        return 0;
    }
};

struct DestructorObject {
    virtual ~DestructorObject() {}
};

template <typename ValueType>
struct ValueWrapper final : public DestructorObject {
    ValueWrapper(const ValueType& v) : value(v) {}
    ValueType value;
};

template <typename T>
static int GenericDestructor(lua_State* l) {
    auto ud = (T*)lua_touserdata(l, 1);
    ud->~T();
    return 0;
}

/** FuncType may be a C-style function or a std::function or a callable object */
template <typename FuncType, typename FuncRetType, typename... FuncArgType>
static int GenericFunction(lua_State* l) {
    auto argoffset = lua_tointeger(l, lua_upvalueindex(1));
    auto wrapper = (ValueWrapper<FuncType>*)lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<sizeof...(FuncArgType)>::Exec(wrapper->value, l, argoffset);
}

}

#endif
