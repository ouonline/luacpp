#ifndef __LUA_CPP_LUA_REF_OBJECT_H__
#define __LUA_CPP_LUA_REF_OBJECT_H__

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include <memory>

namespace luacpp {

class LuaRefObject {
public:
    LuaRefObject(const std::shared_ptr<lua_State>& l, int index) {
        m_l = l;
        m_type = lua_type(l.get(), index);

        lua_pushvalue(l.get(), index);
        m_index = luaL_ref(l.get(), LUA_REGISTRYINDEX);
    }

    LuaRefObject(LuaRefObject&& rhs) {
        MoveFunc(std::move(rhs));
    }

    LuaRefObject& operator=(LuaRefObject&& rhs) {
        auto l = m_l.get();
        if (l) {
            luaL_unref(l, LUA_REGISTRYINDEX, m_index);
        }
        MoveFunc(std::move(rhs));
        return *this;
    }

    LuaRefObject(const LuaRefObject&) = delete;
    LuaRefObject& operator=(const LuaRefObject&) = delete;

    virtual ~LuaRefObject() {
        // m_l is nullptr if object was moved to another object
        if (m_l.get()) {
            luaL_unref(m_l.get(), LUA_REGISTRYINDEX, m_index);
        }
    }

    int GetType() const {
        return m_type;
    }
    const char* GetTypeStr() const {
        return lua_typename(m_l.get(), m_type);
    }

    void PushSelf() const {
        lua_rawgeti(m_l.get(), LUA_REGISTRYINDEX, m_index);
    }

private:
    void MoveFunc(LuaRefObject&& rhs) {
        m_l = std::move(rhs.m_l);
        m_index = rhs.m_index;
        m_type = rhs.m_type;

        rhs.m_index = 0;
        rhs.m_type = LUA_TNIL;
    }

protected:
    std::shared_ptr<lua_State> m_l;

private:
    size_t m_index;
    int m_type;
};

}

#endif
