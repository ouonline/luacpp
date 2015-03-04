#ifndef __LUACPP_HPP__
#define __LUACPP_HPP__

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Guoyu Ou <benogy@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

#include <lua.hpp>

namespace luacpp {

#define METATABLENAME(T) (std::string(typeid(T).name()).append(".o_o").c_str())

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

        size_t m_index;
        int m_type;

        friend class LuaState;
};

class LuaTable;
class LuaFunction;
class LuaUserdata;

class LuaObject : public LuaRefObject {

    public:

        LuaObject(const LuaObject& lobj)
            : LuaRefObject(lobj)
        {}

        std::string tostring() const;
        lua_Number tonumber() const;
        LuaTable totable() const;
        LuaFunction tofunction() const;
        LuaUserdata touserdata() const;

        LuaObject& operator=(const LuaObject& lobj)
        {
            assign(lobj);
            return *this;
        }

    private:

        LuaObject(const std::shared_ptr<lua_State>& l, int index)
            : LuaRefObject(l, index)
        {}

        friend class LuaState;
        friend class LuaTable;
        friend class LuaFunction;
};

class LuaTable : public LuaRefObject {

    public:

        LuaTable(const LuaTable& ltable)
            : LuaRefObject(ltable)
        {}

        LuaObject get(int index) const;
        LuaObject get(const char* name) const;

        void set(int index, const char* str);
        void set(int index, const char* str, size_t len);
        void set(int index, lua_Number);
        // fails only when LuaObject is not in the same LuaState
        bool set(int index, const LuaObject& lobj);
        bool set(int index, const LuaTable& ltable);
        bool set(int index, const LuaFunction& lfunc);
        bool set(int index, const LuaUserdata& lud);

        void set(const char* name, const char* str);
        void set(const char* name, const char* str, size_t len);
        void set(const char* name, lua_Number);
        // fails only when LuaObject is not in the same LuaState
        bool set(const char* name, const LuaObject& lobj);
        bool set(const char* name, const LuaTable& ltable);
        bool set(const char* name, const LuaFunction& lfunc);
        bool set(const char* name, const LuaUserdata& lud);

        bool foreach(const std::function<bool (const LuaObject& key,
                                               const LuaObject& value)>& func) const;

        LuaTable& operator=(const LuaTable& ltable)
        {
            assign(ltable);
            return *this;
        }

    protected:

        LuaTable(const std::shared_ptr<lua_State>& l, size_t index)
            : LuaRefObject(l, index)
        {}

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
            : LuaRefObject(lfunc)
        {}

        template<typename... Argv>
        bool exec(int nresults = 0, std::vector<LuaObject>* res = nullptr,
                  std::string* errstr = nullptr, const Argv&... argv);

        LuaFunction& operator=(const LuaFunction& lfunc)
        {
            assign(lfunc);
            return *this;
        }

    private:

        LuaFunction(const std::shared_ptr<lua_State>& l, size_t index)
            : LuaRefObject(l, index)
        {}

        bool pusharg(const char* str);
        bool pusharg(lua_Number);
        bool pusharg(const LuaObject& lobj);
        bool pusharg(const LuaTable& ltable);
        bool pusharg(const LuaFunction& lfunc);
        bool pusharg(const LuaUserdata& lud);

        bool pusharg(int* argc, std::string* errstr) { return true; }

        template<typename First, typename... Rest>
        bool pusharg(int* argc, std::string* errstr,
                     const First& first, const Rest&... rest);

        friend class LuaState;
        friend class LuaObject;
        friend class LuaTable;
};

template<uint32_t N>
class FunctionCaller {

    public:

        template<typename FuncType, typename... Argv>
        static int exec(FuncType f, lua_State* l, int argoffset,
                        const Argv&... argv)
        {
            return FunctionCaller<N - 1>::exec(f, l, argoffset,
                                               FuncArg(l, N + argoffset),
                                               argv...);
        }

        template<typename T, typename FuncType, typename... Argv>
        static int exec(T* obj, FuncType f, lua_State* l, int argoffset,
                        const Argv&... argv)
        {
            return FunctionCaller<N - 1>::exec(obj, f, l, argoffset,
                                               FuncArg(l, N + argoffset),
                                               argv...);
        }

    private:

        class FuncArg {

