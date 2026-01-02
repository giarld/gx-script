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
};

#endif //GX_GANY_JS_H
