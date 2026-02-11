//
// Created by Gxin on 2023/5/7.
//

#include "gany_lua_impl.h"

#include "lua_function.h"
#include "lua_table.h"

#include "gany_to_lua.h"
#include "gany_class_to_lua.h"

#include <gx/gfile.h>
#include <gx/debug.h>

#include <math.h>

#include <set>
#include <utility>


extern "C"
{
#include "lobject.h"
#include "lstate.h"
}

#define HANDLE_EXCEPTION(e) \
    do {                    \
        std::string exception = std::string("GAnyLua Exception: ") + (e ? e : "??"); \
        return GAnyException(exception);  \
    } while(false)

GAnyLuaImpl::ScriptReader GAnyLuaImpl::sScriptReader = nullptr;

std::set<std::string> GAnyLuaImpl::sSearchPaths = {};

bool GAnyLuaImpl::sDebugMode = false;


GAnyLuaImpl::GAnyLuaImpl()
{
    const GAny classDB = getGAnyClassDB();
    CHECK_CONDITION_S_V(classDB.isUserObject(), "Failed to get GAny ClassDB");

    mL = luaL_newstate();
    luaL_openlibs(mL);

    LuaUserData* userData = new LuaUserData;
    *(void**)lua_getextraspace(mL) = userData;
    userData->ganyModuleIdx = classDB.call("assignModuleIdx").toInt64();
    CHECK_CONDITION_V(userData->ganyModuleIdx > 0);

    GAnyToLua::toLua(mL);
    GAnyClassToLua::toLua(mL);
}

GAnyLuaImpl::~GAnyLuaImpl()
{
    shutdownImpl();
}

lua_State *GAnyLuaImpl::getLuaState() const
{
    return mL;
}

void GAnyLuaImpl::shutdown()
{
    shutdownImpl();
}

void GAnyLuaImpl::shutdownImpl()
{
    //
    {
        GLockerGuard locker(mFuncsLock);
        mLFuncs.clear();
    }
    if (mL) {
        const GAny classDB = getGAnyClassDB();
        CHECK_CONDITION_S_V(classDB.isUserObject(), "Failed to get GAny ClassDB");

        void* storedData = *(void**)lua_getextraspace(mL);
        LuaUserData* userData = (LuaUserData*)storedData;
        classDB.call("releaseModule", userData->ganyModuleIdx);
        delete userData;

        lua_close(mL);
        mL = nullptr;
    }
}

GAny GAnyLuaImpl::requireLs(const std::string &name, const GAny &env)
{
    std::string path = name;
    const GFile file = [&] {
        if (sSearchPaths.empty()) {
            sSearchPaths.insert(GFile::mainDirectory().filePath());
        }

        for (const auto &p: sSearchPaths) {
            GFile dir(p);
            if (!dir.isDirectory()) {
                continue;
            }
            GFile f(dir, name);
            if (f.exists() && f.isFile()) {
                return f;
            }
            f = GFile(dir, name + ".lsc");
            if (f.exists() && f.isFile()) {
                return f;
            }
            f = GFile(dir, name + ".lua");
            if (f.exists() && f.isFile()) {
                return f;
            }
        }

        return GFile();
    }();
    if (!file.exists()) {
        if (!sScriptReader) {
            LogE("requireLs: {} is not found", name);
            return GAny::undefined();
        }
        path = name;
    } else {
        path = file.absoluteFilePath();
    }

    return evalFile(path, env);
}

