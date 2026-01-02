/**
 * @fileoverview GAny QuickJS Binding - Code Hints & Type Definitions
 *
 * 这个文件为通过 gany_to_js.cpp 暴露给 QuickJS 的 GAny API 提供 JSDoc 定义。
 * 它旨在帮助 IDE 提供代码自动补全和类型提示。
 * 注意：由于 GAny 的动态性，此文件可能无法包含所有通过 C++ 注册的类和方法。
 * @version 1.4.2
 */

// ======================================================================
// GAny Core Types & Interfaces
// ======================================================================

/**
 * @class GAnyUserObject
 * @description 代表一个通过 QuickJS 访问的 GAny C++ 对象。
 * 它可以是数字、字符串、布尔值、数组、对象、函数、类或自定义用户对象。
 * 它暴露了一系列以 '_' 开头的通用方法，以及根据 C++ 定义动态绑定的方法/属性。
 */
class GAnyUserObject {

    /**
     * @private
     * @constructor
     * @description GAnyUserObject 不能直接通过 'new GAnyUserObject()' 创建，
     * 而应使用 GAny.create(), GAny.import() 或 C++ 函数的返回值获取。
     */
    constructor() {
        throw new Error("GAnyUserObject cannot be instantiated directly in JS.");
    }

    // --- 通用方法 (来自 setGAnyGeneralProto) ---

    /**
     * 克隆当前的 GAny 对象。
     * @returns {GAnyUserObject} 一个新的 GAnyUserObject 实例，是原始对象的克隆。
     */
    _clone() {}

    /**
     * 获取 GAny 对象在 C++ 中注册的类名（不含命名空间）。
     * @returns {string} C++ 类名。
     */
    _classTypeName() {}

    /**
     * 获取 GAny 对象的类型名称（如 "int", "string", "object", "MyClass" 等）。
     * @returns {string} GAny 类型名称。
     */
    _typeName() {}

    /**
     * 获取 GAny 对象的内部类型枚举值。
     * @returns {number} 对应 AnyType 枚举的值。
     */
    _type() {}

    /**
     * 获取 GAny 对象（通常是数组或字符串）的长度。
     * @returns {number} 长度。
     */
    _length() {}

    /**
     * 获取 GAny 对象（通常是容器）的大小。
     * @returns {number} 大小。
     */
    _size() {}

    /**
     * 获取 GAny 对象的字符串表示形式（通常是调试信息或类结构）。
     * @returns {string} 对象的 dump 信息。
     */
    _dump() {}

    /**
     * 获取此 GAny 对象所属的 GAnyClass 对象。
     * @returns {GAnyClass} 代表此对象类的 GAnyClass 对象。
     */
    _classObject() {}

    /**
     * 检查 GAny 对象是否是指定的 C++ 类型。
     * @param {string} typeName - 要检查的 C++ 类型名称 (例如 "Core.Vector3")。
     * @returns {boolean} 如果是指定类型，则返回 true。
     */
    _is(typeName) {}

    /** @returns {boolean} 是否是 undefined 类型。 */
    _isUndefined() {}
    /** @returns {boolean} 是否是 null 类型。 */
    _isNull() {}
    /** @returns {boolean} 是否是 function 类型。 */
    _isFunction() {}
    /** @returns {boolean} 是否是 class 类型。 */
    _isClass() {}
    /** @returns {boolean} 是否是 exception 类型。 */
    _isException() {}
    /** @returns {boolean} 是否是 enum 类型。 */
    _isObject() {}
    /** @returns {boolean} 是否是 array 类型。 */
    _isArray() {}
    /** @returns {boolean} 是否是 int 类型。 */
    _isInt() {}
    /** @returns {boolean} 是否是 float 类型。 */
    _isFloat() {}
    /** @returns {boolean} 是否是 double 类型。 */
    _isDouble() {}
    /** @returns {boolean} 是否是 number (int, float, double) 类型。 */
    _isNumber() {}
    /** @returns {boolean} 是否是 string 类型。 */
    _isString() {}
    /** @returns {boolean} 是否是 boolean 类型。 */
    _isBoolean() {}
    /** @returns {boolean} 是否是 user object 类型。 */
    _isUserObject() {}

