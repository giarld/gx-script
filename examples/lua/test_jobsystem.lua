---
--- Created by Gxin.
--- DateTime: 2023/6/17 18:04
---

--- JobSystem ---
print("\n --- JobSystem test ---\n");

local JobSystem = GAny._import("Gx.GJobSystem");

local js = JobSystem:new("JobSystem", 2, 1);
js:adopt();

local parent = js:createJob();

js:run(js:createJob(parent, function(tjs, job)
    Log("Job1 run");
end));
js:run(js:createJob(parent, function(tjs, job)
    Log("Job2 run");
end));
js:run(js:createJob(parent, function(tjs, job)
    Log("Job3 run");
end));
js:run(js:createJob(parent, function(tjs, job)
    Log("Job4 run");
end));

js:runAndWait(parent);

js:emancipate();
js = nil;

collectgarbage();