//
// Created by Gxin on 25-5-23.
//

#include "gany_js_impl_qjs.h"
#include "gany_to_qjs.h"

#include "gx/gglobal.h"
#include "gx/gfile.h"
#include "gx/debug.h"

#include <cctype>
#include <set>
#include <string>
#include <vector>


namespace
{
constexpr const char *GAnyModulePrefix = "gany:";

struct GAnyModuleExports
{
    bool valid = false;
    GAny defaultValue;
    std::vector<std::pair<std::string, GAny>> namedExports;
};

static bool isGAnyModuleName(const char *moduleName)
{
    return moduleName && std::string(moduleName).rfind(GAnyModulePrefix, 0) == 0;
}

static bool hasJsModuleSyntax(const std::string &script)
{
    bool atLineStart = true;

    for (size_t i = 0; i < script.size();) {
        const char c = script[i];

        if (c == '\n' || c == '\r') {
            atLineStart = true;
            ++i;
            continue;
        }
        if (atLineStart && (c == ' ' || c == '\t')) {
            ++i;
            continue;
        }
        if (c == '/' && i + 1 < script.size() && script[i + 1] == '/') {
            i += 2;
            while (i < script.size() && script[i] != '\n' && script[i] != '\r') {
                ++i;
            }
            continue;
        }
        if (c == '/' && i + 1 < script.size() && script[i + 1] == '*') {
            i += 2;
            while (i + 1 < script.size() && !(script[i] == '*' && script[i + 1] == '/')) {
                if (script[i] == '\n' || script[i] == '\r') {
                    atLineStart = true;
                }
                ++i;
            }
            if (i + 1 < script.size()) {
                i += 2;
            }
            continue;
        }

        if (atLineStart && (script.compare(i, 6, "import") == 0 || script.compare(i, 6, "export") == 0)) {
            const size_t nextIndex = i + 6;
            const bool isIdentifierTail = nextIndex < script.size()
                                          && (std::isalnum(static_cast<unsigned char>(script[nextIndex])) || script[nextIndex] == '_' || script[nextIndex] == '$');
            if (!isIdentifierTail) {
                size_t tokenIndex = nextIndex;
                while (tokenIndex < script.size() && (script[tokenIndex] == ' ' || script[tokenIndex] == '\t')) {
                    ++tokenIndex;
                }
                if (script.compare(i, 6, "export") == 0 || tokenIndex == script.size() || script[tokenIndex] != '(') {
                    return true;
                }
            }
        }

        atLineStart = false;
        if (c == '\'' || c == '"' || c == '`') {
            const char quote = c;
            ++i;
            while (i < script.size()) {
                if (script[i] == '\\') {
                    i += 2;
                    continue;
                }
                if (script[i] == quote) {
                    ++i;
                    break;
                }
                ++i;
            }
            continue;
        }
        ++i;
    }

    return false;
}

static bool shouldEvalAsJsModule(const std::string &script, const std::string &sourcePath)
{
    const GString path = sourcePath;
    return path.toLower().endWith(".mjs") || hasJsModuleSyntax(script);
}

static GAnyModuleExports getGAnyModuleExports(const std::string &moduleName)
{
    GAnyModuleExports exports;

    if (moduleName.rfind(GAnyModulePrefix, 0) != 0) {
        return exports;
    }

    const std::string importPath = moduleName.substr(std::string(GAnyModulePrefix).size());
    if (importPath.empty()) {
        return exports;
    }

    const bool isWildcardPath = importPath.size() >= 2 && importPath.substr(importPath.size() - 2) == ".*";

    GAny imported = GAny::Import(importPath);
    if (imported.isNull() && !isWildcardPath) {
        imported = GAny::Import(importPath + ".*");
    }

    if (imported.isNull()) {
        return exports;
    }

    exports.valid = true;
    exports.defaultValue = imported;

    std::set<std::string> exportedNames;
    if (imported.isObject()) {
        auto it = imported.iterator();
        while (it.hasNext()) {
            auto item = it.next();
            const std::string name = item.first.castAs<std::string>();
            if (!name.empty() && exportedNames.insert(name).second) {
                exports.namedExports.emplace_back(name, item.second);
            }
        }
    } else if (imported.isClass()) {
        const auto *clazz = imported.as<GAnyClass>();
        if (clazz && !clazz->getName().empty() && exportedNames.insert(clazz->getName()).second) {
            exports.namedExports.emplace_back(clazz->getName(), imported);
        }
    }

    return exports;
}

static std::string getModuleName(JSContext *ctx, JSModuleDef *m)
{
    const JSAtom atom = JS_GetModuleName(ctx, m);
    const JSValue value = JS_AtomToValue(ctx, atom);
    const char *str = JS_ToCString(ctx, value);
    std::string moduleName = str ? str : "";

    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, value);
    JS_FreeAtom(ctx, atom);

