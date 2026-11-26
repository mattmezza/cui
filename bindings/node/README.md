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

`App.run(afterFrame?)` is the synchronous native callback runner for live window
resize. It dispatches handlers after each completed frame, then calls the
optional hook with the app if still open. Callback errors are rethrown after
leaving the native runner. The runner blocks JavaScript timers and I/O until
closure. Do not mix `start()` and `run()` or call `step()`/`run()` recursively
from a handler. See
[runner lifecycle](../../docs/BINDING_NATIVE_RUN.md) for ownership and errors.

DataTable can request another page near the end of its viewport. Use
`app.supports_data_table_paging()`, `table.configure_load_more()` and
`table.poll_load_more()` with your app's request state.
[JavaScript and TypeScript paged inboxes](../../docs/BINDING_BUILDERS.md#page-a-javascript-or-typescript-datatable)

Radio and RadioGroup can animate their selection marks when the bridge supports
the optional motion setting. The delivery form runs headlessly or opens a native
window and updates an arrival estimate in its existing selection callback.
[Radio motion and delivery examples](../../docs/BRIDGE_COMPONENTS.md#animate-retained-radio-selection-from-c)

## Controlled Single/Range Slider (unreleased)

Read [the Node/TypeScript controlled guide](../../docs/NODE_SLIDER_MODEL.md)
and its complete authored UI examples. This is additive to the scalar `slider()`
above: owned copied records, BigInt identities and explicit full-pair receipt
acceptance after stepping. Direct C custom Styled state is not exposed.

Localized TimeField authoring is documented in
[the dedicated guide](../../docs/NODE_TIME_FIELD_LOCALIZED.md), with complete
English/Arabic JS and TS UI examples. This unreleased optional group passed the focused Linux worker gates; root
integration and generated resources remain pending. Legacy `time_field()` remains unchanged.

Retained Attachment built-in actions are optional. Check
`app.supports_attachment_actions()` before mounting a card configured with
`.attachment_actions({open: true, remove: true, retry: true})`. Each call replaces
the choices; `.attachment_actions()` clears them, including on a live card.
These actions are independent of presentation options and previews.

Use `app.step_only()` and manually drain `app.poll_component_notification_v1()`.
This owned shared-FIFO view includes all existing value notifications and
`kind: 'attachment-action'` with typed `action`, `part`, optional `source` and
`eventIndex`. Its common `frame` is a BigInt and `handle` includes slot and
generation. Perform application file work once after polling. Legacy polls
consume the same FIFO and downgrade actions; `run`, `step` and callback dispatch
keep their existing behavior. Unsupported typed calls fail without draining.
Try `CUI_NODE_ADDON=/path/to/cui.node node bindings/node/examples/attachment-actions.cjs`
from the repository root for real keyboard actions in a headless authored UI.

Retained custom Attachment content uses explicit structural slots:

```js
attachment('file', 'Report.pdf').children(
  attachment_slot('details', 'metadata').children(label('Reviewed today')),
  attachment_slot('actions', 'actions', 120).gap(4).children(button('Download'))
)
```

Check `app.supports_attachment_slots()` before mounting. Each card accepts one
slot per role: `media`, `overlay`, `metadata`, `actions`. Slots must be direct
Attachment children; ordinary cards continue to work with older bridges and
addons. Actions require a preferred width greater than zero and at most 4096;
other roles require zero (the default). Gap and disabled use the ordinary builder
methods. Descendant buttons keep their own ordinary click handlers and handles.
Removing a card or slot invalidates its descendants as usual. Media supplies a
fallback; decoded previews and lifecycle feedback can take precedence. Overlay
content is independent. Notification descendants are unsupported.

Run `CUI_NODE_ADDON=/path/to/cui.node node bindings/node/examples/attachment-slots.cjs`
for a headless example. Pass `--font /absolute/font.ttf` to opt into its native window; automated tests
must remain headless. The headless example clicks Inspect using public bounds
and pointer input and verifies that its ordinary handler updates the sibling summary.
