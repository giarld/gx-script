//
// Created by Gxin on 25-5-23.
//

#ifndef GX_GANY_JS_H
#define GX_GANY_JS_H

#include <gx/gany.h>
#include <gx/gbytearray.h>

#include "gx/gfile.h"


/**
 * @class GAnyJS
 */
class GX_API GAnyJS
{
public:
    using FileReader = std::function<GByteArray(const std::string &path)>;
    using ModuleNameNormalizeFunc = std::function<std::string(const std::string &moduleBasePath, const std::string &moduleName)>;
    using ExceptionHandler = std::function<void(const std::string&)>;

public:
    static std::shared_ptr<GAnyJS> threadLocal();

    virtual ~GAnyJS() = default;

public:
    static void setFileReader(FileReader reader);

    static void setModuleNameNormalizeFunc(ModuleNameNormalizeFunc func);

    static void setExceptionHandler(ExceptionHandler handler);

    virtual void shutdown() = 0;

    virtual void gc() = 0;

    virtual bool onUpdate() = 0;

    virtual GAny eval(const std::string &script, const std::string &sourcePath, const GAny &env = GAny::object()) = 0;

    virtual GAny evalByteCode(const GByteArray &bytes, const GAny &env = GAny::object()) = 0;

    virtual GAny evalFile(const std::string &filePath, const GAny &env = GAny::object()) = 0;

    /**
     * 编译 js 代码为字节码
     * @param script
     * @param sourcePath
     * @return 成功返回 GByteArray 类型数据, 失败返回 GAnyException
     */
    virtual GAny compile(const std::string &script, const std::string &sourcePath) = 0;

    /**
     * 设置一个源码行断点。
     * @param file      绝对路径、路径后缀，或 "." 表示当前暂停/执行中的源码文件
     * @param line      1-based 源码行号
     * @param condition 可选断点条件，truthy 时暂停
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny setBreakpoint(const std::string &file, int line, const std::string &condition = "") = 0;

    /**
     * 清除一个源码行断点。
     * @param file 绝对路径、路径后缀，或 "." 表示当前暂停/执行中的源码文件
     * @param line 1-based 源码行号
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny clearBreakpoint(const std::string &file, int line) = 0;

    /**
     * 清除当前运行时中的全部断点。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny clearAllBreakpoints() = 0;

    /**
     * 返回当前运行时中的全部断点。
     * 返回格式与 JS `GxDebugger.listBreakpoints()` 一致。
     */
    virtual GAny listBreakpoints() const = 0;

    /**
     * 请求在下一次可停 JS 字节码位置暂停。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny pause() = 0;

    /**
     * 从 debugger 暂停状态继续执行。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny resume() = 0;

    /**
     * 单步进入。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny stepInto() = 0;

    /**
     * 单步越过。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny stepOver() = 0;

    /**
     * 单步跳出当前 JS 函数。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny stepOut() = 0;

    /**
     * 设置断点命中时是否触发原生 SIGTRAP。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny setTrapOnBreak(bool enabled = true) = 0;

    /**
     * 设置断点命中时是否进入命令行交互模式。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny setInteractiveOnBreak(bool enabled = true) = 0;

    /**
     * 设置断点命中时是否打印当前 JS 调用堆栈。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny setPrintStackOnBreak(bool enabled = true) = 0;

    /**
     * 设置运行时 JS 异常抛出时是否暂停。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny setPauseOnException(bool enabled = true) = 0;

    /**
     * 添加一个 watch 表达式。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny watch(const std::string &expression) = 0;

    /**
     * 清除一个 watch 表达式。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny clearWatch(const std::string &expression) = 0;

    /**
     * 清除当前运行时中的全部 watch 表达式。
     * @return 成功返回 undefined，失败返回 GAnyException
     */
    virtual GAny clearAllWatches() = 0;

    /**
     * 返回当前 debugger 暂停状态快照，结构稳定，便于宿主直接读取字段。
     *
     * 返回对象结构如下：
     * {
     *   paused: bool,
     *   reason: "pause" | "step" | "breakpoint" | "exception" | "debuggerStatement" | null,
     *   location: {
     *     file: string,
     *     line: int,
     *     col: int,
     *     frameId: int64,
     *     frameDepth: int64,
     *     pcOffset: int64
     *   } | null,
     *   pendingPause: bool,
     *   step: {
     *     pending: bool,
     *     kind: "into" | "over" | "out" | null,
     *     origin: {
     *       file: string,
     *       line: int,
     *       col: int,
     *       frameId: int64,
     *       frameDepth: int64,
     *       pcOffset: int64
     *     } | null
     *   },
     *   breakpointsCount: int64,
     *   watchesCount: int64
     * }
     *
     * 语义说明：
     * - paused: 当前是否真的阻塞在 debugger 中
     * - reason: 当前/最近一次暂停原因
     * - location: 当前/最近一次暂停位置
     * - pendingPause: 是否已经请求 pause()，但尚未停到下一个可暂停位置
     * - step.pending: 是否已经请求 step*()，但尚未停到下一处位置
     * - step.kind / step.origin: 当前待执行 step，或导致最近一次 step 暂停的步进信息
     * - breakpointsCount / watchesCount: 当前断点数和 watch 表达式数
     *
     * @return 成功返回上述对象；若运行时未初始化，返回 GAnyException
     */
    virtual GAny getPauseState() const = 0;
};

#endif //GX_GANY_JS_H
