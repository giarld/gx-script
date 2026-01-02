//
// Created by Gxin on 25-5-23.
//

#include "gany_to_qjs.h"

#include "gx/debug.h"


class JSFunctionRef
{
public:
    explicit JSFunctionRef(JSContext *jsCtx, JSValue func)
        : mJsCtx(jsCtx)
    {
        GX_ASSERT(JS_IsFunction(jsCtx, func));
        mJsFunc = JS_DupValue(jsCtx, func);
    }

    ~JSFunctionRef()
    {
        JS_FreeValue(mJsCtx, mJsFunc);
    }

    JSValue getJSValue() const
    {
        return mJsFunc;
    }

private:
    JSContext *mJsCtx;
    JSValue mJsFunc;
};

bool GAnyToQJS::toJS(JS_State *jsState, bool isWorker)
{
    CHECK_CONDITION_R(jsState->ganyClassID == 0, false);

    const GAny classDB = getGAnyClassDB();
    CHECK_CONDITION_S_R(classDB.isUserObject(), false, "Failed to get GAny ClassDB");

    if (!isWorker) {
        jsState->ganyModuleIdx = classDB.call("assignModuleIdx").toInt64();
        CHECK_CONDITION_R(jsState->ganyModuleIdx > 0, false);
    }

    JS_NewClassID(jsState->rt, &jsState->ganyClassID);

    CHECK_CONDITION_S_R(jsState->ganyClassID != 0, false, "Failed to create new class ID");

    constexpr JSClassDef classDef{
        .class_name = "GAny",
        .finalizer = JS_GAnyFinalizer
    };
    JS_NewClass(jsState->rt, jsState->ganyClassID, &classDef);

    const JSValue global = JS_GetGlobalObject(jsState->ctx);
    const JSValue cppObj = JS_NewObject(jsState->ctx);

    JS_SetPropertyStr(jsState->ctx, cppObj, "create", JS_NewCFunction(jsState->ctx, JS_GAnyCreate, "create", 1));
    JS_SetPropertyStr(jsState->ctx, cppObj, "createWorkerCallable", JS_NewCFunction(jsState->ctx, JS_GAnyCreateWorkerCallable, "createWorkerCallable", 1));
    JS_SetPropertyStr(jsState->ctx, cppObj, "import", JS_NewCFunction(jsState->ctx, JS_GAnyImport, "import", 1));
    JS_SetPropertyStr(jsState->ctx, cppObj, "parseJson", JS_NewCFunction(jsState->ctx, JS_GAnyParseJson, "parseJson", 1));
    JS_SetPropertyStr(jsState->ctx, cppObj, "class", JS_NewCFunction(jsState->ctx, JS_GAnyClass, "class", 0));
    JS_SetPropertyStr(jsState->ctx, cppObj, "op", JS_NewCFunction(jsState->ctx, JS_GAnyOp, "op", 0));
    JS_SetPropertyStr(jsState->ctx, cppObj, "bind", JS_NewCFunction(jsState->ctx, JS_GAnyBind, "bind", 2));

    JS_SetPropertyStr(jsState->ctx, global, "GAny", cppObj);

    JS_SetPropertyStr(jsState->ctx, global, "AnyType", makeGAnyToJsValue(jsState, GAny::Import("AnyType"), false));

    JS_FreeValue(jsState->ctx, global);

    return true;
}

bool GAnyToQJS::releaseJS(JS_State *jsState)
{
    CHECK_CONDITION_R(jsState->ganyClassID > 0, false);

    if (jsState->ganyModuleIdx > 0) {
        const GAny classDB = getGAnyClassDB();
        CHECK_CONDITION_S_R(classDB.isUserObject(), false, "Failed to get GAny ClassDB");

        classDB.call("releaseModule", jsState->ganyModuleIdx);
    }
    jsState->ganyModuleIdx = 0;

    return true;
}