    /**
     * 将 GAny 对象转换为字符串。对于非字符串类型，行为类似于 JS 的 toString()。
     * @returns {string} 字符串表示。
     */
    _toString() {}

    /**
     * 将 GAny 对象转换为 32 位整数。
     * @returns {number} 32 位整数。
     */
    _toInt32() {}

    /**
     * 将 GAny 对象转换为 64 位整数 (在 JS 中可能表现为 Number 或 BigInt，取决于 QuickJS 版本和值大小，但通常是 Number)。
     * @returns {number|BigInt} 64 位整数。
     */
    _toInt64() {}

    /**
     * 将 GAny 对象转换为浮点数。
     * @returns {number} 浮点数。
     */
    _toFloat() {}

    /**
     * 将 GAny 对象转换为布尔值。
     * @returns {boolean} 布尔值。
     */
    _toBool() {}

    /**
     * 将 GAny 对象转换为其 JS 原生表示（如果可能）。
     * **注意**: 这可能返回一个包装的 GAnyUserObject 或一个原生的 JS 值。
     * @returns {GAnyUserObject|any} JS 值。
     */
    _toJsValue() {}

    /**
     * 将 GAny 对象转换为 JSON 字符串。
     * @param {number} [indent=-1] - 缩进空格数。-1 表示不缩进。
     * @returns {string} JSON 字符串。
     */
    _toJsonString(indent = -1) {}

    /**
     * 检查 GAny 对象（数组或对象）是否包含指定键或值。
     * @param {any} keyOrValue - 要检查的键或值。
     * @returns {boolean} 如果包含，则返回 true。
     */
    _contains(keyOrValue) {}

    /**
     * 从 GAny 对象（数组或对象）中移除元素。
     * @param {any} keyOrIndex - 要移除的键或索引。
     */
    _erase(keyOrIndex) {}

    /**
     * 向 GAny 对象（数组）末尾添加元素。
     * @param {any} value - 要添加的值。
     */
    _pushBack(value) {}

    /**
     * 设置 GAny 对象（数组或对象）中指定键或索引的值。
     * @param {any} keyOrIndex - 键或索引。
     * @param {any} value - 要设置的值。
     * @returns {boolean} 如果设置成功，则返回 true。
     */
    _setItem(keyOrIndex, value) {}

    /**
     * 获取 GAny 对象（数组或对象）中指定键或索引的值。
     * **注意**: 返回的值可能是包装的 GAnyUserObject 或原生的 JS 值。
     * @param {any} keyOrIndex - 键或索引。
     * @returns {GAnyUserObject|any} 获取到的值。
     */
    _getItem(keyOrIndex) {}

    /**
     * 删除 GAny 对象（数组或对象）中指定键或索引的值。
     * @param {any} keyOrIndex - 键或索引。
     * @returns {boolean} 如果删除成功，则返回 true。
     */
    _delItem(keyOrIndex) {}

    /**
     * 获取 GAny 对象（数组或对象）的迭代器。
     * @returns {GAnyIterator} 一个 GAny 迭代器对象。
     */
    _iterator() {}

    /**
     * [仅用于 GAnyUserObject 或 GAnyIterator] 检查迭代器是否还有下一个元素。
     * GAnyUserObject 本身也可能直接实现迭代器协议的 _hasNext。
     * @returns {boolean} 如果有，则返回 true。
     */
    _hasNext() {}

    /**
     * [仅用于 GAnyUserObject 或 GAnyIterator] 获取迭代器的下一个元素。
     * GAnyUserObject 本身也可能直接实现迭代器协议的 _next。
     * 返回的 GAnyUserObject 代表一个键值对，它拥有 'key' 和 'value' 属性。
     * - item.key: 代表迭代的键（对于数组是索引，对于对象是属性名）。访问时会尽可能转换为原生 JS 类型 (number/string)。
     * - item.value: 代表迭代的值。它本身通常是一个 GAnyUserObject，或者在访问时尽可能转换为原生 JS 类型。
     * @example
     * // let arr = GAny.create([1,2,3]);
     * // let it = arr._iterator(); // 或者 arr 本身就是迭代器
     * // while (it._hasNext()) {
     * //   let i = it._next();
     * //   print("Key:", i.key, "Value:", i.value); // 假设 print 能处理 GAnyUserObject 或其原生转换
     * //   // 若要确保原生，可以：
     * //   // print("Key:", i.getItem('key')._toString(), "Value:", i.getItem('value')._toString());
     * // }
     * @returns {GAnyUserObject} 一个 GAnyUserObject，该对象具有 'key' 和 'value' 属性。
     */
    _next() {}

