#ifndef __LUA_CPP_LUA_OBJECT_H__
#define __LUA_CPP_LUA_OBJECT_H__

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "lua_buffer_ref.h"
#include <utility> // std::move

namespace luacpp {

class LuaTable;
class LuaFunction;
class LuaUserData;

class LuaObject {
public:
    LuaObject(lua_State* l) : m_l(l), m_type(LUA_TNIL), m_ref_index(LUA_REFNIL) {}

    LuaObject(lua_State* l, int index) {
        m_l = l;
        m_type = lua_type(l, index);
        lua_pushvalue(l, index);
        m_ref_index = luaL_ref(l, LUA_REGISTRYINDEX);
    }

    LuaObject(LuaObject&& rhs) {
        MoveFunc(std::move(rhs));
    }

    LuaObject(const LuaObject& rhs) {
        m_l = rhs.m_l;
        m_type = rhs.m_type;
        rhs.PushSelf();
        m_ref_index = luaL_ref(m_l, LUA_REGISTRYINDEX);
    }

    LuaObject& operator=(LuaObject&& rhs) {
        if (&rhs == this) {
            return *this;
        }

        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }
        MoveFunc(std::move(rhs));
        return *this;
    }

    LuaObject& operator=(const LuaObject& rhs) {
        if (&rhs == this) {
            return *this;
        }

        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }

        m_l = rhs.m_l;
        m_type = rhs.m_type;
        rhs.PushSelf();
        m_ref_index = luaL_ref(m_l, LUA_REGISTRYINDEX);
        return *this;
    }

    virtual ~LuaObject() {
        // m_l is nullptr if object was moved to another object
        if (m_l) {
            luaL_unref(m_l, LUA_REGISTRYINDEX, m_ref_index);
        }
    }

    int Type() const {
        return m_type;
    }
    const char* TypeName() const {
        return lua_typename(m_l, m_type);
    }

    template <typename T>
    T ToInteger() const {
        PushSelf();
        auto ret = (T)lua_tointeger(m_l, -1);
        lua_pop(m_l, 1);
        return ret;
    }

    bool ToBool() const;
    lua_Number ToNumber() const;
    LuaBufferRef ToBufferRef() const;

    void PushSelf() const {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, m_ref_index);
    }

private:
    void MoveFunc(LuaObject&& rhs) {
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

}

#endif