GAny GAnyToQJS::makeJsValueToGAny(const JS_State *jsState, const JSValue &value)
{
    JSContext *ctx = jsState->ctx;

    switch (JS_VALUE_GET_NORM_TAG(value)) {
        default:
        case JS_TAG_UNDEFINED:
            return GAny::undefined();
        case JS_TAG_NULL:
            return GAny::null();
        case JS_TAG_SHORT_BIG_INT:
        case JS_TAG_BIG_INT: {
            int64_t v = 0;
            JS_ToBigInt64(ctx, &v, value);
            return GAny(v);
        }
        case JS_TAG_INT: {
            int32_t v = 0;
            JS_ToInt32(ctx, &v, value);
            return GAny(v);
        }
        case JS_TAG_FLOAT64: {
            double v = 0;
            JS_ToFloat64(ctx, &v, value);
            return GAny(v);
        }
        case JS_TAG_BOOL: {
            const bool v = JS_ToBool(ctx, value) == 1;
            return GAny(v);
        }
        case JS_TAG_STRING: {
            const char *s = JS_ToCString(ctx, value);
            const std::string str(s);
            JS_FreeCString(ctx, s);
            return GAny(str);
        }
        case JS_TAG_OBJECT: {
            if (JS_IsFunction(ctx, value)) {
                auto jsFuncRef = std::make_shared<JSFunctionRef>(ctx, value);

                const GAnyFunction func = GAnyFunction::createVariadicFunction(
                    "JSFunction",
                    [jsState, jsFuncRef](const GAny **args, int32_t argc)-> GAny {
                        CHECK_CONDITION_S_R(GThread::currentThreadId() == jsState->threadId,
                                            GAnyException("Prohibit calling JSFunctions across threads"),
                                            "Prohibit calling JSFunctions across threads");

                        std::vector<JSValue> _args;
                        _args.reserve(argc);
                        for (int32_t i = 0; i < argc; i++) {
                            _args.push_back(makeGAnyToJsValue(jsState, *args[i], true));
                        }

                        const JSValue result = JS_Call(jsState->ctx, jsFuncRef->getJSValue(), JS_UNDEFINED, argc, _args.data());
                        if (JS_IsException(result)) {
                            const JSValue exception = JS_GetException(jsState->ctx);
                            const char *msg = JS_ToCString(jsState->ctx, exception);
                            GAny rException = GAnyException(msg);
                            JS_FreeValue(jsState->ctx, exception);

                            JS_FreeValue(jsState->ctx, result);
                            for (const auto &i: _args) {
                                JS_FreeValue(jsState->ctx, i);
                            }

                            JS_Throw(jsState->ctx, JS_NewString(jsState->ctx, rException.as<GAnyException>()->what()));

                            return rException;
                        }

                        GAny r = makeJsValueToGAny(jsState, result);
                        JS_FreeValue(jsState->ctx, result);
                        for (const auto &i: _args) {
                            JS_FreeValue(jsState->ctx, i);
                        }

                        return r;
                    });
                return GAny(func);
            }
            if (JS_IsArray(value)) {
                const JSValue lenVal = JS_GetPropertyStr(ctx, value, "length");
                const int32_t length = JS_VALUE_GET_INT(lenVal);
                JS_FreeValue(jsState->ctx, lenVal);

                GAny rArray = GAny::array();
                for (int32_t i = 0; i < length; i++) {
                    JSValue item = JS_GetPropertyUint32(ctx, value, i);
                    rArray.pushBack(makeJsValueToGAny(jsState, item));
                    JS_FreeValue(jsState->ctx, item);
                }

                return rArray;
            }

            const JSClassID classId = JS_GetClassID(value);
            if (classId == jsState->ganyClassID) {
                const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(value, classId));
                return GAny(*anyV);
            }

            if (JS_IsObject(value)) {
                JSPropertyEnum *props;
                uint32_t length;
                JS_GetOwnPropertyNames(ctx, &props, &length, value,
                                       JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY);

                GAny rObj = GAny::object();

                for (uint32_t i = 0; i < length; ++i) {
                    const JSAtom key = props[i].atom;
                    const JSValue keyVal = JS_AtomToValue(ctx, key);
                    const char *keyStr = JS_ToCString(ctx, keyVal);

                    const JSValue val = JS_GetProperty(ctx, value, key);

                    rObj[std::string(keyStr)] = makeJsValueToGAny(jsState, val);

                    JS_FreeValue(ctx, val);
                    JS_FreeCString(ctx, keyStr);
                    JS_FreeValue(ctx, keyVal);
                    JS_FreeAtom(ctx, key);
                }
                js_free(ctx, props);

                return rObj;
            }
            return GAny::undefined();
        }
        case JS_TAG_EXCEPTION: {
            const JSValue exception = JS_GetException(ctx);

            JSValue nameVal = JS_GetPropertyStr(ctx, exception, "name");
            const char *name = JS_ToCString(ctx, nameVal);

            JSValue msgVal = JS_GetPropertyStr(ctx, exception, "message");
            const char *msg = JS_ToCString(ctx, msgVal);

            const bool isError = JS_IsError(ctx, exception);
            JSValue stackVal;
            if (isError) {
                stackVal = JS_GetPropertyStr(ctx, exception, "stack");
            } else {
                js_std_cmd(/*ErrorBackTrace*/2, ctx, &stackVal);
            }
            const char *stack = JS_ToCString(ctx, stackVal);

            std::stringstream ss;
            ss << name << ": " << msg;
            if (stack && strlen(stack) > 0) {
                ss << "\n"
                    << "Stack:\n"
                    << stack;
            }

            JS_FreeCString(ctx, name);
            JS_FreeValue(ctx, nameVal);
            JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, msgVal);
            JS_FreeCString(ctx, stack);
            JS_FreeValue(ctx, stackVal);
            JS_FreeValue(ctx, exception);

            return GAnyException(ss.str());
        }
    }
}

