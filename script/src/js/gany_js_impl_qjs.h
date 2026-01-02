//
// Created by Gxin on 25-5-23.
//

#ifndef GX_GANY_JS_IMPL_QJS_H
#define GX_GANY_JS_IMPL_QJS_H

#include "gx/gany_js.h"

#include "quickjs-libc.h"

#include <gx/gthread.h>


struct JS_State
{
    JSRuntime *rt = nullptr;
    JSContext *ctx = nullptr;
    GThread::ThreadIdType threadId;
    JSClassID ganyClassID = 0;
    uint64_t ganyModuleIdx = 0;
};

extern JSContext *JS_NewCustomContext(JSRuntime *rt, bool isWorker = false);

extern JSContext *JS_NewWorkerContext(JSRuntime *rt);


class GAnyJSImplQjs : public GAnyJS
{
public:
    explicit GAnyJSImplQjs();

    ~GAnyJSImplQjs() override;

public:
    void shutdown() override;

    void gc() override;

    bool onUpdate() override;

    GAny eval(const std::string &script, const std::string &sourcePath, const GAny &env) override;

    GAny evalByteCode(const GByteArray &bytes, const GAny &env) override;

    GAny evalFile(const std::string &filePath, const GAny &env) override;

    GAny compile(const std::string &script, const std::string &sourcePath) override;

public:
    const JS_State *getJSState() const;

    static JSModuleDef *JS_moduleLoader(JSContext *ctx, const char *moduleName, void *opaque);

    static char *JS_moduleNormalizeFunc(JSContext *ctx, const char *moduleBaseName, const char *moduleName, void *opaque);

private:
    void init();

    void shutdownImpl();

private:
    friend class GAnyJS;
    friend class GAnyToQJS;

    static FileReader sFileReader;
    static ModuleNameNormalizeFunc sModuleNormalizeFunc;
    static ExceptionHandler sExceptionHandler;

    JSRuntime *mJsRuntime;
    JSContext *mJSContext;
};

#endif //GX_GANY_JS_IMPL_QJS_H
