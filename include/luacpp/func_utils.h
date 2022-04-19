#ifndef __LUA_CPP_FUNC_UTILS_H__
#define __LUA_CPP_FUNC_UTILS_H__

extern "C" {
#include "lua.h"
}

#include "lua_string_ref.h"
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

    operator bool() const {
        return lua_toboolean(m_l, m_index);
    }

    operator int8_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint8_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator int16_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint16_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator int32_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint32_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator int64_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator uint64_t() const {
        return lua_tointeger(m_l, m_index);
    }
    operator float() const {
        return lua_tonumber(m_l, m_index);
    }
    operator double() const {
        return lua_tonumber(m_l, m_index);
    }

    template <typename T>
    operator T*() const {
        return (T*)lua_touserdata(m_l, m_index);
    }

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
    operator LuaUserData() const;

private:
    lua_State* m_l;
    int m_index;
};

/* -------------------------------------------------------------------------- */

static inline void PushValue(lua_State* l, bool arg) {
    lua_pushboolean(l, arg);
}

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

static inline void PushValue(lua_State* l, const char* arg) {
    lua_pushstring(l, arg);
}
static inline void PushValue(lua_State* l, const LuaStringRef& arg) {
    lua_pushlstring(l, (const char*)arg.base, arg.size);
}

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

template <typename... Ts>
struct TypeHolder final {};

template <typename T>
struct FunctionTraits;

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType(FuncArgType...)> {
    using return_type = FuncRetType;
    using argument_type_holder = TypeHolder<FuncArgType...>;
    using std_function_type = std::function<FuncRetType(FuncArgType...)>;
};

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<std::function<FuncRetType(FuncArgType...)>> final
    : public FunctionTraits<FuncRetType(FuncArgType...)> {};

template <typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (*)(FuncArgType...)> final
    : public FunctionTraits<FuncRetType(FuncArgType...)> {};

template <typename ClassType, typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (ClassType::*)(FuncArgType...)> final
    : public FunctionTraits<FuncRetType(FuncArgType...)> {};

template <typename ClassType, typename FuncRetType, typename... FuncArgType>
struct FunctionTraits<FuncRetType (ClassType::*)(FuncArgType...) const>
    : public FunctionTraits<FuncRetType(FuncArgType...)> {};

// for lambda functions that will be deduced to `FuncRetType (ClassType::*)(FuncArgType...) const`
template <typename ClassType>
struct FunctionTraits final : public FunctionTraits<decltype(&ClassType::operator())> {};

/* -------------------------------------------------------------------------- */

template <uint32_t N>
struct FunctionCaller final {
    template <typename FuncType, typename... Argv>
    static int Execute(FuncType&& f, lua_State* l, int argoffset, Argv&&... argv) {
        return FunctionCaller<N - 1>::Execute(std::forward<FuncType>(f), l, argoffset, ValueConverter(l, N + argoffset),
                                              std::forward<Argv>(argv)...);
    }
};

template <>
struct FunctionCaller<0> final {
    template <typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Execute(FuncRetType (*f)(FuncArgType...), lua_State* l, int, Argv&&... argv) {
        PushValue(l, f(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename... FuncArgType, typename... Argv>
    static int Execute(void (*f)(FuncArgType...), lua_State*, int, Argv&&... argv) {
        f(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Execute(const std::function<FuncRetType(FuncArgType...)>& f, lua_State* l, int, Argv&&... argv) {
        PushValue(l, f(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename... FuncArgType, typename... Argv>
    static int Execute(const std::function<void(FuncArgType...)>& f, lua_State*, int, Argv&&... argv) {
        f(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Execute(FuncRetType (T::*f)(FuncArgType...), lua_State* l, int, Argv&&... argv) {
        auto obj = (T*)lua_touserdata(l, 1);
        PushValue(l, (obj->*f)(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename T, typename... FuncArgType, typename... Argv>
    static int Execute(void (T::*f)(FuncArgType...), lua_State* l, int, Argv&&... argv) {
        auto obj = (T*)lua_touserdata(l, 1);
        (obj->*f)(std::forward<Argv>(argv)...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
    static int Execute(FuncRetType (T::*f)(FuncArgType...) const, lua_State* l, int, Argv&&... argv) {
        auto obj = (T*)lua_touserdata(l, 1);
        PushValue(l, (obj->*f)(std::forward<Argv>(argv)...));
        return 1;
    }

    template <typename T, typename... FuncArgType, typename... Argv>
    static int Execute(void (T::*f)(FuncArgType...) const, lua_State* l, int, Argv&&... argv) {
        auto obj = (T*)lua_touserdata(l, 1);
        (obj->*f)(std::forward<Argv>(argv)...);
        return 0;
    }
};

struct DestructorObject {
    virtual ~DestructorObject() {}
};

template <typename FuncType>
struct FuncWrapper final : public DestructorObject {
    FuncWrapper(const FuncType& v) : f(v) {}
    FuncWrapper(FuncType&& v) : f(std::move(v)) {}
    FuncType f;
};

template <typename T>
static int luacpp_generic_destructor(lua_State* l) {
    auto ud = (T*)lua_touserdata(l, 1);
    ud->~T();
    return 0;
}

/** FuncType may be a c-style function or a std::function or a callable object */
template <typename FuncType, typename... FuncArgType>
static int luacpp_generic_function(lua_State* l) {
    auto argoffset = lua_tointeger(l, lua_upvalueindex(1));
    auto wrapper = (FuncWrapper<FuncType>*)lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<sizeof...(FuncArgType)>::Execute(wrapper->f, l, argoffset);
}

} // namespace luacpp

#endif