JSValue GAnyToQJS::makeGAnyToJsValue(const JS_State *jsState, const GAny &value, bool unwrap)
{
    JSContext *ctx = jsState->ctx;

    if (unwrap) {
        switch (value.type()) {
            case AnyType::undefined_t: {
                return JS_UNDEFINED;
            }
            case AnyType::null_t: {
                return JS_NULL;
            }
            case AnyType::boolean_t: {
                return JS_NewBool(ctx, *value.as<bool>());
            }
            case AnyType::int_t: {
                const int64_t v = value.castAs<int64_t>();
                return JS_NewInt64(ctx, v);
            }
            case AnyType::float_t: {
                return JS_NewFloat64(ctx, value.castAs<float>());
            }
            case AnyType::double_t: {
                return JS_NewFloat64(ctx, value.castAs<double>());
            }
            case AnyType::string_t: {
                return JS_NewString(ctx, value.as<std::string>()->c_str());
            }
            case AnyType::array_t: {
                JSValue arr = JS_NewArray(ctx);
                for (uint32_t i = 0; i < value.length(); ++i) {
                    JS_SetPropertyUint32(ctx, arr, i, makeGAnyToJsValue(jsState, value[i], true));
                }
                return arr;
            }
            case AnyType::object_t: {
                JSValue obj = JS_NewObject(ctx);
                auto it = value.iterator();
                while (it.hasNext()) {
                    auto item = it.next();
                    std::string key = item.first.castAs<std::string>();
                    JS_SetPropertyStr(ctx, obj, key.c_str(), makeGAnyToJsValue(jsState, item.second, true));
                }
                return obj;
            }
            case AnyType::exception_t: {
                JS_ThrowInternalError(ctx, "%s", value.as<GAnyException>()->what());
                return JS_EXCEPTION;
            }
            default:
                break;
        }
    }

    GAny *any = GX_NEW(GAny, value);

    const JSValue proto = JS_NewObject(ctx);

    setGAnyGeneralProto(jsState, proto);

    if (value.isClass()) {
        JS_SetPropertyStr(ctx, proto, "new", JS_NewCFunction(ctx, JS_GAnyClassNew, "new", 0));

        const GAnyClass &clazz = *value.as<GAnyClass>();

        const auto constants = clazz.getConstants();
        for (const auto &c: constants) {
            makeObjectItemGetSet(ctx, proto, c.first, true, false);
        }

        const auto members = clazz.getMembers(false);
        for (const auto &m: members) {
            if (m.second.isFunction()) {
                const GAnyFunction &vFunction = *m.second.as<GAnyFunction>();
                if (vFunction.isStatic()) {
                    const JSValue _nameValue = JS_NewString(ctx, m.first.c_str());

                    JSValue data[1] = {_nameValue};
                    const JSValue func = JS_NewCFunctionData(ctx, JS_GAnyCallMethod, 0, 0, 1, data);

                    JS_SetPropertyStr(ctx, proto, m.first.c_str(), func);

                    JS_FreeValue(ctx, _nameValue);
                }
            }
        }
    }

    const GAnyClass &clazz = value.classObject();
    const auto members = clazz.getMembers(true);
    for (const auto &i: members) {
        std::string _name = i.first;
        GAny _member = i.second;

        const JSValue _nameValue = JS_NewString(ctx, _name.c_str());

        if (_member.isFunction()) {
            JSValue data[1] = {_nameValue};
            const JSValue func = JS_NewCFunctionData(ctx, JS_GAnyCallMethod, 0, 0, 1, data);

            JS_SetPropertyStr(ctx, proto, _name.c_str(), func);
        } else if (_member.isProperty()) {
            makeObjectItemGetSet(ctx, proto, _name, true, true);
        }

        JS_FreeValue(ctx, _nameValue);
    }

    const JSValue obj = JS_NewObjectProtoClass(ctx, proto, jsState->ganyClassID);

    JS_FreeValue(ctx, proto);

    JS_SetOpaque(obj, any);
    return obj;
}