            public:

                FuncArg(lua_State* l, int index)
                    : m_l(l), m_index(index)
                {}

                operator lua_Number () const { return lua_tonumber(m_l, m_index); }
                operator const char* () const { return lua_tostring(m_l, m_index); }

                template<typename T>
                operator T* () const
                {
                    T** ud = (T**)lua_touserdata(m_l, m_index);
                    return *ud;
                }

            private:

                lua_State* m_l;
                int m_index;
        };
};

template<>
class FunctionCaller<0> {

    public:

        template<typename FuncRetType, typename... FuncArgType, typename... Argv>
        static int exec(FuncRetType (*f)(FuncArgType...), lua_State* l, int,
                        const Argv&... argv)
        {
            pushres(l, f(argv...));
            return 1;
        }

        template<typename... FuncArgType, typename... Argv>
        static int exec(void (*f)(FuncArgType...), lua_State* l, int,
                        const Argv&... argv)
        {
            f(argv...);
            return 0;
        }


        template<typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
        static int exec(T* obj, FuncRetType (T::*f)(FuncArgType...), lua_State* l, int,
                        const Argv&... argv)
        {
            pushres(l, (obj->*f)(argv...));
            return 1;
        }

        template<typename T, typename... FuncArgType, typename... Argv>
        static int exec(T* obj, void (T::*f)(FuncArgType...), lua_State* l, int,
                        const Argv&... argv)
        {
            (obj->*f)(argv...);
            return 0;
        }

        template<typename T, typename FuncRetType, typename... FuncArgType, typename... Argv>
        static int exec(T* obj, FuncRetType (T::*f)(FuncArgType...) const, lua_State* l, int,
                        const Argv&... argv)
        {
            pushres(l, (obj->*f)(argv...));
            return 1;
        }

        template<typename T, typename... FuncArgType, typename... Argv>
        static int exec(T* obj, void (T::*f)(FuncArgType...) const, lua_State* l, int,
                        const Argv&... argv)
        {
            (obj->*f)(argv...);
            return 0;
        }

    private:

        static inline void pushres(lua_State* l, lua_Number n)
        {
            lua_pushnumber(l, n);
        }

        template<typename T>
        static inline void pushres(lua_State* l, T* obj)
        {
            T** ud = (T**)lua_newuserdata(l, sizeof(T*));
            *ud = obj;
            luaL_setmetatable(l, METATABLENAME(T));
        }
};

template<typename FuncRetType, typename... FuncArgType>
static int l_function(lua_State* l)
{
    typedef FuncRetType (*func_t)(FuncArgType...);

    int argoffset = lua_tonumber(l, lua_upvalueindex(1));
    auto func = (func_t)lua_touserdata(l, lua_upvalueindex(2));
    return FunctionCaller<sizeof...(FuncArgType)>::exec(func, l, argoffset);
}

template<typename T>
class LuaClass : protected LuaRefObject {

    public:

        LuaClass(const LuaClass<T>& lclass)
            : LuaRefObject(lclass)
        {}

        LuaClass<T>& operator=(const LuaClass<T>& lclass)
        {
            assign(lclass);
            return *this;
        }

        template<typename... FuncArgType>
        LuaClass<T>& setconstructor()
        {
            pushself();

            lua_pushinteger(m_l.get(), 1); // argument offset
            lua_pushlightuserdata(m_l.get(), (void*)(constructor<FuncArgType...>));
            lua_pushcclosure(m_l.get(), l_function<T*, FuncArgType...>, 2);
            lua_setfield(m_l.get(), -2, "__call");

            lua_pop(m_l.get(), 1);

            return *this;
        }

        // register common member function
        template<typename FuncRetType, typename... FuncArgType>
        LuaClass<T>& set(const char* name, FuncRetType (T::*f)(FuncArgType...))
        {
            typedef MemberFuncWrapper<FuncRetType, FuncArgType...> wrapper_t;

            luaL_getmetatable(m_l.get(), METATABLENAME(T)); // metatable of userdata

            auto wrapper = (wrapper_t*)lua_newuserdata(m_l.get(), sizeof(wrapper_t));
            wrapper->f = f;
            lua_pushcclosure(m_l.get(), memberfunc<FuncRetType, FuncArgType...>, 1);
            lua_setfield(m_l.get(), -2, name);

            lua_pop(m_l.get(), 1);

            return *this;
        }

