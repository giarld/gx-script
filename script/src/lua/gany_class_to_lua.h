//
// Created by Gxin on 2023/5/9.
//

#ifndef GX_SCRIPT_GANY_CLASS_TO_LUA_H
#define GX_SCRIPT_GANY_CLASS_TO_LUA_H

#include <lua.hpp>


/**
 * @class GAnyClassToLua
 * @brief Bind GAnyClass to Lua
 */
class GAnyClassToLua
{
public:
    static void toLua(lua_State *L);

private:
    static int regClass(lua_State *L);
};

#endif //GX_SCRIPT_GANY_CLASS_TO_LUA_H