GAny GAnyLuaImpl::eval(const std::string &script, std::string sourcePath, const GAny &env)
{
    if (sourcePath.empty()) {
        GString scriptStr = script;
        // Ensure that UTF-8 characters are not truncated
        if (scriptStr.count() > 512) {
            scriptStr = scriptStr.left(512) + "...";
        }
        sourcePath = scriptStr.toStdString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    GByteArray buffer;
    buffer.write(script.data(), script.size());
    return loadScriptFromBuffer(buffer, sourcePath, env);
}

GAny GAnyLuaImpl::evalFile(const std::string &filePath, const GAny &env)
{
    GByteArray buffer;
    if (sScriptReader) {
        buffer = sScriptReader(filePath);
    } else {
        GFile file(filePath);

        if (!file.exists()) {
            return GAnyException("Run lua script error: file(" + filePath + ") does not exist.");
        }

        if (file.open(GFile::ReadOnly | GFile::Binary)) {
            buffer = file.read();
            file.close();
        } else {
            return GAnyException("Open file failure.");
        }
    }

    if (!buffer.isEmpty()) {
        return loadScriptFromBuffer(buffer, "@" + filePath, env);
    }

    return GAny::undefined();
}

GAny GAnyLuaImpl::evalBuffer(const GByteArray &buffer, std::string sourcePath, const GAny &env)
{
    if (sourcePath.empty()) {
        sourcePath = "@buffer://" + GByteArray::md5Sum(buffer).toHexString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    return loadScriptFromBuffer(buffer, sourcePath, env);
}

void GAnyLuaImpl::gc()
{
    lua_gc(mL, LUA_GCCOLLECT, 0);
}

bool GAnyLuaImpl::gcStep(int32_t kb)
{
    return lua_gc(mL, LUA_GCSTEP, kb) != 0;
}

int32_t GAnyLuaImpl::gcSetStepMul(int32_t mul)
{
    return lua_gc(mL, LUA_GCSETSTEPMUL, mul);
}

int32_t GAnyLuaImpl::gcSetPause(int32_t pause)
{
    return lua_gc(mL, LUA_GCSETPAUSE, pause);
}

void GAnyLuaImpl::gcStop()
{
    lua_gc(mL, LUA_GCSTOP, 0);
}

void GAnyLuaImpl::gcRestart()
{
    lua_gc(mL, LUA_GCRESTART, 0);
}

bool GAnyLuaImpl::gcIsRunning()
{
    return lua_gc(mL, LUA_GCISRUNNING, 0) != 0;
}

int32_t GAnyLuaImpl::gcGetCount()
{
    return lua_gc(mL, LUA_GCCOUNT, 0);
}

void GAnyLuaImpl::gcModeGen()
{
    lua_gc(mL, LUA_GCGEN, 0);
}

void GAnyLuaImpl::gcModeInc()
{
    lua_gc(mL, LUA_GCINC, 0);
}

void GAnyLuaImpl::setDebugMode(bool debugMode)
{
    sDebugMode = debugMode;
}

void GAnyLuaImpl::setScriptReader(ScriptReader reader)
{
    sScriptReader = std::move(reader);
}

void GAnyLuaImpl::addSearchPath(const std::string &path)
{
    if (sSearchPaths.empty()) {
        sSearchPaths.insert(GFile::mainDirectory().filePath());
    }

    const GFile f(path);
    if (f.exists() && f.isDirectory()) {
        sSearchPaths.insert(path);
    }
}

void GAnyLuaImpl::removeSearchPath(const std::string &path)
{
    sSearchPaths.erase(path);
}


GAny GAnyLuaImpl::loadScriptFromBuffer(const GByteArray &buffer, const std::string &sourcePath, const GAny &env) const
{
    lua_State *L = mL;

    if (buffer.size() - buffer.readPos() > 4) {
        char head[4];
        buffer.read(head, 4);
        if (head[0] == static_cast<char>(0xff) && head[1] == 'l' && head[2] == 's' && head[3] == static_cast<char>(0xee)) {
            GByteArray data;
            buffer >> data;

            if (GByteArray::isCompressed(data)) {
                data = GByteArray::uncompress(data);
            }

            if (luaL_loadbuffer(
                L, reinterpret_cast<const char *>(data.data()),
                static_cast<size_t>(data.size()),
                sourcePath.c_str()) != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                HANDLE_EXCEPTION(err);
            }

            setEnvironment(L, env, lua_gettop(L));

            if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                const char *err = lua_tostring(L, -1);
                HANDLE_EXCEPTION(err);
            }
            GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
            lua_pop(L, 1);
            return ret;
        }
        buffer.seekReadPos(SEEK_CUR, -4);
    }

    if (luaL_loadbuffer(
        L, reinterpret_cast<const char *>(buffer.data()),
        static_cast<size_t>(buffer.size()),
        sourcePath.c_str()) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        HANDLE_EXCEPTION(err);
    }

    setEnvironment(L, env, lua_gettop(L));

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        HANDLE_EXCEPTION(err);
    }
    GAny ret = makeLuaObjectToGAny(L, lua_gettop(L));
    lua_pop(L, 1);
    return ret;
}

