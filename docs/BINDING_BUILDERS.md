# Build a native UI in Python, JavaScript or TypeScript

The component builders let you author a UI tree in your language. CUI performs
layout and input handling in C; the optional SDL adapter draws a native window.
The bridge currently exposes rows, columns, labels, buttons, checkbox, radio,
switch, toggle, slider, progress, badge, input, textarea, number input,
collapsible, alert, radio group, tabs, accordion, select, combo box and split dropdown button. Composite
options and panels use `item` children. Python, JavaScript, TypeScript,
[Go](GO_BUILDERS.md) and [Rust](RUST_BUILDERS.md) wrap these real CUI components.
The [coverage table](BRIDGE_COMPONENTS.md) records each component's limits;
remaining catalog components are still being implemented.

## Dependencies and compiler

Build from the repository root with CMake 3.20+, Ninja, a C11 compiler and
Python 3.9+. GCC and Clang work on Linux. Python uses standard-library `ctypes`,
without pip dependencies. Node additionally needs Node 18+ and its development
headers (`node_api.h`); no npm packages or node-gyp are required. The TypeScript
example uses Node's built-in type stripping, tested here with Node 25.

Native windows additionally require installed SDL3 3.2+, FreeType, HarfBuzz,
pkg-config and an existing TrueType/OpenType font. CMake discovers and links
these libraries. Set the font path explicitly. A missing native adapter reports
an error when you request SDL; it does not silently select headless mode.

These bindings have Linux runtime evidence. Windows DLL exports and Node import
library discovery, and macOS Node linker settings, are provided; their native
runtime behavior is not verified. See the [platform matrix](PLATFORM_MATRIX.md).

## Python: build and launch

```sh
cmake -S bindings/bridge -B build-bridge -G Ninja
cmake --build build-bridge --target cui_bridge test_bridge_core test_bridge_components
ctest --test-dir build-bridge -R '^bridge_' --output-on-failure
export CUI_BRIDGE_LIBRARY="$PWD/build-bridge/libcui_bridge.so"
export PYTHONPATH="$PWD/bindings/python"
export CUI_TEST_FONT=/path/to/your/font.ttf
python bindings/bridge/examples/save_changes.py
```

The example opens a native window with a Save button and a status label. Clicking
Save updates the label. Its UI is defined in Python:

```python
import os
from cui_bridge import App, button, label, row

status = label("Ready", key="status")
save = (button("save").primary().rounded_md().size("base")
        .label("Save changes").on_click(lambda event: status.label("Saved")))

with App(backend="sdl", title="My CUI app", width=480, height=240,
         font_path=os.environ["CUI_TEST_FONT"]) as app:
    app.mount(row().gap_units(2).children(save, status))
    app.run()
```

For a window-free check, use `App()` without SDL. After mounting, call
`app.step()`, obtain `(x, y, w, h) = app.bounds(save)`, queue pointer down/up
inside that rectangle, call `app.step()` and then `app.dispatch()`. The checked-in
[Python tests](../bindings/python/test_bridge.py) demonstrate this using actual
CUI hit testing. Headless mode computes layout and draw commands; it does not
paint a window or provide native font metrics.

## JavaScript and TypeScript: build and launch

```sh
cmake -S bindings/node -B build-node -G Ninja
cmake --build build-node --target cui_node_bindings test_bridge_core test_bridge_components
ctest --test-dir build-node -R '^(bridge_|node_)' --output-on-failure
node bindings/node/examples/save.cjs --headless
export CUI_FONT_PATH=/path/to/your/font.ttf
node bindings/node/examples/save.cjs
node bindings/typescript/examples/save.ts
```

The shared bridge and Node addon are built together. CMake supplies include paths
and linker flags; the addon remains in the build directory. If you choose another
directory, set `CUI_NODE_ADDON` to its absolute `cui.node` path. On Windows the
Node project also requires `node.lib` and reports an actionable configure error
if it is unavailable.

Maintainers can repeat the Linux native checks without desktop windows by
configuring the Node build with `-DCUI_BRIDGE_NATIVE_TESTS=ON` and
`-DCUI_TEST_FONT=/path/to/font.ttf`, then running
`python scripts/test_local.py --build-dir build-node -- -R '^(bridge_|node_)'`.
These opt-in checks need Xvfb, xwininfo, xwd, xdotool and ImageMagick's convert.
The runner clears inherited desktop sockets and launches a disposable display;
native probes refuse to run without that isolation marker.

