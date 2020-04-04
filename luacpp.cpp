#include "luacpp.h"
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

void LuaRefObject::assign(const LuaRefObject& lobj) {
    if (m_index != lobj.m_index || m_l != lobj.m_l) {
        luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);

        m_l = lobj.m_l;
        m_type = lobj.m_type;

        lua_rawgeti(lobj.m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
        m_index = luaL_ref(m_l.get(), LUA_REGISTRYINDEX);
    }
}

void LuaRefObject::pushself() const {
    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_index);
}

bool LuaRefObject::pushobject(const LuaRefObject& lobj) {
    if (m_l != lobj.m_l) {
        return false;
    }

    lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, lobj.m_index);
    return true;
}

/* ------------------------------------------------------------------------- */

bool LuaObject::tobool() const {
    return lua_toboolean(m_l.get(), -1);
}

string LuaObject::tostring() const {
    const char* str;
    size_t len;

    pushself();
    str = lua_tolstring(m_l.get(), -1, &len);
    string ret(str, len);
    lua_pop(m_l.get(), 1);

    return ret;
}

lua_Number LuaObject::tonumber() const {
    pushself();
    lua_Number ret = lua_tonumber(m_l.get(), -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaObject::totable() const {
    pushself();
    LuaTable ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaFunction LuaObject::tofunction() const {
    pushself();
    LuaFunction ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaUserdata LuaObject::touserdata() const {
    pushself();
    LuaUserdata ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

/* ------------------------------------------------------------------------- */

LuaObject LuaTable::get(int index) const {
    pushself();
    lua_rawgeti(m_l.get(), -1, index);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

LuaObject LuaTable::get(const char* name) const {
    pushself();
    lua_getfield(m_l.get(), -1, name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 2);

    return ret;
}

bool LuaTable::set(int index, const char* str) {
    pushself();
    lua_pushstring(m_l.get(), str);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::set(int index, const char* str, size_t len) {
    pushself();
    lua_pushlstring(m_l.get(), str, len);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::set(int index, lua_Number value) {
    pushself();
    lua_pushnumber(m_l.get(), value);
    lua_rawseti(m_l.get(), -2, index);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::setobject(int index, const LuaRefObject& lobj) {
    pushself();

    bool ok = pushobject(lobj);
    if (ok) {
        lua_rawseti(m_l.get(), -2, index);
    }
    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::set(int index, const LuaObject& lobj) {
    return setobject(index, lobj);
}

bool LuaTable::set(int index, const LuaTable& ltable) {
    return setobject(index, ltable);
}

bool LuaTable::set(int index, const LuaFunction& lfunc) {
    return setobject(index, lfunc);
}

bool LuaTable::set(int index, const LuaUserdata& lud) {
    return setobject(index, lud);
}

bool LuaTable::set(const char* name, const char* str) {
    pushself();
    lua_pushstring(m_l.get(), str);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::set(const char* name, const char* str, size_t len) {
    pushself();
    lua_pushlstring(m_l.get(), str, len);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::set(const char* name, lua_Number value) {
    pushself();
    lua_pushnumber(m_l.get(), value);
    lua_setfield(m_l.get(), -2, name);
    lua_pop(m_l.get(), 1);
    return true;
}

bool LuaTable::setobject(const char* name, const LuaRefObject& lobj) {
    pushself();

    bool ok = pushobject(lobj);
    if (ok) {
        lua_setfield(m_l.get(), -2, name);
    }
    lua_pop(m_l.get(), 1);

    return ok;
}

bool LuaTable::set(const char* name, const LuaObject& lobj) {
    return setobject(name, lobj);
}

bool LuaTable::set(const char* name, const LuaTable& ltable) {
    return setobject(name, ltable);
}

bool LuaTable::set(const char* name, const LuaFunction& lfunc) {
    return setobject(name, lfunc);
}

bool LuaTable::set(const char* name, const LuaUserdata& lud) {
    return setobject(name, lud);
}

bool LuaTable::foreach(const function<bool (const LuaObject& key,
                                            const LuaObject& value)>& func) const {
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

bool LuaFunction::pusharg(const char* str) {
    lua_pushstring(m_l.get(), str);
    return true;
}

bool LuaFunction::pusharg(lua_Number num) {
    lua_pushnumber(m_l.get(), num);
    return true;
}

bool LuaFunction::pusharg(const LuaObject& lobj) {
    return pushobject(lobj);
}

bool LuaFunction::pusharg(const LuaTable& ltable) {
    return pushobject(ltable);
}

bool LuaFunction::pusharg(const LuaFunction& lfunc) {
    return pushobject(lfunc);
}

bool LuaFunction::pusharg(const LuaUserdata& lud) {
    return pushobject(lud);
}

/* ------------------------------------------------------------------------- */

bool LuaState::set(const char* name, const char* str) {
    lua_pushstring(m_l.get(), str);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::set(const char* name, const char* str, size_t len) {
    lua_pushlstring(m_l.get(), str, len);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::set(const char* name, lua_Number value) {
    lua_pushnumber(m_l.get(), value);
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::setobject(const char* name, const LuaRefObject& lobj) {
    if (m_l != lobj.m_l) {
        return false;
    }

    lobj.pushself();
    lua_setglobal(m_l.get(), name);
    return true;
}

bool LuaState::set(const char* name, const LuaObject& lobj) {
    return setobject(name, lobj);
}

bool LuaState::set(const char* name, const LuaTable& ltable) {
    return setobject(name, ltable);
}

bool LuaState::set(const char* name, const LuaFunction& lfunc) {
    return setobject(name, lfunc);
}

bool LuaState::set(const char* name, const LuaUserdata& lud) {
    return setobject(name, lud);
}

LuaObject LuaState::get(const char* name) const {
    lua_getglobal(m_l.get(), name);
    LuaObject ret(m_l, -1);
    lua_pop(m_l.get(), 1);

    return ret;
}

LuaTable LuaState::newtable(const char* name) {
    lua_newtable(m_l.get());
    LuaTable ret(m_l, -1);

    if (name) {
        lua_setglobal(m_l.get(), name);
    } else {
        lua_pop(m_l.get(), 1);
    }

    return ret;
}

bool LuaState::dostring(const char* chunk, string* errstr, int nresults,
                        const function<bool (int, const LuaObject&)>& resfunc) {
    bool ok = (luaL_dostring(m_l.get(), chunk) == 0);
    if (ok) {
        if (nresults > 0) {
            if (resfunc) {
                for (int i = nresults; i > 0; --i) {
                    if (!resfunc(nresults - i, LuaObject(m_l, -i))) {
                        break;
                    }
                }
            }

            lua_pop(m_l.get(), nresults);
        }
    } else {
        if (errstr) {
            *errstr = lua_tostring(m_l.get(), -1);
        }
        lua_pop(m_l.get(), 1);
    }

    return ok;
}

bool LuaState::dofile(const char* script, string* errstr, int nresults,
                      const function<bool (int, const LuaObject&)>& resfunc) {
    bool ok = (luaL_dofile(m_l.get(), script) == 0);
    if (ok) {
        if (nresults > 0) {
            if (resfunc) {
                for (int i = nresults; i > 0; --i) {
                    if (!resfunc(nresults - i, LuaObject(m_l, -i))) {
                        break;
                    }
                }
            }

            lua_pop(m_l.get(), nresults);
        }
    } else {
        if (errstr) {
            *errstr = lua_tostring(m_l.get(), -1);
        }
        lua_pop(m_l.get(), 1);
    }

    return ok;
}

}
