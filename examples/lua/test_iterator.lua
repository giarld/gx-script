---
--- Created by Gxin.
--- DateTime: 2023/8/23 14:52
---

print("\n --- Iterator test ---\n");

--- 数组迭代器
do
    Log("1. Array");
    local array = GAny._array({1, 2.333, "Four", {1,2,3}});

    -- 1. foreach
    Log("foreach:");
    array:forEach(function(i)
        Log("\t", i);
    end);

    Log("java style iterator:");
    local it = array:_iterator();
    while (it:_hasNext()) do
        local i = it:_next();
        Log("\t[", i.key, "]: ", i.value);
    end

    Log("Reverse iterator:");
    it:toBack();
    while (it:hasPrevious()) do
        local i = it:previous();
        Log("\t[", i.key, "]: ", i.value);
    end

    Log("lua style iterator:");
    for i, v in pairs(array) do
        Log("\t[", i, "]: ", v);
    end
end

print()
--- 对象迭代器
do
    Log("2. Object");
    local array = GAny._object({
        a = 1, b = 3.14, c = "sss", d = {1,2,3,4}
    });

    -- 1. foreach
    Log("foreach:");
    array:forEach(function(k, v)
        Log("\t", k, ", ", v);
    end);

    Log("java style iterator:");
    local it = array:iterator();
    while (it:hasNext()) do
        local i = it:next();
        Log("\t", i.key, ", ", i.value);
    end

    Log("lua style iterator:");
    for k, v in pairs(array) do
        Log("\t", k, ", ", v);
    end
end

print();
--- 自定义迭代器
do
    Log("4. Custom class iterator");
    --- 自定义类型的迭代器实现: 迭代器实现 hasNext()->bool, next()->GAnyIteratorItem 方法即可.
    local TestIterator = GAnyClass.Class({
        __name = "TestIterator",
        [MetaFunctionS.Init] = function(self, v)
            self.iterator = v.obj:iterator();
        end,
        hasNext = function(self)
            return self.iterator:hasNext();
        end,
        next = function(self)
            return self.iterator:next();
        end
    });

    --- 提供迭代器的类型, 需要提供 iterator 方法, 返回自定义或原生迭代器
    local Test = GAnyClass.Class({
        __name = "Test",
        [MetaFunctionS.Init] = function(self)
            self.obj = {
                a = 1, b = 2.22, c = "hhhh", d = { 1, 2, 3 }
            }
        end,
        iterator = function(self)
            return TestIterator:new(self);
        end
    });

    local o = Test:new();

    Log("foreach o:");
    for k, v in pairs(o) do
        Log("\t", k, ", ", v);
    end
end