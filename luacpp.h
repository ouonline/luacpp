#ifndef __LUACPP_H__
#define __LUACPP_H__

#include <memory>
#include <string>
#include <functional>
#include <stdexcept>

#include "src/lua.hpp"

namespace luacpp {

#define METATABLENAME(T) (std::string(typeid(T).name()).append(".o_o"))

class LuaState;

class LuaRefObject {
public:
    int type() const { return m_type; }
    const char* typestr() const { return lua_typename(m_l.get(), m_type); }

protected:
    LuaRefObject(const std::shared_ptr<lua_State>& l, int index);
    LuaRefObject(const LuaRefObject& lobj);
    virtual ~LuaRefObject();

    void assign(const LuaRefObject& lobj);

    void pushself() const;
    bool pushobject(const LuaRefObject& lobj);

protected:
    std::shared_ptr<lua_State> m_l;

private:
    friend class LuaState;
    size_t m_index;
    int m_type;
};

class LuaTable;
class LuaFunction;
class LuaUserdata;

class LuaObject : public LuaRefObject {
public:
    LuaObject(const LuaObject& lobj) : LuaRefObject(lobj) {}

    bool isnil() const { return type() == LUA_TNIL; }
    bool isbool() const { return type() == LUA_TBOOLEAN; }
    bool isnumber() const { return type() == LUA_TNUMBER; }
    bool isstring() const { return type() == LUA_TSTRING; }
    bool istable() const { return type() == LUA_TTABLE; }
    bool isfunction() const { return type() == LUA_TFUNCTION; }
    bool isuserdata() const { return type() == LUA_TUSERDATA; }
    bool isthread() const { return type() == LUA_TTHREAD; }
    bool islightuserdata() const { return type() == LUA_TLIGHTUSERDATA; }

    bool tobool() const;
    std::string tostring() const;
    lua_Number tonumber() const;
    LuaTable totable() const;
    LuaFunction tofunction() const;
    LuaUserdata touserdata() const;

    LuaObject& operator=(const LuaObject& lobj) {
        assign(lobj);
        return *this;
    }

private:
    LuaObject(const std::shared_ptr<lua_State>& l, int index)
        : LuaRefObject(l, index) {}

    friend class LuaState;
    friend class LuaTable;
    friend class LuaFunction;
};

class LuaTable : public LuaRefObject {
public:
    LuaTable(const LuaTable& ltable)
        : LuaRefObject(ltable) {}

    LuaObject get(int index) const;
    LuaObject get(const char* name) const;

    bool set(int index, const char* str);
    bool set(int index, const char* str, size_t len);
    bool set(int index, lua_Number);
    // fails only when LuaObject is not in the same LuaState
    bool set(int index, const LuaObject& lobj);
    bool set(int index, const LuaTable& ltable);
    bool set(int index, const LuaFunction& lfunc);
    bool set(int index, const LuaUserdata& lud);

    bool set(const char* name, const char* str);
    bool set(const char* name, const char* str, size_t len);
    bool set(const char* name, lua_Number);
    // fails only when LuaObject is not in the same LuaState
    bool set(const char* name, const LuaObject& lobj);
    bool set(const char* name, const LuaTable& ltable);
    bool set(const char* name, const LuaFunction& lfunc);
    bool set(const char* name, const LuaUserdata& lud);

    bool foreach(const std::function<bool (const LuaObject& key,
                                           const LuaObject& value)>& func) const;

    LuaTable& operator=(const LuaTable& ltable) {
        assign(ltable);
        return *this;
    }

protected:
    LuaTable(const std::shared_ptr<lua_State>& l, size_t index)
        : LuaRefObject(l, index) {}

private:
    bool setobject(int index, const LuaRefObject& lobj);
    bool setobject(const char* name, const LuaRefObject& lobj);

    friend class LuaState;
    friend class LuaObject;
    friend class LuaFunction;
};

class LuaFunction : public LuaRefObject {
public:
    LuaFunction(const LuaFunction& lfunc)
        : LuaRefObject(lfunc) {}

    template <typename ... Argv>
    bool exec(std::string* errstr = nullptr,
              const std::function<bool (int nresults)>& before_proc = nullptr,
              const std::function<bool (int i, const LuaObject&)>& proc = nullptr,
              const Argv&... argv) {
        pushself();

        int argc = 0;
        if (!pusharg(&argc, errstr, std::forward<const Argv&>(argv)...)) {
            lua_pop(m_l.get(), argc + 1 /* self */);
            return false;
        }

        return invoke(sizeof...(Argv), errstr, before_proc, proc);
    }

