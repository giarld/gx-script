//
// Created by Gxin on 2023/5/9.
//

#ifndef GX_SCRIPT_GANY_TO_LUA_H
#define GX_SCRIPT_GANY_TO_LUA_H

#include "gany_lua_impl.h"


struct LuaUserData
{
    uint64_t ganyModuleIdx;
};

/**
 * @class GAnyToLua
 * @brief Register GAny with Lua's tool class
 */
class GAnyToLua
{
public:
    static void toLua(lua_State *L);

private:
    static void registerEnumAnyType(lua_State *L);

    static void registerEnumMetaFunction(lua_State *L);

    static void registerEnumMetaFunctionS(lua_State *L);

    static void registerRequireLs(lua_State *L);

    static void registerLog(lua_State *L);

private: /// ToLua
    static int noneNewIndex(lua_State *L);

    static int requireLs(lua_State *L);

    static int printLogF(int level, lua_State *L);

    static int printLog(lua_State *L);

    static int printLogD(lua_State *L);

    static int printLogW(lua_State *L);

    static int printLogE(lua_State *L);


    static int regGAnyCreate(lua_State *L);

    static int regGAnyGC(lua_State *L);

    static int regGAnyLIndex(lua_State *L);

    static int regGAnyLNewIndex(lua_State *L);

    static int regGAnyNew(lua_State *L);

    static int regGAnyToString(lua_State *L);

    static int regGAnyLName(lua_State *L);

    static int regGAnyLCall(lua_State *L);

    static int regGAnyLLen(lua_State *L);

    static int regGAnyLAdd(lua_State *L);

    static int regGAnyLSub(lua_State *L);

    static int regGAnyLMul(lua_State *L);

    static int regGAnyLDiv(lua_State *L);

    static int regGAnyLUnm(lua_State *L);

    static int regGAnyLMod(lua_State *L);

    static int regGAnyLBNot(lua_State *L);

    static int regGAnyLBAnd(lua_State *L);

    static int regGAnyLBOr(lua_State *L);

    static int regGAnyLBXor(lua_State *L);

    static int regGAnyLEq(lua_State *L);

    static int regGAnyLLt(lua_State *L);

    static int regGAnyLLe(lua_State *L);

    static int regGAnyPairs(lua_State *L);

    static int regGAnyPairsClosure(lua_State *L);

    static int regGAnyCall(lua_State *L);

    static int regGAnyEqualTo(lua_State *L);

    static int regGAnyDump(lua_State *L);

    static int regGAnyObject(lua_State *L);

    static int regGAnyArray(lua_State *L);

    static int regGAnyUndefined(lua_State *L);

    static int regGAnyNull(lua_State *L);

    static int regGAnyClone(lua_State *L);

    static int regGAnyClassTypeName(lua_State *L);

    static int regGAnyTypeName(lua_State *L);

    static int regGAnyType(lua_State *L);

    static int regGAnyClassObject(lua_State *L);

    static int regGAnyLength(lua_State *L);

    static int regGAnySize(lua_State *L);

    static int regGAnyIs(lua_State *L);

    static int regGAnyIsUndefined(lua_State *L);

    static int regGAnyIsNull(lua_State *L);

    static int regGAnyIsFunction(lua_State *L);

    static int regGAnyIsClass(lua_State *L);

    static int regGAnyIsException(lua_State *L);

    static int regGAnyIsProperty(lua_State *L);

    static int regGAnyIsObject(lua_State *L);

    static int regGAnyIsArray(lua_State *L);

    static int regGAnyIsInt(lua_State *L);

    static int regGAnyIsFloat(lua_State *L);

    static int regGAnyIsDouble(lua_State *L);

    static int regGAnyIsNumber(lua_State *L);

    static int regGAnyIsString(lua_State *L);

    static int regGAnyIsBoolean(lua_State *L);

    static int regGAnyIsUserObject(lua_State *L);

    static int regGAnyIsTable(lua_State *L);

    static int regGAnyGet(lua_State *L);

    static int regGAnyGetItem(lua_State *L);

    static int regGAnySetItem(lua_State *L);

    static int regGAnyDelItem(lua_State *L);

    static int regGAnyContains(lua_State *L);

    static int regGAnyErase(lua_State *L);

    static int regGAnyPushBack(lua_State *L);

    static int regGAnyClear(lua_State *L);

    static int regGAnyIterator(lua_State *L);

    static int regGAnyHasNext(lua_State *L);

    static int regGAnyNext(lua_State *L);

    static int regGAnyToInt32(lua_State *L);

    static int regGAnyToInt64(lua_State *L);

    static int regGAnyToFloat(lua_State *L);

    static int regGAnyToDouble(lua_State *L);

    static int regGAnyToBool(lua_State *L);

    static int regGAnyToJsonString(lua_State *L);

    static int regGAnyToTable(lua_State *L);

    static int regGAnyToObject(lua_State *L);

    static int regGAnyParseJson(lua_State *L);

    static int regGAnyImport(lua_State *L);

    static int regGAnyExport(lua_State *L);

    static int regGAnyLoad(lua_State *L);

    static int regGAnyBind(lua_State *L);
};

#endif //GX_SCRIPT_GANY_TO_LUA_H
