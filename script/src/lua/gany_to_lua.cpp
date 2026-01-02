//
// Created by Gxin on 2023/5/9.
//

#include "gany_to_lua.h"

#include "lua_table.h"

#include <gx/debug.h>


void GAnyToLua::toLua(lua_State *L)
{
    const luaL_Reg staticMethods[] = {
        {"_create", regGAnyCreate},
        {"_object", regGAnyObject},
        {"_array", regGAnyArray},
        {"_undefined", regGAnyUndefined},
        {"_null", regGAnyNull},
        {"_parseJson", regGAnyParseJson},
        {"_equalTo", regGAnyEqualTo},
        {"_import", regGAnyImport},
        {"_export", regGAnyExport},
        {"_load", regGAnyLoad},
        {"_bind", regGAnyBind},

        {nullptr, nullptr}
    };

    const luaL_Reg methods[] = {
        {"__gc", regGAnyGC},
        {"__tostring", regGAnyToString},
        {"__index", regGAnyLIndex},
        {"__newindex", regGAnyLNewIndex},
        {"__call", regGAnyLCall},
        {"__name", regGAnyLName},
        {"__len", regGAnyLLen},
        {"__add", regGAnyLAdd},
        {"__sub", regGAnyLSub},
        {"__mul", regGAnyLMul},
        {"__div", regGAnyLDiv},
        {"__unm", regGAnyLUnm},
        {"__mod", regGAnyLMod},
        {"__bnot", regGAnyLBNot},
        {"__band", regGAnyLBAnd},
        {"__bor", regGAnyLBOr},
        {"__bxor", regGAnyLBXor},
        {"__eq", regGAnyLEq},
        {"__lt", regGAnyLLt},
        {"__le", regGAnyLLe},
        {"__pairs", regGAnyPairs},
        {"new", regGAnyNew},
        {"_call", regGAnyCall},
        {"_dump", regGAnyDump},
        {"_clone", regGAnyClone},
        {"_classTypeName", regGAnyClassTypeName},
        {"_typeName", regGAnyTypeName},
        {"_type", regGAnyType},
        {"_classObject", regGAnyClassObject},
        {"_length", regGAnyLength},
        {"_size", regGAnySize},
        {"_is", regGAnyIs},
        {"_isUndefined", regGAnyIsUndefined},
        {"_isNull", regGAnyIsNull},
        {"_isFunction", regGAnyIsFunction},
        {"_isClass", regGAnyIsClass},
        {"_isException", regGAnyIsException},
        {"_isProperty", regGAnyIsProperty},
        {"_isObject", regGAnyIsObject},
        {"_isArray", regGAnyIsArray},
        {"_isInt", regGAnyIsInt},
        {"_isFloat", regGAnyIsFloat},
        {"_isDouble", regGAnyIsDouble},
        {"_isNumber", regGAnyIsNumber},
        {"_isString", regGAnyIsString},
        {"_isBoolean", regGAnyIsBoolean},
        {"_isUserObject", regGAnyIsUserObject},
        {"_isTable", regGAnyIsTable},
        {"_get", regGAnyGet},
        {"_getItem", regGAnyGetItem},
        {"_setItem", regGAnySetItem},
        {"_delItem", regGAnyDelItem},
        {"_contains", regGAnyContains},
        {"_erase", regGAnyErase},
        {"_pushBack", regGAnyPushBack},
        {"_clear", regGAnyClear},
        {"_iterator", regGAnyIterator},
        {"_hasNext", regGAnyHasNext},
        {"_next", regGAnyNext},
        {"_toString", regGAnyToString},
        {"_toInt32", regGAnyToInt32},
        {"_toInt64", regGAnyToInt64},
        {"_toFloat", regGAnyToFloat},
        {"_toDouble", regGAnyToDouble},
        {"_toBool", regGAnyToBool},
        {"_toJsonString", regGAnyToJsonString},
        {"_toTable", regGAnyToTable},
        {"_toObject", regGAnyToObject},

        {nullptr, nullptr}
    };

    lua_newtable(L);
    int top = lua_gettop(L);

    const luaL_Reg *f;
    for (f = staticMethods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }

    lua_setglobal(L, "GAny");

    luaL_newmetatable(L, "GAny");
    top = lua_gettop(L);

    lua_pushliteral(L, "_name");
    lua_pushstring(L, "GAny");
    lua_settable(L, top);

    for (f = methods; f->func; f++) {
        lua_pushstring(L, f->name);
        lua_pushcfunction(L, f->func);
        lua_settable(L, top);
    }
    lua_pop(L, lua_gettop(L));


    registerEnumAnyType(L);
    registerEnumMetaFunction(L);
    registerEnumMetaFunctionS(L);
    registerRequireLs(L);
    registerLog(L);
}

