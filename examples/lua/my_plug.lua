---
--- Created by Gxin.
--- DateTime: 2022/6/20 18:48
---
print("Plugin MyType");

Env = Env or GAny._object();   -- 使用Env局部环境变量存储共享变量, 作用域为当前文件

--- GAny内置的面向对象，Lua中注册的类型可以在其他语言中使用，比如C#、Java等，也可以在不同的Lua虚拟机实例中使用
--- * GAnyClass创建的类型，使用是线程安全的，包括函数调用，可以在不同的线程中使用，成员函数的作用域和GAnyFunction一致，
--- 只能访问self和自身参数，不能访问作用域以外的常量和变量。
local TypeBase = GAnyClass.Class({
    __namespace = "L",
    __name = "MyTypeBase",
    __doc = "My lua type base",
    func = function(self)
        if not Env.sa then
            Env.sa = 0;
        end
        Env.sa = Env.sa + 1;
        print("call func: ", Env.sa);
    end
});

local typeClassInfo = {
    __namespace = "L",
    __name = "MyType",
    __doc = "My lua type",
    __inherit = TypeBase,
    __str = function(self)
        return "This is MyType";
    end,
    [MetaFunctionS.Length] = function(self)
        return 3;
    end,
    MyEnum = { A = 1, B = 2, C = 3 },
    c2 = {
        get = function(self)
            return self.c[2];
        end,
        set = function(self, v)
            self.c[2] = v;
        end
    }
}

function typeClassInfo:__init(va, vb)
    self.a = va;
    self.b = vb;
    self.c = { 1, 2, 3, self:func2(3, 4) };
end

function typeClassInfo:func2(va, vb)
    return va + vb;
end
typeClassInfo.__i_func2 = {
    doc = "Function 2.",
    args = {"va:any", "vb:any"},
    returnType = "any"
}

local Type = GAnyClass.Class(typeClassInfo);
-- 在lua中重载和覆盖都不会生效

GAny._export(Type); -- 注册类型
