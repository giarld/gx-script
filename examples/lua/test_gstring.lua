---
--- Created by Gxin.
--- DateTime: 2022/5/21 13:10
---

--- GString ---
print("\n --- GString test ---\n");

local GString = GAny._import("Gx.GString");

gs = GString([[Hello 你好]]);
print("count = ".. gs:count());
print("length = ".. gs:_length());
print("__len = ".. #gs);
Log("str = ", gs);


local gs2 = GString("123");
local gs3 = GString("1234");

print("gs2:", gs2);
print("gs3:", gs3);
print("gs2 == gs3: ", (gs2 == gs3));
print("gs2 < gs3: ", (gs2 < gs3));
print("gs2 >= gs3: ", (gs2 >= gs3));
print("gs2 ~= gs3: ", (gs2 ~= gs3));
print("gs2 > gs3: ", (gs2 > gs3));
print("gs2 <= gs3: ", (gs2 <= gs3));

print("gs2 + gs3: ", (gs2 + gs3):_toString());

local gs4 = GString("abc");
gs4:append("def");
gs4:insert(1, "123");
print("gs4: ", gs4);
print("gs4 is empty: ", gs4:isEmpty());
print("gs4 startWith a123: ", gs4:startWith("a123"));
print("gs4 endWith cdef: ", gs4:endWith("cdef"));

gs4 = GString("abc,def,ghi");
print("gs4 split by ',' size: ", #gs4:split(","), ", [1]: ", gs4:split(","):_toTable()[1]);


local gs5 = GString("a:{}, b:{}, c:{}, d:{}"):arg(1):arg(false):arg(3.14):arg(gs4:split(",")[1]);
print("gs5: ", gs5);

local gs6 = GString("a:{1}, b:{2}, c:{3}, d:{4}")
              :arg("1", 1)
              :arg("3", false)
              :arg("2", 3.14)
              :arg("4", "abc");
print("gs6: ", gs6);

local gs7 = GString.fromCodepoint(0x4e8e);
print(string.format("gs8: 0x%04x, %s", gs7:codepoint(0), gs7));

print("int toString: ", GString.toString(123456789));
print("float toString: ", GString.toString(3.1415926));
print("bool toString: ", GString.toString(true));

print("table toString: ", GString.toString({
    1, 2, s = 3, gs, true, false, "123", {1, 2, 3},
    f = function() print("f") end
}));

collectgarbage();