void GAnyToLua::registerEnumAnyType(lua_State *L)
{
    const std::vector<std::pair<const char *, int> > enums = {
        {"undefined_t", static_cast<int>(AnyType::undefined_t)},
        {"null_t", static_cast<int>(AnyType::null_t)},
        {"boolean_t", static_cast<int>(AnyType::boolean_t)},
        {"int_t", static_cast<int>(AnyType::int_t)},
        {"float_t", static_cast<int>(AnyType::float_t)},
        {"double_t", static_cast<int>(AnyType::double_t)},
        {"string_t", static_cast<int>(AnyType::string_t)},
        {"array_t", static_cast<int>(AnyType::array_t)},
        {"object_t", static_cast<int>(AnyType::object_t)},
        {"function_t", static_cast<int>(AnyType::function_t)},
        {"class_t", static_cast<int>(AnyType::class_t)},
        {"property_t", static_cast<int>(AnyType::property_t)},
        {"exception_t", static_cast<int>(AnyType::exception_t)},
        {"user_obj_t", static_cast<int>(AnyType::user_obj_t)}
    };

    // table
    lua_newtable(L);
    const int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    const int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushinteger(L, i.second);
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "AnyType");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerEnumMetaFunction(lua_State *L)
{
    const std::vector<std::pair<const char *, int> > enums = {
        {"Init", static_cast<int>(MetaFunction::Init)},
        {"Negate", static_cast<int>(MetaFunction::Negate)},
        {"Addition", static_cast<int>(MetaFunction::Addition)},
        {"Subtraction", static_cast<int>(MetaFunction::Subtraction)},
        {"Multiplication", static_cast<int>(MetaFunction::Multiplication)},
        {"Division", static_cast<int>(MetaFunction::Division)},
        {"Modulo", static_cast<int>(MetaFunction::Modulo)},
        {"BitXor", static_cast<int>(MetaFunction::BitXor)},
        {"BitOr", static_cast<int>(MetaFunction::BitOr)},
        {"BitAnd", static_cast<int>(MetaFunction::BitAnd)},
        {"EqualTo", static_cast<int>(MetaFunction::EqualTo)},
        {"LessThan", static_cast<int>(MetaFunction::LessThan)},
        {"Length", static_cast<int>(MetaFunction::Length)},
        {"ToString", static_cast<int>(MetaFunction::ToString)},
        {"ToInt", static_cast<int>(MetaFunction::ToInt)},
        {"ToFloat", static_cast<int>(MetaFunction::ToFloat)},
        {"ToDouble", static_cast<int>(MetaFunction::ToDouble)},
        {"ToBoolean", static_cast<int>(MetaFunction::ToBoolean)},
        {"ToObject", static_cast<int>(MetaFunction::ToObject)},
        {"FromObject", static_cast<int>(MetaFunction::FromObject)}
    };

    // table
    lua_newtable(L);
    const int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    const int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushinteger(L, i.second);
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "MetaFunction");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerEnumMetaFunctionS(lua_State *L)
{
    const std::vector<std::pair<const char *, const char *> > enums = {
        {"Init", MetaFunctionNames[static_cast<size_t>(MetaFunction::Init)]},
        {"Negate", MetaFunctionNames[static_cast<size_t>(MetaFunction::Negate)]},
        {"Addition", MetaFunctionNames[static_cast<size_t>(MetaFunction::Addition)]},
        {"Subtraction", MetaFunctionNames[static_cast<size_t>(MetaFunction::Subtraction)]},
        {"Multiplication", MetaFunctionNames[static_cast<size_t>(MetaFunction::Multiplication)]},
        {"Division", MetaFunctionNames[static_cast<size_t>(MetaFunction::Division)]},
        {"Modulo", MetaFunctionNames[static_cast<size_t>(MetaFunction::Modulo)]},
        {"BitXor", MetaFunctionNames[static_cast<size_t>(MetaFunction::BitXor)]},
        {"BitOr", MetaFunctionNames[static_cast<size_t>(MetaFunction::BitOr)]},
        {"BitAnd", MetaFunctionNames[static_cast<size_t>(MetaFunction::BitAnd)]},
        {"EqualTo", MetaFunctionNames[static_cast<size_t>(MetaFunction::EqualTo)]},
        {"LessThan", MetaFunctionNames[static_cast<size_t>(MetaFunction::LessThan)]},
        {"Length", MetaFunctionNames[static_cast<size_t>(MetaFunction::Length)]},
        {"ToString", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToString)]},
        {"ToInt", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToInt)]},
        {"ToFloat", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToFloat)]},
        {"ToDouble", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToDouble)]},
        {"ToBoolean", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToBoolean)]},
        {"ToObject", MetaFunctionNames[static_cast<size_t>(MetaFunction::ToObject)]},
        {"FromObject", MetaFunctionNames[static_cast<size_t>(MetaFunction::FromObject)]}
    };

    // table
    lua_newtable(L);
    const int tTop = lua_gettop(L);

    // mt
    lua_newtable(L);
    const int top = lua_gettop(L);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, top);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, noneNewIndex);
    lua_settable(L, top);

    for (const auto &i: enums) {
        lua_pushstring(L, i.first);
        lua_pushstring(L, i.second);
        lua_settable(L, top);
    }

    lua_setmetatable(L, tTop);
    lua_setglobal(L, "MetaFunctionS");

    lua_pop(L, lua_gettop(L));
}