```js
const {App, row, label, button} = require('./bindings/node');
const status = label('Ready');
const save = button('save').primary().rounded_md().base()
  .label('Save changes').on_click(() => status.label('Saved'));
const app = new App({backend: 'sdl', width: 480, height: 240,
  title: 'My CUI app', fontPath: process.env.CUI_FONT_PATH});
app.mount(row().gap_units(2).children(save, status));
app.start();
```

`start()` schedules frames with Node timers so other timers and I/O can run.
Closing the native window disposes the app. Use `stop()` to stop scheduling,
`close()` to request closure, and `dispose()` to release resources explicitly.
The [TypeScript example](../bindings/typescript/examples/save.ts) uses the same
builders and [type declarations](../bindings/typescript/index.d.ts). Node type
stripping executes the example; it does not perform static type checking.

## Styling, state and callbacks

Buttons support primary, secondary, outline and ghost variants, disabled state,
theme rounding or an explicit radius, and custom hover colors for primary,
secondary and outline variants. Sizes cover `xs`, `sm`, `base`, `md`, `lg`, `xl`,
`2xl` through `9xl`; `base` is the default. Python uses `.size("sm")`; JS/TS
also offer `.sm()`, `.md()` and `.lg()`. `rounded_md()` maps to the existing
larger rounding preset. Arbitrary CSS properties and per-button padding overrides
are not exposed yet.

`gap_px(8)` means eight logical pixels. `gap_units(2)` also means eight logical
pixels, using four pixels per unit. `gap(8)` retains the explicit-pixel meaning.
Python's `step()` and `dispatch()` are separate; JS `step()` performs both.
Callbacks execute after the C frame has finished, so exceptions remain in your
language. A callback can update labels, disable a button or close the app.

The language layer retains its tree; the bridge owns copied strings, stable IDs
and button state and composes immediate CUI frames. Nodes belong to one app.
Disposed or stale handles are rejected. Keep each app on one thread, explicitly
release it, and drain its bounded event queue. Strings must be valid UTF-8 without
embedded NUL, at most 4096 bytes. Trees support up to 128 levels including their
root; deeper construction is rejected before frame submission.
See the [C ABI guide](../bindings/bridge/docs/README.md)
and [public header](../include/cui/cui_bridge.h) for capacities and status codes.

[Go builders](GO_BUILDERS.md) and [Rust builders](RUST_BUILDERS.md) now construct
real component trees in their own language and dispatch copied native events.
The older [C-host examples](LANGUAGE_BINDINGS.md) remain useful for embedding an
existing C-authored application.

## Form controls and copied change events

`on_change` receives a snapshot captured when the C component changed. Read
`event.checked`, `event.value`, or `event.text`; these remain accurate even when
multiple frames are queued before Python dispatch. JavaScript dispatches after
each `step()`. Programmatic setters update the model without emitting user events.
`radio` is standalone; use `radio_group` with `item` children for exclusive choice.

```python
from cui_bridge import App, column, checkbox, input, label, slider

status = label("Ready")
notifications = checkbox("notifications", "Enable notifications").checked()
name = input("name", "Alex").on_change(lambda event: status.label(event.text))
volume = slider("volume").range(0, 100, 1).value(50)
with App() as app:
    app.mount(column().gap_units(2).children(name, notifications, volume, status))
    app.step()  # Headless layout; choose backend="sdl" and font_path for a native window.
```

```javascript
const cui = require('./bindings/node/index.cjs');
const status = cui.label('Ready');
const name = cui.input('name', 'Alex').on_change(event => status.label(event.text));
const app = new cui.App();
app.mount(cui.column().gap_units(2).children(
  name, cui.checkbox('notifications', 'Enable notifications').checked(),
  cui.slider('volume').range(0, 100, 1).value(50), status
)).step();
app.dispose();
```

The Python and Node `test_components` fixtures exercise real pointer hit testing,
disabled behavior, numeric ranges, UTF-8 edits, grouped selection and panel visibility.
Design sizes, rounding, and custom hover styling currently apply to Button;
other component styling APIs are a separate unfinished bridge capability.