    return moduleName;
}

static int JS_ganyModuleInit(JSContext *ctx, JSModuleDef *m)
{
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
    const GAnyModuleExports exports = getGAnyModuleExports(getModuleName(ctx, m));
    if (!exports.valid) {
        JS_ThrowReferenceError(ctx, "Could not load GAny module");
        return -1;
    }

    JS_SetModuleExport(ctx, m, "default", GAnyToQJS::makeGAnyToJsValue(jsState, exports.defaultValue, exports.defaultValue.isObject()));
    for (const auto &item: exports.namedExports) {
        JS_SetModuleExport(ctx, m, item.first.c_str(), GAnyToQJS::makeGAnyToJsValue(jsState, item.second, item.second.isObject()));
    }

    return 0;
}
}


static void jsStateObjectFinalizer(JSRuntime *, JSValue val)
{
    const JSClassID classId = JS_GetClassID(val);

    JS_State *state = static_cast<JS_State *>(JS_GetOpaque(val, classId));
    if (state) {
        GAnyToQJS::releaseJS(state);
        GX_DELETE(state);
    }
}

JSContext * JS_NewCustomContext(JSRuntime *rt, bool isWorker)
{
    JS_SetModuleLoaderFunc(rt, GAnyJSImplQjs::JS_moduleNormalizeFunc, GAnyJSImplQjs::JS_moduleLoader, nullptr);

    JSContext *ctx = JS_NewContext(rt);
    if (!ctx) {
        return nullptr;
    }

    /* system modules */
    js_init_module_std(ctx, "qjs:std");
    js_init_module_os(ctx, "qjs:os");
    js_init_module_bjson(ctx, "qjs:bjson");

    js_std_add_helpers(ctx, 0, nullptr);

    JS_State *jsState = GX_NEW(JS_State);

    jsState->rt = rt;
    jsState->ctx = ctx;
    jsState->threadId = GThread::currentThreadId();

    GAnyToQJS::toJS(jsState, isWorker);

    JS_SetContextOpaque(ctx, jsState);

    // Hosting the lifecycle for JSContext
    constexpr JSClassDef classDef{
        .class_name = "JsState",
        .finalizer = jsStateObjectFinalizer
    };

    JSClassID stateClassId = 0;
    JS_NewClassID(rt, &stateClassId);

    JS_NewClass(rt, stateClassId, &classDef);

    const JSValue jsStateObj = JS_NewObjectClass(ctx, stateClassId);
    GX_ASSERT(JS_IsObject(jsStateObj));

    JS_SetOpaque(jsStateObj, jsState);

    const JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "JSState", jsStateObj);
    JS_FreeValue(ctx, global);

    return ctx;
}

JSContext *JS_NewWorkerContext(JSRuntime *rt)
{
    return JS_NewCustomContext(rt, true);
}

// ================================================================

static GAnyJS::FileReader sDefFileReader = [](const std::string &path) -> GByteArray {
    const GString tPath = path;

    GFile file(path);

    GFile::OpenModeFlags openModeFlags = GFile::ReadOnly;
    if (!tPath.toLower().endWith(".js")) {
        openModeFlags |= GFile::Binary;
    }

    GByteArray bytes;
    if (file.open(openModeFlags)) {
        bytes = file.read();
        file.close();
    }
    return bytes;
};

static GAnyJS::ModuleNameNormalizeFunc sDefModuleNormalizeFunc = [](const std::string &moduleBasePath, const std::string &moduleName) -> std::string {
    GFile baseDir(moduleBasePath);
    if (!baseDir.isDirectory()) {
        baseDir = baseDir.parent();
    }

    GFile moduleFile(baseDir, moduleName);
    if (moduleFile.fileSuffix().empty()) {
        moduleFile = GFile(baseDir, moduleName + ".js");
    }
    return moduleFile.absoluteFilePath();
};