void GAnyToLua::registerRequireLs(lua_State *L)
{
    lua_pushcfunction(L, requireLs);
    lua_setglobal(L, "requireLs");
}

void GAnyToLua::registerLog(lua_State *L)
{
    lua_pushcfunction(L, printLog);
    lua_setglobal(L, "Log");

    lua_pushcfunction(L, printLogD);
    lua_setglobal(L, "LogD");

    lua_pushcfunction(L, printLogW);
    lua_setglobal(L, "LogW");

    lua_pushcfunction(L, printLogE);
    lua_setglobal(L, "LogE");
}


int GAnyToLua::noneNewIndex(lua_State *L)
{
    luaL_error(L, "Cannot insert content into the current table");
    return 0;
}

int GAnyToLua::requireLs(lua_State *L)
{
    const int n = lua_gettop(L);
    if (n >= 1) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            luaL_error(L, "requireLs error: the arg1(name) requires a string");
            return 0;
        }
        const std::string name = lua_tostring(L, 1);

        GAny env;
        if (n >= 2) {
            if (!GAnyLuaImpl::isGAnyLuaObj(L, 2) && !lua_istable(L, 2)) {
                luaL_error(L, "requireLs error: the arg2(env) requires a GAny object or table");
                return 0;
            }
            env = GAnyLuaImpl::makeLuaObjectToGAny(L, 2).toObject();
        } else {
            env = GAny::object();
        }

        const auto tlLua = GAnyLua::threadLocal();
        GAnyLuaImpl::pushGAny(L, tlLua->requireLs(name, env));

        return 1;
    }
    luaL_error(L, "requireLs error: no relevant overloaded forms found");
    return 0;
}

int GAnyToLua::printLogF(int level, lua_State *L)
{
    const int n = lua_gettop(L);
    std::stringstream msg;

    for (int i = 1; i <= n; i++) {
        GAny v = GAnyLuaImpl::makeLuaObjectToGAny(L, i);
        if (v.isException()) {
            luaL_error(L, v.as<GAnyException>()->what());
            return 0;
        }
        msg << v.toString();
    }

    lua_getglobal(L, "debug");
    lua_pushstring(L, "getinfo");
    lua_gettable(L, -2);

    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, 2);
        lua_pushstring(L, "nSl");
        if (lua_pcall(L, 2, 1, 0) == LUA_OK && lua_istable(L, -1)) {
            std::stringstream log;

            lua_pushstring(L, "short_src");
            lua_gettable(L, -2);
            if (lua_isstring(L, -1)) {
                log << lua_tostring(L, -1);
            } else {
                log << "??";
            }
            lua_pop(L, 1);
            log << "(";

            lua_pushstring(L, "currentline");
            lua_gettable(L, -2);
            if (lua_isinteger(L, -1)) {
                log << lua_tointeger(L, -1);
            } else {
                log << "?";
            }
            lua_pop(L, 1);
            log << ") : " << msg.str();
            gx::debugPrintf(level, log.str().c_str());
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return 0;
}