void GAnyLuaImpl::addLFunctionRef(const std::shared_ptr<LuaFunction> &ref)
{
    GLockerGuard locker(mFuncsLock);
    mLFuncs.push_back(ref);
}

void GAnyLuaImpl::removeLFunctionRef(const std::shared_ptr<LuaFunction> &ref)
{
    GLockerGuard locker(mFuncsLock);
    const auto fIt = std::find_if(
        mLFuncs.begin(), mLFuncs.end(),
        [&](const auto &v) {
            return v == ref;
        });
    if (fIt != mLFuncs.end()) {
        mLFuncs.erase(fIt);
    }
}

void GAnyLuaImpl::pushGAny(lua_State *L, const GAny &v)
{
    GAny *obj = GX_NEW(GAny, v);

    void **p = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
    *p = obj;
    luaL_getmetatable(L, "GAny");
    lua_setmetatable(L, -2);
}

int GAnyLuaImpl::findUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    bool hasEnv = false;
    int upIdx = 1;
    do {
        const char *envName = lua_getupvalue(L, funcIdx, upIdx);
        if (envName == nullptr) {
            break;
        }
        if (strcmp(envName, name) == 0) {
            lua_pop(L, 1);
            hasEnv = true;
            break;
        }
        lua_pop(L, 1);
        upIdx += 1;
    } while (true);

    return hasEnv ? upIdx : 0;
}

bool GAnyLuaImpl::getUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    const int upIdx = findUpValue(L, funcIdx, name);
    if (upIdx == 0) {
        return false;
    }
    lua_getupvalue(L, funcIdx, upIdx);
    return true;
}

bool GAnyLuaImpl::setUpValue(lua_State *L, int funcIdx, const char *name)
{
    GX_ASSERT(funcIdx > 0);
    const int upIdx = findUpValue(L, funcIdx, name);
    if (upIdx == 0) {
        return false;
    }
    const char *upName = lua_setupvalue(L, funcIdx, upIdx);
    if (upName == nullptr) {
        lua_pop(L, 1);
        return false;
    }
    return strcmp(upName, name) == 0;
}

void GAnyLuaImpl::setEnvironment(lua_State *L, const GAny &env, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    const int upIdx = findUpValue(L, funcIdx, "_ENV");
    if (upIdx == 0) {
        return;
    }

    const int bTop = lua_gettop(L);
    lua_newtable(L);
    // _G -> metadata.__index
    lua_newtable(L);
    int top = lua_gettop(L);
    lua_pushliteral(L, "__index");
    lua_getglobal(L, "_G");
    lua_settable(L, top);

    // _ENV
    lua_setmetatable(L, -2);
    top = lua_gettop(L);
    if (env.isObject()) {
        lua_pushliteral(L, "Env");
        pushGAny(L, env);
        lua_settable(L, top);

        const auto &envObj = *env.as<GAnyObject>();

        for (auto it = envObj.var.begin(); it != envObj.var.end(); ++it) {
            lua_pushstring(L, it->first.c_str());
            pushGAny(L, it->second);
            lua_settable(L, top);
        }
    }
    lua_setupvalue(L, funcIdx, upIdx);

    const int eTop = lua_gettop(L);
    if (eTop - bTop > 0) {
        lua_pop(L, eTop - bTop);
    }
}

GAny GAnyLuaImpl::getEnvironment(lua_State *L, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    const int upIdx = findUpValue(L, funcIdx, "_ENV");

    if (upIdx == 0) {
        return GAny::undefined();
    }
    lua_getupvalue(L, funcIdx, upIdx);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "Env");
        if (isGAnyLuaObj(L, -1)) {
            const GAny *obj = glua_getcppobject(L, GAny, -1);
            GAny lEnv = obj ? *obj : GAny::undefined();
            lua_pop(L, 2);
            return lEnv;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return GAny::undefined();
}