        // register member function with const qualifier
        template<typename FuncRetType, typename... FuncArgType>
        LuaClass<T>& set(const char* name, FuncRetType (T::*f)(FuncArgType...) const)
        {
            typedef MemberFuncWrapper<FuncRetType, FuncArgType...> wrapper_t;

            luaL_getmetatable(m_l.get(), METATABLENAME(T)); // metatable of userdata

            auto wrapper = (wrapper_t*)lua_newuserdata(m_l.get(), sizeof(wrapper_t));
            wrapper->fc = f;
            lua_pushcclosure(m_l.get(), constmemberfunc<FuncRetType, FuncArgType...>, 1);
            lua_setfield(m_l.get(), -2, name);

            lua_pop(m_l.get(), 1);

            return *this;
        }

        // register static member function
        template<typename FuncRetType, typename... FuncArgType>
        LuaClass<T>& set(const char* name, FuncRetType (*func)(FuncArgType...))
        {
            lua_pushinteger(m_l.get(), 1); // argument offset, skip `self` in argv[0]
            lua_pushlightuserdata(m_l.get(), (void*)func);
            lua_pushcclosure(m_l.get(), l_function<FuncRetType, FuncArgType...>, 2);

            // can be used without being instantiated
            pushself();
            lua_pushvalue(m_l.get(), -2); // the function
            lua_setfield(m_l.get(), -2, name);

            // can be used by instances
            luaL_getmetatable(m_l.get(), METATABLENAME(T)); // metatable of userdata
            lua_pushvalue(m_l.get(), -3); // the function
            lua_setfield(m_l.get(), -2, name);

            lua_pop(m_l.get(), 3);

            return *this;
        }

    private:

        template<typename FuncRetType, typename... FuncArgType>
        union MemberFuncWrapper {
            FuncRetType (T::*f)(FuncArgType...);
            FuncRetType (T::*fc)(FuncArgType...) const;
        };

    private:

        LuaClass(const char* name, const std::shared_ptr<lua_State>& l, int index)
            : LuaRefObject(l, index)
        {
            // metatable for the class table

            pushself();

            lua_pushvalue(l.get(), -1);
            lua_setmetatable(l.get(), -2);

            lua_pushvalue(l.get(), -1);
            lua_setfield(l.get(), -2, "__index");

            lua_pushinteger(l.get(), 1); // argument offset
            lua_pushlightuserdata(l.get(), (void*)constructor<>);
            lua_pushcclosure(l.get(), l_function<T*>, 2); // default constructor
            lua_setfield(l.get(), -2, "__call");

            // metatable for userdata

            luaL_newmetatable(l.get(), METATABLENAME(T));

            lua_pushvalue(l.get(), -1);
            lua_setfield(l.get(), -2, "__index");

            lua_pushcfunction(m_l.get(), destructor);
            lua_setfield(m_l.get(), -2, "__gc");

            lua_pop(m_l.get(), 2);
        }

        template<typename... FuncArgType>
        static T* constructor(FuncArgType... argv)
        {
            return new T(argv...);
        }

        static int destructor(lua_State* l)
        {
            T** ud = (T**)lua_touserdata(l, 1);
            delete *ud;
            return 0;
        }

        template<typename FuncRetType, typename... FuncArgType>
        static int memberfunc(lua_State* l)
        {
            T** ud = (T**)lua_touserdata(l, 1);
            auto wrapper = (MemberFuncWrapper<FuncRetType, FuncArgType...>*)
                lua_touserdata(l, lua_upvalueindex(1));
            return FunctionCaller<sizeof...(FuncArgType)>::exec(*ud, wrapper->f, l, 1);
        }

        template<typename FuncRetType, typename... FuncArgType>
        static int constmemberfunc(lua_State* l)
        {
            T** ud = (T**)lua_touserdata(l, 1);
            auto wrapper = (MemberFuncWrapper<FuncRetType, FuncArgType...>*)
                lua_touserdata(l, lua_upvalueindex(1));
            return FunctionCaller<sizeof...(FuncArgType)>::exec(*ud, wrapper->fc, l, 1);
        }

        friend class LuaState;
};

class LuaUserdata : public LuaRefObject {