int GAnyToLua::printLog(lua_State *L)
{
    return printLogF(0, L);
}

int GAnyToLua::printLogD(lua_State *L)
{
    return printLogF(1, L);
}

int GAnyToLua::printLogW(lua_State *L)
{
    return printLogF(2, L);
}

int GAnyToLua::printLogE(lua_State *L)
{
    return printLogF(3, L);
}


int GAnyToLua::regGAnyCreate(lua_State *L)
{
    GAny argv;
    if (lua_gettop(L) >= 1) {
        argv = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    }
    if (argv.isException()) {
        luaL_error(L, argv.as<GAnyException>()->what());
        return 0;
    }

    GAny *obj = GX_NEW(GAny, argv);

    void **p = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
    *p = obj;
    luaL_getmetatable(L, "GAny");
    lua_setmetatable(L, -2);

    return 1;
}

int GAnyToLua::regGAnyGC(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __gc error: null object");
        return 0;
    }
    GX_DELETE(self);

    return 0;
}

int GAnyToLua::regGAnyLIndex(lua_State *L)
{
    if (lua_type(L, -1) == LUA_TSTRING) {
        const char *name = lua_tostring(L, -1);
        luaL_getmetatable(L, "GAny");
        lua_getfield(L, -1, name);
        if (lua_iscfunction(L, -1)) {
            return 1;
        }
        lua_pop(L, 2);
    }

    if (lua_gettop(L) != 2) {
        luaL_error(L, "Call GAny __index error: Number of abnormal parameters");
        return 0;
    }

    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __index error: null object");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }
    GAnyLuaImpl::makeGAnyToLuaObject(L, self->getItem(key), true);
    return 1;
}

int GAnyToLua::regGAnyLNewIndex(lua_State *L)
{
    if (lua_gettop(L) != 3) {
        luaL_error(L, "Call GAny __newindex error: Number of abnormal parameters");
        return 0;
    }

    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __newindex error: null object");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }
    GAny val = GAnyLuaImpl::makeLuaObjectToGAny(L, 3);
    if (val.isException()) {
        luaL_error(L, val.as<GAnyException>()->what());
        return 0;
    }
    self->setItem(key, val);
    return 0;
}

int GAnyToLua::regGAnyNew(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny new error: null object");
        return 0;
    }

    const int nargs = lua_gettop(L) - 1;

    std::vector<GAny> args;
    args.reserve(nargs);
    for (int i = 0; i < nargs; i++) {
        GAny v = GAnyLuaImpl::makeLuaObjectToGAny(L, i + 2);
        if (v.isException()) {
            luaL_error(L, v.as<GAnyException>()->what());
            return 0;
        }
        args.push_back(v);
    }

    GAny ret = self->_call(args);
    if (ret.isException()) {
        luaL_error(L, ret.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, ret);
    return 1;
}

int GAnyToLua::regGAnyToString(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __tostring error: null object");
        return 0;
    }

    const std::string str = self->toString();
    lua_pushstring(L, str.c_str());
    return 1;
}

int GAnyToLua::regGAnyLName(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __name error: null object");
        return 0;
    }
    const std::string name = self->classTypeName();
    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyLCall(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __call error: null object");
        return 0;
    }

    const int nargs = lua_gettop(L) - 1;

    std::vector<GAny> args;
    args.reserve(nargs);

    int begin = 0;
    if (self->isFunction()) {
        const auto &fn = self->unsafeAs<GAnyFunction>()->getName();
        if (!fn.empty() && fn[0] == '@') {
            begin = 1;
        }
    }
    for (int i = begin; i < nargs; i++) {
        GAny v = GAnyLuaImpl::makeLuaObjectToGAny(L, i + 2);
        if (v.isException()) {
            luaL_error(L, v.as<GAnyException>()->what());
            return 0;
        }
        args.push_back(v);
    }

    GAny r = self->_call(args);
    if (r.isException()) {
        luaL_error(L, r.as<GAnyException>()->what());
        return 0;
    }
    GAnyLuaImpl::makeGAnyToLuaObject(L, r, true);
    return 1;
}

