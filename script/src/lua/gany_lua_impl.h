//
// Created by Gxin on 2023/5/7.
//

#ifndef GX_SCRIPT_GANY_LUA_IMPL_H
#define GX_SCRIPT_GANY_LUA_IMPL_H

#include "gx/gany_lua.h"

#include <gx/gmutex.h>

#include <lua.hpp>


#define glua_getcppobject(L, CLASS, i)  (lua_isuserdata(L, i) ? *(CLASS**)lua_touserdata(L, i) : nullptr)

class LuaFunction;

struct UpValueItem
{
    int upIdx{};
    GAny val;
    bool luaType{};
};

class GAnyLuaImpl : public GAnyLua
{
public:
    explicit GAnyLuaImpl();

    ~GAnyLuaImpl() override;

    lua_State *getLuaState() const;

    void shutdown() override;

    void shutdownImpl();

public:
    GAny requireLs(const std::string &name, const GAny &env) override;

    GAny eval(const std::string &script, std::string sourcePath, const GAny &env) override;

    GAny evalFile(const std::string &filePath, const GAny &env) override;

    GAny evalBuffer(const GByteArray &buffer, std::string sourcePath, const GAny &env) override;

    void gc() override;

    bool gcStep(int32_t kb) override;

    int32_t gcSetStepMul(int32_t mul) override;

    int32_t gcSetPause(int32_t pause) override;

    void gcStop() override;

    void gcRestart() override;

    bool gcIsRunning() override;

    int32_t gcGetCount() override;

    void gcModeGen() override;

    void gcModeInc() override;

    bool operator==(const GAnyLuaImpl &rhs) const
    {
        return this->mL == rhs.mL;
    }

    /**
     * @brief Set debugging mode
     * @param debugMode
     */
    static void setDebugMode(bool debugMode);

    /**
     * @brief   Set up a script reader. If a custom script reader is set up,
     *          the custom reader will be called when using "scriptFile" and "requireLs" to read the script file.
     * @param reader
     */
    static void setScriptReader(ScriptReader reader);

    static void addSearchPath(const std::string &path);

    static void removeSearchPath(const std::string &path);

public:
    GByteArray compileCode(const std::string &code, std::string sourcePath, bool strip) override;

    GByteArray compileFile(const std::string &filePath, bool strip) override;

private:
    GAny loadScriptFromBuffer(const GByteArray &buffer, const std::string &sourcePath, const GAny &env) const;

    void addLFunctionRef(const std::shared_ptr<LuaFunction> &ref);

    void removeLFunctionRef(const std::shared_ptr<LuaFunction> &ref);

public:    /// Tools
    /**
     * @brief Place a GAny object on the specified Lua stack
     * @param L
     * @param v
     */
    static void pushGAny(lua_State *L, const GAny &v);

    /**
     * @brief Find the up value of the specified name from the specified function on the specified Lua stack
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static int findUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Obtain the up value of the specified Lua function, return true if successful,
     *        and place the up value at the top of the stack
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static bool getUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Set the top finger of the stack to the specified upper value
     * @param L
     * @param funcIdx
     * @param name
     * @return
     */
    static bool setUpValue(lua_State *L, int funcIdx, const char *name);

    /**
     * @brief Set the specified env(G) to the env(G) of the specified Lua function
     * @param L
     * @param env
     * @param funcIdx
     */
    static void setEnvironment(lua_State *L, const GAny &env, int funcIdx);

    /**
     * @brief Get env(G) of the specified Lua function
     * @param L
     * @param funcIdx
     * @return
     */
    static GAny getEnvironment(lua_State *L, int funcIdx);

    /**
     * @brief Dump all up values except for global variables
     * @param L
     * @param funcIdx
     * @return
     */
    static std::vector<UpValueItem> dumpUpValue(lua_State *L, int funcIdx);

    /**
     * @brief Fill the function's up value table with the up value held
     * @param L
     * @param funcIdx
     * @param upValues
     */
    static void storeUpValue(lua_State *L, int funcIdx, const std::vector<UpValueItem> &upValues);

    /**
     * @brief Make Lua function as GAny function
     * @param L
     * @param idx
     * @return
     */
    static GAny makeLuaFunctionToGAny(lua_State *L, int idx);

    /**
     * @brief Make Lua object as GAny object
     * @param L
     * @param idx
     * @return
     */
    static GAny makeLuaObjectToGAny(lua_State *L, int idx);

    /**
     * @brief Make GAny object as Lua object
     * @param L
     * @param value
     * @param useGAnyTable
     * @return
     */
    static void makeGAnyToLuaObject(lua_State *L, const GAny &value, bool useGAnyTable = false);

    /**
     * @brief Determine whether the corresponding Lua object is a GAny object
     * @param L
     * @param idx
     * @return
     */
    static bool isGAnyLuaObj(lua_State *L, int idx);

    static GAny getGAnyClassDB();

private:
    GByteArray compile(const GByteArray &buffer, const std::string &sourcePath, bool strip) const;

private:
    friend struct GLuaFunctionRef;

    lua_State *mL = nullptr;

    std::vector<std::shared_ptr<LuaFunction>> mLFuncs;
    GMutex mFuncsLock;

    static ScriptReader sScriptReader;
    static std::set<std::string> sSearchPaths;

    static bool sDebugMode;
};

#endif //GX_SCRIPT_GANY_LUA_IMPL_H
