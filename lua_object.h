#ifndef __LUA_CPP_LUA_OBJECT_H__
#define __LUA_CPP_LUA_OBJECT_H__

#include "lua_ref_object.h"
#include "lua_string_ref.h"

namespace luacpp {

class LuaTable;
class LuaFunction;
class LuaUserData;

class LuaObject final : public LuaRefObject {
public:
    LuaObject(const std::shared_ptr<lua_State>& l, int index) : LuaRefObject(l, index) {}

    LuaObject(LuaObject&&) = default;
    LuaObject& operator=(LuaObject&&) = default;
    LuaObject(const LuaObject&) = delete;
    LuaObject& operator=(const LuaObject&) = delete;

    bool IsNil() const {
        return GetType() == LUA_TNIL;
    }
    bool IsBoolean() const {
        return GetType() == LUA_TBOOLEAN;
    }
    bool IsNumber() const {
        return GetType() == LUA_TNUMBER;
    }
    bool IsString() const {
        return GetType() == LUA_TSTRING;
    }
    bool IsTable() const {
        return GetType() == LUA_TTABLE;
    }
    bool IsFunction() const {
        return GetType() == LUA_TFUNCTION;
    }
    bool IsUserData() const {
        return GetType() == LUA_TUSERDATA;
    }
    bool IsThread() const {
        return GetType() == LUA_TTHREAD;
    }
    bool IsLightUserData() const {
        return GetType() == LUA_TLIGHTUSERDATA;
    }

    template <typename T>
    T ToInteger() const {
        PushSelf();
        auto ret = (T)lua_tointeger(m_l.get(), -1);
        lua_pop(m_l.get(), 1);
        return ret;
    }

    bool ToBool() const;
    lua_Number ToNumber() const;
    LuaStringRef ToString() const;
    LuaTable ToTable() const;
    LuaFunction ToFunction() const;
    LuaUserData ToUserData() const;

private:
    template <typename T>
    T ToTypedObject() const {
        PushSelf();
        T ret(m_l, -1);
        lua_pop(m_l.get(), 1);
        return ret;
    }
};

}

#endif