std::vector<UpValueItem> GAnyLuaImpl::dumpUpValue(lua_State *L, int funcIdx)
{
    GX_ASSERT(funcIdx > 0);
    std::vector<UpValueItem> upValues;
    int upIdx = 1;
    do {
        const char *name = lua_getupvalue(L, funcIdx, upIdx);
        if (name == nullptr) {
            break;
        }
        if (strcmp(name, "_ENV") == 0) {
            lua_pop(L, 1);
            upIdx += 1;
            continue;
        }

        if (isGAnyLuaObj(L, -1)) {
            upValues.push_back({upIdx, *glua_getcppobject(L, GAny, lua_gettop(L)), false});
        } else {
            upValues.push_back({upIdx, makeLuaObjectToGAny(L, lua_gettop(L)), true});
        }

        lua_pop(L, 1);
        upIdx += 1;
    } while (true);

    return upValues;
}

void GAnyLuaImpl::storeUpValue(lua_State *L, int funcIdx, const std::vector<UpValueItem> &upValues)
{
    GX_ASSERT(funcIdx > 0);
    for (const auto &item: upValues) {
        if (item.luaType) {
            makeGAnyToLuaObject(L, item.val);
        } else {
            pushGAny(L, item.val);
        }
        if (lua_setupvalue(L, funcIdx, item.upIdx) == nullptr) {
            LogW("GAnyLua, storeUpValue lua_setupvalue error, index: {}", item.upIdx);
            lua_pop(L, 1);
        }
    }
}

GAny GAnyLuaImpl::makeLuaFunctionToGAny(lua_State *L, int idx)
{
    /// Dump the G of a function
    std::weak_ptr<GAnyValue> lEnvRef = getEnvironment(L, idx).value();
    /// Dump the upper value of a function
    std::vector<UpValueItem> upValues = dumpUpValue(L, idx);

    /// Dump function
    auto vm = std::dynamic_pointer_cast<GAnyLuaImpl>(threadLocal());
    auto lFunc = std::make_shared<LuaFunction>(L, idx);
    vm->addLFunctionRef(lFunc);

    auto funcRef = std::make_shared<GLuaFunctionRef>();
    funcRef->byteCode = lFunc->dump();
    funcRef->func = lFunc;

    std::stringstream fns;
    fns << "LuaFunction<" << std::hex << lFunc.get() << ">";

    std::string fn = fns.str();
    /// Build GAnyFunction, which will proxy the call from GAny to Lua function
    GAnyFunction func = GAnyFunction::createVariadicFunction(
        fn,
        [funcRef, lEnvRef, fn, upValues](const GAny **args, int32_t argc) -> GAny {
            const auto tVM = std::dynamic_pointer_cast<GAnyLuaImpl>(threadLocal());
            if (!tVM) {
                HANDLE_EXCEPTION("Failed to get thread local lua vm!");
            }

            lua_State *LL = tVM->getLuaState();

            const auto tLFunc = funcRef->func.lock();
            const auto lEnv = lEnvRef.lock();

            while (tLFunc) {
                /// If the current thread is the thread created by the Lua function, call it directly
                if (!tLFunc->checkVM()) {
                    break;
                }

                tLFunc->push(LL);
                if (lEnv) {
                    setEnvironment(LL, GAny::fromValue(lEnv), lua_gettop(LL));
                }

                /// Fill arguments
                for (int32_t i = 0; i < argc; i++) {
                    makeGAnyToLuaObject(LL, *args[i]);
                }

                /// Call
                if (lua_pcall(LL, argc, 1, 0) != LUA_OK) {
                    const char *err = lua_tostring(LL, -1);
                    HANDLE_EXCEPTION(err);
                }

                /// Conversion return value
                GAny ret = makeLuaObjectToGAny(LL, lua_gettop(LL));
                lua_pop(LL, 1);
                return ret;
            }

            /// If the current thread is not the thread where the Lua function was created,
            /// call the function from the function bytecode.
            if (luaL_loadbuffer(
                LL,
                reinterpret_cast<const char *>(funcRef->byteCode.data()),
                static_cast<size_t>(funcRef->byteCode.size()),
                fn.c_str()) != LUA_OK) {
                const char *err = lua_tostring(LL, -1);
                HANDLE_EXCEPTION(err);
            }

            if (lEnv) {
                setEnvironment(LL, GAny::fromValue(lEnv), lua_gettop(LL));
            }
            /// Calling a function from bytecode requires filling in its up value
            storeUpValue(LL, lua_gettop(LL), upValues);

            /// Fill arguments
            for (int32_t i = 0; i < argc; i++) {
                makeGAnyToLuaObject(LL, *args[i]);
            }

            /// Call
            if (lua_pcall(LL, argc, 1, 0) != LUA_OK) {
                const char *err = lua_tostring(LL, -1);
                HANDLE_EXCEPTION(err);
            }

            /// Conversion return value
            GAny ret = makeLuaObjectToGAny(LL, lua_gettop(LL));
            lua_pop(LL, 1);
            return ret;
        });
    if (sDebugMode) {
        func.setBoundData(lFunc->getInfo());
    }

    return func;
}

