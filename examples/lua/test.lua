---
--- Created by Gxin.
--- DateTime: 2022/5/15 10:34
---

print("\n----- GAny -----");

-- Env 表示当前脚本内的局部环境变量，可以在 GAnyLua 执行 eval, evalFile, evalBuffer 时传入，也可以在Lua中创建：如下:
Env = Env or GAny._object();
-- 不同执行批次的 G 是不一样的(沙盒环境)，G可以用于沙盒内的全局变量存储也可以在一个沙盒内的函数间共享.
print("Env.a = ", Env.a);
print("_ENV.a = ", _ENV.a);

print("Call Env.globalFunc: ", globalFunc("HELLO"));

-- lua table是lua特有类型，其他语言通过LuaTable类型可以操作table，同时也不妨碍lua使用自己的语言特性存储数据和计算
local any = GAny._create({ 1, 3, 5, a = "hello", b = { 1, 2, 3 }, [0.12] = "ddd" });
collectgarbage();

print("any type: " .. any:_type());
print("any type name: " .. any:_typeName());
print("any class type name: " .. any:_classTypeName());
print("any = " .. any:_toString());
print("any length: " .. #any);

print("any.c = ", any.b);

if any:_isTable() then
    local org = any:_toTable(); --- trans to lua type (table)
    print("org type: " .. type(org));
else
    LogE("any not a table");
end

local item = any:_getItem("a");
print("item: " .. item:_toString());
local item2 = any:_getItem(GAny._create("b"));
print("item: " .. item2:_toString());

any:_setItem("c", 3.1415926);
any:_setItem(4, true);
any:_setItem("4", false);
any:_setItem(0.12, "ccc");
any:_delItem(1);
any.a = "world";    -- 可以这么做
any[2] = 11111;     -- 也可以这么做

print("any = " .. any:_toString());
print("any length: " .. #any);

collectgarbage();

local any2 = GAny._create(any);
print("any2 type: " .. any2:_classTypeName());
print("retAny2 = " .. any2:_toString());

local any5 = GAny._object({ d = 2 });
any5:_setItem("a", 1);
any5["b"] = "dsd";
any5.c = 3.14;
print("any5.length: " .. any5:_length());
print("any5[a]: ", any5["a"]);
print("any5.b: ", any5.b);

print("iterator: ");
--- java 风格的迭代器 (非线程安全)
local any5It = any5:iterator();
while (any5It:hasNext()) do
    local v = any5It:next();
    print("> " .. v.key .. ", " .. v.value);
end

print();
print("foreach: ");
--- foreach 迭代器 (线程安全)
any5:forEach(function(k, v)
    print("> " .. k .. ", " .. v);
end);
print();
print("pairs foreach: ");
--- lua 风格迭代器
for k, v in pairs(any5) do
    print("> " .. k .. ", " .. v);
end
print();

local anyArray = GAny._array({ 1, "2", 3, 4.5 });
print("anyArray: ", anyArray);

--- GAny包装的函数会转换为GAnyFunction对象, GAnyFunction存储Lua函数的字节码和引用，
--- 当创建函数和调用函数在同一线程(真线程)时, 直接执行函数的引用, 当创建函数的线程与调用函数的
--- 线程不是一个线程时, 从字节码加载和执行函数，多线程执行lua函数时, 上值为拷贝的上值,
--- 当上值为lua基本类型时, 无法在线程间完成该上值的同步(不同线程间同一个对象的不同拷贝),
--- 如果需要上值能够在线程间同步, 可以选择 GAnyObject 或自定义对象.
--- *注: GAnyFunction 包装后的 Lua 函数无法返回多个返回值

--- 1. 单线程时, 与普通lua函数没有区别
local v1 = 1;
local v2 = 2;
local anyFunc1 = GAny._create(function()
    return v1 + v2;
end)
print("anyFunc1 ret: ", anyFunc1());

anyFunc1 = nil;
--collectgarbage();

--- 2. 当存在多线程调用时, 可以使用 G 共享全局变量
--- 但是只有一个返回值能够被接受到
local anyFunc2 = GAny._create(function(va, vb)
    print(string.format("anyFunc2: a = %d, b = %d", va, vb));
    return va + vb + Env.ta, va - vb;
end);

Env.ta = 1;
local ret = anyFunc2(10, 20);    -- ret GAny<int64>, 多返回值是lua的语言特性，GAny不接受这一特性，只取第一个返回值
print("anyFunc2 ret: ", ret);    -- 这里的返回值转换为int32是安全的

local anyFunc3 = GAny._create(function()
    print("anyFunc2 upVal: ", _upVal);
    return v1 + v2;
end);
print("anyFunc3 ret: ", anyFunc3());

--- parse json
---@type GAny
local jsonObj = GAny._parseJson('{"a":1,"b":1.2,"c":"str"}');
print("jsonObj: ", jsonObj:_toJsonString());

---@type GAny
local GFile = GAny._import("Gx.GFile");
local file = GFile:new("test.txt");
print("file path: " .. file:absoluteFilePath());  -- 可以像这样直接调用成员函数
if file:open(GFile.ReadWrite) then
    file:write("123_ABC_abc_一二三");
    file:close();
end

local file2 = GFile:new("test.txt");
if file2:open(GFile.ReadOnly) then
    -- 可以使用_call调用成员函数
    local s = file2:_call("readAll");
    print("file2 content: ", s);
end

print("\n----- Lua plugin -----");

requireLs("my_plug", Env);

local MyType = GAny._import("L.MyType");
print(MyType:_dump());

print("MyType.func signature: ", MyType.func:signature());

local myObj = MyType:new(1, 2.0);
print("myObj type: " .. myObj:_classTypeName());
myObj:func();

print("myObj.MyEnum = ", myObj.MyEnum);

local func2Caller = GAny._bind(myObj, "func2");
print("call func2Caller: ", func2Caller(1, 2));

print("myObj.len = ", #myObj);
print("myObj.a = ", myObj.a);
print("myObj.b = ", myObj.b);
print("myObj.c = ", myObj.c);

myObj.a = 3;
print(string.format("myObj.a: %s", myObj.a));

myObj.b = "change to string";
print(string.format("myObj.b: %q", myObj.b));

myObj.c2 = 5.8767;
print(string.format("myObj.c2: %s", myObj.c2));
print(string.format("myObj.c: %s", myObj.c));   -- c2 实际存储在c[2].

print("----- test end -----");

return "test end";