int GAnyToLua::regGAnyLLen(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __len error: null object");
        return 0;
    }

    const size_t len = self->length();
    lua_pushnumber(L, static_cast<double>(len));
    return 1;
}

int GAnyToLua::regGAnyLAdd(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs + rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLSub(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs - rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLMul(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs * rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLDiv(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs / rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLUnm(lua_State *L)
{
    const GAny *lhs = glua_getcppobject(L, GAny, 1);
    if (!lhs) {
        luaL_error(L, "Call GAny __unm error: null object");
        return 0;
    }

    const GAny s = -*lhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLMod(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs % rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBNot(lua_State *L)
{
    const GAny *lhs = glua_getcppobject(L, GAny, 1);
    if (!lhs) {
        luaL_error(L, "Call GAny __bnot error: null object");
        return 0;
    }

    const GAny s = ~*lhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBAnd(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs & rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBOr(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs | rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLBXor(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const GAny s = lhs ^ rhs;
    if (s.isException()) {
        luaL_error(L, s.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, s);
    return 1;
}

int GAnyToLua::regGAnyLEq(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const bool s = lhs == rhs;
    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyLLt(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const bool s = lhs < rhs;
    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyLLe(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const bool s = lhs <= rhs;
    lua_pushboolean(L, s);
    return 1;
}

int GAnyToLua::regGAnyPairs(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny __pairs error: null object");
        return 0;
    }

    auto iterator = self->iterator();
    if (iterator.isException()) {
        luaL_error(L, iterator.as<GAnyException>()->what());
        return 0;
    }

    lua_pushcfunction(L, GAnyToLua::regGAnyPairsClosure);
    GAnyLuaImpl::pushGAny(L, iterator);
    lua_pushnil(L);
    return 3;
}

int GAnyToLua::regGAnyPairsClosure(lua_State *L)
{
    GAny iterator = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (iterator.isException()) {
        luaL_error(L, iterator.as<GAnyException>()->what());
        return 0;
    }

    if (iterator.hasNext()) {
        const auto item = iterator.next();
        GAnyLuaImpl::makeGAnyToLuaObject(L, item.first);
        GAnyLuaImpl::makeGAnyToLuaObject(L, item.second);
        return 2;
    }
    return 0;
}

int GAnyToLua::regGAnyCall(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _call error: missing method name");
        return 0;
    }

    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _call error: null object");
        return 0;
    }

    if (lua_type(L, 2) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _call error: missing method name");
        return 0;
    }

    const std::string method = lua_tostring(L, 2);

    const int nargs = lua_gettop(L) - 2;

    std::vector<GAny> args;
    args.reserve(nargs);

    for (int i = 0; i < nargs; i++) {
        GAny v = GAnyLuaImpl::makeLuaObjectToGAny(L, i + 3);
        if (v.isException()) {
            luaL_error(L, v.as<GAnyException>()->what());
            return 0;
        }
        args.push_back(v);
    }
    GAny r = self->_call(method, args);
    if (r.isException()) {
        luaL_error(L, r.as<GAnyException>()->what());
        return 0;
    }
    GAnyLuaImpl::makeGAnyToLuaObject(L, r, true);
    return 1;
}

int GAnyToLua::regGAnyEqualTo(lua_State *L)
{
    GAny lhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (lhs.isException()) {
        luaL_error(L, lhs.as<GAnyException>()->what());
        return 0;
    }

    GAny rhs = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (rhs.isException()) {
        luaL_error(L, rhs.as<GAnyException>()->what());
        return 0;
    }

    const bool s = lhs == rhs;

    lua_pushboolean(L, !!s);
    return 1;
}

int GAnyToLua::regGAnyDump(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _dump error: null object");
        return 0;
    }

    std::stringstream ss;
    if (self->is<GAnyClass>()) {
        ss << self->as<GAnyClass>();
    } else if (self->is<GAnyFunction>()) {
        ss << self->as<GAnyFunction>();
    } else {
        ss << *self;
    }

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

int GAnyToLua::regGAnyObject(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        GAnyLuaImpl::pushGAny(L, GAny::object());
        return 1;
    }

    if (lua_istable(L, 1)) {
        GAny r = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
        if (r.isException()) {
            luaL_error(L, r.as<GAnyException>()->what());
            return 0;
        }
        if (r.is<LuaTable>()) {
            GAnyLuaImpl::pushGAny(L, r.toObject());
            return 1;
        }
    }

    GAnyLuaImpl::pushGAny(L, GAny::object());
    return 1;
}

int GAnyToLua::regGAnyArray(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        GAnyLuaImpl::pushGAny(L, GAny::array());
        return 1;
    }

    if (lua_istable(L, 1)) {
        GAny r = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
        if (r.isException()) {
            luaL_error(L, r.as<GAnyException>()->what());
            return 0;
        }
        if (r.is<LuaTable>()) {
            const GAny obj = r.toObject();
            if (obj.isArray()) {
                GAnyLuaImpl::pushGAny(L, obj);
                return 1;
            }
        }
    }

    GAnyLuaImpl::pushGAny(L, GAny::array());
    return 1;
}

int GAnyToLua::regGAnyUndefined(lua_State *L)
{
    GAnyLuaImpl::pushGAny(L, GAny::undefined());
    return 1;
}

int GAnyToLua::regGAnyNull(lua_State *L)
{
    GAnyLuaImpl::pushGAny(L, GAny::null());
    return 1;
}

int GAnyToLua::regGAnyClone(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _clone error: null object");
        return 0;
    }

    const GAny ret = self->clone();
    if (ret.isException()) {
        luaL_error(L, ret.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, ret);
    return 1;
}

int GAnyToLua::regGAnyClassTypeName(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _classTypeName error: null object");
        return 0;
    }

    const std::string name = self->classTypeName();
    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyTypeName(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _typeName error: null object");
        return 0;
    }

    const std::string name = self->typeName();
    lua_pushstring(L, name.c_str());
    return 1;
}

int GAnyToLua::regGAnyType(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _type error: null object");
        return 0;
    }

    const int type = static_cast<int>(self->type());
    lua_pushinteger(L, type);
    return 1;
}

int GAnyToLua::regGAnyClassObject(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _classObject error: null object");
        return 0;
    }

    const GAny classObj = self->classObject();
    GAnyLuaImpl::pushGAny(L, classObj);
    return 1;
}

int GAnyToLua::regGAnyLength(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _length error: null object");
        return 0;
    }

    const size_t len = self->length();
    lua_pushnumber(L, static_cast<double>(len));
    return 1;
}

int GAnyToLua::regGAnySize(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _size error: null object");
        return 0;
    }

    const size_t size = self->size();
    lua_pushnumber(L, static_cast<double>(size));
    return 1;
}

int GAnyToLua::regGAnyIs(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _is error: null object");
        return 0;
    }

    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _is error: null object");
        return 0;
    }

    if (lua_type(L, 2) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _is error: arg1 not a string");
        return 0;
    }
    const std::string arg1 = lua_tostring(L, 2);

    lua_pushboolean(L, !!(self->is(arg1)));
    return 1;
}

