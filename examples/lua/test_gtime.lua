---
--- Created by Gxin.
--- DateTime: 2022/5/23 19:59
---

--- GTime ---
print("\n --- GTime test ---\n");

local GTime = GAny._import("Gx.GTime");
local GThread = GAny._import("Gx.GThread");

local time1 = GTime(GTime.SystemClock);
time1:update();
print("time1:", time1);

local time2 = GTime.currentSystemTime();
print("time2:", time2);

local time3 = GTime.currentSteadyTime();
print("time3:", time3);

GThread.mSleep(1000);

local time4 = GTime.currentSteadyTime();
print(string.format("time diff: %f sec", time4:secsDTo(time3)));
print(string.format("time diff: %d sec", time4:secsTo(time3)));
print(string.format("time diff: %d mill sec", time4:milliSecsTo(time3)));
print(string.format("time diff: %d micro sec", time4:microSecsTo(time3)));
print(string.format("time diff: %d nano sec", time4:nanoSecsTo(time3)));
print(string.format("time4 - time3 = %s", (time4 - time3):_toString()));

local time5 = GTime.currentSystemTime();
print("time5:", time5:_toString("hh:mm:ss.zzz AP"));
time5:addMilliSecs(1000);
print("time5 new:", time5:_toString("HH:mm:ss.zzz AP"));

--- GTimer ---
print("\n --- GTimer test ---\n");

local GTimer = GAny._import("Gx.GTimer");
local GTimerScheduler = GAny._import("Gx.GTimerScheduler");

local scheduler = GTimerScheduler.create("Main");
GTimerScheduler.makeGlobal(scheduler);

scheduler:post(function()
    print("Post timeout");
end, 1000);

local timer = GTimer:new();
local c = 0;
timer:timerEvent(function()
    c = c + 1;
    print("Timer timeout:", c);
    if c >= 5 then
        scheduler:stop();
    end
end);
timer:start(500);

scheduler:run();