    public:

        LuaUserdata(const LuaUserdata& lud)
            : LuaRefObject(lud)
        {}

        LuaUserdata& operator=(const LuaUserdata& lud)
        {
            assign(lud);
            return *this;
        }

        template<typename T>
        T* object() const
        {
            pushself();
            T** ud = (T**)lua_touserdata(m_l.get(), -1);
            T* ret = *ud;
            lua_pop(m_l.get(), 1);

            return ret;
        }

    private:

        LuaUserdata(const std::shared_ptr<lua_State>& l, int index)
            : LuaRefObject(l, index)
        {}

        friend class LuaObject;
        friend class LuaState;
};

class LuaState {

    public:

        LuaState()
            : m_l(luaL_newstate(), lua_close)
        {
            luaL_openlibs(m_l.get());
        }

        lua_State* ptr() const { return m_l.get(); }

        LuaObject get(const char* name) const;

        void set(const char* name, const char* str);
        void set(const char* name, const char* str, size_t len);
        void set(const char* name, lua_Number);
        // fails only when LuaObject is not in the current LuaState
        bool set(const char* name, const LuaObject& lobj);
        bool set(const char* name, const LuaTable& ltable);
        bool set(const char* name, const LuaFunction& lfunc);
        bool set(const char* name, const LuaUserdata& lud);

        LuaTable newtable(const char* name = nullptr);

        template<typename FuncRetType, typename... FuncArgType>
        LuaFunction newfunction(FuncRetType (*)(FuncArgType...),
                                const char* name = nullptr);

        template<typename T, typename... Argv>
        LuaUserdata newuserdata(const char* name = nullptr,
                                const Argv&... argv);

        template<typename T>
        LuaClass<T> newclass(const char* name);

        bool dostring(const char* chunk, std::string* errstr = nullptr);
        bool dofile(const char* script, std::string* errstr = nullptr);

    private:

        std::shared_ptr<lua_State> m_l;

    private:

        bool setobject(const char* name, const LuaRefObject& lobj);

        // no copying
        LuaState(const LuaState& l);
        LuaState& operator=(const LuaState& l);
};

/* ------------------------------------------------------------------------- */

LuaRefObject::LuaRefObject(const std::shared_ptr<lua_State>& l, int index)
{
    m_l = l;
    m_type = lua_type(l.get(), index);

    lua_pushvalue(l.get(), index);
    m_index = luaL_ref(l.get(), LUA_REGISTRYINDEX);
}

LuaRefObject::LuaRefObject(const LuaRefObject& lobj)
{
    m_l = lobj.m_l;
    m_type = lobj.m_type;

    lua_rawgeti(lobj.m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
    m_index = luaL_ref(m_l.get(), LUA_REGISTRYINDEX);
}

LuaRefObject::~LuaRefObject()
{
    luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);
}

void LuaRefObject::assign(const LuaRefObject& lobj)
{
    if (m_index != lobj.m_index || m_l != lobj.m_l) {
        luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);

        m_l = lobj.m_l;
        m_type = lobj.m_type;

        lua_rawgeti(lobj.m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
        m_index = luaL_ref(m_l.get(), LUA_REGISTRYINDEX);
    }
}

void LuaRefObject::pushself() const
{
    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_index);
}

bool LuaRefObject::pushobject(const LuaRefObject& lobj)
{
    if (m_l != lobj.m_l)
        return false;

    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
    return true;
}

/* ------------------------------------------------------------------------- */

std::string LuaObject::tostring() const
{
    const char* str;
    size_t len;

    pushself();
    str = lua_tolstring(m_l.get(), -1, &len);
    std::string ret(str, len);
    lua_pop(m_l.get(), 1);

    return ret;
}

