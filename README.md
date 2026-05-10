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
