---
--- Created by Gxin.
--- DateTime: 2022/5/21 23:05
---

--- TaskSystem ---
print("\n --- TaskSystem test ---\n");

local TaskSystem = GAny._import("Gx.GTaskSystem");
local GThread = GAny._import("Gx.GThread");
local GTimer = GAny._import("Gx.GTimer");

local ts = TaskSystem:new("TaskSystem", 3);
ts:start();

Log("ts thread count: ", ts:threadCount());

Log("main thread id: ", GThread.currentThreadId());

local task = ts:submit(function(v1, v2, v3, v4, v5)
    Log("task1 start, thread id: ", GThread.currentThreadId());

    return {v1, v2, v3, v4, v5};
end, 1, 2, {1, 2, 3}, {a = 1, b = 3.14}, "String");

ts:submitFront(function()
    Log("task3 start, thread id: ", GThread.currentThreadId());
    for i = 1, 10 do
        Log("task3, i: ", i);
        GThread.mSleep(100);
    end
end);

local task4 = ts:submit(function()
    Log("task4 thread id: ", GThread.currentThreadId());
    for i = 1, 10 do
        Log("task4, i: ", i);
        GThread.mSleep(100);
    end
    return 123;
end);

local ret = task:get();
Log("task1 ret: ", ret);

Log("Task4 result: ", task4:get());
Log("Waiting for task to end.")

ts:stopAndWait();

collectgarbage();
