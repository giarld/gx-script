//
// Created by Gxin on 2023/5/8.
//

#include "lua_function.h"

#include "gany_lua_impl.h"

#include <gx/debug.h>


int luaDumpWriter(lua_State *, const void *p, size_t sz, void *ud)
{
    GByteArray &buff = *static_cast<GByteArray *>(ud);
    buff.write(p, static_cast<int64_t>(sz));
    return 0;
}

LuaFunction::LuaFunction(lua_State *L, int idx)
    : mLuaVM(std::dynamic_pointer_cast<GAnyLuaImpl>(GAnyLua::threadLocal()))
{
    if (!lua_isfunction(L, idx)) {
        mLuaVM.reset();
        return;
    }
    lua_pushvalue(L, idx);
    mFunRef = luaL_ref(L, LUA_REGISTRYINDEX);
}

LuaFunction::~LuaFunction()
{
    const auto vm = mLuaVM.lock();
    if (vm && mFunRef != 0) {
        luaL_unref(vm->getLuaState(), LUA_REGISTRYINDEX, mFunRef);
    }
}

bool LuaFunction::valid() const
{
    return !mLuaVM.expired() && mFunRef != 0;
}

bool LuaFunction::checkVM() const
{
    return mLuaVM.lock() == GAnyLua::threadLocal();
}

void LuaFunction::push(lua_State *L) const
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, mFunRef);
    GX_ASSERT(lua_isfunction(L, -1));
}

GByteArray LuaFunction::dump() const
{
    const auto vm = mLuaVM.lock();
    if (!vm) {
        return GByteArray();
    }
    lua_State *L = vm->getLuaState();
    push(L);
    GByteArray buff;
    const int ok = lua_dump(L, luaDumpWriter, &buff, false);
    GX_ASSERT_S(ok == LUA_OK, "Dump lua function failure.");
    lua_pop(L, 1);
    return buff;
}

GAny LuaFunction::getInfo() const
{
    const auto vm = mLuaVM.lock();
    if (!vm) {
        return GByteArray();
    }
    lua_State *L = vm->getLuaState();

    GAny info;

    lua_getglobal(L, "debug");
    lua_pushstring(L, "getinfo");
    lua_gettable(L, -2);
    if (lua_isfunction(L, -1)) {
        push(L); // arg
        if (lua_pcall(L, 1, 1, 0) == LUA_OK && lua_istable(L, -1)) {
            lua_pushstring(L, "short_src");
            lua_gettable(L, -2);
            if (lua_isstring(L, -1)) {
                info["src"] = std::string(lua_tostring(L, -1));
            }
            lua_pop(L, 1);

            lua_pushstring(L, "linedefined");
            lua_gettable(L, -2);
            if (lua_isinteger(L, -1)) {
                info["linedefined"] = lua_tointeger(L, -1);
            }
            lua_pop(L, 1);

            lua_pushstring(L, "lastlinedefined");
            lua_gettable(L, -2);
            if (lua_isinteger(L, -1)) {
                info["lastlinedefined"] = lua_tointeger(L, -1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return info;
}

GLuaFunctionRef::~GLuaFunctionRef()
{
    const auto funcRef = func.lock();
    if (funcRef) {
        const auto vm = funcRef->mLuaVM.lock();
        if (vm) {
            vm->removeLFunctionRef(funcRef);
        }
    }
}
