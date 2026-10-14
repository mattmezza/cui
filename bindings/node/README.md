# Native CUI for Node.js and TypeScript

This binding creates a real CUI scene. The N-API addon calls the shared C bridge; CUI computes layout, renders through its software or SDL backend, and queues input events. JavaScript owns callbacks and updates nodes after `step()` returns from C. The fluent API currently exposes row, column, label, and button. Other manifest components are not exposed by this binding.

## Build and run

Prerequisites: CMake, Ninja, C compiler with C11, Node.js 18+ and its `node_api.h` development header. The repository's pinned native dependencies are used by its CMake build. From the repository root:

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=ON
cmake --build build-node --target cui_node_bindings test_bridge_core
ctest --test-dir build-node -R "^(bridge_core|node_binding)$" --output-on-failure
node bindings/node/examples/save.cjs --headless
```

The build places `cui.node` in `build-node/`; set `CUI_NODE_ADDON=/absolute/path/to/cui.node` if using another build directory. The private CMake project builds the C bridge and links the addon; there are no npm packages or node-gyp steps. For an actual window, ensure CMake reports that SDL3 and text adapters are enabled, and provide a font file:

```sh
CUI_FONT_PATH=$(fc-match -f '%{file}\n' sans | head -1) node bindings/node/examples/save.cjs
```

The SDL backend creates a native window. Headless mode uses the same CUI scene/layout/input path and software renderer without opening a window. The example exits on Ctrl+C. Run a native example under a fresh Xvfb display when testing on a server; do not point it at an active desktop display.

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

`gap(n)` and `gap_px(n)` specify logical pixels. `gap_units(n)` multiplies by 4, so `gap_units(2)` is 8 logical pixels. Buttons support `primary`, `secondary`, `outline`, `ghost`, size methods from `inherit()` through `xl()` plus `size('2xl')` through `size('9xl')`, rounding methods, `disabled`, and `hover_color('#RRGGBB[AA]')`. `rounded_md()` maps to the bridge's large rounding preset; `rounded_px(n)` sets an exact radius. Unsupported style methods are absent.

`App.step(dt)` renders one frame, then drains click events and calls handlers. A handler may update labels and button styles for the next frame. `App.start(fps)` schedules `step` with Node timers, keeping the event loop responsive. `App.stop()` stops the timer; `App.close()` requests CUI closure and stops it; `App.dispose()` releases native resources. Host tests can queue `pointer` and `key` events, then call `step`. Handles belong to one app, and operations after disposal or removal throw.