void GAnyToQJS::setGAnyGeneralProto(const JS_State *jsState, JSValue proto)
{
    JSContext *ctx = jsState->ctx;

    JS_SetPropertyStr(ctx, proto, "_clone", JS_NewCFunction(ctx, JS_GAnyClone, "_clone", 0));
    JS_SetPropertyStr(ctx, proto, "_classTypeName", JS_NewCFunction(ctx, JS_GAnyClassTypeName, "_classTypeName", 0));
    JS_SetPropertyStr(ctx, proto, "_typeName", JS_NewCFunction(ctx, JS_GAnyTypeName, "_typeName", 0));
    JS_SetPropertyStr(ctx, proto, "_type", JS_NewCFunction(ctx, JS_GAnyType, "_type", 0));
    JS_SetPropertyStr(ctx, proto, "_length", JS_NewCFunction(ctx, JS_GAnyLength, "_length", 0));
    JS_SetPropertyStr(ctx, proto, "_size", JS_NewCFunction(ctx, JS_GAnySize, "_size", 0));
    JS_SetPropertyStr(ctx, proto, "_dump", JS_NewCFunction(ctx, JS_GAnyDump, "_dump", 0));
    JS_SetPropertyStr(ctx, proto, "_classObject", JS_NewCFunction(ctx, JS_GAnyClassObject, "_classObject", 0));

    JS_SetPropertyStr(ctx, proto, "_is", JS_NewCFunction(ctx, JS_GAnyIs, "_is", 1));
    JS_SetPropertyStr(ctx, proto, "_isUndefined", JS_NewCFunction(ctx, JS_GAnyIsUndefined, "_isUndefined", 0));
    JS_SetPropertyStr(ctx, proto, "_isNull", JS_NewCFunction(ctx, JS_GAnyIsNull, "_isNull", 0));
    JS_SetPropertyStr(ctx, proto, "_isFunction", JS_NewCFunction(ctx, JS_GAnyIsFunction, "_isFunction", 0));
    JS_SetPropertyStr(ctx, proto, "_isClass", JS_NewCFunction(ctx, JS_GAnyIsClass, "_isClass", 0));
    JS_SetPropertyStr(ctx, proto, "_isException", JS_NewCFunction(ctx, JS_GAnyIsException, "_isException", 0));
    JS_SetPropertyStr(ctx, proto, "_isObject", JS_NewCFunction(ctx, JS_GAnyIsObject, "_isObject", 0));
    JS_SetPropertyStr(ctx, proto, "_isArray", JS_NewCFunction(ctx, JS_GAnyIsArray, "_isArray", 0));
    JS_SetPropertyStr(ctx, proto, "_isInt", JS_NewCFunction(ctx, JS_GAnyIsInt, "_isInt", 0));
    JS_SetPropertyStr(ctx, proto, "_isFloat", JS_NewCFunction(ctx, JS_GAnyIsFloat, "_isFloat", 0));
    JS_SetPropertyStr(ctx, proto, "_isDouble", JS_NewCFunction(ctx, JS_GAnyIsDouble, "_isDouble", 0));
    JS_SetPropertyStr(ctx, proto, "_isNumber", JS_NewCFunction(ctx, JS_GAnyIsNumber, "_isNumber", 0));
    JS_SetPropertyStr(ctx, proto, "_isString", JS_NewCFunction(ctx, JS_GAnyIsString, "_isString", 0));
    JS_SetPropertyStr(ctx, proto, "_isBoolean", JS_NewCFunction(ctx, JS_GAnyIsBoolean, "_isBoolean", 0));
    JS_SetPropertyStr(ctx, proto, "_isUserObject", JS_NewCFunction(ctx, JS_GAnyIsUserObject, "_isUserObject", 0));

    JS_SetPropertyStr(ctx, proto, "_toString", JS_NewCFunction(ctx, JS_GAnyToString, "_toString", 0));

    // Symbol.toPrimitive
    // `${obj}`
    {
        const JSValue global = JS_GetGlobalObject(ctx);
        const JSValue symbolObj = JS_GetPropertyStr(ctx, global, "Symbol");
        const JSValue toPrim = JS_GetPropertyStr(ctx, symbolObj, "toPrimitive");
        const JSAtom toPrimAtom = JS_ValueToAtom(ctx, toPrim);
        JS_FreeValue(ctx, toPrim);
        JS_FreeValue(ctx, symbolObj);

        JS_SetProperty(ctx, proto, toPrimAtom, JS_NewCFunction(ctx, JS_GAnyToPrimitive, "[Symbol.toPrimitive]", 1));
        JS_FreeAtom(ctx, toPrimAtom);
        JS_FreeValue(ctx, global);
    }

    JS_SetPropertyStr(ctx, proto, "_toInt32", JS_NewCFunction(ctx, JS_GAnyToInt32, "_toInt32", 0));
    JS_SetPropertyStr(ctx, proto, "_toInt64", JS_NewCFunction(ctx, JS_GAnyToInt64, "_toInt64", 0));
    JS_SetPropertyStr(ctx, proto, "_toFloat", JS_NewCFunction(ctx, JS_GAnyToFloat, "_toFloat", 0));
    JS_SetPropertyStr(ctx, proto, "_toBool", JS_NewCFunction(ctx, JS_GAnyToBool, "_toBool", 0));
    JS_SetPropertyStr(ctx, proto, "_toJsValue", JS_NewCFunction(ctx, JS_GAnyToJsValue, "_toJsValue", 0));

    JS_SetPropertyStr(ctx, proto, "_toJsonString", JS_NewCFunction(ctx, JS_GAnyToJsonString, "_toJsonString", 0));
    JS_SetPropertyStr(ctx, proto, "_contains", JS_NewCFunction(ctx, JS_GAnyContains, "_contains", 1));
    JS_SetPropertyStr(ctx, proto, "_erase", JS_NewCFunction(ctx, JS_GAnyErase, "_erase", 1));
    JS_SetPropertyStr(ctx, proto, "_pushBack", JS_NewCFunction(ctx, JS_GAnyPushBack, "_pushBack", 1));
    JS_SetPropertyStr(ctx, proto, "_setItem", JS_NewCFunction(ctx, JS_GAnySetItem, "_setItem", 2));
    JS_SetPropertyStr(ctx, proto, "_getItem", JS_NewCFunction(ctx, JS_GAnyGetItem, "_getItem", 1));
    JS_SetPropertyStr(ctx, proto, "_delItem", JS_NewCFunction(ctx, JS_GAnyDelItem, "_delItem", 1));
    JS_SetPropertyStr(ctx, proto, "_iterator", JS_NewCFunction(ctx, JS_GAnyIterator, "_iterator", 0));
    JS_SetPropertyStr(ctx, proto, "_hasNext", JS_NewCFunction(ctx, JS_GAnyHasNext, "_hasNext", 0));
    JS_SetPropertyStr(ctx, proto, "_next", JS_NewCFunction(ctx, JS_GAnyNext, "_next", 0));

    JS_SetPropertyStr(ctx, proto, "_call", JS_NewCFunction(ctx, JS_GAnyCall, "_call", 0));
}