constexpr double EPS = 1e-6;

GAny GAnyLuaImpl::makeLuaObjectToGAny(lua_State *L, int idx)
{
    GX_ASSERT(idx > 0);
    const int type = lua_type(L, idx);
    switch (type) {
        default:
        case LUA_TNONE:
            return GAny::undefined();
        case LUA_TNIL:
            return GAny::null();
        case LUA_TBOOLEAN:
            return !!(lua_toboolean(L, idx));
        case LUA_TLIGHTUSERDATA:
            return GAny::undefined();
        case LUA_TNUMBER: {
            const double num = lua_tonumber(L, idx);
            if (num - std::floor(num) < EPS) {
                return static_cast<int64_t>(num);
            }
            return num;
        }
        case LUA_TSTRING:
            return std::string(lua_tostring(L, idx));
        case LUA_TTABLE: {
            thread_local std::set<intptr_t> tableStack;
            const auto ptr = reinterpret_cast<intptr_t>(lua_topointer(L, idx));
            if (tableStack.contains(ptr)) {
                tableStack.clear();
                std::stringstream exss;
                exss << "LuaTable has a circular reference at 0x" << std::uppercase << std::hex << ptr;
                return GAnyException(exss.str());
            }
            tableStack.insert(ptr);
            auto table = std::make_shared<LuaTable>(L, idx);
            tableStack.erase(ptr);
            return table;
        }
        case LUA_TFUNCTION:
            return makeLuaFunctionToGAny(L, idx);
        case LUA_TUSERDATA:
            GAny *obj = glua_getcppobject(L, GAny, idx);
            return obj ? *obj : GAny::null();
    }
}

void GAnyLuaImpl::makeGAnyToLuaObject(lua_State *L, const GAny &value, bool useGAnyTable)
{
    switch (value.type()) {
        case AnyType::undefined_t:
        case AnyType::null_t: {
            lua_pushnil(L);
            return;
        }
        case AnyType::boolean_t: {
            lua_pushboolean(L, *value.as<bool>());
            return;
        }
        case AnyType::int_t: {
            const int64_t v = value.castAs<int64_t>();
            if (v >= INT32_MIN && v <= INT32_MAX) {
                lua_pushinteger(L, static_cast<int>(v));
            } else {
                lua_pushnumber(L, static_cast<double>(v));
            }
            return;
        }
        case AnyType::float_t: {
            lua_pushnumber(L, *value.as<float>());
            return;
        }
        case AnyType::double_t: {
            lua_pushnumber(L, *value.as<double>());
            return;
        }
        case AnyType::string_t: {
            lua_pushstring(L, value.as<std::string>()->c_str());
            return;
        }
        default:
            break;
    }

    if (!useGAnyTable && value.isUserObject() && value.is<LuaTable>()) {
        value.as<LuaTable>()->push(L);
        return;
    }

    pushGAny(L, value);
}

