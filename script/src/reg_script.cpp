//
// Created by Gxin on 2023/5/9.
//

#include "gx/reg_script.h"

#include "gx/gany.h"
#include <gx/gbytearray.h>
#include <gx/gfile.h>
#include <gx/debug.h>

#include "lua/lua_table.h"
#include "lua/gany_lua_impl.h"

#include "gx/gany_js.h"


using namespace gany;

REGISTER_GANY_MODULE(GxScript)
{
    Class<LuaTable>("L", "LuaTable", "lua table compatible types.")
        .construct<>()
        .construct<const LuaTable &>()
        .func(MetaFunction::ToString, &LuaTable::toString)
        .func(MetaFunction::Length, &LuaTable::length)
        .func(MetaFunction::SetItem, &LuaTable::setItem)
        .func(MetaFunction::GetItem, &LuaTable::getItem)
        .func(MetaFunction::DelItem, &LuaTable::delItem)
        .func(MetaFunction::ToObject, &LuaTable::toObject)
        .func("iterator", &LuaTable::iterator, {.doc = "Get iterator."})
        .staticFunc("fromGAnyObject", &LuaTable::fromGAnyObject);

    // GAny LuaTable iterator, Special provision of reverse iteration function
    GAnyClass::Class<LuaTableIterator>()
        ->setName("LuaTableIterator")
        .setNameSpace("L")
        .setDoc("Lua table iterator.")
        .func("hasNext", &LuaTableIterator::hasNext)
        .func("next", &LuaTableIterator::next)
        .func("remove", &LuaTableIterator::remove)
        .func("hasPrevious", &LuaTableIterator::hasPrevious)
        .func("previous", &LuaTableIterator::previous)
        .func("toFront", &LuaTableIterator::toFront)
        .func("toBack", &LuaTableIterator::toBack);

    // Add LuaTable serialization and deserialization capabilities to GByteArray
    GAnyClass::Class<GByteArray>()
        ->func("writeTable", [](GByteArray &self, const LuaTable &value) {
            GByteArray buf;
            LuaTable::writeToByteArray(buf, value);
            self.write(buf);
        })
        .func("readTable", [](GByteArray &self) {
            GByteArray buf;
            self.read(buf);
            return LuaTable::readFromByteArray(buf);
        });

    Class<GAnyLua>("Gx", "GAnyLua", "GAny lua vm.")
        .staticFunc("threadLocal", &GAnyLua::threadLocal)
        .func("shutdown", &GAnyLua::shutdown,
              {
                  .doc = "Actively shut down the virtual machine. \n"
                  "After shutting down, the current virtual machine will become completely outdated. \n"
                  "Do not end a non current thread virtual machine as it will cause unpredictable errors."
              })
        .func("eval", [](GAnyLua &self, const std::string &script) {
            return self.eval(script);
        }, {
            .doc = "Load and run Lua program from text. \n"
            "script: Lua script text; \n"
            "return: Returns the return value of the script.",
            .args = {"script"}
        })
        .func("eval", [](GAnyLua &self, const std::string &script, const GAny &env) {
            return self.eval(script, "", env);
        }, {
            .doc = "Load and run Lua program from text. \n"
            "script: Lua script text; \n"
            "env: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
            "return: Returns the return value of the script.",
            .args = {"script", "env"}
        })
        .func("eval",
              [](GAnyLua &self, const std::string &script, const std::string &sourcePath, const GAny &env) {
                  return self.eval(script, sourcePath, env);
              }, {
                  .doc = "Load and run Lua program from text. \n"
                  "script: Lua script text; \n"
                  "sourcePath: Code source path (file path or URI); \n"
                  "env: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
                  "return: Returns the return value of the script.",
                  .args = {"script", "sourcePath", "env"}
              })
        .func("evalFile", [](GAnyLua &self, const std::string &filePath) {
            return self.evalFile(filePath);
        }, {
            .doc = "Loading and Running Lua Programs from Files. \n"
            "filePath: Lua script or bytecode file path; \n"
            "return: Returns the return value of the script.",
            .args = {"filePath"}
        })
        .func("evalFile", [](GAnyLua &self, const std::string &filePath, const GAny &env) {
            return self.evalFile(filePath, env);
        }, {
            .doc = "Loading and Running Lua Programs from Files. \n"
            "filePath: Lua script or bytecode file path; \n"
            "env: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
            "return: Returns the return value of the script.",
            .args = {"filePath", "env"}
        })
        .func("evalBuffer", [](GAnyLua &self, const GByteArray &buffer) {
            return self.evalBuffer(buffer);
        }, {
            .doc = "Loading and Running Lua Programs from Bytes Arrays. \n"
            "buffer: Lua script or bytecode data stream Bytes Arrays; \n"
            "return: Returns the return value of the script.",
            .args = {"buffer"}
        })
        .func("evalBuffer", [](GAnyLua &self, const GByteArray &buffer, const GAny &env) {
            return self.evalBuffer(buffer, "", env);
        }, {
            .doc = "Loading and Running Lua Programs from Bytes Arrays. \n"
            "buffer: Lua script or bytecode data stream Bytes Arrays; \n"
            "env: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
            "return: Returns the return value of the script.",
            .args = {"buffer", "env"}
        })
        .func("evalBuffer",
              [](GAnyLua &self, const GByteArray &buffer, const std::string &sourcePath, const GAny &env) {
                  return self.evalBuffer(buffer, sourcePath, env);
              }, {
                  .doc = "Loading and Running Lua Programs from Bytes Arrays. \n"
                  "buffer: Lua script or bytecode data stream Bytes Arrays; \n"
                  "sourcePath: Code source path (file path or URI); \n"
                  "env: The environment variable (data) passed to Lua program must be a GAnyObject; \n"
                  "return: Returns the return value of the script.",
                  .args = {"buffer", "sourcePath", "env"}
              })
        .func("gc", &GAnyLua::gc, {.doc = "Trigger garbage collection for Lua virtual machine."})
        .func("gcStep", &GAnyLua::gcStep, {.doc = "GC step, Only incremental mode is valid.", .args = {"kb"}})
        .func("gcSetStepMul", &GAnyLua::gcSetStepMul, {.doc = "Set GC step rate, Only incremental mode is valid.", .args = {"mul"}})
        .func("gcSetPause", &GAnyLua::gcSetPause, {.doc = "Set GC step interval rate, Only incremental mode is valid.", .args = {"pause"}})
        .func("gcStop", &GAnyLua::gcStop, {.doc = "Stop garbage collector."})
        .func("gcRestart", &GAnyLua::gcRestart, {.doc = "Restart the garbage collector."})
        .func("gcIsRunning", &GAnyLua::gcIsRunning, {.doc = "Returns whether the garbage collector is running."})
        .func("gcGetCount", &GAnyLua::gcGetCount, {.doc = "Returns the amount of memory used by the current Lua virtual machine (in kb)."})
        .func("gcModeGen", &GAnyLua::gcModeGen, {.doc = "Switch garbage collector to generational mode."})
        .func("gcModeInc", &GAnyLua::gcModeInc, {.doc = "Switch the garbage collector to incremental mode."})
        .staticFunc("setDebugMode", &GAnyLua::setDebugMode, {.doc = "Set debugging mode.", .args = {"debugMode"}})
        .staticFunc("setScriptReader", &GAnyLua::setScriptReader,
                    {
                        .doc = "Set up a script reader. If a custom script reader is set up, "
                        "the custom reader will be called when using \"scriptFile\" and \"requireLs\" to read the script file.",
                        .args = {"reader"}
                    })
        .staticFunc("addSearchPath", &GAnyLua::addSearchPath,
                    {
                        .doc = "Add script file search path.",
                        .args = {"path"}
                    })
        .staticFunc("removeSearchPath", &GAnyLua::removeSearchPath,
                    {
                        .doc = "Remove script file search path.",
                        .args = {"path"}
                    })
        .func("compileCode", &GAnyLua::compileCode,
              {
                  .doc = "Compile from code to generate bytecode.\n"
                  "code: Lua source code;\n"
                  "sourcePath: Code source path (file path or URI);\n"
                  "strip: Strip debug information;\n"
                  "return: bytecode.",
                  .args = {"code", "sourcePath", "strip"}
              })
        .func("compileFile", &GAnyLua::compileFile,
              {
                  .doc = "Load code from source code file and generate bytecode.\n"
                  "filePath: Path to Lua source code file;\n"
                  "strip: Strip debug information;\n"
                  "return: bytecode.",
                  .args = {"filePath", "strip"}
              });

    Class<GAnyJS>("Gx", "GAnyJS", "GAny js vm.")
        .staticFunc("threadLocal", GAnyJS::threadLocal)
        .staticFunc("setFileReader", &GAnyJS::setFileReader)
        .staticFunc("setModuleNameNormalizeFunc", &GAnyJS::setModuleNameNormalizeFunc)
        .func("shutdown", &GAnyJS::shutdown)
        .func("gc", &GAnyJS::gc)
        .func("onUpdate", &GAnyJS::onUpdate)
        .func("eval", [](GAnyJS &self, const std::string &script, const std::string &sourcePath, const GAny &env) {
            return self.eval(script, sourcePath, env);
        })
        .func("eval", [](GAnyJS &self, const std::string &script, const std::string &sourcePath) {
            return self.eval(script, sourcePath);
        })
        .func("evalByteCode", [](GAnyJS &self, const GByteArray &bytes, const GAny &env) {
            return self.evalByteCode(bytes, env);
        })
        .func("evalByteCode", [](GAnyJS &self, const GByteArray &bytes) {
            return self.evalByteCode(bytes);
        })
        .func("evalFile", [](GAnyJS &self, const std::string &filePath, const GAny &env) {
            return self.evalFile(filePath, env);
        })
        .func("evalFile", [](GAnyJS &self, const std::string &filePath) {
            return self.evalFile(filePath);
        })
        .func("compile", &GAnyJS::compile);
}