void GAnyToQJS::JS_GAnyFinalizer(JSRuntime *, JSValue val)
{
    const JSClassID classId = JS_GetClassID(val);
    const GAny *any = static_cast<GAny *>(JS_GetOpaque(val, classId));
    GX_DELETE(any);
}

JSValue GAnyToQJS::JS_GAnyCreate(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const GAny inVal = makeJsValueToGAny(jsState, argv[0]);
    GAny *any = GX_NEW(GAny, inVal);

    const JSValue proto = JS_NewObject(ctx);

    setGAnyGeneralProto(jsState, proto);

    const JSValue obj = JS_NewObjectProtoClass(ctx, proto, jsState->ganyClassID);

    JS_FreeValue(ctx, proto);

    JS_SetOpaque(obj, any);
    return obj;
}

JSValue GAnyToQJS::JS_GAnyCreateWorkerCallable(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 1 && argc != 2) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JSValue argv0 = argv[0];
    if (!JS_IsString(argv0)) {
        JS_ThrowInternalError(ctx, "Wrong argument type, must be a string");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const char *str = JS_ToCString(ctx, argv0);
    const std::string input(str);
    JS_FreeCString(ctx, str);

    GAny env;
    if (argc == 2) {
        const JSValue argv1 = argv[1];

        env = makeJsValueToGAny(jsState, argv1);
    }

    if (!env.isObject()) {
        env = GAny::object();
    }

    const JSAtom basenameAtom = JS_GetScriptOrModuleName(ctx, 1);
    if (basenameAtom == JS_ATOM_NULL) {
        JS_ThrowTypeError(ctx, "could not determine calling script or module name");
        return JS_EXCEPTION;
    }
    const char *basename = JS_AtomToCString(ctx, basenameAtom);

    const std::string basenameStr = basename;

    JS_FreeCString(ctx, basename);
    JS_FreeAtom(ctx, basenameAtom);

    const GAnyFunction threadFunc = GAnyFunction::createVariadicFunction(
        "JSWorkerCallable",
        [input, basenameStr, env](const GAny **, int32_t)-> GAny {
            const auto js = GAnyJS::threadLocal();
            CHECK_CONDITION_S_R(js != nullptr,
                                GAnyException("Unable to obtain the JS instance of the current thread"),
                                "Unable to obtain the JS instance of the current thread");

            if (!input.empty() && input[0] == '.') {
                const std::string modulePath = GAnyJSImplQjs::sModuleNormalizeFunc(basenameStr, input);
                return js->evalFile(modulePath, env);
            }
            return js->eval(input, "<thread_call>", env);
        });

    return makeGAnyToJsValue(jsState, threadFunc, false);
}

JSValue GAnyToQJS::JS_GAnyImport(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue argv0 = argv[0];
    if (!JS_IsString(argv0)) {
        JS_ThrowTypeError(ctx, "Argument must be a string");
        return JS_EXCEPTION;
    }

    const char *str = JS_ToCString(ctx, argv0);
    const std::string path(str);
    JS_FreeCString(ctx, str);

    const GAny ret = GAny::Import(path);
    if (ret.isNull()) {
        JS_ThrowInternalError(ctx, "Class not found");
        return JS_EXCEPTION;
    }

    return makeGAnyToJsValue(jsState, ret, ret.isObject());
}

