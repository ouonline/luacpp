#include "luacpp.h"
#include <iostream>
using namespace std;

namespace luacpp {

LuaRefObject::LuaRefObject(const shared_ptr<lua_State>& l, int index) {
    m_l = l;
    m_type = lua_type(l.get(), index);

    lua_pushvalue(l.get(), index);
    m_index = luaL_ref(l.get(), LUA_REGISTRYINDEX);
}

LuaRefObject::LuaRefObject(const LuaRefObject& lobj) {
    m_l = lobj.m_l;
    m_type = lobj.m_type;

    lua_rawgeti(lobj.m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
    m_index = luaL_ref(m_l.get(), LUA_REGISTRYINDEX);
}

LuaRefObject::~LuaRefObject() {
    luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);
}

void LuaRefObject::Assign(const LuaRefObject& lobj) {
    if (m_index != lobj.m_index || m_l != lobj.m_l) {
        luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);

        m_l = lobj.m_l;
        m_type = lobj.m_type;

        lua_rawgeti(lobj.m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
        m_index = luaL_ref(m_l.get(), LUA_REGISTRYINDEX);
    }
}

void LuaRefObject::PushSelf() const {
    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_index);
}

bool LuaRefObject::PushObject(const LuaRefObject& lobj) {
    if (m_l != lobj.m_l) {
        return false;
    }

    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
    return true;
}

/* ------------------------------------------------------------------------- */

bool LuaObject::ToBool() const {
    return lua_toboolean(m_l.get(), -1);
}

string LuaObject::ToString() const {
    const char* str;
    size_t len;

    PushSelf();
    str = lua_tolstring(m_l.get(), -1, &len);
    string ret(str, len);
    lua_pop(m_l.get(), 1);

    return ret;
}