lua_Number LuaObject::tonumber() const
{
    pushself();
    lua_Number ret = lua_tonumber(m_l.get(), -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaObject::totable() const
{
    pushself();
    LuaTable ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaFunction LuaObject::tofunction() const
{
    pushself();
    LuaFunction ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaUserdata LuaObject::touserdata() const
{
    pushself();
    LuaUserdata ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

/* ------------------------------------------------------------------------- */

LuaObject LuaTable::get(int index) const
{
    pushself();
    lua_rawgeti(m_l.get(), -1, index);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

LuaObject LuaTable::get(const char* name) const
{
    pushself();
    lua_getfield(m_l.get(), -1, name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

void LuaTable::set(int index, const char* str)
{
    pushself();
    lua_pushstring(m_l.get(), str);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
}

void LuaTable::set(int index, const char* str, size_t len)
{
    pushself();
    lua_pushlstring(m_l.get(), str, len);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
}

void LuaTable::set(int index, lua_Number value)
{
    pushself();
    lua_pushnumber(m_l.get(), value);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
}

bool LuaTable::setobject(int index, const LuaRefObject& lobj)
{
    pushself();

    bool ok = pushobject(lobj);
    if (ok)
        lua_rawseti(m_l.get(), -2, index);

    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::set(int index, const LuaObject& lobj)
{
    return setobject(index, lobj);
}

bool LuaTable::set(int index, const LuaTable& ltable)
{
    return setobject(index, ltable);
}

bool LuaTable::set(int index, const LuaFunction& lfunc)
{
    return setobject(index, lfunc);
}

bool LuaTable::set(int index, const LuaUserdata& lud)
{
    return setobject(index, lud);
}

void LuaTable::set(const char* name, const char* str)
{
    pushself();
    lua_pushstring(m_l.get(), str);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
}

void LuaTable::set(const char* name, const char* str, size_t len)
{
    pushself();
    lua_pushlstring(m_l.get(), str, len);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
}

void LuaTable::set(const char* name, lua_Number value)
{
    pushself();
    lua_pushnumber(m_l.get(), value);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
}

bool LuaTable::setobject(const char* name, const LuaRefObject& lobj)
{
    pushself();

    bool ok = pushobject(lobj);
    if (ok)
        lua_setfield(m_l.get(), -2, name);

    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::set(const char* name, const LuaObject& lobj)
{
    return setobject(name, lobj);
}

bool LuaTable::set(const char* name, const LuaTable& ltable)
{
    return setobject(name, ltable);
}

bool LuaTable::set(const char* name, const LuaFunction& lfunc)
{
    return setobject(name, lfunc);
}

bool LuaTable::set(const char* name, const LuaUserdata& lud)
{
    return setobject(name, lud);
}

bool LuaTable::foreach(const std::function<bool (const LuaObject& key,
                                                 const LuaObject& value)>& func) const
{
    pushself();

    lua_pushnil(m_l.get());
    while (lua_next(m_l.get(), -2) != 0) {
        if (!func(LuaObject(m_l, -2), LuaObject(m_l, -1))) {
            lua_pop(m_l.get(), 3);
            return false;
        }

        lua_pop(m_l.get(), 1);
    }

    lua_pop(m_l.get(), 1);
    return true;
}

/* ------------------------------------------------------------------------- */

bool LuaFunction::pusharg(const char* str)
{
    lua_pushstring(m_l.get(), str);
    return true;
}

bool LuaFunction::pusharg(lua_Number num)
{
    lua_pushnumber(m_l.get(), num);
    return true;
}

bool LuaFunction::pusharg(const LuaObject& lobj)
{
    return pushobject(lobj);
}

bool LuaFunction::pusharg(const LuaTable& ltable)
{
    return pushobject(ltable);
}

bool LuaFunction::pusharg(const LuaFunction& lfunc)
{
    return pushobject(lfunc);
}

bool LuaFunction::pusharg(const LuaUserdata& lud)
{
    return pushobject(lud);
}

template<typename First, typename... Rest>
bool LuaFunction::pusharg(int* argc, std::string* errstr,
                          const First& first, const Rest&... rest)
{
    if (!pusharg(first)) {
        if (errstr)
            *errstr = "argv are not in the same LuaState";
        return false;
    }

    ++(*argc);

    return pusharg(argc, errstr, std::forward<const Rest&>(rest)...);
}

template<typename... Argv>
bool LuaFunction::exec(int nresults, std::vector<LuaObject>* res,
                       std::string* errstr, const Argv&... argv)
{
    pushself();

    int argc = 0;
    if (!pusharg(&argc, errstr, std::forward<const Argv&>(argv)...)) {
        lua_pop(m_l.get(), argc + 1 /* self */);
        return false;
    }

    bool ok = (lua_pcall(m_l.get(), sizeof...(Argv), nresults, 0) == 0);
    if (ok) {
        if (res) {
            res->clear();
            res->reserve(nresults);
            for (int i = nresults; i > 0; --i)
                res->push_back(LuaObject(m_l, -i));
        }

        lua_pop(m_l.get(), nresults);
    } else {
        if (errstr)
            *errstr = lua_tostring(m_l.get(), -1);

        lua_pop(m_l.get(), 1);
    }

    return ok;
}

/* ------------------------------------------------------------------------- */

void LuaState::set(const char* name, const char* str)
{
    lua_pushstring(m_l.get(), str);
    lua_setglobal(m_l.get(), name);
}

void LuaState::set(const char* name, const char* str, size_t len)
{
    lua_pushlstring(m_l.get(), str, len);
    lua_setglobal(m_l.get(), name);
}

void LuaState::set(const char* name, lua_Number value)
{
    lua_pushnumber(m_l.get(), value);
    lua_setglobal(m_l.get(), name);
}

bool LuaState::setobject(const char* name, const LuaRefObject& lobj)
{
    if (m_l != lobj.m_l)
        return false;

    lobj.pushself();
    lua_setglobal(m_l.get(), name);

    return true;
}

bool LuaState::set(const char* name, const LuaObject& lobj)
{
    return setobject(name, lobj);
}

bool LuaState::set(const char* name, const LuaTable& ltable)
{
    return setobject(name, ltable);
}

bool LuaState::set(const char* name, const LuaFunction& lfunc)
{
    return setobject(name, lfunc);
}

bool LuaState::set(const char* name, const LuaUserdata& lud)
{
    return setobject(name, lud);
}

LuaObject LuaState::get(const char* name) const
{
    lua_getglobal(m_l.get(), name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaState::newtable(const char* name)
{
    lua_newtable(m_l.get());
    LuaTable ret(m_l, -1);

    if (name)
        lua_setglobal(m_l.get(), name);
    else
        lua_pop(m_l.get(), 1);

    return ret;
}

template<typename FuncRetType, typename... FuncArgType>
LuaFunction LuaState::newfunction(FuncRetType (*func)(FuncArgType...),
                                  const char* name)
{
    lua_pushinteger(m_l.get(), 0); // argument offset
    lua_pushlightuserdata(m_l.get(), (void*)func);
    lua_pushcclosure(m_l.get(), l_function<FuncRetType, FuncArgType...>, 2);

    LuaFunction lfunc(m_l, -1);

    if (name)
        lua_setglobal(m_l.get(), name);
    else
        lua_pop(m_l.get(), 1);

    return lfunc;
}

template<typename T, typename... Argv>
LuaUserdata LuaState::newuserdata(const char* name, const Argv&... argv)
{
    luaL_getmetatable(m_l.get(), METATABLENAME(T));
    if (lua_isnil(m_l.get(), -1)) {
        lua_pop(m_l.get(), 1);
        throw std::runtime_error("LuaState::newuserdata(): type `"
                                 + std::string(METATABLENAME(T))
                                 + "` not found.");
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

template<typename T>
LuaClass<T> LuaState::newclass(const char* name)
{
    luaL_getmetatable(m_l.get(), METATABLENAME(T));
    if (!lua_isnil(m_l.get(), -1)) {
        lua_pop(m_l.get(), 1);
        throw std::runtime_error("import class `"
                                 + std::string(METATABLENAME(T))
                                 + "` more than once!");
    }

    lua_newtable(m_l.get());
    LuaClass<T> ret(name, m_l, -1);
    lua_setglobal(m_l.get(), name);

    return ret;
}

bool LuaState::dostring(const char* chunk, std::string* errstr)
{
    bool ok = (luaL_dostring(m_l.get(), chunk) == 0);

    if (!ok) {
        if (errstr)
            *errstr = lua_tostring(m_l.get(), -1);

        lua_pop(m_l.get(), 1);
    }

    return ok;
}

bool LuaState::dofile(const char* script, std::string* errstr)
{
    bool ok = (luaL_dofile(m_l.get(), script) == 0);

    if (!ok) {
        if (errstr)
            *errstr = lua_tostring(m_l.get(), -1);

        lua_pop(m_l.get(), 1);
    }

    return ok;
}

}

#endif
