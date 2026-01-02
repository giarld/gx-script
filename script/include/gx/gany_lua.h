//
// Created by Gxin on 24-7-10.
//

#ifndef GX_SCRIPT_GANY_LUA_H
#define GX_SCRIPT_GANY_LUA_H

#include <gx/gany.h>
#include <gx/gbytearray.h>


/**
 * @class GAnyLua
 * @brief Enhanced by GAny, Lua virtual machine supports true multithreading. <br>
 * Features: <br>
 * 1. Start and stop the Lua virtual machine corresponding to the thread; <br>
 * 2. Execute Lua scripts from text, files, or binary streams; <br>
 * 3. Provide Lua virtual machine function setting interface; <br>
 * 4. Exception handling; <br>
 * 5. Directly load and use the GAny plugin; <br>
 * 6. Independent environmental variables and sharing of selectable environmental variables with maximum degrees of freedom; <br>
 * 7. Provide requireLs, which are more convenient and powerful than require; <br>
 * 8. You can directly call the types or returned functions created in Lua through GAny.
 */
class GX_API GAnyLua
{
public:
    using ScriptReader = std::function<GByteArray(const std::string &path)>;

public:
    /**
     * @brief Get the current thread's GAnyLua
     * @return
     */
    static std::shared_ptr<GAnyLua> threadLocal();

    virtual ~GAnyLua() = default;

public:
    /**
     * @brief Actively shut down the virtual machine. After shutting down,
     *        the current virtual machine will become completely outdated.
     *        Do not end a non-current thread virtual machine as it will cause unpredictable errors
     */
    virtual void shutdown() = 0;

    /**
     * @brief Load Lua script file from the set G Any plugin search path and execute it
     * @param name  Script file name (may not have a suffix)
     * @param env   Transferred environment variables
     * @return
     */
    virtual GAny requireLs(const std::string &name, const GAny &env) = 0;

    /**
     * @brief Load and run Lua program from text
     * @param script        Lua script text
     * @param sourcePath    Code source path (file path or URI)
     * @param env           The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Return the script execution result, and if an exception occurs, return a "GAnyException" object
     */
    virtual GAny eval(const std::string &script, std::string sourcePath = "", const GAny &env = GAny::object()) = 0;

    /**
     * @brief Loading and Running Lua Programs from Files
     * @param filePath  Lua script or bytecode file path
     * @param env       The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Return the script execution result, and if an exception occurs, return a "GAnyException" object
     */
    virtual GAny evalFile(const std::string &filePath, const GAny &env = GAny::object()) = 0;

    /**
     * @brief Loading and Running Lua Programs from Bytes Arrays
     * @param buffer    Lua script or bytecode data stream Bytes Arrays
     * @param sourcePath    Code source path (file path or URI)
     * @param env       The environment variable (data) passed to Lua program must be a GAnyObject
     * @return Return the script execution result, and if an exception occurs, return a "GAnyException" object
     */
    virtual GAny evalBuffer(const GByteArray &buffer, std::string sourcePath = "", const GAny &env = GAny::object()) = 0;

    /**
     * @brief Trigger garbage collection for Lua virtual machine
     */
    virtual void gc() = 0;

    /**
     * @brief GC step, Only incremental mode is valid
     * @param kb
     * @return
     */
    virtual bool gcStep(int32_t kb) = 0;

    /**
     * @brief Set GC step rate, Only incremental mode is valid
     * @param mul
     * @return
     */
    virtual int32_t gcSetStepMul(int32_t mul) = 0;

    /**
     * @brief Set GC step interval rate, Only incremental mode is valid
     * @param pause
     * @return
     */
    virtual int32_t gcSetPause(int32_t pause) = 0;

    /**
     * @brief Stop garbage collector
     */
    virtual void gcStop() = 0;

    /**
     * @brief Restart the garbage collector
     */
    virtual void gcRestart() = 0;

    /**
     * @brief Returns whether the garbage collector is running
     * @return
     */
    virtual bool gcIsRunning() = 0;

    /**
     * @brief Returns the amount of memory used by the current Lua virtual machine (in kb)
     * @return
     */
    virtual int32_t gcGetCount() = 0;

    /**
     * @brief Switch garbage collector to generational mode
     */
    virtual void gcModeGen() = 0;

    /**
     * @brief Switch the garbage collector to incremental mode
     */
    virtual void gcModeInc() = 0;

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

    /**
     * @brief Add script file search path.
     * @param path
     */
    static void addSearchPath(const std::string &path);

    /**
     * @brief Remove script file search path.
     * @param path
     */
    static void removeSearchPath(const std::string &path);

    /**
     * @brief Compile from code to generate bytecode
     * @param code          Lua source code
     * @param sourcePath    Code source path (file path or URI)
     * @param strip         Strip debug information
     * @return  bytecode
     */
    virtual GByteArray compileCode(const std::string &code, std::string sourcePath, bool strip) = 0;

    /**
     * @brief Load code from source code file and generate bytecode
     * @param filePath  Path to Lua source code file
     * @param strip     Strip debug information
     * @return  bytecode
     */
    virtual GByteArray compileFile(const std::string &filePath, bool strip) = 0;
};

#endif //GX_SCRIPT_GANY_LUA_H
