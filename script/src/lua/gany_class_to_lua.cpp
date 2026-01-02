//
// Created by Gxin on 2023/5/9.
//

#include "gany_class_to_lua.h"

#include "lua_table.h"
#include "gany_lua_impl.h"


void GAnyClassToLua::toLua(lua_State *L)
{
    constexpr luaL_Reg staticMethods[] = {
        {"Class", regClass},
        {nullptr, nullptr}
    };

    lua_newtable(L);
    const int top = lua_gettop(L);

    for (const luaL_Reg *f = staticMethods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }

    lua_setglobal(L, "GAnyClass");

    lua_pop(L, lua_gettop(L));
}

int GAnyClassToLua::regClass(lua_State *L)
{
    const int n = lua_gettop(L);
    if (n == 1) {
        if (!lua_istable(L, 1)) {
            luaL_error(L, "Call GAnyClass Create error: The parameter must be a table.");
            return 0;
        }

        const auto defObject = LuaTable(L, 1).toObject();
        if (!defObject.isObject()) {
            luaL_error(L, "Call GAnyClass Create error: Table is not a valid structure.");
            return 0;
        }

        const auto clazz = GAnyClass::createFromGAnyObject(defObject);
        if (!clazz) {
            luaL_error(L, "Call GAnyClass Create error: Create class failed.");
            return 0;
        }

        GAnyLuaImpl::pushGAny(L, GAny(clazz));
        return 1;
    }

    luaL_error(L, "Call GAnyClass Create error: unsupported overloaded usage.");
    return 0;
}