static GAnyJS::ExceptionHandler sDefExceptionHandler = [](const std::string &msg) {
    fprintf(stderr, "%s\n", msg.c_str());
};

GAnyJS::FileReader GAnyJSImplQjs::sFileReader = sDefFileReader;
GAnyJS::ModuleNameNormalizeFunc GAnyJSImplQjs::sModuleNormalizeFunc = sDefModuleNormalizeFunc;
GAnyJS::ExceptionHandler GAnyJSImplQjs::sExceptionHandler = sDefExceptionHandler;


std::shared_ptr<GAnyJS> GAnyJS::threadLocal()
{
    thread_local auto vm = std::make_shared<GAnyJSImplQjs>();
    return vm;
}

void GAnyJS::setFileReader(FileReader reader)
{
    if (reader) {
        GAnyJSImplQjs::sFileReader = std::move(reader);
    } else {
        GAnyJSImplQjs::sFileReader = sDefFileReader;
    }
}

void GAnyJS::setModuleNameNormalizeFunc(ModuleNameNormalizeFunc func)
{
    if (func) {
        GAnyJSImplQjs::sModuleNormalizeFunc = std::move(func);
    } else {
        GAnyJSImplQjs::sModuleNormalizeFunc = sDefModuleNormalizeFunc;
    }
}

void GAnyJS::setExceptionHandler(ExceptionHandler handler)
{
    if (handler) {
        GAnyJSImplQjs::sExceptionHandler = std::move(handler);
    } else {
        GAnyJSImplQjs::sExceptionHandler = sDefExceptionHandler;
    }
}

GAnyJSImplQjs::GAnyJSImplQjs()
{
    init();
}

GAnyJSImplQjs::~GAnyJSImplQjs()
{
    shutdownImpl();
}

void GAnyJSImplQjs::shutdown()
{
    shutdownImpl();
}

void GAnyJSImplQjs::gc()
{
    CHECK_CONDITION_V(mJsRuntime != nullptr);

    JS_RunGC(mJsRuntime);
}

bool GAnyJSImplQjs::onUpdate()
{
    CHECK_CONDITION_R(mJSContext != nullptr, false);

    const int r = js_std_loop(mJSContext);
    if (r && sExceptionHandler) {
        const JSValue exceptionVal = JS_GetException(mJSContext);

        std::string exceptionMsg;

        JSValue val;
        const bool isError = JS_IsError(mJSContext, exceptionVal);

        const char *str = JS_ToCString(mJSContext, exceptionVal);
        if (str) {
            exceptionMsg += std::string(str);
            JS_FreeCString(mJSContext, str);
        }

        if (isError) {
            val = JS_GetPropertyStr(mJSContext, exceptionVal, "stack");
        } else {
            js_std_cmd(/*ErrorBackTrace*/2, mJSContext, &val);
        }
        if (!JS_IsUndefined(val)) {
            exceptionMsg += "\n";
            str = JS_ToCString(mJSContext, val);
            if (str) {
                exceptionMsg += std::string(str);
                JS_FreeCString(mJSContext, str);
            }
            JS_FreeValue(mJSContext, val);
        }
        JS_FreeValue(mJSContext, exceptionVal);

        sExceptionHandler(exceptionMsg);
    }

    return r == 0;
}