JSValue GAnyToQJS::JS_GAnyParseJson(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JSValue argv0 = argv[0];
    if (!JS_IsString(argv0)) {
        JS_ThrowTypeError(ctx, "Argument must be a string");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const char *str = JS_ToCString(ctx, argv0);
    const std::string jsonStr(str);
    JS_FreeCString(ctx, str);

    const GAny rObj = GAny::parseJson(jsonStr);
    return makeGAnyToJsValue(jsState, rObj, true);
}

JSValue GAnyToQJS::JS_GAnyClass(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc == 0) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JSValue argv0 = argv[0];
    if (!JS_IsObject(argv0)) {
        JS_ThrowTypeError(ctx, "Argument must be a object");
        return JS_EXCEPTION;
    }

    bool doExport = false;
    if (argc == 2 && JS_IsBool(argv[1])) {
        const int r = JS_ToBool(ctx, argv[1]);
        if (r < 0) {
            return JS_EXCEPTION;
        }
        doExport = !!r;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny object = makeJsValueToGAny(jsState, argv0);
    if (!object.isObject()) {
        JS_ThrowTypeError(ctx, "Argument must be a object");
        return JS_EXCEPTION;
    }

    const auto clazz = GAnyClass::createFromGAnyObject(object);
    if (!clazz) {
        JS_ThrowInternalError(ctx, "Class creation failed");
        return JS_EXCEPTION;
    }

    if (doExport) {
        if (jsState->ganyModuleIdx > 0) {
            const GAny classDB = getGAnyClassDB();
            classDB.call("beginModule", jsState->ganyModuleIdx);
            GAny::Export(clazz);
            classDB.call("endModule");
        } else {
            JS_ThrowInternalError(ctx, "Exporting classes in Worker threads is not allowed");
            return JS_EXCEPTION;
        }
    }

    return makeGAnyToJsValue(jsState, clazz, false);
}

JSValue GAnyToQJS::JS_GAnyOp(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 3 && argc != 2) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    if (argc == 2 && JS_IsString(argv[0])) {
        const char *opStr = JS_ToCString(ctx, argv[0]);
        const std::string op = opStr;
        JS_FreeCString(ctx, opStr);

        const GAny v = makeJsValueToGAny(jsState, argv[1]);

        if (op == "~") {
            return makeGAnyToJsValue(jsState, ~v, true);
        }
        if (op == "-") {
            return makeGAnyToJsValue(jsState, -v, true);
        }
        JS_ThrowInternalError(ctx, "This operator is not a monocular operator");
        return JS_EXCEPTION;
    }

    if (!JS_IsString(argv[1])) {
        JS_ThrowInternalError(ctx, "The second arg must be a string");
        return JS_EXCEPTION;
    }

    const char *opStr = JS_ToCString(ctx, argv[1]);
    const std::string op = opStr;
    JS_FreeCString(ctx, opStr);

    const GAny lhs = makeJsValueToGAny(jsState, argv[0]);
    const GAny rhs = makeJsValueToGAny(jsState, argv[2]);

    if (op == "+") {
        return makeGAnyToJsValue(jsState, lhs + rhs, true);
    }
    if (op == "-") {
        return makeGAnyToJsValue(jsState, lhs - rhs, true);
    }
    if (op == "*") {
        return makeGAnyToJsValue(jsState, lhs * rhs, true);
    }
    if (op == "/") {
        return makeGAnyToJsValue(jsState, lhs / rhs, true);
    }

    if (op == "%") {
        return makeGAnyToJsValue(jsState, lhs % rhs, true);
    }
    if (op == "|") {
        return makeGAnyToJsValue(jsState, lhs | rhs, true);
    }
    if (op == "&") {
        return makeGAnyToJsValue(jsState, lhs & rhs, true);
    }

    if (op == "==") {
        return JS_NewBool(ctx, lhs == rhs);
    }
    if (op == "!=") {
        return JS_NewBool(ctx, lhs != rhs);
    }
    if (op == "<") {
        return JS_NewBool(ctx, lhs < rhs);
    }
    if (op == ">") {
        return JS_NewBool(ctx, lhs > rhs);
    }
    if (op == "<=") {
        return JS_NewBool(ctx, lhs <= rhs);
    }
    if (op == ">=") {
        return JS_NewBool(ctx, lhs >= rhs);
    }
    JS_ThrowInternalError(ctx, "Invalid operator");
    return JS_EXCEPTION;
}

JSValue GAnyToQJS::JS_GAnyBind(JSContext *ctx, JSValue /*thisVal*/, int argc, JSValue *argv)
{
    if (argc != 2) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue argv0 = argv[0];
    const JSValue argv1 = argv[1];

    if (!JS_IsString(argv1)) {
        JS_ThrowTypeError(ctx, "Argument 1 must be a string");
        return JS_EXCEPTION;
    }

    const GAny obj = makeJsValueToGAny(jsState, argv0);
    if (!obj) {
        JS_ThrowTypeError(ctx, "Argument 0 invalid");
        return JS_EXCEPTION;
    }

    const char *str = JS_ToCString(ctx, argv0);
    const std::string methodName(str);
    JS_FreeCString(ctx, str);

    const GAny rObj = GAny::Bind(obj, str);
    return makeGAnyToJsValue(jsState, rObj, false);
}

JSValue GAnyToQJS::JS_GAnyClone(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return makeGAnyToJsValue(jsState, anyV->clone(), false);
}

JSValue GAnyToQJS::JS_GAnyClassTypeName(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const std::string tn = anyV->classTypeName();
    return JS_NewStringLen(ctx, tn.c_str(), tn.length());
}

JSValue GAnyToQJS::JS_GAnyTypeName(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const std::string tn = anyV->typeName();
    return JS_NewStringLen(ctx, tn.c_str(), tn.length());
}

JSValue GAnyToQJS::JS_GAnyType(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewInt32(ctx, static_cast<int32_t>(anyV->type()));
}

JSValue GAnyToQJS::JS_GAnyLength(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewInt64(ctx, anyV->length());
}

JSValue GAnyToQJS::JS_GAnySize(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewInt64(ctx, anyV->size());
}

JSValue GAnyToQJS::JS_GAnyDump(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    std::stringstream ss;
    if (anyV->is<GAnyClass>()) {
        ss << *anyV->as<GAnyClass>();
    } else if (anyV->is<GAnyFunction>()) {
        ss << *anyV->as<GAnyFunction>();
    } else {
        ss << *anyV;
    }

    return JS_NewStringLen(ctx, ss.str().c_str(), ss.str().length());
}

JSValue GAnyToQJS::JS_GAnyClassObject(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return makeGAnyToJsValue(jsState, anyV->classObject(), false);
}

