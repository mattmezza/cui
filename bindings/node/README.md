# Native CUI for Node.js and TypeScript

This binding creates a real CUI scene. The N-API addon calls the shared C bridge; CUI computes layout and queues input events. The SDL backend renders an actual native window. JavaScript owns callbacks and updates nodes after `step()` returns from C. The fluent API currently exposes row, column, label, and button. Other manifest components are not exposed by this binding.

## Build and run

Prerequisites: CMake, Ninja, C compiler with C11, Node.js 18+ and its `node_api.h` development header. Native windows require installed SDL3 3.2 or newer, FreeType, and HarfBuzz development packages; CMake does not fetch them by default. From the repository root:

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=ON
cmake --build build-node --target cui_node_bindings test_bridge_core
ctest --test-dir build-node -R "^(bridge_core|node_binding|node_addon_safety)$" --output-on-failure
node bindings/node/examples/save.cjs --headless
```

The build places `cui.node` in `build-node/`; set `CUI_NODE_ADDON=/absolute/path/to/cui.node` if using another build directory. The private CMake project builds the C bridge and links the addon; there are no npm packages or node-gyp steps. For an actual window, ensure CMake reports that SDL3 and text adapters are enabled, and provide a font file:

```sh
CUI_FONT_PATH=/path/to/font.ttf node bindings/node/examples/save.cjs
```

The SDL backend creates a native window. Headless mode runs CUI scene construction, layout, and input without opening a window. The example exits on Ctrl+C. For a finite native smoke check under a fresh Xvfb display:

```sh
CUI_NODE_ADDON="$PWD/build-node/cui.node" CUI_FONT_PATH=/path/to/font.ttf \
  xvfb-run -a node bindings/node/test/native.test.cjs
```

This checks window creation, a frame, button click, close, and disposal. Do not point it at an active desktop display.

Node 25 can run the TypeScript example directly with type stripping:

```sh
node --experimental-strip-types bindings/typescript/examples/save.ts
```

For older Node versions, compile the TypeScript file with a TypeScript compiler already available in your project. The declarations are in `bindings/typescript/index.d.ts` and exported by the Node package's `types` field.

## API

```js
const {App, row, column, label, button} = require('./bindings/node');
const status = label('Ready');
const save = button('save').primary().rounded_md().label('Save changes')
  .on_click(() => status.label('Saved'));
const app = new App({backend: 'headless', width: 480, height: 240});
app.mount(column().gap_units(2).children(row().children(save, status)));
app.step();
const b = save.bounds();
app.pointer('move', b.x + 2, b.y + 2)
  .pointer('down', b.x + 2, b.y + 2)
  .pointer('up', b.x + 2, b.y + 2);
app.step();
app.dispose();
```

`gap(n)` and `gap_px(n)` specify logical pixels. `gap_units(n)` multiplies by 4, so `gap_units(2)` is 8 logical pixels. Buttons support `primary`, `secondary`, `outline`, `ghost`, a BASE default size, methods `inherit()`, `xs()`, `sm()`, `base()`, `md()`, `lg()`, `xl()` (with `small()`, `medium()`, `large()` aliases), and `size(name)` for the full INHERIT–9XL scale, rounding methods, `disabled`, and `hover_color('#RRGGBB[AA]')`. `rounded_md()` maps to the bridge's large rounding preset; `rounded_px(n)` sets an exact radius. Unsupported style methods are absent.

`App.step(dt)` renders one frame, then drains click events and calls handlers. A handler may update labels and button styles for the next frame. `App.start(fps)` schedules `step` with Node timers, keeping the event loop responsive. `App.stop()` stops the timer; `App.close()` requests CUI closure and stops it; `App.dispose()` releases native resources. Host tests can queue `pointer` and `key` events, then call `step`. Handles belong to one app, and operations after disposal or removal throw.

`App.run()` is the synchronous native callback runner for live window resize.
It dispatches handlers after each completed frame and returns on close. Callback
errors are rethrown after leaving the native runner. Unlike timer-based `start()`,
it blocks JavaScript timers and I/O until closure. Do not mix the two loops or
call `step()`/`run()` recursively from a handler. See
[runner lifecycle](../../docs/BINDING_NATIVE_RUN.md) for ownership and errors.