int GAnyToLua::regGAnyIsUndefined(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isUndefined error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isUndefined());
    return 1;
}

int GAnyToLua::regGAnyIsNull(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isNull error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isNull());
    return 1;
}

int GAnyToLua::regGAnyIsFunction(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isFunction error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isFunction());
    return 1;
}

int GAnyToLua::regGAnyIsClass(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isClass error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isClass());
    return 1;
}

int GAnyToLua::regGAnyIsException(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isException error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isException());
    return 1;
}

int GAnyToLua::regGAnyIsProperty(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isProperty error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isProperty());
    return 1;
}

int GAnyToLua::regGAnyIsObject(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isObject error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isObject());
    return 1;
}

int GAnyToLua::regGAnyIsArray(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isArray error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isArray());
    return 1;
}

int GAnyToLua::regGAnyIsInt(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isInt error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isInt());
    return 1;
}

int GAnyToLua::regGAnyIsFloat(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isFloat error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isFloat());
    return 1;
}

int GAnyToLua::regGAnyIsDouble(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isDouble error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isDouble());
    return 1;
}

int GAnyToLua::regGAnyIsNumber(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isNumber error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isNumber());
    return 1;
}

int GAnyToLua::regGAnyIsString(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isString error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isString());
    return 1;
}

int GAnyToLua::regGAnyIsBoolean(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isBoolean error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isBoolean());
    return 1;
}

