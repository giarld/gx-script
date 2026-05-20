# gx-script

## Import GAny types in JavaScript

GAny classes exported by native modules can be imported from JavaScript with the
virtual module prefix `gany:`.

```js
import Gix, { GuiWidget, GuiText } from "gany:Gix";
import Math, { Float3, Float4 } from "gany:Math";
import { GTimer } from "gany:Gx";
```

`gany:<namespace>` first tries `GAny::Import("<namespace>")`. If no exact class
is found, it falls back to `GAny::Import("<namespace>.*")` and exposes the
namespace members as named ES module exports. The module default export is the
whole imported GAny object.

```js
import * as Math from "gany:Math";

const color = Math.Float3.new(1, 0, 0);
```

An exact class path can also be imported directly.

```js
import { Float3 } from "gany:Math.Float3";

const position = Float3.new(0, 1, 0);
```

To use a GAny class as a JavaScript constructor, pass
`{ constructor: true }` when importing an exact class path.

```js
const Float3Ctor = GAny.import("Math.Float3", { constructor: true });
const position = new Float3Ctor(0, 1, 0);
```

Dynamic `import()` is also supported. Use it when the GAny module is only needed
inside an async flow or callback.

```js
const { Float3 } = await import("gany:Math");

const color = Float3.new(1, 0, 0);
```

Without top-level `await`, use the returned promise.

```js
import("gany:Math").then(({ Float3 }) => {
    const color = Float3.new(1, 0, 0);
});
```

This is equivalent to the older runtime import style:

```js
const Math = GAny.import("Math.*");
const Float3 = Math.Float3;
```

Files with the `.mjs` suffix, or JavaScript files that contain top-level
`import` / `export`, are evaluated as ES modules. The script environment passed
from C++ is exposed as `Env` during module evaluation. Capture any needed values
at module top level before using them in callbacks.

## QuickJS Debugger

The QuickJS runtime exposes a lightweight debugger through the global
`GxDebugger` object. It supports source-line breakpoints, exception
breakpoints, `debugger;` statements, manual pause/resume, single stepping,
stack printing, watch expressions, and an optional interactive command-line
mode.

```js
GxDebugger.setBreakpoint(".", 27);
GxDebugger.setPauseOnException(true);
GxDebugger.watch("self.a + b");
GxDebugger.setInteractiveOnBreak(true);
GxDebugger.setTrapOnBreak(false);
```

`debugBreak()` is also available when native debugger integration is needed. It
raises `SIGTRAP` on platforms that support it.

### Breakpoints

Breakpoints are set by source file and 1-based line number.

```js
GxDebugger.setBreakpoint(".", 27);
GxDebugger.setBreakpoint("examples/js/test.js", 44);
GxDebugger.setBreakpoint(".", 52, { condition: "a > 10" });
GxDebugger.clearBreakpoint(".", 27);
GxDebugger.clearAllBreakpoints();
```

The file argument may be an absolute path, a path suffix such as
`"examples/js/test.js"`, or `"."` for the current executing source file.

`GxDebugger.listBreakpoints()` returns data suitable for a breakpoint panel:

```js
[
    { id: 1, file: "/path/to/test.js", line: 27, condition: "" },
    { id: 2, file: "examples/js/test.js", line: 44, condition: "a > 10" }
]
```

Conditional breakpoints evaluate `condition` in the current frame locals. A
truthy result pauses execution. If condition evaluation throws, the debugger
prints the exception and pauses instead of silently skipping the breakpoint.

### Pause And Step

`GxDebugger.pause()` requests a pause at the next stoppable JavaScript bytecode
position. Once paused, execution can be resumed or stepped:

```js
GxDebugger.pause();
GxDebugger.isPaused();
GxDebugger.getPauseState();
GxDebugger.resume();
GxDebugger.stepInto();
GxDebugger.stepOver();
GxDebugger.stepOut();
```

`stepInto()` enters JavaScript calls, `stepOver()` skips over calls in the
current frame, and `stepOut()` runs until the current JavaScript function
returns to its caller.

JavaScript `debugger;` statements also pause through the same GxDebugger flow:

```js
debugger;
```

`GxDebugger.getPauseState()` always returns a stable object shape:

```js
{
    paused: false,
    reason: null,
    location: null,
    pendingPause: false,
    step: {
        pending: false,
        kind: null,
        origin: null
    },
    breakpointsCount: 0,
    watchesCount: 0
}
```

`paused` reflects whether execution is currently blocked inside the debugger.
When that is true, `reason` is one of `"pause"`, `"step"`, `"breakpoint"`,
`"exception"`, or `"debuggerStatement"`. `location` and `step.origin`
contain `file`, `line`, `col`, `frameId`, `frameDepth`, and `pcOffset`.
`step.pending` is only true while a step request is in flight; after a
step stop, `step.kind` and `step.origin` still describe the step that led
to the current pause.

Host code can read the same snapshot shape through `GAnyJS::getPauseState()`:

```cpp
const auto js = GAnyJS::threadLocal();
const GAny pauseState = js->getPauseState();
```

In non-interactive break flows, the last pause snapshot remains visible
through `reason`, `location`, and `step` until a later `resume()` /
`stepInto()` / `stepOver()` / `stepOut()` / `pause()` request overwrites
or clears it, but `paused` becomes `false` once execution has continued.
That retained snapshot is not treated as a live paused frame for step
origin capture; later step requests start fresh unless they are issued
from an active interactive pause.

### Exception Breakpoints

Enable pause-on-throw when you want the debugger to stop at the throw site
before a surrounding `try/catch` handles it:

```js
GxDebugger.setPauseOnException(true);
```

When enabled, runtime `throw`, rejected `await`, and runtime errors such as
`TypeError` pause at the current JavaScript source location. Compile-time
syntax errors do not go through this breakpoint.

### Break Behavior

By default, a breakpoint prints the current location and stack, then raises
`SIGTRAP` so a native debugger can stop there.

```js
GxDebugger.setPrintStackOnBreak(true);
GxDebugger.setTrapOnBreak(true);
```

Use interactive mode when you want breakpoints to block and wait for commands
instead of always trapping into the native debugger:

```js
GxDebugger.setInteractiveOnBreak(true);
GxDebugger.setTrapOnBreak(false);
```

When a breakpoint is hit, type `h` or `help` at the `(gxdbg)` prompt for the
full command help.

Common commands:

```text
c, continue, resume       Continue execution.
s, step, stepInto         Step into the next JavaScript source location.
n, next, stepOver         Step over JavaScript calls.
finish, out, stepOut      Run until the current function returns.
bt, backtrace             Print the JavaScript stack.
locals, vars, scope       Print current frame arguments and local variables.
args                      Alias for locals; args and locals share one view.
w, watch, watches         Print all watch expressions.
p <expr>, print <expr>    Evaluate one expression in current frame locals.
lb, breakpoints           List breakpoints.
b <file>:<line>           Add a breakpoint. Use . for the current file.
b <file>:<line> if <expr> Add a conditional breakpoint.
d <id|file:line>          Delete a breakpoint by id or location.
q, quit                   Stop execution with a debugger error.
```

Examples:

```text
b .:42
b .:42 if a > 10
b examples/js/test.js:27
d 2
locals
p self.a + b
```

### Watch Expressions

Watch expressions are evaluated when execution pauses. They can reference the
current frame's arguments and local variables.

```js
GxDebugger.watch("self.a");
GxDebugger.watch("a + b + c");
GxDebugger.clearWatch("self.a");
GxDebugger.clearAllWatches();
```

Watch evaluation is disabled while the expression itself is running, so watches
do not recursively trigger debugger stops.