JSValue GAnyToQJS::JS_GAnyIs(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JSValue argv0 = argv[0];
    if (!JS_IsString(argv0)) {
        JS_ThrowTypeError(ctx, "Argument must be a string");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const char *_typeName = JS_ToCString(ctx, argv[0]);
    const std::string typeName = _typeName;
    JS_FreeCString(ctx, _typeName);

    return JS_NewBool(ctx, anyV->is(typeName));
}

JSValue GAnyToQJS::JS_GAnyIsUndefined(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isUndefined());
}

JSValue GAnyToQJS::JS_GAnyIsNull(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isNull());
}

JSValue GAnyToQJS::JS_GAnyIsFunction(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isFunction());
}

JSValue GAnyToQJS::JS_GAnyIsClass(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isClass());
}

JSValue GAnyToQJS::JS_GAnyIsException(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isException());
}

JSValue GAnyToQJS::JS_GAnyIsObject(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isObject());
}

JSValue GAnyToQJS::JS_GAnyIsArray(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isArray());
}

JSValue GAnyToQJS::JS_GAnyIsInt(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isInt());
}

JSValue GAnyToQJS::JS_GAnyIsFloat(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isFloat());
}

JSValue GAnyToQJS::JS_GAnyIsDouble(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isDouble());
}

JSValue GAnyToQJS::JS_GAnyIsNumber(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isNumber());
}

JSValue GAnyToQJS::JS_GAnyIsString(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isString());
}

JSValue GAnyToQJS::JS_GAnyIsBoolean(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isBoolean());
}

JSValue GAnyToQJS::JS_GAnyIsUserObject(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->isUserObject());
}

JSValue GAnyToQJS::JS_GAnyClassNew(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    if (!anyV->isClass()) {
        JS_ThrowInternalError(ctx, "This is not a class");
        return JS_EXCEPTION;
    }

    std::vector<GAny> tArgs;
    tArgs.reserve(argc);
    for (int i = 0; i < argc; i++) {
        tArgs.push_back(makeJsValueToGAny(jsState, argv[i]));
    }

    GAny rObj = anyV->_call(tArgs);
    if (rObj.isException()) {
        JS_ThrowInternalError(ctx, "%s", rObj.as<GAnyException>()->what());
        return JS_EXCEPTION;
    }

    return makeGAnyToJsValue(jsState, rObj, false);
}

JSValue GAnyToQJS::JS_GAnyToString(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const std::string str = anyV->toString();
    return JS_NewStringLen(ctx, str.c_str(), str.length());
}

JSValue GAnyToQJS::JS_GAnyToInt32(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const int32_t v = anyV->toInt32();
    return JS_NewInt32(ctx, v);
}

JSValue GAnyToQJS::JS_GAnyToInt64(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const int64_t v = anyV->toInt64();
    return JS_NewInt64(ctx, v);
}

JSValue GAnyToQJS::JS_GAnyToFloat(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const double v = anyV->toDouble();
    return JS_NewFloat64(ctx, v);
}

JSValue GAnyToQJS::JS_GAnyToBool(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const bool v = anyV->toBool();
    return JS_NewBool(ctx, v);
}

JSValue GAnyToQJS::JS_GAnyToJsValue(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return makeGAnyToJsValue(jsState, *anyV, false);
}

JSValue GAnyToQJS::JS_GAnyToPrimitive(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    std::string hint = "default";
    if (argc >= 1 && JS_IsString(argv[0])) {
        const char *cStr = JS_ToCString(ctx, argv[0]);
        hint = cStr;
        JS_FreeCString(ctx, cStr);
    }

    JSValue rVal = JS_EXCEPTION;

    if (hint == "string" || hint == "default") {
        const std::string str = anyV->toString();
        rVal = JS_NewStringLen(ctx, str.c_str(), str.length());
    } else if (hint == "number" && anyV->isNumber()) {
        if (anyV->isInt()) {
            const int64_t v = *anyV->as<int64_t>();
            rVal = JS_NewInt64(ctx, v);
        } else if (anyV->isDouble()) {
            rVal = JS_NewFloat64(ctx, *anyV->as<double>());
        } else if (anyV->isFloat()) {
            rVal = JS_NewFloat64(ctx, *anyV->as<float>());
        }
    }

    return rVal;
}

JSValue GAnyToQJS::JS_GAnyToJsonString(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    int32_t indent = -1;
    if (argc > 0) {
        JS_ToInt32(ctx, &indent, argv[0]);
    }

    const std::string json = anyV->toJsonString(indent);
    return JS_NewStringLen(ctx, json.c_str(), json.length());
}

JSValue GAnyToQJS::JS_GAnyContains(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->contains(makeJsValueToGAny(jsState, argv[0])));
}

JSValue GAnyToQJS::JS_GAnyErase(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    anyV->erase(makeJsValueToGAny(jsState, argv[0]));
    return JS_UNDEFINED;
}

JSValue GAnyToQJS::JS_GAnyPushBack(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    anyV->pushBack(makeJsValueToGAny(jsState, argv[0]));
    return JS_UNDEFINED;
}

JSValue GAnyToQJS::JS_GAnyClear(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    anyV->clear();
    return JS_UNDEFINED;
}

