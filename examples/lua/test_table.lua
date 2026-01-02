---
--- Created by Gxin.
--- DateTime: 2022/11/7 17:42
---

print("\n --- LuaTable test ---\n");

--[[
GAny模拟了一个LuaTable，使得table的生命周期从lua vm中解耦，并方便其他语言与lua进行数据交换。
LuaTable可以转换为符合标准的Json, 也可以从Json构造出LuaTable。
并且扩展了lua table的可用性，包括输出为字符串，转换为数组，序列化到GByteArray等。
]]

local GByteArray = GAny._import("Gx.GByteArray");

local func = function()
    return 123;
end;
local gb = GByteArray:new();
gb:writeString("Hello");
local t = GAny._create({
    [0] = 0, 1, 2, false, true, { 4, 5, 6 }, 3.1415926, "Hello", func,
    {
        vt = { 1, 2, 3, 4 },
        vt2 = { a = 1, b = 3.14, c = "HHH", d = true },
        gb = gb,
        [{ 1, 2, 3 }] = 988,
        [true] = false,
        [3.14] = "PI"
    }
});

print("t len:", #t);
print("t =", t, "\n");

local tl = t:_toTable();
print("tl type:", type(tl));

--- @type GAny
local object = t:_toObject();
print("object =", object);

--- @type GAny
local f = object[8];
print("f type:", f:_typeName());
if f:_isFunction() then
    print("call f:", f());
end

if object[5]:_isArray() then
    print("object[5]:", object[5]);
end

--- @type GAny
local gb2 = t[9].gb;
if gb2 and gb2:_is("GByteArray") then
    local s = gb2:readString();
    print("gb2 s:", s);
end

print("\n --- test table r/w --- \n");

local t2 = GAny._create({ 1, 2, 3, a = 4, b = "hello", c = 3.14 });
print("t2 =", t2);
t2.a = "hello table";
t2[4] = "4";
print("1. modified t2 =", t2);
t2:_delItem(4); -- GAny _delItem 操作可以删除元素
t2.b = nil;            -- 设置为nil也可以删除元素
print("2. modified t2 =", t2);

print("t2[\"a\"] :", t2["a"]);
print("t2.a:", t2.a);

print("\n --- test table from json --- \n");

local jObj = GAny._parseJson('{"a":1,"b":3.14,"c":"Hello","e":[1,2,3]}');
print("json:", jObj);
local jTable = jObj:_toTable();
print("jTable type:", type(jTable));
local jTableR = GAny._create(jTable);
print("jTable=", jTableR);
print("jTable to json:", jTableR:_toObject():_toJsonString(2));

print("forEach jObj:");
jObj:forEach(function(k, v)
    print(string.format("> %s: %s", k, v));
end);
