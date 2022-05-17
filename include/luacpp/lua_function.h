#ifndef __LUA_CPP_LUA_FUNCTION_H__
#define __LUA_CPP_LUA_FUNCTION_H__

#include "lua_object.h"
#include "func_utils.h"
#include <string>

namespace luacpp {

class LuaFunction final : public LuaRefObject {
public:
    LuaFunction(lua_State* l, int index) : LuaRefObject(l, index) {}
    LuaFunction(LuaObject&& lobj) : LuaRefObject(std::move(lobj)) {}
    LuaFunction(const LuaObject& lobj) : LuaRefObject(lobj) {}
    LuaFunction(LuaFunction&&) = default;
    LuaFunction(const LuaFunction&) = default;

    LuaFunction& operator=(LuaFunction&&) = default;
    LuaFunction& operator=(const LuaFunction&) = default;

    /** i starts from 0 */
    template <typename... Argv>
    bool Execute(const std::function<bool(uint32_t i, const LuaObject&)>& callback = nullptr,
                 std::string* errstr = nullptr, Argv&&... argv) {
        PushSelf();
        PushValues(m_l, std::forward<Argv>(argv)...);
        return Invoke(callback, sizeof...(Argv), errstr);
    }

private:
    // i starts from 0
    bool Invoke(const std::function<bool(uint32_t i, const LuaObject)>& callback, int argc, std::string* errstr) {
        const int top = lua_gettop(m_l) - argc - 1 /* the function itself */;

        bool ok = (lua_pcall(m_l, argc, LUA_MULTRET, 0) == LUA_OK);
        if (!ok) {
            if (errstr) {
                *errstr = lua_tostring(m_l, -1);
            }
            lua_pop(m_l, 1);
            return false;
        }

        if (!callback) {
            return true;
        }

        const int nresults = lua_gettop(m_l) - top;
        for (int i = nresults; i > 0; --i) {
            if (!callback(nresults - i, LuaObject(m_l, -i))) {
                break;
            }
        }

        if (nresults > 0) {
            lua_pop(m_l, nresults);
        }

        return true;
    }
};

} // namespace luacpp

#endif