bool GAnyLuaImpl::isGAnyLuaObj(lua_State *L, int idx)
{
    if (!lua_isuserdata(L, idx)) {
        return false;
    }
    lua_getmetatable(L, idx);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "_name");
        if (lua_isstring(L, -1)) {
            const char *name = lua_tostring(L, -1);
            const bool is = strcmp(name, "GAny") == 0;
            lua_pop(L, 2);
            return is;
        }
        lua_pop(L, 2);
        return false;
    }

    lua_pop(L, 1);

    return false;
}

/// =======================================

static int luaDumpWriter(lua_State *, const void *p, size_t sz, void *ud)
{
    GByteArray &buff = *static_cast<GByteArray *>(ud);
    buff.write(p, sz);
    return 0;
}

GByteArray GAnyLuaImpl::compileCode(const std::string &code, std::string sourcePath, bool strip)
{
    if (sourcePath.empty()) {
        GString scriptStr = code;
        // Ensure that UTF-8 characters are not truncated
        if (scriptStr.count() > 512) {
            scriptStr = scriptStr.left(512) + "...";
        }
        sourcePath = scriptStr.toStdString();
    } else if (sourcePath[0] != '@') {
        sourcePath = "@" + sourcePath;
    }
    GByteArray buffer;
    buffer.write(code.data(), code.size());
    return compile(buffer, sourcePath, strip);
}

GByteArray GAnyLuaImpl::compileFile(const std::string &filePath, bool strip)
{
    GByteArray buffer;
    if (sScriptReader) {
        buffer = sScriptReader(filePath);
    } else {
        GFile file(filePath);

        if (!file.exists()) {
            LogE("Run lua script error: file({}) does not exist.", filePath);
            return GByteArray();
        }

        if (file.open(GFile::ReadOnly | GFile::Binary)) {
            buffer = file.read();
            file.close();
        } else {
            LogE("Open file failure.");
            return GByteArray();
        }
    }

    if (!buffer.isEmpty()) {
        return compile(buffer, "@" + filePath, strip);
    }

    return GByteArray();
}

GByteArray GAnyLuaImpl::compile(const GByteArray &buffer, const std::string &sourcePath, bool strip) const
{
    lua_State *L = mL;
    if (luaL_loadbuffer(L, reinterpret_cast<const char *>(buffer.data()),
                        static_cast<size_t>(buffer.size()),
                        sourcePath.c_str()) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        LogE("{}", err);
        return GByteArray();
    }

    GByteArray buff;
    if (lua_dump(L, luaDumpWriter, &buff, strip) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        LogE("Dump lua code failure: {}", err);
        buff.clear();
    }

    lua_pop(L, lua_gettop(L));
    return buff;
}

GAny GAnyLuaImpl::getGAnyClassDB()
{
    if (!pfnGanyGetEnv) {
        return GAny();
    }

    GAny envObj;
    pfnGanyGetEnv(&envObj);
    if (!envObj.isObject()) {
        return GAny();
    }

    GAny classDB = (*envObj.as<GAnyObject>())["__CLASS_DB"];
    if (!classDB.isUserObject()) {
        return GAny();
    }
    return classDB;
}

/// ================ GAnyLua ================

std::shared_ptr<GAnyLua> GAnyLua::threadLocal()
{
    thread_local auto vm = std::make_shared<GAnyLuaImpl>();
    return vm;
}

void GAnyLua::setDebugMode(bool debugMode)
{
    GAnyLuaImpl::setDebugMode(debugMode);
}

void GAnyLua::setScriptReader(ScriptReader reader)
{
    GAnyLuaImpl::setScriptReader(std::move(reader));
}

void GAnyLua::addSearchPath(const std::string &path)
{
    GAnyLuaImpl::addSearchPath(path);
}

void GAnyLua::removeSearchPath(const std::string &path)
{
    GAnyLuaImpl::removeSearchPath(path);
}
