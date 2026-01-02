---
--- Created by Gxin.
--- DateTime: 2022/6/12 16:05
---

print("\nlua state2 call lua state1 class\n");

local MyType = GAny._import("L.MyType");
local TaskSystem = GAny._import("Gx.GTaskSystem");

-- myObj的所有成员都托管于C++内存中，所有操作都是VM无关的，不会出现LuaVM A定义的类在LuaVM B中创建对象，执行操作对LuaVM A产生影响的情况
local myObj = MyType:new("hello", 3.14);
print("myObj type: " .. myObj:_classTypeName());
myObj:func();

print("myObj.a = ", myObj.a);
print("myObj.b = ", myObj.b);
print("myObj.c = ", myObj.c);

-- lua表会被GAny系统转换为LuaTable类型的对象.
myObj.c = { a = 1, b = 2, c = 3 };
print("myObj.c = ", myObj.c);
myObj.c.a = 2;
myObj.c:_setItem(1, 2);
myObj.c:_delItem(2);

local ts = TaskSystem:new("TaskSystem", 1);
ts:start();

local task = ts:submit(function(obj)
    print("task callback");

    local temp = GAny._object({
        a = 1
    });
    obj.c.d = function()
        print(string.format("task callback d: %s", temp.a));
        temp.a = temp.a + 1;
    end;
    obj.c.d();
end, myObj);
task:wait();
ts:stop();    -- 线程到此结束, 线程的虚拟机也会被销毁

print("myObj.c = ", myObj.c);
myObj.c.d();        -- 其实是执行d函数的字节码，所以不依赖生成它的Lua VM.
myObj.c.d();        -- 测试 GAny 对象空间跨线程共享