    LuaFunction& operator=(const LuaFunction& lfunc) {
        assign(lfunc);
        return *this;
    }

private:
    LuaFunction(const std::shared_ptr<lua_State>& l, size_t index)
        : LuaRefObject(l, index) {}

    bool pusharg(const char* str);
    bool pusharg(lua_Number);
    bool pusharg(const LuaObject& lobj);
    bool pusharg(const LuaTable& ltable);
    bool pusharg(const LuaFunction& lfunc);
    bool pusharg(const LuaUserdata& lud);

    bool pusharg(int*, std::string*) { return true; }

    template <typename First, typename ... Rest>
    bool pusharg(int* argc, std::string* errstr,
                 const First& first, const Rest&... rest) {
        if (!pusharg(first)) {
            if (errstr) {
                *errstr = "argv are not in the same LuaState";
            }
            return false;
        }

        ++(*argc);
        return pusharg(argc, errstr, std::forward<const Rest&>(rest)...);
    }

    bool invoke(int argc, std::string* errstr,
                const std::function<bool (int)>& before_proc,
                const std::function<bool (int, const LuaObject&)>& proc);

    friend class LuaState;
    friend class LuaObject;
    friend class LuaTable;
};

template <uint32_t N>
class FunctionCaller {
public:
    template <typename FuncType, typename ... Argv>
    static int exec(FuncType f, lua_State* l, int argoffset,
                    const Argv&... argv) {
        return FunctionCaller<N - 1>::exec(f, l, argoffset,
                                           FuncArg(l, N + argoffset),
                                           argv...);
    }

    template <typename T, typename FuncType, typename ... Argv>
    static int exec(T* obj, FuncType f, lua_State* l, int argoffset,
                    const Argv&... argv) {
        return FunctionCaller<N - 1>::exec(obj, f, l, argoffset,
                                           FuncArg(l, N + argoffset),
                                           argv...);
    }

private:
    class FuncArg {
    public:
        FuncArg(lua_State* l, int index)
            : m_l(l), m_index(index) {}

        operator lua_Number () const { return lua_tonumber(m_l, m_index); }
        operator const char* () const { return lua_tostring(m_l, m_index); }

        template <typename T>
        operator T* () const {
            T** ud = (T**)lua_touserdata(m_l, m_index);
            return *ud;
        }

    private:
        lua_State* m_l;
        int m_index;
    };
};

template <>
class FunctionCaller<0> {
public:
    template <typename FuncRetType, typename ... FuncArgType, typename ... Argv>
    static int exec(FuncRetType (*f)(FuncArgType...), lua_State* l, int,
                    const Argv&... argv) {
        pushres(l, f(argv...));
        return 1;
    }

    template <typename ... FuncArgType, typename ... Argv>
    static int exec(void (*f)(FuncArgType...), lua_State*, int,
                    const Argv&... argv) {
        f(argv...);
        return 0;
    }

    template <typename FuncRetType, typename ... FuncArgType, typename ... Argv>
    static int exec(const std::function<FuncRetType (FuncArgType...)>& f,
                    lua_State* l, int, const Argv&... argv) {
        pushres(l, f(argv...));
        return 1;
    }