int GAnyToLua::regGAnyIsUserObject(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isUserObject error: null object");
        return 0;
    }

    lua_pushboolean(L, self->isUserObject());
    return 1;
}

int GAnyToLua::regGAnyIsTable(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _isTable error: null object");
        return 0;
    }

    const int is = self->isUserObject() && self->is<LuaTable>();
    lua_pushboolean(L, is);
    return 1;
}

int GAnyToLua::regGAnyGet(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _get error: null object");
        return 0;
    }

    GAnyLuaImpl::makeGAnyToLuaObject(L, *self);
    return 1;
}

int GAnyToLua::regGAnyGetItem(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _getItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _getItem error: need a parameter");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }
    const GAny val = self->getItem(key);
    if (val.isException()) {
        luaL_error(L, val.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, val);
    return 1;
}

int GAnyToLua::regGAnySetItem(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _setItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 3) {
        luaL_error(L, "Call GAny _setItem error: two parameters are required");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }

    GAny val = GAnyLuaImpl::makeLuaObjectToGAny(L, 3);
    if (val.isException()) {
        luaL_error(L, val.as<GAnyException>()->what());
        return 0;
    }

    const bool ret = self->setItem(key, val);
    lua_pushboolean(L, ret);
    return 1;
}

int GAnyToLua::regGAnyDelItem(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _delItem error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _delItem error: need a parameter");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }

    const bool ret = self->delItem(key);
    lua_pushboolean(L, ret);
    return 1;
}

int GAnyToLua::regGAnyContains(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _contains error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _contains error: need a parameter");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }

    const bool v = self->contains(key);
    lua_pushboolean(L, v);
    return 1;
}

int GAnyToLua::regGAnyErase(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _erase error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _erase error: need a parameter");
        return 0;
    }

    GAny key = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (key.isException()) {
        luaL_error(L, key.as<GAnyException>()->what());
        return 0;
    }

    self->erase(key);
    return 0;
}

int GAnyToLua::regGAnyPushBack(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _pushBack error: null object");
        return 0;
    }

    if (lua_gettop(L) < 2) {
        luaL_error(L, "Call GAny _pushBack error: need a parameter");
        return 0;
    }

    GAny v = GAnyLuaImpl::makeLuaObjectToGAny(L, 2);
    if (v.isException()) {
        luaL_error(L, v.as<GAnyException>()->what());
        return 0;
    }

    self->pushBack(v);
    return 0;
}

int GAnyToLua::regGAnyClear(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _clear error: null object");
        return 0;
    }

    self->clear();
    return 0;
}

int GAnyToLua::regGAnyIterator(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _iterator error: null object");
        return 0;
    }

    GAny iterator = self->iterator();
    if (iterator.isException()) {
        luaL_error(L, iterator.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, iterator);
    return 1;
}

int GAnyToLua::regGAnyHasNext(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _hasNext error: null object");
        return 0;
    }

    lua_pushboolean(L, self->hasNext());
    return 1;
}

int GAnyToLua::regGAnyNext(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _next error: null object");
        return 0;
    }

    GAny v = self->next();
    if (v.isException()) {
        luaL_error(L, v.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, v);
    return 1;
}

int GAnyToLua::regGAnyToInt32(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt32 error: null object");
        return 0;
    }

    const int v = self->toInt32();
    lua_pushinteger(L, v);
    return 1;
}

int GAnyToLua::regGAnyToInt64(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toInt64 error: null object");
        return 0;
    }

    const double v = static_cast<double>(self->toInt64());
    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToFloat(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toFloat error: null object");
        return 0;
    }

    const double v = self->toFloat();
    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToDouble(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toDouble error: null object");
        return 0;
    }

    const double v = self->toDouble();
    lua_pushnumber(L, v);
    return 1;
}

