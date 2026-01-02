---
--- Created by Gxin.
--- DateTime: 2022/6/18 14:34
---


--- GThread ---
print("\n --- GThread test ---\n");

local GThread = GAny._import("Gx.GThread");
local GMutex = GAny._import("Gx.GMutex");

Log("main thread id: ", GThread.currentThreadId());

local lock = GMutex:new();

local th = GThread:new(function()
    Log("th start, thread id: ", GThread.currentThreadId());
    for i = 1, 20 do
        lock:lock(function()
            Log("th, i: ", i);
            GThread.mSleep(100);
        end);
    end
    Log("th end.");
end);

th:start();

-- 能够从日志观察到中间段main和th的输出是互相交替的，说明锁生效了.
for i = 1, 20 do
    lock:lock();
    Log("main, i: ", i);
    lock:unlock();
end

th:join();
