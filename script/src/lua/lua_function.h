//
// Created by Gxin on 2023/5/8.
//

#ifndef GX_SCRIPT_LUA_FUNCTION_H
#define GX_SCRIPT_LUA_FUNCTION_H

#include <gx/gbytearray.h>

#include <memory>

#include <lua.hpp>


class GAnyLuaImpl;

/**
 * @brief Wrapping Lua functions to assist in persisting and multithreading Lua functions
 */
class LuaFunction
{
public:
    /**
     * @brief Constructor
     * @param L     Current Lua State
     * @param idx   Index of functions that need to be persisted in Lua stack
     */
    explicit LuaFunction(lua_State *L, int idx);

    ~LuaFunction();

    LuaFunction(const LuaFunction &) = delete;

    LuaFunction(LuaFunction &&b) noexcept = delete;

    LuaFunction &operator=(const LuaFunction &) = delete;

    LuaFunction &operator=(LuaFunction &&b) noexcept = delete;

public:
    /**
     * @brief Determine whether the current Lua function is valid
     *        (whether it corresponds to the current LuaState and whether the function exists)
     * @return
     */
    bool valid() const;

    /**
     * @brief Determine whether the Lua vm to which the current function belongs is the Lua vm of the current thread
     * @return
     */
    bool checkVM() const;

    /**
     * @brief Push the current Lua function onto the stack
     * @param L
     */
    void push(lua_State *L) const;

    /**
     * @brief Dump current lua function into bytecode
     * @return
     */
    GByteArray dump() const;

    /**
     * @brief Get information about functions
     * @return
     */
    GAny getInfo() const;

private:
    friend struct GLuaFunctionRef;

    std::weak_ptr<GAnyLuaImpl> mLuaVM;
    int mFunRef = 0;
};

/**
 * @brief Reference to Lua functions held and managed by GAny for lifecycle
 */
struct GLuaFunctionRef
{
    GByteArray byteCode;
    std::weak_ptr<LuaFunction> func;

    ~GLuaFunctionRef();
};

#endif //GX_SCRIPT_LUA_FUNCTION_H