lua_Number LuaObject::ToNumber() const {
    PushSelf();
    lua_Number ret = lua_tonumber(m_l.get(), -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaObject::ToTable() const {
    PushSelf();
    LuaTable ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaFunction LuaObject::ToFunctiong() const {
    PushSelf();
    LuaFunction ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaUserData LuaObject::ToUserData() const {
    PushSelf();
    LuaUserData ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

/* ------------------------------------------------------------------------- */

LuaObject LuaTable::Get(int index) const {
    PushSelf();
    lua_rawgeti(m_l.get(), -1, index);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

LuaObject LuaTable::Get(const char* name) const {
    PushSelf();
    lua_getfield(m_l.get(), -1, name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

bool LuaTable::Set(int index, const char* str) {
    PushSelf();
    lua_pushstring(m_l.get(), str);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::Set(int index, const char* str, size_t len) {
    PushSelf();
    lua_pushlstring(m_l.get(), str, len);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::Set(int index, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l.get(), value);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::SetObject(int index, const LuaRefObject& lobj) {
    PushSelf();

    bool ok = PushObject(lobj);
    if (ok) {
        lua_rawseti(m_l.get(), -2, index);
    }
    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::Set(int index, const LuaObject& lobj) {
    return SetObject(index, lobj);
}

bool LuaTable::Set(int index, const LuaTable& ltable) {
    return SetObject(index, ltable);
}

bool LuaTable::Set(int index, const LuaFunction& lfunc) {
    return SetObject(index, lfunc);
}

bool LuaTable::Set(int index, const LuaUserData& lud) {
    return SetObject(index, lud);
}

bool LuaTable::Set(const char* name, const char* str) {
    PushSelf();
    lua_pushstring(m_l.get(), str);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::Set(const char* name, const char* str, size_t len) {
    PushSelf();
    lua_pushlstring(m_l.get(), str, len);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::Set(const char* name, lua_Number value) {
    PushSelf();
    lua_pushnumber(m_l.get(), value);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::SetObject(const char* name, const LuaRefObject& lobj) {
    PushSelf();

    bool ok = PushObject(lobj);
    if (ok) {
        lua_setfield(m_l.get(), -2, name);
    }
    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::Set(const char* name, const LuaObject& lobj) {
    return SetObject(name, lobj);
}

bool LuaTable::Set(const char* name, const LuaTable& ltable) {
    return SetObject(name, ltable);
}

bool LuaTable::Set(const char* name, const LuaFunction& lfunc) {
    return SetObject(name, lfunc);
}

bool LuaTable::Set(const char* name, const LuaUserData& lud) {
    return SetObject(name, lud);
}

bool LuaTable::ForEach(const function<bool (const LuaObject& key,
                                            const LuaObject& value)>& func) const {
    PushSelf();

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

bool LuaFunction::PushArg(const char* str) {
    lua_pushstring(m_l.get(), str);
    return true;
}

bool LuaFunction::PushArg(lua_Number num) {
    lua_pushnumber(m_l.get(), num);
    return true;
}

bool LuaFunction::PushArg(const LuaObject& lobj) {
    return PushObject(lobj);
}

bool LuaFunction::PushArg(const LuaTable& ltable) {
    return PushObject(ltable);
}

bool LuaFunction::PushArg(const LuaFunction& lfunc) {
    return PushObject(lfunc);
}

bool LuaFunction::PushArg(const LuaUserData& lud) {
    return PushObject(lud);
}

bool LuaFunction::Invoke(int argc, string* errstr, LuaFunctionHelper* helper) {
    const int top = lua_gettop(m_l.get()) - argc - 1 /* the function itself */;

    bool ok = (lua_pcall(m_l.get(), argc, LUA_MULTRET, 0) == LUA_OK);
    if (!ok) {
        if (errstr) {
            *errstr = lua_tostring(m_l.get(), -1);
        }
        lua_pop(m_l.get(), 1);
        return false;
    }

    if (!helper) {
        return true;
    }

    const int nresults = lua_gettop(m_l.get()) - top;

    if (!helper->BeforeProcess(nresults)) {
        return true;
    }

    for (int i = nresults; i > 0; --i) {
        if (!helper->Process(nresults - i, LuaObject(m_l, -i))) {
            break;
        }
    }

    helper->AfterProcess();

    if (nresults > 0) {
        lua_pop(m_l.get(), nresults);
    }

    return true;
}

/* ------------------------------------------------------------------------- */

bool LuaState::Set(const char* name, const char* str) {
    lua_pushstring(m_l.get(), str);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::Set(const char* name, const char* str, size_t len) {
    lua_pushlstring(m_l.get(), str, len);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::Set(const char* name, lua_Number value) {
    lua_pushnumber(m_l.get(), value);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::SetObject(const char* name, const LuaRefObject& lobj) {
    if (m_l != lobj.m_l) {
        return false;
    }

    lobj.PushSelf();
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::Set(const char* name, const LuaObject& lobj) {
    return SetObject(name, lobj);
}

bool LuaState::Set(const char* name, const LuaTable& ltable) {
    return SetObject(name, ltable);
}

bool LuaState::Set(const char* name, const LuaFunction& lfunc) {
    return SetObject(name, lfunc);
}

bool LuaState::Set(const char* name, const LuaUserData& lud) {
    return SetObject(name, lud);
}

LuaObject LuaState::Get(const char* name) const {
    lua_getglobal(m_l.get(), name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaState::CreateTable(const char* name) {
    lua_newtable(m_l.get());
    LuaTable ret(m_l, -1);

    if (name) {
        lua_setglobal(m_l.get(), name);
    } else {
        lua_pop(m_l.get(), 1);
    }

    return ret;
}

bool LuaState::DoString(const char* chunk, string* errstr,
                        LuaFunctionHelper* helper) {
    bool ok = (luaL_loadstring(m_l.get(), chunk) == LUA_OK);
    if (!ok) {
        if (errstr) {
            *errstr = lua_tostring(m_l.get(), -1);
        }
        lua_pop(m_l.get(), 1);
        return false;
    }

    LuaFunction f(m_l, -1);
    ok = f.Exec(errstr, helper);
    lua_pop(m_l.get(), 1); // function generated by luaL_loadstring()
    return ok;
}

bool LuaState::DoFile(const char* script, string* errstr,
                      LuaFunctionHelper* helper) {
    bool ok = (luaL_loadfile(m_l.get(), script) == LUA_OK);
    if (!ok) {
        if (errstr) {
            *errstr = lua_tostring(m_l.get(), -1);
        }
        lua_pop(m_l.get(), 1);
        return false;
    }

    LuaFunction f(m_l, -1);
    ok = f.Exec(errstr, helper);
    lua_pop(m_l.get(), 1); // function generated by luaL_loadfile()
    return ok;
}

}
