//
// Created by Gxin on 2022/4/8.
//

#include <cstdlib>

#define USE_GANY_CORE
#include <gx/gany.h>

#include <gx/reg_gx.h>
#include <gx/reg_script.h>
#include <gx/gany_lua.h>

#include <gx/gfile.h>
#include <gx/debug.h>

#include <iostream>


int main(int argc, char *argv[])
{
#if GX_PLATFORM_WINDOWS
    system("chcp 65001");
#endif

    initGAnyCore();

    GANY_LOAD_MODULE(Gx);
    GANY_LOAD_MODULE(GxScript);

    GAnyLua::setDebugMode(true);

    // 自定义读取器
    GAnyLua::setScriptReader([](const std::string &path) {
        GByteArray buffer;
        GFile file(path);
        if (file.exists() && file.open(GFile::ReadOnly | GFile::Binary)) {
            buffer = file.read();
            file.close();
        }
        return buffer;
    });

    auto lua = GAnyLua::threadLocal();

    lua->gcSetPause(100);

    std::stringstream ss;
    ss << "1. test.lua" << std::endl;
    ss << "2. test_gstring.lua" << std::endl;
    ss << "3. test_gfile.lua" << std::endl;
    ss << "4. test_gbytearray.lua" << std::endl;
    ss << "5. test_gtime.lua" << std::endl;
    ss << "6. test_gthread.lua" << std::endl;
    ss << "7. test_tasksystem.lua" << std::endl;
    ss << "8. test_jobsystem.lua" << std::endl;
    ss << "9. test_table.lua" << std::endl;
    ss << "10. test_env" << std::endl;
    ss << "11. test_iterator" << std::endl;
    ss << "0. all" << std::endl;

    std::cout << ss.str();

    int32_t index = 0;

    std::cout << "input: ";
    std::cin >> index;

    switch (index) {
        default:
        case 0:
        case 1: {
            GFile scriptDir(GFile(GX_EXAMPLE_DIR), "lua/");
            GFile evalFile(scriptDir, "test.lua");

            GAny env = GAny::object();
            env["a"] = 21;
            env["workDir"] = scriptDir.absoluteFilePath();
            env["globalFunc"] = [](const std::string &s) {
                return "***_" + s + "_***";
            };

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.toString());
                return EXIT_FAILURE;
            }

            printf("script ret = %s\n", ret.toString().c_str());

            lua->gc();

            printf("\nCpp use Lua MyType\n\n");

            auto MyType = GAny::Import("L.MyType");

            auto myTypeFunc = MyType.getItem("func2");
            if (myTypeFunc.isFunction()) {
                GAny boundData = myTypeFunc.as<GAnyFunction>()->getBoundData();
                Log("MyType.func2 info: \n{}", boundData.toJsonString(2));
            }

            std::cout << MyType << std::endl;
            auto myObj = MyType(3, 3.14);
            myObj.call("func");
            printf("myObj.func2(100, 2) = %d\n", myObj.call("func2", 100, 2).toInt32());
            printf("myObj.a = %d\n", myObj.getItem("a").toInt32());
            printf("myObj.b = %s\n", myObj.getItem("b").toString().c_str());
            printf("myObj.c = %s\n", myObj.getItem("c").toString().c_str());

            auto c = myObj.getItem("c");
            c.setItem(3, "c++ add");
            c.setItem("t", false);

            auto ts = GAny::Import("Gx.GTaskSystem")(1);
            ts.call("start");

            auto task = ts.call("submit", [&]() {
                // C++ 多线程操作Lua Table.
                c.delItem(1);
                c.setItem(4, 3.14f);
            });
            task.call("wait");

            printf("c.length = %d\n", (int) c.length());
            printf("c.toString = %s\n", c.toString().c_str());
            printf("myObj.c = %s\n", myObj.getItem("c").toString().c_str());

            ts.call("submit", [&]() {
                auto lua2 = GAnyLua::threadLocal();
                GFile evalFile2(GFile(GX_EXAMPLE_DIR), "lua/test_lua_class_multi_state.lua");
                lua2->evalFile(evalFile2.absoluteFilePath(), env);
                lua2->gc();
            });

            ts.call("stopAndWait");
        }
            if (index != 0) {
                break;
            }
        case 2: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_gstring.lua");

            GAny env = GAny::object();

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 3: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_gfile.lua");

            GAny env = GAny::object();

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 4: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_gbytearray.lua");

            if (evalFile.open(GFile::ReadOnly | GFile::Binary)) {
                GAny env = GAny::object();

                GByteArray buffer = evalFile.read();
                GAny ret = lua->evalBuffer(buffer, evalFile.filePath(), env);
                if (ret.isException()) {
                    LogE("{}", ret.as<GAnyException>()->what());
                    return EXIT_FAILURE;
                }
            }
        }
            if (index != 0) {
                break;
            }
        case 5: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_gtime.lua");

            GAny env = GAny::object();
            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 6: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_gthread.lua");

            GAny env = GAny::object();

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 7: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_tasksystem.lua");

            GAny env = GAny::object();
            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 8: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_jobsystem.lua");

            GAny env = GAny::object();

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 9: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_table.lua");

            GAny env = GAny::object();

            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
            if (index != 0) {
                break;
            }
        case 10: {
            std::string script = R"(print("\n --- Env test ---\n");
if G then
    print("func lua type:", type(G.func));
    if G.func then
        print("func GAny type:", G.func:_typeName());
        G.func();

        G.lFunc = function()
            print("call lua func");
        end
    end
else
    print("G undefined.");
end
)";
            GAny env = GAny::object();
            env["func"] = []() {
                printf("call func\n");
            };

            GAny ret = lua->eval(script, "", env);    // set env
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }

            if (env["lFunc"].isFunction()) {
                env["lFunc"]();
            }

            ret = lua->eval(script);         // not set env
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
            env.erase("func");
        }
            if (index != 0) {
                break;
            }
        case 11: {
            GFile evalFile(GFile(GX_EXAMPLE_DIR), "lua/test_iterator.lua");

            GAny env = GAny::object();
            GAny ret = lua->evalFile(evalFile.absoluteFilePath(), env);
            if (ret.isException()) {
                LogE("{}", ret.as<GAnyException>()->what());
                return EXIT_FAILURE;
            }
        }
    }

    lua->gc();
    lua->shutdown();

    return EXIT_SUCCESS;
}