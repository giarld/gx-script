//
// Created by Gxin on 25-5-24.
//

#include <cstdlib>

#define USE_GANY_CORE
#include <gx/gany.h>

#include <gx/reg_gx.h>
#include <gx/reg_script.h>
#include <gx/gany_js.h>

#include "gx/debug.h"
#include "gx/gfile.h"


using namespace gany;

struct MyType
{
    int32_t v;

    constexpr static int32_t M = 10;

    void callbackV(std::function<void(int32_t)> cb) const
    {
        cb(v);
    }
};

int main(int argc, char *argv[])
{
#if GX_PLATFORM_WINDOWS
    system("chcp 65001");
#endif

    initGAnyCore();

    GANY_LOAD_MODULE(Gx);
    GANY_LOAD_MODULE(GxScript);

    Class<MyType>("", "MyType", "")
        .construct<>()
        .readWrite("v", &MyType::v)
        .constant("M", MyType::M)
        .func("callbackV", &MyType::callbackV);

    const auto js = GAnyJS::threadLocal();

    // GFile f = GFile(GX_EXAMPLE_DIR).concat("js/test.js");
    // if (f.open(GFile::ReadOnly)) {
    //     const GString script = f.readAll();
    //     f.close();
    //
    //     GAny compileRet = js->compile(script.toStdString(), f.absoluteFilePath());
    //     if (compileRet.isException()) {
    //         LogE("Compile Error: \n{}", compileRet.as<GAnyException>()->toString());
    //     } else {
    //         const GByteArray *bytecode = compileRet.as<GByteArray>();
    //
    //         GAny env;
    //         env["title"] = "TestScriptJS";
    //         const GAny ret = js->evalByteCode(*bytecode, env);
    //         Log("Script return: \n{}", ret.toString());
    //     }
    // }

    const std::string path = GFile(GX_EXAMPLE_DIR).concat("js/test.js").absoluteFilePath();

    GAny env;
    env["title"] = "TestScriptJS";

    const GAny ret = js->evalFile(path, env);
    if (ret.isException()) {
        LogE("Script Error: \n{}", ret.toString());
    } else {
        Log("Script Return: \n{}", ret.toString());
    }

    js->onUpdate();

    js->gc();

    // 主线程必须主动停机
    js->shutdown();

    return EXIT_SUCCESS;
}