    template <typename ... FuncArgType, typename ... Argv>
    static int exec(const std::function<void (FuncArgType...)>& f,
                    lua_State*, int, const Argv&... argv) {
        f(argv...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename ... FuncArgType, typename ... Argv>
    static int exec(T* obj, FuncRetType (T::*f)(FuncArgType...), lua_State* l, int,
                    const Argv&... argv) {
        pushres(l, (obj->*f)(argv...));
        return 1;
    }

    template <typename T, typename ... FuncArgType, typename ... Argv>
    static int exec(T* obj, void (T::*f)(FuncArgType...), lua_State*, int,
                    const Argv&... argv) {
        (obj->*f)(argv...);
        return 0;
    }

    template <typename T, typename FuncRetType, typename ... FuncArgType, typename ... Argv>
    static int exec(T* obj, FuncRetType (T::*f)(FuncArgType...) const, lua_State* l, int,
                    const Argv&... argv) {
        pushres(l, (obj->*f)(argv...));
        return 1;
    }

    template <typename T, typename ... FuncArgType, typename ... Argv>
    static int exec(T* obj, void (T::*f)(FuncArgType...) const, lua_State*, int,
                    const Argv&... argv) {
        (obj->*f)(argv...);
        return 0;
    }

private:
    static inline void pushres(lua_State* l, lua_Number n) {
        lua_pushnumber(l, n);
    }

    template <typename T>
    static inline void pushres(lua_State* l, T* obj) {
        static const std::string metatable(METATABLENAME(T));

        T** ud = (T**)lua_newuserdata(l, sizeof(T*));
        *ud = obj;
        luaL_setmetatable(l, metatable.c_str());
    }
};

template <typename FuncRetType, typename ... FuncArgType>
static int l_function(lua_State* l) {
    typedef FuncRetType (*func_t)(FuncArgType...);

    int argoffset = lua_tonumber(l, lua_upvalueindex(1));
    auto func = (func_t)lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<sizeof...(FuncArgType)>::exec(func, l, argoffset);
}

template <typename FuncRetType, typename ... FuncArgType>
static int l_std_function(lua_State* l) {
    int argoffset = lua_tonumber(l, lua_upvalueindex(1));
    auto func = (std::function<FuncRetType (FuncArgType...)>*)
        lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<sizeof...(FuncArgType)>::exec(*func, l, argoffset);
}

template <typename T>
class LuaClass : protected LuaRefObject {
public:
    LuaClass(const LuaClass<T>& lclass)
        : LuaRefObject(lclass), m_metatable_name(METATABLENAME(T)) {}

    LuaClass<T>& operator=(const LuaClass<T>& lclass) {
        assign(lclass);
        return *this;
    }

    template <typename ... FuncArgType>
    LuaClass<T>& setconstructor() {
        pushself();

        lua_pushinteger(m_l.get(), 1); // argument offset
        lua_pushlightuserdata(m_l.get(), (void*)(constructor<FuncArgType...>));
        lua_pushcclosure(m_l.get(), l_function<T*, FuncArgType...>, 2);
        lua_setfield(m_l.get(), -2, "__call");

        lua_pop(m_l.get(), 1);

        return *this;
    }

    // register common member function
    template <typename FuncRetType, typename ... FuncArgType>
    LuaClass<T>& set(const char* name, FuncRetType (T::*f)(FuncArgType...)) {
        typedef MemberFuncWrapper<FuncRetType, FuncArgType...> wrapper_t;

        luaL_getmetatable(m_l.get(), m_metatable_name.c_str()); // metatable of userdata

        auto wrapper = (wrapper_t*)lua_newuserdata(m_l.get(), sizeof(wrapper_t));
        wrapper->f = f;
        lua_pushcclosure(m_l.get(), memberfunc<FuncRetType, FuncArgType...>, 1);
        lua_setfield(m_l.get(), -2, name);

        lua_pop(m_l.get(), 1);

        return *this;
    }

    // register member function with const qualifier
    template <typename FuncRetType, typename ... FuncArgType>
    LuaClass<T>& set(const char* name, FuncRetType (T::*f)(FuncArgType...) const) {
        typedef MemberFuncWrapper<FuncRetType, FuncArgType...> wrapper_t;

        luaL_getmetatable(m_l.get(), m_metatable_name.c_str()); // metatable of userdata

        auto wrapper = (wrapper_t*)lua_newuserdata(m_l.get(), sizeof(wrapper_t));
        wrapper->fc = f;
        lua_pushcclosure(m_l.get(), constmemberfunc<FuncRetType, FuncArgType...>, 1);
        lua_setfield(m_l.get(), -2, name);

        lua_pop(m_l.get(), 1);

        return *this;
    }

    // register static member function
    template <typename FuncRetType, typename ... FuncArgType>
    LuaClass<T>& set(const char* name, FuncRetType (*func)(FuncArgType...)) {
        lua_pushinteger(m_l.get(), 1); // argument offset, skip `self` in argv[0]
        lua_pushlightuserdata(m_l.get(), (void*)func);
        lua_pushcclosure(m_l.get(), l_function<FuncRetType, FuncArgType...>, 2);

        // can be used without being instantiated
        pushself();
        lua_pushvalue(m_l.get(), -2); // the function
        lua_setfield(m_l.get(), -2, name);

        // can be used by instances
        luaL_getmetatable(m_l.get(), m_metatable_name.c_str()); // metatable of userdata
        lua_pushvalue(m_l.get(), -3); // the function
        lua_setfield(m_l.get(), -2, name);

        lua_pop(m_l.get(), 3);

        return *this;
    }

    // register common lua function
    LuaClass<T>& set(const char* name, int (*func)(lua_State*)) {
        luaL_getmetatable(m_l.get(), m_metatable_name.c_str()); // metatable of userdata
        lua_pushcclosure(m_l.get(), func, 0);
        lua_setfield(m_l.get(), -2, name);
        lua_pop(m_l.get(), 1);
        return *this;
    }

private:
    const std::string m_metatable_name;

private:
    template <typename FuncRetType, typename ... FuncArgType>
    union MemberFuncWrapper {
        FuncRetType (T::*f)(FuncArgType...);
        FuncRetType (T::*fc)(FuncArgType...) const;
    };

    LuaClass(const std::shared_ptr<lua_State>& l, int index)
        : LuaRefObject(l, index), m_metatable_name(METATABLENAME(T)) {}

    template <typename ... FuncArgType>
    static T* constructor(FuncArgType... argv) {
        return new T(argv...);
    }

    static int destructor(lua_State* l) {
        T** ud = (T**)lua_touserdata(l, 1);
        delete *ud;
        return 0;
    }

    template <typename FuncRetType, typename ... FuncArgType>
    static int memberfunc(lua_State* l) {
        T** ud = (T**)lua_touserdata(l, 1);
        auto wrapper = (MemberFuncWrapper<FuncRetType, FuncArgType...>*)
            lua_touserdata(l, lua_upvalueindex(1));
        return FunctionCaller<sizeof...(FuncArgType)>::exec(*ud, wrapper->f, l, 1);
    }

    template <typename FuncRetType, typename ... FuncArgType>
    static int constmemberfunc(lua_State* l) {
        T** ud = (T**)lua_touserdata(l, 1);
        auto wrapper = (MemberFuncWrapper<FuncRetType, FuncArgType...>*)
            lua_touserdata(l, lua_upvalueindex(1));
        return FunctionCaller<sizeof...(FuncArgType)>::exec(*ud, wrapper->fc, l, 1);
    }

private:
    friend class LuaState;
};

class LuaUserdata : public LuaRefObject {
public:
    LuaUserdata(const LuaUserdata& lud)
        : LuaRefObject(lud) {}

    LuaUserdata& operator=(const LuaUserdata& lud) {
        assign(lud);
        return *this;
    }

    template <typename T>
    T* object() const {
        pushself();
        T** ud = (T**)lua_touserdata(m_l.get(), -1);
        T* ret = *ud;
        lua_pop(m_l.get(), 1);

        return ret;
    }

private:
    LuaUserdata(const std::shared_ptr<lua_State>& l, int index)
        : LuaRefObject(l, index) {}

    friend class LuaObject;
    friend class LuaState;
};

class LuaState {
public:
    LuaState() : m_l(luaL_newstate(), lua_close) {
        luaL_openlibs(m_l.get());
    }

    lua_State* ptr() const { return m_l.get(); }

    LuaObject get(const char* name) const;

    bool set(const char* name, const char* str);
    bool set(const char* name, const char* str, size_t len);
    bool set(const char* name, lua_Number);
    // fails only when LuaObject is not in the current LuaState
    bool set(const char* name, const LuaObject& lobj);
    bool set(const char* name, const LuaTable& ltable);
    bool set(const char* name, const LuaFunction& lfunc);
    bool set(const char* name, const LuaUserdata& lud);

    template <typename T>
    bool set(const char* name, const LuaClass<T>& lclass) {
        return setobject(name, lclass);
    }

    LuaTable newtable(const char* name = nullptr);

    template <typename FuncRetType, typename ... FuncArgType>
    LuaFunction newfunction(FuncRetType (*func)(FuncArgType...),
                            const char* name = nullptr) {
        lua_pushinteger(m_l.get(), 0); // argument offset
        lua_pushlightuserdata(m_l.get(), (void*)func);
        lua_pushcclosure(m_l.get(), l_function<FuncRetType, FuncArgType...>, 2);

        LuaFunction lfunc(m_l, -1);

        if (name) {
            lua_setglobal(m_l.get(), name);
        } else {
            lua_pop(m_l.get(), 1);
        }

        return lfunc;
    }

    template <typename FuncRetType, typename ... FuncArgType>
    LuaFunction newfunction(const std::function<FuncRetType (FuncArgType...)>& func,
                            const char* name = nullptr) {
        typedef std::function<FuncRetType (FuncArgType...)> func_t;
        static const std::string metatable(METATABLENAME(func_t));

        lua_pushinteger(m_l.get(), 0); // argument offset

        auto ud = (func_t*)lua_newuserdata(m_l.get(), sizeof(func_t));
        new (ud) func_t(func);

        if (luaL_newmetatable(m_l.get(), metatable.c_str()) != 0) {
            lua_pushcclosure(m_l.get(), std_function_destructor<FuncRetType, FuncArgType...>, 0);
            lua_setfield(m_l.get(), -2, "__gc");
        }
        lua_setmetatable(m_l.get(), -2);

        lua_pushcclosure(m_l.get(), l_std_function<FuncRetType, FuncArgType...>, 2);

        LuaFunction lfunc(m_l, -1);

        if (name) {
            lua_setglobal(m_l.get(), name);
        } else {
            lua_pop(m_l.get(), 1);
        }

        return lfunc;
    }

    template <typename T, typename ... Argv>
    LuaUserdata newuserdata(const char* name = nullptr,
                            const Argv&... argv) {
        static const std::string metatable(METATABLENAME(T));

        luaL_getmetatable(m_l.get(), metatable.c_str());
        if (lua_isnil(m_l.get(), -1)) {
            lua_pop(m_l.get(), 1);
            throw std::runtime_error("LuaState::newuserdata(): type `"
                                     + metatable + "` not found.");
        }

        T** ud = (T**)lua_newuserdata(m_l.get(), sizeof(T*));
        *ud = new T(argv...);

        lua_pushvalue(m_l.get(), -2); // the metatable
        lua_setmetatable(m_l.get(), -2);

        LuaUserdata ret(m_l, -1);

        if (name) {
            lua_setglobal(m_l.get(), name);
            lua_pop(m_l.get(), 1); // the metatable
        } else {
            lua_pop(m_l.get(), 2);
        }

        return ret;
    }

    template <typename T>
    LuaClass<T> newclass(const char* name = nullptr) {
        static const std::string metatable(METATABLENAME(T));

        lua_State* l = m_l.get();
        if (luaL_newmetatable(l, metatable.c_str()) == LUA_OK) {
            lua_getfield(l, -1, "__host_class__");
            if (lua_istable(l, -1)) {
                goto found;
            } else {
                lua_pop(l, 2);
                throw std::runtime_error("error class `" + metatable +
                                         "`: __host_class__ is not a table.");
            }
        }

        lua_newtable(l);

        // metatable for the class itself

        lua_pushvalue(l, -1);
        lua_setmetatable(l, -2);

        lua_pushvalue(l, -1);
        lua_setfield(l, -2, "__index");

        // metatable for userdata

        lua_pushvalue(l, -2);
        lua_setfield(l, -3, "__index");

        lua_pushcfunction(l, LuaClass<T>::destructor);
        lua_setfield(l, -3, "__gc");

        lua_pushvalue(l, -1);
        lua_setfield(l, -3, "__host_class__"); // the class itself

    found:
        LuaClass<T> ret(m_l, -1);
        if (name) {
            lua_setglobal(l, name);
        } else {
            lua_pop(l, 1);
        }

        lua_pop(l, 1); // the meta table
        return ret;
    }

    bool dostring(const char* chunk, std::string* errstr = nullptr,
                  const std::function<bool (int nresults)>& before_proc = nullptr,
                  const std::function<bool (int i, const LuaObject&)>& proc = nullptr);
    bool dofile(const char* script, std::string* errstr = nullptr,
                const std::function<bool (int nresults)>& before_proc = nullptr,
                const std::function<bool (int i, const LuaObject&)>& proc = nullptr);

private:
    std::shared_ptr<lua_State> m_l;

private:
    template <typename FuncRetType, typename ... FuncArgType>
    static int std_function_destructor(lua_State* l) {
        typedef std::function<FuncRetType (FuncArgType...)> func_t;
        auto ud = (func_t*)lua_touserdata(l, -1);
        ud->~func_t();
        return 0;
    }

    bool setobject(const char* name, const LuaRefObject& lobj);

    // no copying
    LuaState(const LuaState& l);
    LuaState& operator=(const LuaState& l);
};

}

#endif