    /**
     * [仅用于 GAnyFunction] 调用 GAny 函数。
     * @param {...any} args - 传递给函数的参数。
     * @returns {GAnyUserObject|any} 函数调用的结果。
     */
    _call(...args) {}

    /**
     * @description GAnyUserObject 实例可以拥有在 C++ 中动态定义的属性和方法。
     * 例如：如果 C++ 中注册了一个名为 'myMethod' 的方法，可以直接调用 `instance.myMethod()`。
     * 对于属性，可以使用 `instance.myProperty` 或 `instance.myProperty = value`。
     * 由于这些成员是动态绑定的，它们无法在此静态定义中列出。
     * 请参考相关的 C++ GAnyClass 定义来了解特定实例可用的成员。
     * @example
     * let myInstance = GAny.import("MyNamespace.MyType").new();
     * // 假设 MyType 在 C++ 中定义了 myDynamicMethod 和 myDynamicProperty
     * myInstance.myDynamicMethod("hello");
     * myInstance.myDynamicProperty = 123;
     * console.log(myInstance.myDynamicProperty);
     */
}

/**
 * @class GAnyIterator
 * @description 代表一个 GAny 对象的迭代器。
 * 通常通过调用 GAnyUserObject 的 `_iterator()` 方法获得。
 * @extends GAnyUserObject
 */
class GAnyIterator extends GAnyUserObject {
    /**
     * 检查迭代器是否还有下一个元素。
     * @returns {boolean} 如果有，则返回 true。
     */
    _hasNext() {}

    /**
     * 获取迭代器的下一个元素。
     * 返回的 GAnyUserObject 代表一个键值对，它拥有 'key' 和 'value' 属性。
     * - `item.key`: 代表迭代的键（对于数组是索引，对于对象是属性名）。访问时，其值可能是一个原生JS类型（number/string）或一个GAnyUserObject。
     * - `item.value`: 代表迭代的值。访问时，其值可能是一个原生JS类型或一个GAnyUserObject。
     * @example
     * // let arr = GAny.create([1, 2, 3]);
     * // let it = arr._iterator();
     * // while (it._hasNext()) {
     * //   let i = it._next(); // i 是一个 GAnyUserObject, 代表键值对
     * //   // 假设 print 可以处理 GAnyUserObject 或其 .key .value 属性返回的原生/GAnyUserObject
     * //   print(">", i.key, i.value);
     * //   // 更明确地处理，假设 i.key 和 i.value 返回的是 GAnyUserObject
     * //   // print(">> Key:", i.getItem('key')._toString(), "Value:", i.getItem('value')._toString());
     * // }
     * @returns {GAnyUserObject} 一个 GAnyUserObject，该对象具有 'key' 和 'value' 属性。
     */
    _next() {}
}

/**
 * @class GAnyClass
 * @description 代表一个 GAny Class 定义，导入或创建在 JS 中。
 * 它继承自 GAnyUserObject 并添加了 'new' 方法用于实例化。
 * 它也可能包含静态方法和属性。
 * @extends GAnyUserObject
 */
class GAnyClass extends GAnyUserObject {
    /**
     * @private
     * @constructor
     */
    constructor() {
        super();
        throw new Error("GAnyClass cannot be instantiated directly in JS.");
    }

    /**
     * @description GAnyClass 对象可以拥有在 C++ 中动态定义的静态属性和方法。
     * 例如：如果 C++ 中注册了一个名为 'MyStaticMethod' 的静态方法，可以调用 `MyGAnyClass.MyStaticMethod()`。
     * 由于这些成员是动态绑定的，它们无法在此静态定义中列出。
     * 请参考相关的 C++ GAnyClass 定义来了解特定类可用的静态成员。
     * @example
     * let MyTypeClass = GAny.import("MyNamespace.MyType");
     * // 假设 MyType 在 C++ 中定义了 MyStaticMethod 和 MyStaticProperty
     * MyTypeClass.MyStaticMethod();
     * MyTypeClass.MyStaticProperty = "static value";
     */
}


