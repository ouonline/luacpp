#ifndef __LUA_CPP_LUA_REF_OBJECT_H__
#define __LUA_CPP_LUA_REF_OBJECT_H__

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "func_utils.h"
#include <utility> // std::move

namespace luacpp {

class LuaRefObject {
public:
    LuaRefObject(lua_State* l) : m_l(l), m_type(LUA_TNIL), m_ref_index(LUA_REFNIL) {}

    LuaRefObject(lua_State* l, int index) {
        m_l = l;
        m_type = lua_type(l, index);
        lua_pushvalue(l, index);
        m_ref_index = luaL_ref(l, LUA_REGISTRYINDEX);
    }

    LuaRefObject(LuaRefObject&& rhs) {
        MoveFunc(std::move(rhs));
    }

    LuaRefObject(const LuaRefObject& rhs) {
        m_l = rhs.m_l;
        m_type = rhs.m_type;
        PushValue(m_l, rhs);
        m_ref_index = luaL_ref(m_l, LUA_REGISTRYINDEX);
    }

    LuaRefObject& operator=(LuaRefObject&& rhs) {
        if (&rhs == this) {
            return *this;
        }

        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }
        MoveFunc(std::move(rhs));
        return *this;
    }

    LuaRefObject& operator=(const LuaRefObject& rhs) {
        if (&rhs == this) {
            return *this;
        }

        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }

        m_l = rhs.m_l;
        m_type = rhs.m_type;
        PushValue(m_l, rhs);
        m_ref_index = luaL_ref(m_l, LUA_REGISTRYINDEX);
        return *this;
    }

    virtual ~LuaRefObject() {
        // m_l is nullptr if object was moved to another object
        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }
    }

    int GetType() const {
        return m_type;
    }
    const char* GetTypeName() const {
        return lua_typename(m_l, m_type);
    }
    int GetRefIndex() const {
        return m_ref_index;
    }

protected:
    void PushSelf() const {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_ref_index);
    }

private:
    void MoveFunc(LuaRefObject&& rhs) {
        m_l = rhs.m_l;
        m_type = rhs.m_type;
        m_ref_index = rhs.m_ref_index;

        rhs.m_l = nullptr;
        rhs.m_type = LUA_TNIL;
        rhs.m_ref_index = LUA_REFNIL;
    }

protected:
    lua_State* m_l;

private:
    int m_type;
    int m_ref_index;
};

} // namespace luacpp

#endif