int GAnyToLua::regGAnyToBool(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toBool error: null object");
        return 0;
    }

    const bool v = self->toBool();
    lua_pushboolean(L, v);
    return 1;
}

int GAnyToLua::regGAnyToJsonString(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toJsonString error: null object");
        return 0;
    }

    int indent = -1;
    if (lua_gettop(L) >= 2 && lua_isinteger(L, 2)) {
        indent = lua_tointeger(L, 2);
    }

    const std::string v = self->toJsonString(indent);
    lua_pushstring(L, v.c_str());
    return 1;
}

int GAnyToLua::regGAnyToTable(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toTable error: null object");
        return 0;
    }

    const LuaTable lTable = LuaTable::fromGAnyObject(*self);
    lTable.push(L);
    return 1;
}

int GAnyToLua::regGAnyToObject(lua_State *L)
{
    const GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny _toObject error: null object");
        return 0;
    }

    const GAny obj = self->toObject();
    if (obj.isException()) {
        luaL_error(L, obj.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, obj);
    return 1;
}

int GAnyToLua::regGAnyParseJson(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        luaL_error(L, "Call GAny _pushBack error: need a parameter");
        return 0;
    }
    if (lua_type(L, 1) != LUA_TSTRING) {
        luaL_error(L, "Call GAny _pushBack error: the arg1 requires a string");
        return 0;
    }

    const std::string json = lua_tostring(L, 1);

    const GAny obj = GAny::parseJson(json);
    if (obj.isException()) {
        luaL_error(L, obj.as<GAnyException>()->what());
        return 0;
    }

    GAnyLuaImpl::pushGAny(L, obj);
    return 1;
}

int GAnyToLua::regGAnyImport(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        luaL_error(L, "Call GAny Import error: Missing parameters");
        return 0;
    }

    if (lua_isstring(L, 1)) {
        const std::string path = lua_tostring(L, 1);
        GAny ret = GAny::Import(path);
        if (ret.isException()) {
            luaL_error(L, ret.as<GAnyException>()->what());
            return 0;
        }
        GAnyLuaImpl::pushGAny(L, ret);
    } else {
        GAnyLuaImpl::pushGAny(L, GAny::undefined());
    }
    return 1;
}

int GAnyToLua::regGAnyExport(lua_State *L)
{
    GAny *self = glua_getcppobject(L, GAny, 1);
    if (!self) {
        luaL_error(L, "Call GAny Export error: null object");
        return 0;
    }

    void* storedData = *(void**)lua_getextraspace(L);
    LuaUserData* userData = (LuaUserData*)storedData;

    const GAny classDB = GAnyLuaImpl::getGAnyClassDB();
    classDB.call("beginModule", userData->ganyModuleIdx);

    GAny::Export(*self->as<std::shared_ptr<GAnyClass> >());

    classDB.call("endModule");

    return 0;
}

int GAnyToLua::regGAnyLoad(lua_State *L)
{
    if (lua_gettop(L) == 0) {
        luaL_error(L, "Call GAny Import error: Missing parameters");
        return 0;
    }

    if (lua_isstring(L, 1)) {
        const std::string path = lua_tostring(L, 1);
        lua_pushboolean(L, GAny::Load(path));
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int GAnyToLua::regGAnyBind(lua_State *L)
{
    const int n = lua_gettop(L);
    if (n != 2) {
        luaL_error(L, "Call GAny Connect error: The number of parameters must be 2");
        return 0;
    }

    const GAny obj = GAnyLuaImpl::makeLuaObjectToGAny(L, 1);
    if (obj.isUndefined() || obj.isNull()) {
        luaL_error(L, "Call GAny Connect error: Invalid parameter 1");
        return 0;
    }
    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Call GAny Connect error: Invalid parameter 2");
        return 0;
    }

    const std::string methodName = lua_tostring(L, 2);
    GAny caller = GAny::Bind(obj, methodName);
    if (caller.isException()) {
        luaL_error(L, caller.as<GAnyException>()->what());
        return 0;
    }

    // 重新修饰前缀, 防止与通过 lua index 特性产生的 Caller 一样调用时被移除第一个参数(self)
    GAnyFunction *func = caller.unsafeAs<GAnyFunction>();
    func->setName("L" + func->getName());

    GAnyLuaImpl::pushGAny(L, caller);
    return 1;
}