GAny GAnyJSImplQjs::eval(const std::string &script, const std::string &sourcePath, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    if (shouldEvalAsJsModule(script, sourcePath)) {
        GAny envObj;
        if (env.isObject()) {
            envObj = env;
        } else {
            envObj = GAny::object();
        }

        const JSValue global = JS_GetGlobalObject(ctx);
        const JSAtom envAtom = JS_NewAtom(ctx, "Env");
        const int hasOldEnv = JS_HasProperty(ctx, global, envAtom);
        JSValue oldEnv = JS_UNDEFINED;
        if (hasOldEnv > 0) {
            oldEnv = JS_GetProperty(ctx, global, envAtom);
        }

        JS_SetProperty(ctx, global, envAtom, GAnyToQJS::makeGAnyToJsValue(jsState, envObj, true));

        JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(),
                                  sourcePath.empty() ? "<input>" : sourcePath.c_str(),
                                  JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(funcObj) && js_module_set_import_meta(ctx, funcObj, false, false) < 0) {
            JS_FreeValue(ctx, funcObj);
            funcObj = JS_EXCEPTION;
        }

        JSValue r = JS_EXCEPTION;
        if (!JS_IsException(funcObj)) {
            r = JS_EvalFunction(ctx, funcObj);
            r = js_std_await(ctx, r);
        }

        GAny result = GAnyToQJS::makeJsValueToGAny(jsState, JS_IsException(funcObj) ? funcObj : r);

        JS_FreeValue(ctx, r);
        if (hasOldEnv > 0) {
            JS_SetProperty(ctx, global, envAtom, oldEnv);
        } else {
            JS_FreeValue(ctx, oldEnv);
            JS_DeleteProperty(ctx, global, envAtom, 0);
        }
        JS_FreeAtom(ctx, envAtom);
        JS_FreeValue(ctx, global);

        js_std_loop(ctx);

        return result;
    }

    const JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(),
                        sourcePath.empty() ? "<input>" : sourcePath.c_str(),
                        JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(funcObj)) {
        GAny exception = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return exception;
    }

    GAny envObj;
    if (env.isObject()) {
        envObj = env;
    } else {
        envObj = GAny::object();
    }

    const JSValue global = JS_GetGlobalObject(ctx);

    JSValue argv[] = {
        GAnyToQJS::makeGAnyToJsValue(jsState, envObj, true)
    };
    const JSValue r = JS_Call(ctx, funcObj, global, 1, argv);

    GAny result = GAnyToQJS::makeJsValueToGAny(jsState, r);

    JS_FreeValue(ctx, r);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, funcObj);
    JS_FreeValue(ctx, argv[0]);

    js_std_loop(ctx);

    return result;
}

GAny GAnyJSImplQjs::evalByteCode(const GByteArray &bytes, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue bytecodeFunc = JS_ReadObject(ctx, bytes.data(), bytes.size(), JS_READ_OBJ_BYTECODE);

    if (JS_IsException(bytecodeFunc)) {
        GAny e = GAnyToQJS::makeJsValueToGAny(jsState, bytecodeFunc);
        JS_FreeValue(ctx, bytecodeFunc);
        return e;
    }

    const JSValue funcObj = JS_EvalFunction(ctx, bytecodeFunc);
    if (JS_IsException(funcObj)) {
        GAny e = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return e;
    }
    if (!JS_IsFunction(ctx, funcObj)) {
        GAny e = GAnyException("It's not a function");
        JS_FreeValue(ctx, funcObj);
        return e;
    }

    GAny envObj;
    if (env.isObject()) {
        envObj = env;
    } else {
        envObj = GAny::object();
    }

    const JSValue global = JS_GetGlobalObject(mJSContext);

    JSValue argv[] = {
        GAnyToQJS::makeGAnyToJsValue(jsState, envObj, false)
    };
    const JSValue r = JS_Call(mJSContext, funcObj, global, 1, argv);

    GAny result = GAnyToQJS::makeJsValueToGAny(jsState, r);

    JS_FreeValue(mJSContext, r);
    JS_FreeValue(mJSContext, global);
    JS_FreeValue(mJSContext, funcObj);
    JS_FreeValue(mJSContext, argv[0]);

    js_std_loop(mJSContext);

    return result;
}

GAny GAnyJSImplQjs::evalFile(const std::string &filePath, const GAny &env)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    const GFile file(filePath);

    const GByteArray bytes = sFileReader(filePath);
    if (bytes.isEmpty()) {
        return GAnyException("Failed to read file");
    }

    const GString suffix = GString(file.fileSuffix()).toLower();
    if (suffix == "js" || suffix == "mjs") {
        const std::string content(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        return eval(content, file.absoluteFilePath(), env);
    }
    return evalByteCode(bytes, env);
}