JSValue GAnyToQJS::JS_GAnySetItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 2) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const GAny key = makeJsValueToGAny(jsState, argv[0]);
    const GAny value = makeJsValueToGAny(jsState, argv[1]);

    return JS_NewBool(ctx, anyV->setItem(key, value));
}

JSValue GAnyToQJS::JS_GAnyGetItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const GAny key = makeJsValueToGAny(jsState, argv[0]);

    return makeGAnyToJsValue(jsState, anyV->getItem(key), true);
}

JSValue GAnyToQJS::JS_GAnyDelItem(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    if (argc != 1) {
        JS_ThrowInternalError(ctx, "Wrong number of argc");
        return JS_EXCEPTION;
    }

    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const GAny key = makeJsValueToGAny(jsState, argv[0]);

    return JS_NewBool(ctx, anyV->delItem(key));
}

JSValue GAnyToQJS::JS_GAnyIterator(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const GAny it = anyV->iterator();
    return makeGAnyToJsValue(jsState, it, false);
}

JSValue GAnyToQJS::JS_GAnyHasNext(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    return JS_NewBool(ctx, anyV->hasNext());
}

JSValue GAnyToQJS::JS_GAnyNext(JSContext *ctx, JSValue thisVal, int /*argc*/, JSValue */*argv*/)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const GAny rV = anyV->next();
    return makeGAnyToJsValue(jsState, rV, false);
}

JSValue GAnyToQJS::JS_GAnyCall(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    if (!anyV->isFunction()) {
        JS_ThrowTypeError(ctx, "Not a function");
        return JS_EXCEPTION;
    }

    std::vector<GAny> tArgs;
    tArgs.reserve(argc);
    for (int i = 0; i < argc; i++) {
        tArgs.push_back(makeJsValueToGAny(jsState, argv[i]));
    }

    GAny rObj = anyV->_call(tArgs);
    if (rObj.isException()) {
        JS_ThrowInternalError(ctx, "%s", rObj.as<GAnyException>()->what());
        return JS_EXCEPTION;
    }

    return makeGAnyToJsValue(jsState, rObj, true);
}


JSValue GAnyToQJS::JS_GAnyCallMethod(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv, int /*magic*/, JSValue *funcData)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const JSValue methodNameVal = funcData[0];
    const char *_methodName = JS_ToCString(ctx, methodNameVal);
    if (!_methodName)
        return JS_EXCEPTION;

    const std::string methodName(_methodName);
    JS_FreeCString(ctx, _methodName);

    std::vector<GAny> tArgs;
    tArgs.reserve(argc);
    for (int i = 0; i < argc; i++) {
        tArgs.push_back(makeJsValueToGAny(jsState, argv[i]));
    }

    GAny rObj = anyV->_call(methodName, tArgs);
    if (rObj.isException()) {
        JS_ThrowInternalError(ctx, "%s", rObj.as<GAnyException>()->what());
        return JS_EXCEPTION;
    }

    return makeGAnyToJsValue(jsState, rObj, true);
}

JSValue GAnyToQJS::JS_GAnyGetterSetter(JSContext *ctx, JSValue thisVal, int argc, JSValue *argv, int magic, JSValue *funcData)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    GAny *anyV = static_cast<GAny *>(JS_GetOpaque(thisVal, jsState->ganyClassID));

    const JSValue propNameVal = funcData[0];
    const char *_propName = JS_ToCString(ctx, propNameVal);
    if (!_propName)
        return JS_EXCEPTION;

    const std::string propName(_propName);
    JS_FreeCString(ctx, _propName);

    if (magic == 0) {
        // Get
        GAny rObj = anyV->getItem(propName);
        if (rObj.isException()) {
            JS_ThrowInternalError(ctx, "%s", rObj.as<GAnyException>()->what());
            return JS_EXCEPTION;
        }
        return makeGAnyToJsValue(jsState, rObj, true);
    }
    if (magic == 1) {
        // Set
        if (argc != 1) {
            JS_ThrowInternalError(ctx, "wrong number of arguments");
            return JS_EXCEPTION;
        }
        anyV->setItem(propName, makeJsValueToGAny(jsState, argv[0]));
    }
    return JS_UNDEFINED;
}

// ================================================================

GAny GAnyToQJS::getGAnyClassDB()
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

void GAnyToQJS::makeObjectItemGetSet(JSContext *ctx, const JSValue &proto, const std::string &name, bool canGet, bool canSet)
{
    const JSValue nameValue = JS_NewString(ctx, name.c_str());

    JSValue data[1] = {nameValue};
    const JSValue getterFunc = canGet ? JS_NewCFunctionData(ctx, JS_GAnyGetterSetter, 0, 0, 1, data) : JS_UNDEFINED;
    const JSValue setterFunc = canSet ? JS_NewCFunctionData(ctx, JS_GAnyGetterSetter, 0, 1, 1, data) : JS_UNDEFINED;

    const JSAtom nameAtom = JS_NewAtom(ctx, name.c_str());
    JS_DefinePropertyGetSet(ctx, proto, nameAtom, getterFunc, setterFunc, JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, nameAtom);

    JS_FreeValue(ctx, nameValue);
}
