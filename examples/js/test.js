(function(Env) {
    async function main() {
        let say = await import("./test_module.js");
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
                self._setItem("a", a);
                self._setItem("b", b);
            },
            pt: function (self) {
                console.log("MM: a =", self._getItem("a"), ", b =", self._getItem("b"), ", c =", self.c);
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

        const MyJsType = GAny.import("Js.MyJsType");

        let o = MyJsType.new(1, 2)
        o.c = 123;
        o.pt();

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
        let ts = tTaskSystem.new("TaskSystem", 1);
        ts.start();

        let taskParams = {
            v1: 123,
            v2: 456
        };
        let cc = GAny.createWorkerCallable("./test_task.js", taskParams);

        let task = ts.submit(cc);
        let taskRet = task.get();
        console.log("Task Ret:", taskRet);

        ts.stopAndWait();
    }

    main().then();

    return "Happy End";
})