GAny GAnyJSImplQjs::compile(const std::string &script, const std::string &sourcePath)
{
    CHECK_CONDITION_R(mJSContext != nullptr, GAnyException("JS runtime not initialized"));

    JSContext *ctx = mJSContext;
    const JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));

    const JSValue funcObj = JS_Eval(ctx, script.c_str(), script.size(), sourcePath.c_str(),
                                    JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(funcObj)) {
        GAny exception = GAnyToQJS::makeJsValueToGAny(jsState, funcObj);
        JS_FreeValue(ctx, funcObj);
        return exception;
    }

    size_t bytecodeLen;
    uint8_t *bytecodeBuf = JS_WriteObject(ctx, &bytecodeLen, funcObj, JS_WRITE_OBJ_BYTECODE);

    if (!bytecodeBuf) {
        JS_FreeValue(ctx, funcObj);
        return GAnyException("Failed to write bytecode");
    }

    GByteArray result(bytecodeBuf, bytecodeLen);

    JS_FreeValue(ctx, funcObj);
    js_free(ctx, bytecodeBuf);

    return result;
}

// ================================================================

const JS_State * GAnyJSImplQjs::getJSState() const
{
    CHECK_CONDITION_R(mJSContext != nullptr, nullptr);

    return static_cast<JS_State *>(JS_GetContextOpaque(mJSContext));
}

JSModuleDef *GAnyJSImplQjs::JS_moduleLoader(JSContext *ctx, const char *moduleName, void *)
{
    if (isGAnyModuleName(moduleName)) {
        const GAnyModuleExports exports = getGAnyModuleExports(moduleName);
        if (!exports.valid) {
            JS_ThrowReferenceError(ctx, "Could not load GAny module '%s'", moduleName);
            return nullptr;
        }

        JSModuleDef *m = JS_NewCModule(ctx, moduleName, JS_ganyModuleInit);
        if (!m) {
            return nullptr;
        }

        if (JS_AddModuleExport(ctx, m, "default") < 0) {
            return nullptr;
        }
        for (const auto &item: exports.namedExports) {
            if (JS_AddModuleExport(ctx, m, item.first.c_str()) < 0) {
                return nullptr;
            }
        }

        return m;
    }

    const GByteArray buf = sFileReader(moduleName);
    if (buf.isEmpty()) {
        JS_ThrowReferenceError(ctx, "Could not load module filename '%s'", moduleName);
        return nullptr;
    }

    /* compile the module */
    const JSValue funcVal = JS_Eval(ctx, reinterpret_cast<const char *>(buf.data()), buf.size(), moduleName,
                                    JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(funcVal))
        return nullptr;

    if (js_module_set_import_meta(ctx, funcVal, false, false) < 0) {
        JS_FreeValue(ctx, funcVal);
        return nullptr;
    }

    /* the module is already referenced, so we must free it */
    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(funcVal));
    JS_FreeValue(ctx, funcVal);

    return m;
}

char *GAnyJSImplQjs::JS_moduleNormalizeFunc(JSContext *ctx, const char *moduleBaseName, const char *moduleName, void *)
{
    if (moduleName[0] != '.') {
        /* if no initial dot, the module name is not modified */
        return js_strdup(ctx, moduleName);
    }

    const std::string normPath = sModuleNormalizeFunc(moduleBaseName, moduleName);

    char *rPath = js_strdup(ctx, normPath.c_str());

    return rPath;
}

// ================================================================

void GAnyJSImplQjs::init()
{
    static bool sInitedQJS = false;
    if (!sInitedQJS) {
        js_std_set_worker_new_context_func(JS_NewWorkerContext);
        sInitedQJS = true;
    }

    mJsRuntime = JS_NewRuntime();

    js_std_init_handlers(mJsRuntime);

    /* exit on unhandled promise rejections */
    JS_SetHostPromiseRejectionTracker(mJsRuntime, js_std_promise_rejection_tracker, nullptr);

    mJSContext = JS_NewCustomContext(mJsRuntime);
}

void GAnyJSImplQjs::shutdownImpl()
{
    if (mJsRuntime != nullptr) {
        JSRuntime *rt = mJsRuntime;
        JSContext *ctx = mJSContext;

        JS_State *jsState = static_cast<JS_State *>(JS_GetContextOpaque(ctx));
        // 可能存在 Js 中 Export 类型的情况, 则需要在此处先进行反注册, jsState 释放会在 jsStateObjectFinalizer 中完成.
        // 异步 Worker 不被允许 Export 所以无需担心.
        GAnyToQJS::releaseJS(jsState);

        JS_RunGC(mJsRuntime);

        js_std_free_handlers(rt);

        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);

        mJsRuntime = nullptr;
    }
}
