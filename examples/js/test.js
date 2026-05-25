(function(Env) {
    async function main() {
        const say = await import("./test_module.js");
        let os = await import("qjs:os");

        say.sayHi("JS");
        os.setTimeout(()=> {
            say.sayBye("JS");
        }, 1000);

        console.log(Env);

        GAny.class({
            __name: "MyJsType",
            __namespace: "Js",
            __doc: "A Js GAny Class",
            __init: function (self, a, b) {
                self.a = a;
                self._setItem("b", b);
            },
            pt: function (self) {
                console.log("MM: a =", self._getItem("a"), ", b =", self.b, ", c =", self.c);
            },
            c: {
                set: function (self, v) {
                    self._setItem("m_c", v);
                },
                get: function (self) {
                    return self._getItem("m_c");
                }
            }
        }, true);

        // const MyJsType = GAny.import("Js.MyJsType");
        const {MyJsType} = await import("gany:Js");

        let o = MyJsType.new(1, 2)
        o.c = 123;
        o.pt();

        const MyJsCtor = GAny.import("Js.MyJsType", { constructor: true });

        let o2 = new MyJsCtor(3, 4);
        o2.c = 456;
        o2.pt();
        console.log("js class:", o2 instanceof MyJsCtor, o2._isUserObject(), GAny.create(o2)._isUserObject());

        let dyn = GAny.create({});
        dyn.name = "gany";
        dyn["version"] = 1;
        console.log("dynamic object:", dyn.name, dyn._getItem("name"), dyn.version, dyn._toJsonString());

        //=================

        console.log("123 + 456 =", GAny.op(123, "+", 456));
        console.log("123 - 456 =", GAny.op(123, "-", 456));
        console.log("123 * 456 =", GAny.op(123, "*", 456));
        console.log("123 / 456 =", GAny.op(123, "/", 456));
        console.log("123 > 456 =", GAny.op(123, ">", 456));
        console.log("123 >= 456 =", GAny.op(123, ">=", 456));
        console.log("123 < 456 =", GAny.op(123, "<", 456));
        console.log("123 <= 456 =", GAny.op(123, "<=", 456));
        console.log("0xf0 | 0x0f =", GAny.op(0xf0, "|", 0x0f));
        console.log("-123 =", GAny.op("-", 123));

        //=================

        /**
         * @type {GTaskSystem}
         */
        const tTaskSystem = GAny.import("Gx.GTaskSystem");
        let ts = tTaskSystem.new("TaskSystem", 2);
        ts.start();

        // --- Example 1: original string-based worker (unchanged) ---
        let taskParams = {
            v1: 123,
            v2: 456
        };
        let cc = GAny.createWorkerCallable("./test_task.js", taskParams);
        let task = ts.submit(cc);
        let taskRet = task.get();
        console.log("Task Ret (source):", taskRet);

        // --- Example 2: pre-compiled bytecode worker ---
        let bc = GAny.compileWorkerScript("./test_task.js");
        console.log("Bytecode compiled, size:", bc.size());

        let cc2 = GAny.createWorkerCallable(bc, { v1: 100, v2: 200 });
        let task2 = ts.submit(cc2);
        let taskRet2 = task2.get();
        console.log("Task Ret (bytecode):", taskRet2);

        // --- Example 3: inline script via bytecode ---
        let bc3 = GAny.compileWorkerScript(
            "(function(Env) { console.log('Inline worker running'); return Env.x * Env.y; })",
            "<inline>"
        );
        let cc3 = GAny.createWorkerCallable(bc3, { x: 7, y: 8 });
        let task3 = ts.submit(cc3);
        let taskRet3 = task3.get();
        console.log("Task Ret (inline bytecode):", taskRet3);

        ts.stopAndWait();
    }

    main().then();

    return "Happy End";
})