// ======================================================================
// Global GAny Object & Functions
// ======================================================================

/**
 * @namespace GAny
 * @description 全局 GAny 对象，提供创建、导入和管理 GAny 对象的入口点。
 */
const GAny = {
    /**
     * 创建一个新的 GAny 对象。
     * 如果传入 JS 基础类型，会尝试转换为对应的 GAny 类型。
     * 如果传入 JS 对象或数组，会创建 GAny 对象或数组。
     * @param {any} value - 要包装或转换的 JS 值。
     * @returns {GAnyUserObject|any} 新创建的 GAny 对象。
     */
    create: function(value) {},

    /**
     * 创建一个多线程可调用的调用器函数
     * @param {string} input - 输入, 脚本代码或脚本文件路径
     * @param {GAnyUserObject|object} env - 环境变量对象, 可选
     * @returns {GAnyUserObject} 新创建的 GAny 函数对象, 可使用 _call 调用，或传递给线程。
     */
    createWorkerCallable: function(input, env = {}) {},

    /**
     * 从 C++ 注册表中导入一个 GAny 类。
     * @param {string} path - 类的完整路径 (例如 "Core.Vector3" 或 "MyGame.Player")。
     * @returns {GAnyUserObject|any} 代表导入的 GAny 类的 GAny 对象 (可以使用 `new` 创建实例)。
     */
    import: function(path) { },

    /**
     * 解析 JSON 字符串并返回一个 GAny 对象 (通常是 GAnyUserObject 或 GAnyArray)。
     * @param {string} jsonString - 要解析的 JSON 字符串。
     * @returns {GAnyUserObject|any} 解析得到的 GAny 对象或原生 JS 值 (取决于实现)。
     */
    parseJson: function(jsonString) {},

    /**
     * 绑定对象的成员函数为一个调用。
     * @param {GAnyUserObject} obj
     * @param {string} methodName
     * @returns {GAnyUserObject|any} GAny Caller。
     */
    bind: function(obj, methodName) {},

    /**
     * 从一个 JS 对象定义来创建一个 GAny 类。
     * 该对象可以包含以下特殊键来定义类的行为和元数据：
     * - `__name`: (string) 类的名称。
     * - `__namespace`: (string, 可选) 类的命名空间。
     * - `__doc`: (string, 可选) 类的文档字符串。
     * - `__init`: (function(self: GAnyUserObject, ...args: any[]): void, 可选) 构造函数。
     * - `__neg`, `__add`, `__sub`, `__mul`, `__div`, `__mod`, `__xor`, `__or`, `__and`, `__not`: (function(self: GAnyUserObject, [other: any]): any, 可选) 一元或二元操作符。
     * - `__eq`, `__lt`: (function(self: GAnyUserObject, other: any): boolean, 可选) 比较操作符。
     * - `__getitem`: (function(self: GAnyUserObject, key: any): any, 可选) 获取子项 (例如 obj[key])。
     * - `__setitem`: (function(self: GAnyUserObject, key: any, value: any): void|boolean, 可选) 设置子项 (例如 obj[key] = value)。
     * - `__delitem`: (function(self: GAnyUserObject, key: any): void|boolean, 可选) 删除子项。
     * - `__len`: (function(self: GAnyUserObject): number, 可选) 获取长度 (例如 obj.length)。
     * - `__str`: (function(self: GAnyUserObject): string, 可选) 转换为字符串。
     * - `__to_int`, `__to_float`, `__to_double`: (function(self: GAnyUserObject): number, 可选) 转换为数字类型。
     * - `__to_bool`: (function(self: GAnyUserObject): boolean, 可选) 转换为布尔类型。
     * - `__to_object`: (function(self: GAnyUserObject): object|GAnyUserObject, 可选) 转换为 JS 对象。
     * - `__from_object`: (function(self: GAnyUserObject, obj: object): void, 可选) 从 JS 对象初始化。
     * 还可以定义普通方法和属性 (使用 get/set)。
     *
     * @param {object} classDef - 定义类结构的 JS 对象。
     * @param {string} classDef.__name - 类的名称。
     * @param {string} [classDef.__namespace] - 类的命名空间 (可选)。
     * @param {string} [classDef.__doc] - 类的文档字符串 (可选)。
     * @param {function(self: GAnyUserObject, ...args: any[]): void} [classDef.__init] - 构造函数。第一个参数 'self' 是新实例。
     * @param {function(self: GAnyUserObject): any} [classDef.__neg] - 一元负操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__add] - 加法操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__sub] - 减法操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__mul] - 乘法操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__div] - 除法操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__mod] - 取模操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__xor] - 位异或操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__or] - 位或操作。
     * @param {function(self: GAnyUserObject, other: any): any} [classDef.__and] - 位与操作。
     * @param {function(self: GAnyUserObject): any} [classDef.__not] - 位非操作。
     * @param {function(self: GAnyUserObject, other: any): boolean} [classDef.__eq] - 等于比较。
     * @param {function(self: GAnyUserObject, other: any): boolean} [classDef.__lt] - 小于比较。
     * @param {function(self: GAnyUserObject, key: any): any} [classDef.__getitem] - 获取子项。
     * @param {function(self: GAnyUserObject, key: any, value: any): void|boolean} [classDef.__setitem] - 设置子项。
     * @param {function(self: GAnyUserObject, key: any): void|boolean} [classDef.__delitem] - 删除子项。
     * @param {function(self: GAnyUserObject): number} [classDef.__len] - 获取长度。
     * @param {function(self: GAnyUserObject): string} [classDef.__str] - 转换为字符串。
     * @param {function(self: GAnyUserObject): number} [classDef.__to_int] - 转换为整数。
     * @param {function(self: GAnyUserObject): number} [classDef.__to_float] - 转换为浮点数 (float)。
     * @param {function(self: GAnyUserObject): number} [classDef.__to_double] - 转换为浮点数 (double)。
     * @param {function(self: GAnyUserObject): boolean} [classDef.__to_bool] - 转换为布尔值。
     * @param {function(self: GAnyUserObject): object|GAnyUserObject} [classDef.__to_object] - 转换为普通 JS 对象。
     * @param {function(self: GAnyUserObject, obj: object): void} [classDef.__from_object] - 从普通 JS 对象进行初始化。
     * @param {...function(self: GAnyUserObject, ...args: any[]): any} [classDef.methodName] - 类的成员函数。第一个参数 'self' 是实例。
     * @param {...{get: function(self: GAnyUserObject): any, set: function(self: GAnyUserObject, value: any): void}} [classDef.propertyName] - 类的属性定义。
     * @param {boolean} [doExport=false] - 是否将此类导出到 C++ 的全局注册表。
     * @returns {GAnyClass} 代表新定义的 GAny 类的 GAny 对象。
     */
    class: function(classDef, doExport = false) {},

    /**
     * 形式一: 对两个变量进行运算
     * lhs op rhs
     *  param {any} lhs - 变量A
     *  param {string} op - 操作符, 支持的操作符: +, -, *, /, %, |, &, ==, !=, <, >, <=, >=
     *  param {any} rhs - 变量B
     *
     * 形式二: 对变量进行单目运算符运算
     * op val
     *  param {string} op - 操作符, 支持的操作符: ~, -
     *  param {any} val - 变量
     */
    op: function(...args) {}
};

// ======================================================================
// Global AnyType Enum
// ======================================================================

/**
 * @readonly
 * @enum {number}
 * @description GAny 内部类型枚举。
 */
const AnyType = {
    /** @property {number} undefined (void) */
    undefined_t: 0,
    /** @property {number} null (nullptr) */
    null_t: 1,
    /** @property {number} boolean (bool) */
    boolean_t: 2,
    /** @property {number} int (int64_t) */
    int_t: 3,
    /** @property {number} float */
    float_t: 4,
    /** @property {number} double */
    double_t: 5,
    /** @property {number} string */
    string_t: 6,
    /** @property {number} array (ordered collection of values) */
    array_t: 7,
    /** @property {number} object (unordered set of name/value pairs) */
    object_t: 8,
    /** @property {number} function expressions */
    function_t: 9,
    /** @property {number} class expressions */
    class_t: 10,
    /** @property {number} class property */
    property_t: 11,
    /** @property {number} exception (GAnyException) */
    exception_t: 12,
    /** @property {number} user object */
    user_obj_t: 13
};
