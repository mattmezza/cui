# Build a native UI in Python, JavaScript or TypeScript

For installable local wheel, npm, Go module and Rust crate artifacts, see the
[packaging guide](PACKAGING.md). Linux alpha packages are tested outside the
checkout; registry publication and other platform packages remain pending.

The component builders let you author a UI tree in your language. CUI performs
layout and input handling in C; the optional SDL adapter draws a native window.
Native builder windows default to visible creation. Python `initially_hidden=True`
and JavaScript/TypeScript `initiallyHidden: true` request hidden creation directly
in SDL's constructor, before the first frame. See [native visibility](#native-window-visibility).

The bridge exposes tested API subsets for all 78 catalog families through native CUI renderers and app services,
plus rows, columns, radio groups and composite items. Python, JavaScript,
TypeScript, [Go](GO_BUILDERS.md) and [Rust](RUST_BUILDERS.md) construct these
components in their own language and receive copied events after C frames.
This is a tested API subset for each family. The
[component coverage and limitations](BRIDGE_COMPONENTS.md) identify unsupported
options; the [machine-readable coverage](binding-coverage.json) keeps all 78 entries.
Clipboard and Theme are app services; the other families have component builders.
This is language authoring coverage, not complete option or feature parity.

## Dependencies and compiler

Build from the repository root with CMake 3.20+, Ninja, a C11 compiler and
Python 3.10+. GCC and Clang work on Linux. Python uses standard-library `ctypes`,
without pip dependencies. Node additionally needs Node 18+ and its development
headers (`node_api.h`); no npm packages or node-gyp are required. The TypeScript
example uses Node's built-in type stripping, tested here with Node 25.

Native windows additionally require installed SDL3 3.2+, FreeType, HarfBuzz,
pkg-config and an existing TrueType/OpenType font. CMake discovers and links
these libraries. Set the font path explicitly. A missing native adapter reports
an error when you request SDL; it does not silently select headless mode.

The portable C bridge and Python have Linux, macOS and Windows CI evidence for
the previously published control subset. Node, Go and Rust runtime validation
uses Linux. The expanded model tests run headlessly on Linux; native Windows
and macOS binding applications have not been validated. Windows DLL exports,
Node import library discovery and macOS Node linker settings are provided. See the [platform matrix](PLATFORM_MATRIX.md).

## Python: build and launch

The [Attachment guide](BRIDGE_COMPONENTS.md#show-an-attachment-preview-in-a-language-builder)
shows fitted file labels and copied RGBA previews in a runnable example.

The [Icon and Badge guide](PYTHON_ICON_BADGE.md) shows how to share custom SVG
artwork, wrap controls in typed Badge indicators, and update them from callbacks.

The [responsive Skeleton guide](PYTHON_SKELETON.md) includes a Python-authored
profile UI that replaces loading placeholders with content. See the
[JavaScript and TypeScript](NODE_SKELETON.md), [Go](GO_SKELETON.md) and
[Rust](RUST_SKELETON.md) examples use the same composition.

The [Spinner guide](SPINNER.md#use-spinner-from-a-language-builder) shows how
to add a rotating catalog icon while a callback-driven operation is pending.

The [live inbox example](#keep-a-python-datatables-scroll-position-during-updates)
shows a keyed DataTable update from a callback.

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

The [workspace preferences guide](BRIDGE_COMPONENTS.md#style-disclosure-from-language-builders) shows styled Accordion and Collapsible panels with queued change callbacks in Python, JavaScript and TypeScript.

## Configure Pagination from Python

Use `pagination(key)` to retain the current page and register `on_change` for commits. Extended navigation requires a compatible V1 bridge. Check `app.supports_pagination_options()` before using it with a dynamically selected library. The standard numbered pager works with earlier providers.

```python
from cui_bridge import PaginationConfig, pagination

pager = pagination("results-pages").configure(PaginationConfig(
    chevrons=True, jump_menu=True, visible_pages=5, available_width=320,
    group="Pages de résultats", previous="Précédente", next="Suivante",
    earlier_pages="Pages précédentes", later_pages="Pages suivantes",
)).design_size("base").custom_size(52.5)
pager.total(24).on_change(lambda event: print(f"Loading page {int(event.value)}"))
```

Supply the current container's inner logical width to wrap between whole controls, or use zero for one row. Copy a revised width into the mounted pager with `configure` on the owner thread. `get_config()` returns the current options, and `configure(None)` clears them. The design size is independent and can be reset with `design_size("inherit")`. Use `custom_size(52.5)` for an exact logical-pixel height and `get_custom_size()` to read it. Heights from 1 through 512 accept fractions. Set zero to return to the named scale. If the provider can vary, check `app.supports_pagination_custom_size()` before opting in. Opening, scrolling and dismissing a jump menu leave the selected page unchanged. Callbacks run through normal bridge dispatch after a committed page change.

The [runnable results example](../bindings/python/examples/pagination.py) updates real result labels after a page change. Run it from the repository root with the Python binding and matching bridge available:

```sh
PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" \
  python bindings/python/examples/pagination.py
```

Append `--font /absolute/path/to/font.ttf` to run the SDL window. Build the bridge with SDL support for that mode.

## Keep an inbox in place while rows change

The retained VirtualList in Python, JavaScript and TypeScript can opt into
scroll anchoring across live row edits. Append a leaf Item and submit the
full order and heights in one copied projection. Ordinary row scrolling
still reports offset changes. The runnable inboxes show how Button callbacks
prepend a message or change a row's height. [Live inbox guide](VIRTUAL_LIST.md#live-updates-from-the-language-builders)

## Keep a Python DataTable's scroll position during updates

Use the keyword argument `row_keys` when mounting and updating a DataTable.

```python
from cui_bridge import data_table

headers = ["Subject", "Sender"]
rows = [["Release notes", "Build team"], ["Design review", "Morgan"]]
inbox = data_table("inbox").table_data(headers, rows, row_keys=[203, 204])

# Call on the UI thread after mounting, for example from a dispatched callback.
inbox.table_data(headers, [["New message", "Workspace"], *rows],
                 row_keys=[301, 203, 204])
```

Assign each row a distinct integer key from 1 through 4,294,967,295. Reuse
keys across replacements. The builder copies the matrix and keys. It keeps
the first visible row and its intra-row offset where scroll bounds allow.
Selection and keyboard focus follow surviving keys. `table_state()["row"]`
and change notifications report the selected key. `current_cell(row, column)`
and `table_cell_bounds(row, column)` take zero-based matrix indices.
The ordinary setters accept 32 rows, 16 columns, and 128 UTF-8 bytes per string.

Use `app.supports_keyed_data_table()` to check the loaded provider. Older
libraries accept `table_data(headers, rows)` with positional keys and reset
behavior. An explicit keyed request on those libraries raises `RuntimeError`
and leaves the previous table intact. Use `row_keys=[]` to submit an empty
keyed table.

Run the [complete inbox](../bindings/bridge/examples/data_table_live.py) from
the repository root after [building the Python bridge](#python-build-and-launch).
The default run opens no window and checks that the visible row stays in
place after the Load latest button inserts two rows above it.

```sh
export CUI_BRIDGE_LIBRARY="$PWD/build-bridge/libcui_bridge.so"
export PYTHONPATH="$PWD/bindings/python"
python bindings/bridge/examples/data_table_live.py
```

For interactive SDL, provide an installed font using `--font /path/to/font.ttf`.
This example applies local data from a callback.
[Load more when scrolling reaches the end](#page-a-python-datatable)

## Page a Python DataTable

Check `app.supports_data_table_paging()` before mounting a tree with paging.
The bridge must export the complete V1 API. A bridge without paging can still
use its existing positional or keyed table setters. On an unmounted builder,
`configure_load_more(enabled=False)` clears the paging opt-in for that fallback.

```python
from cui_bridge import data_table

inbox = (data_table("inbox")
         .table_data(["Subject"], initial_messages, row_keys=initial_keys)
         .configure_load_more(threshold=1))

# After mounting and stepping, on the UI thread.
request = inbox.poll_load_more()
if request is not None:
    inbox.configure_load_more(enabled=False)
    # Enqueue the fetch using app-owned request identity and cursor.
```

`threshold` is the number of rows remaining beyond the visible window, from
0 through 32. With zero, demand appears when the window reaches the last row.
You can bind headers with empty rows and keys to request the initial page.
`poll_load_more()` consumes one immutable `DataTableLoadMore` record or
returns `None`. The record holds the Core frame sequence, row count, exclusive
visible index range, last row key and capacity. It stays readable after the
app closes. A row key describes local data. Keep the service cursor and
request generation in your application.

Keep one request in flight. While loading, turn paging off and continue
drawing the existing table and a status label. Apply accepted responses
on the UI thread with `table_data(..., row_keys=...)` to preserve scroll
position and selection. Re-enable paging when the source has another page.
Demand stops when the table reaches the active row capacity.

For a native loop, pass `app.run(after_frame=update)`. The callable receives
the App after a successful step and regular event dispatch. It runs
synchronously on the owner thread. A close requested during dispatch skips
the hook. Use a worker for blocking I/O and poll completed work in the hook.
For your own loop, call `poll_load_more()` after `step()`.

Run the [offline paged inbox](../bindings/bridge/examples/data_table_paging.py)
from the repository root after [building the Python bridge](#python-build-and-launch).
It opens no window by default. It exercises scrolling, selection during
loading, retry, stale response rejection, and appending a final page.

```sh
export CUI_BRIDGE_LIBRARY="$PWD/build-bridge/libcui_bridge.so"
export PYTHONPATH="$PWD/bindings/python"
python bindings/bridge/examples/data_table_paging.py
```

For the native demo, build with SDL enabled and add `--font /path/to/font.ttf`.
The demo uses a local source and simulates latency without blocking the UI.

### Retain more than 32 rows in Python

`App.data_table_max_row_capacity()` reports the loaded provider's usable
opt-in limit, up to 10,000 rows in Python. It returns `None` when the provider
lacks the complete capacity API. Choose a capacity within that limit and
supply it on every opted-in update, along with stable row keys.

```python
from cui_bridge import App, column, data_table

with App() as app:
    maximum = app.data_table_max_row_capacity()
    if maximum is None or maximum < 64:
        raise RuntimeError("This inbox requires a provider with at least 64 rows")
    messages = data_table("messages").visual_size(360, 160)
    messages.table_data(["Subject"], [], row_keys=[], row_capacity=64)
    app.mount(column("inbox").children(messages.configure_load_more()))
    app.step()
    first_page = messages.poll_load_more()
    assert first_page and first_page.capacity == 64
    messages.table_data(["Subject"], [[f"Message {i}"] for i in range(40)],
                        row_keys=range(1, 41), row_capacity=64)
    app.step()
```

This example also requires the V1 paging API to poll for the first page.
For a mounted widget, set data on the owner's UI thread between steps.
The row capacity can change with a successful replacement and is reported
in the next demand record. Leave `row_capacity` out to use the ordinary
32-row setter. A rejected replacement preserves the old data and paging
record. The native provider's [32 MiB app-wide budget](BRIDGE_COMPONENTS.md#retain-more-than-32-datatable-rows-in-c)
applies to the copied C matrices. Apply large snapshots when data arrives
and reuse them across frames.

Run `python bindings/bridge/examples/data_table_paging.py --larger` after
setting the same bridge and import paths as above. The example grows to
40 rows, preserves the selected row and its position, and discards a late
response. Add `--font /path/to/font.ttf` for the interactive SDL inbox.

## JavaScript and TypeScript: build and launch

The [Attachment guide](BRIDGE_COMPONENTS.md#show-an-attachment-preview-in-a-language-builder)
shows fitted file labels and copied RGBA previews in a runnable example.

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
The Icon and Badge inbox demonstrates shared SVG, nested controls and
layout that stays in place as a callback clears a count.
[JavaScript and TypeScript inbox](NODE_ICON_BADGE.md).

## Configure Pagination from JavaScript and TypeScript

Use `pagination(key)` with the options object below. TypeScript exports `PaginationConfig` and returns a read-only copy from `get_config()`. Check `app.supports_pagination_options()` if the provider can vary. An earlier provider can mount the ordinary pager and deliver its page-change events.

```js
const {pagination} = require('@cui/native');

const pager = pagination('results').total(24).design_size('base').custom_size(52.5).configure({
  chevrons: true, jumpMenu: true, visiblePages: 5, availableWidth: 320,
  group: 'Pages de résultats', previous: 'Précédente', next: 'Suivante',
  earlierPages: 'Pages précédentes', laterPages: 'Pages suivantes',
}).on_change(event => console.log(`Loading page ${event.value}`));
```

The bridge copies the strings. Provide a logical inner width for wrapping, or use zero for one row. A mounted pager can be reconfigured on its owner thread. Call `configure(null)` or `reset_config()` to clear presentation options. Use `design_size('inherit')` to reset the separate named size. `custom_size(52.5)` sets the control height in logical pixels and accepts values from 1 through 512. Read it with `get_custom_size()` and pass zero to restore the named size. Check `app.supports_pagination_custom_size()` independently of V1 options for a varying provider. A page-change callback fires when navigation commits, including when a jump menu selects a page.

Run either checked-in example from the repository root with `CUI_NODE_ADDON` pointing to the built addon:

```sh
node bindings/node/examples/pagination-options.cjs
node bindings/typescript/examples/pagination_options.ts
```

Both examples exercise the result counter in headless mode. The JavaScript example accepts a font path as its first argument to open an SDL window. The TypeScript command uses Node's type-stripping support. [JavaScript source](../bindings/node/examples/pagination-options.cjs) · [TypeScript source](../bindings/typescript/examples/pagination_options.ts)

For the same retained pager on other hosts, see the [Go results example](GO_BUILDERS.md#configure-pagination-from-go) and [Rust results example](RUST_BUILDERS.md#configure-pagination-from-rust).

## Keep a JavaScript or TypeScript DataTable's scroll position during updates

Pass the row keys as the third argument to `table_data`.

```javascript
const {data_table} = require('./bindings/node/index.cjs');

const headers = ['Subject', 'Sender'];
const rows = [['Release notes', 'Build team'], ['Design review', 'Morgan']];
const inbox = data_table('inbox').table_data(headers, rows, [203, 204]);

// Apply on the UI thread after mounting, for example from an on_click callback.
inbox.table_data(headers, [['New message', 'Workspace'], ...rows], [301, 203, 204]);
```

Attach the array to both the initial data and later replacements. The builder
copies headers, cells, and keys. Use one distinct integer from 1 through
4,294,967,295 per row, with the same key wherever that logical row appears.
The ordinary setters accept up to 32 rows, 16 columns, and 128 UTF-8 bytes per string.
`table_state().row` and change events report the selected key.
Look up cells and layout bounds by zero-based matrix indices.

`app.supports_keyed_data_table()` checks the addon and shared bridge. Older
providers still support `table_data(headers, rows)` with one-based positional
keys and reset behavior. An explicit keyed request on an older provider
throws before changing the current table or attaching a new tree.

After [building the addon](#javascript-and-typescript-build-and-launch), run
the [JavaScript inbox](../bindings/node/examples/data-table-live.cjs) from the
repository root. It opens no window by default and verifies that the visible
row stays in place after the **Load latest** button inserts two messages.
Pass a font path as the first argument for an interactive SDL window.

```sh
export CUI_NODE_ADDON="$PWD/build-node/cui.node"
node bindings/node/examples/data-table-live.cjs
```

The [TypeScript inbox](../bindings/typescript/examples/data_table_live.ts)
uses the same API. With an existing TypeScript compiler, this command checks
the declarations and runs the compiled example with the addon above.

```sh
python bindings/node/test/run-data-table-types.py --case keyed \
  --tsc "$(command -v tsc)" --node "$(command -v node)" \
  --output build-node/keyed-data-table-types
```

## Page a JavaScript or TypeScript DataTable

Check `app.supports_data_table_paging()` before opting in. It checks the addon
and the complete V1 API in the bridge. To mount a previously configured tree
on an older provider, clear the opt-in with `configure_load_more({enabled: false})`
before mounting. Existing positional and keyed setters remain available on
providers that support each setter.

```javascript
const {data_table} = require('./bindings/node/index.cjs');

const inbox = data_table('inbox')
  .table_data(['Subject'], initialMessages, initialKeys)
  .configure_load_more({threshold: 1});

// After mounting and stepping, on the UI thread.
const request = inbox.poll_load_more();
if (request) {
  inbox.configure_load_more({enabled: false});
  // Schedule the next page using an app-owned request identity and cursor.
}
```

The threshold is the number of rows beyond the visible window, from 0 through
32. The default is 0. You can also mount with headers and empty rows to request
the first page. `poll_load_more()` consumes one frozen `DataTableLoadMore` record
or returns `undefined`. `frameSequence` is `bigint`. The other fields are numbers
and the visible index range is half-open. Polling leaves the normal callback
queue alone, and a copied record remains readable after unmount or close.

Keep a single request in flight and disable paging while it runs. The rows
remain interactive. Verify the response against the current request before
appending through `table_data(headers, rows, rowKeys)` on the UI thread. This
preserves the viewport and selection. Enable paging again when more data is
available. Demand stops at the active row capacity.

`app.run(afterFrame)` calls a synchronous hook after painting and regular
event dispatch. Closing from regular dispatch skips the hook. The native runner
blocks Node's event loop. For Promise or timer-based transports, drive the UI
from your own Node loop with `app.step()` and poll after each successful step.

The [JavaScript inbox](../bindings/node/examples/data-table-paging.cjs) checks
selection during loading, retry, rejection of a stale response, and an anchored
final append. Pass a font path as the first argument to try the native demo
with an SDL-enabled bridge. The [TypeScript inbox](../bindings/typescript/examples/data_table_paging.ts)
uses the same API and verifies an anchored append to 40 rows with strict
types. With the addon built and a TypeScript compiler, run from the repository
root:

```sh
export CUI_NODE_ADDON="$PWD/build-node/cui.node"
node bindings/node/examples/data-table-paging.cjs
python bindings/node/test/run-data-table-types.py --case paging \
  --tsc "$(command -v tsc)" --node "$(command -v node)" \
  --output build-node/data-table-paging-types
```

### Retain more than 32 rows in JavaScript or TypeScript

Query `app.data_table_max_row_capacity()` for the usable opt-in limit,
up to 10,000 rows. It returns `undefined` when the addon or provider lacks the
capacity API. Pass stable row keys and `{rowCapacity}` as the fourth argument
on each opted-in replacement. Omit the fourth argument to use the ordinary
32-row setters.

```javascript
const {App, column, data_table} = require('./bindings/node/index.cjs');

const app = new App({backend: 'headless'});
try {
  const maximum = app.data_table_max_row_capacity();
  if (maximum === undefined || maximum < 64)
    throw new Error('This inbox requires capacity for 64 rows');
  const table = data_table('inbox').table_data(['Subject'], [], [], {rowCapacity: 64});
  app.mount(column().children(table)).step();
  const keys = Array.from({length: 40}, (_, i) => i + 1);
  table.table_data(['Subject'], keys.map(key => [`Message ${key}`]), keys, {rowCapacity: 64});
  app.step();
} finally {
  app.dispose();
}
```

Update on the UI thread between steps, as with ordinary keyed snapshots.
The next paging record reports the active capacity. A rejected replacement
preserves the previous data and pending demand. The [32 MiB C retention
budget](BRIDGE_COMPONENTS.md#retain-more-than-32-datatable-rows-in-c) applies
per app. The addon also uses bounded temporary marshalling storage, up to
about 21 MiB on 64-bit hosts for 10,000 rows of 16 columns. Apply large
snapshots when data arrives and reuse them across frames.

Run `node bindings/node/examples/data-table-paging.cjs --larger` with the
same addon path as above. The example grows from 8 to 40 rows and preserves
selection and position. Add a font path to try it interactively with SDL.

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
Disposed or stale handles are rejected. Keep each app on one thread and serialize **all calls across all apps**: the
core uses a process-global Clay context. Explicitly release each app, and drain its bounded event queue. Most strings must be valid UTF-8 without
embedded NUL, at most 4096 bytes. Clipboard accepts 64 KiB; Settings search accepts
256 bytes; chart labels and table cells accept 128 bytes. Trees support up to 128 levels including their
root; deeper construction is rejected before frame submission.
See the [C ABI guide](../bindings/bridge/docs/README.md)
and [public header](../include/cui/cui_bridge.h) for capacities and status codes.

[Go builders](GO_BUILDERS.md) and [Rust builders](RUST_BUILDERS.md) now construct
real component trees in their own language and dispatch copied native events.
The [Go SVG resource guide](GO_ICON_RESOURCES.md) shows how to prepare artwork
with the bundled build or a compatible shared bridge.
The [Go inbox](GO_ICON_BADGE.md) uses those resources in configured Icons and Badges.
The [Rust SVG resource guide](RUST_ICON_RESOURCES.md) shows the headless preparation API.
The [Rust inbox](RUST_ICON_BADGE.md) adds a shared retained Icon and Badge plus child callbacks.
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

## Dialogs, trees, images and date entry

Surface builders use `open()` and `on_close()`; popovers and hover surfaces
require a live `anchor(node)`. Dialog, Sheet and Popover accept child components.
The [retained account menu](POPOVER_ANCHORS.md#retained-account-menu) demonstrates
anchor placement, focus entry and Escape dismissal in all five languages.
AlertDialog uses its native confirmation actions. A Tree uses up to 64 flat Item
children in preorder; Item `step_status(depth)` sets depth and `checked(True)`
starts a branch expanded. In JavaScript use `checked(true)`.

`image_rgba(pixels, width, height, stride)` accepts decoded RGBA8 bytes. Both the
builder and C bridge own copies. Dimensions are 1–2048, stride is at least
`width * 4`, and byte count must equal `stride * height`, at most 16 MiB. Decode
PNG/JPEG files in your application. Attachment currently exposes metadata and
progress; its bridge has no file transfer, remove or retry actions.

Calendar and DatePicker accept `date_value(year, month, day)`, including old
dates, and expose `current_date()`. The current bridge uses English single-date
mode. TimeField uses `time_value(hour, minute, second)` and `current_time()`.
ColorPicker uses packed `0xRRGGBBAA` with `color_rgba()` and `current_color()`;
its event copies both the packed number and `#RRGGBBAA` text. OTPInput uses
`total(cells)` and `current_text()`; send one cell per queued text-input event.

The runnable workspace examples compose navigation tabs, account fields,
notification preferences, date entry and a confirmation dialog. They create
actual component trees. Use `--headless` for their deterministic interaction
checks or omit it for a native window:

```sh
export PYTHONPATH="$PWD/bindings/python"
export CUI_BRIDGE_LIBRARY="$PWD/build-bridge/libcui_bridge.so"
python bindings/bridge/examples/workspace.py --headless
# Native: set CUI_TEST_FONT to an existing font first.
python bindings/bridge/examples/workspace.py
node bindings/node/examples/workspace.cjs --headless
# Native: set CUI_FONT_PATH to an existing font first.
node bindings/node/examples/workspace.cjs
```

Only Button exposes the bridge's design sizes, rounding and hover-color styling
at this stage. Other families use native defaults and the specific options
listed in the coverage guide; a constructor does not imply all native options
are mapped. Headless tests cover events and lifetime; they do not prove native
pixel rendering on every operating system.

Form accepts Item fields whose children are real controls; Item label/detail
supply field metadata. Disabling Form or a field suppresses its child controls.
InputGroup uses editable text and Item button addons; Item `step_status(0..3)`
chooses the addon alignment and `on_click` receives addon presses.
FocusTrap accepts children and `open()`/`is_open()`. Its open modal focus scope
blocks pointer actions outside the subtree; closing restores outside interaction.

### Notification cards

Set a card's title and detail before opening it. Body activation emits Click and
starts dismissal. Close emits only the Close event after the exit animation.
`is_open()` becomes false as soon as dismissal starts. To reopen the same node,
wait for its Close event. The bridge supports in-app delivery. The immediate C
manager also accepts custom rendering and hooks for host-managed OS delivery.

Enable wrapping and a separate Close control before hover for the app's shared
stack. The option applies from the next step, including cards that are already
open. Pass false to restore the default presentation. Check support first with
an installed shared provider. Both presentations deliver the same queued events.

| Binding | Probe | Enable |
| --- | --- | --- |
| C | Look up the optional setter when loading a shared provider | `cui_bridge_set_responsive_notifications_v1(app, 1)` |
| Python | `app.supports_responsive_notifications()` | `app.set_responsive_notifications(True)` |
| JavaScript and TypeScript | `app.supports_responsive_notifications()` | `app.set_responsive_notifications(true)` |
| Go | `supported, err := app.SupportsResponsiveNotifications()` | `app.SetResponsiveNotifications(true)` |
| Rust | `app.supports_responsive_notifications()` | `app.set_responsive_notifications(true)?` |

A retained card can own one optional Button action. The Button fills a dedicated
row below the message, leaving the Close lane clear at narrow widths. The action
card uses wrapping and a separate Close target even when the app uses the original
presentation for its other cards. The action delivers a normal Button click with
the Button as its event target. It leaves the card open so your handler can update
the view or dismiss the card explicitly. Removing the card removes the Button.
Removing the Button in the C bridge frees the slot for a replacement.

| Binding | Probe | Create a Review action |
| --- | --- | --- |
| C | Look up `cui_bridge_notification_action_create_v1` with a shared provider | `cui_bridge_notification_action_create_v1(app, notice, "review", &review)`, then `cui_bridge_set_text(app, review, "Review")` |
| Python | `app.supports_notification_action()` | `notice.action(button("review").label("Review").on_click(review_report))` |
| JavaScript and TypeScript | `app.supports_notification_action()` | `notice.action(button("review").label("Review").on_click(reviewReport))` |
| Go | `supported, err := app.SupportsNotificationAction()` | `notice.Action(cui.Button("review").Text("Review").OnClick(reviewReport))` |
| Rust | `app.supports_notification_action()` | `let review = app.notification_action(notice, "review", "Review")?;` |

Use the ordinary Button callback or poll its event through the retained event
API. Polling and callback dispatch in the bindings drain the same queue. Use one
path for each action's side effect. Python, JavaScript, TypeScript and Go attach
the action before mounting. Older providers can mount the same card without an
action. Probe first when your app supports more than one provider version.

Retained cards are persistent by default. Enable auto-hide before opening a
card to close it after five seconds in the present phase. Hover and keyboard
focus inside the notification stack pause its timers. Changing the setting
while a card is visible applies on its next opening. Expiry emits one Close
after exit. Body Click remains reserved for an actual activation. A card with
a Button action can opt into this same timer.

| Binding | Probe | Configure the next opening |
| --- | --- | --- |
| C | Look up `cui_bridge_notification_set_auto_hide_v1` with a shared provider | `cui_bridge_notification_set_auto_hide_v1(app, notice, 1)` |
| Python | `app.supports_notification_auto_hide()` | `notice.auto_hide(True)` |
| JavaScript and TypeScript | `app.supports_notification_auto_hide()` | `notice.auto_hide(true)` |
| Go | `supported, err := app.SupportsNotificationAutoHide()` | `notice.AutoHide(true)` |
| Rust | `app.supports_notification_auto_hide()` | `app.notification_auto_hide(notice, true)?` |

Older providers can still mount plain persistent cards. For SDL `run()`, the
runner keeps advancing a stack while a notification wake is pending. For a
manually stepped app, supply elapsed time and continue stepping through exit.
The wrapper uses the existing event queue for both paths.

The workspace example disables the trigger until the card has closed, and the
Review action updates the workspace status while the card stays open. With a
built bridge, run it from the repository root. Omit `--font` to exercise the
same interactions headlessly. Add `--auto-hide` for a timed status card that
re-enables the trigger when it expires. [Workspace example](../bindings/bridge/examples/notification_workspace.py)

```sh
PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" \
  python bindings/bridge/examples/notification_workspace.py --font /path/to/Inter-Regular.ttf
```

The window requires the SDL bridge. For compiler, dependency and linker setup,
see the [native quickstart](NATIVE_APP_STARTER.md) and the bridge build commands
at the start of this guide.

## Native fonts and fallback faces

Pass an existing font file that covers your text. Missing glyphs cause a render
error. Python and JavaScript/TypeScript let you replace the SDL font's ordered,
owned fallback chain with `app.font_fallbacks(["/absolute/path/symbols.ttf"])`.
The chain accepts at most 32 paths; `[]` clears it. Configure it before frames.
Go provides `Options.FontFallbacks` and `SetFontFallbacks`; Rust provides
`app.font_fallbacks(&["/absolute/path/symbols.ttf"])`. The bridge opens and owns
the fallback faces and rejects an invalid replacement without discarding the
previous chain. Fallback registration requires SDL; headless mode reports that
the backend is unavailable. It does not scan your machine for fonts.

## Sidebar navigation with editable pages

These examples define their layout and controls in Python or JavaScript. The
Sidebar selects Account or Preferences; Tabs provides the page content. Account
contains a native Input and a Save callback that reads its current text. No
C-authored application is launched behind the language API.

```sh
# Use the bridge and addon builds from the earlier sections.
python bindings/bridge/examples/sidebar_workspace.py --headless
node bindings/node/examples/sidebar-workspace.cjs --headless
# Open native windows when you want to interact with the example.
CUI_TEST_FONT=/absolute/path/font.ttf python bindings/bridge/examples/sidebar_workspace.py
CUI_FONT_PATH=/absolute/path/font.ttf node bindings/node/examples/sidebar-workspace.cjs
```

```python
from cui_bridge import (button, checkbox, column, input, item, label,
                        root_view, row, sidebar, tabs)

status = label('Ready', key='status')
name = input('name', 'Alex Morgan')
email = checkbox('email', 'Email notifications').checked()
save = button('save').label('Save changes').primary().rounded_md().on_click(
    lambda event: status.label('Saved account: ' + name.current_text()))
account = item('account-page', 'Account').children(
    column().gap_units(3).children(label('Account details'), name, save, status))
preferences = item('preferences-page', 'Preferences').children(
    column().gap_units(3).children(label('Notifications'), email))
pages = tabs('pages').children(account, preferences).select(account)
account_action = item('account-action', 'Account').on_click(lambda event: pages.select(account))
preferences_action = item('preferences-action', 'Preferences').on_click(lambda event: pages.select(preferences))
navigation = sidebar('navigation').visual_size(180, 280).children(account_action, preferences_action)
root = root_view().padding(24).children(column().gap_units(4).children(
    label('CUI workspace'), row().gap_units(6).children(navigation, pages)))
```

Mount `root` on the SDL `App` described above and run its event loop. The checked-in
[Python example](../bindings/bridge/examples/sidebar_workspace.py) includes both
the window loop and a deterministic headless interaction check. The
[JavaScript example](../bindings/node/examples/sidebar-workspace.cjs) builds the
same UI; [Go](GO_BUILDERS.md) and [Rust](RUST_BUILDERS.md) each include a composed
workspace with navigation, editing, callbacks and a confirmation dialog.

## Retained models and native interaction

The language builders retain a tree and owned data. CUI renders immediate frames
from that tree and manages native state for editors, scrolling, selection, dock
layouts, questionnaires and Settings. Event callbacks run after each C frame,
using copied snapshots rather than live pointers. With Python, dispatch events
explicitly after stepping; Node dispatches as part of `step()`.

MessageScroller contains Item messages; `detail` is the sender and
`step_status(0/1)` selects incoming/outgoing. It follows the newest message on
its initial frame. Carousel contains Item slides and provides `select` and
`selected`. Set a direct slide Item's `carousel_basis(0.5)` to fit two slides.
Check `app.supports_carousel_basis()` before using this with an older bridge.
Use `carousel_axis("vertical")` on the Carousel after checking
`app.supports_carousel_axis()`. Vertical fractions use the visual height.
Call `carousel_looping()` after checking `app.supports_carousel_looping()` to let
Carousels with enough slides wrap at the ends. Call `carousel_controls()` when
`app.supports_carousel_controls()` reports support. Arrows and pagination can be
enabled independently.
Use `visual_size(0, height)` for a growing fractional Carousel. See the
[retained Carousel example](CAROUSEL.md#retained-slides).
Resizable contains Item panels with `panel_size(100..4096)` widths;
real divider dragging updates the panel and emits its numeric change event.
VirtualList contains up to 64 keyed Item rows in a fixed-height vertical viewport.
`scroll_to` and `scroll_offset` work on MessageScroller and VirtualList.

Editor owns multiline text and exposes `current_text` and copied change events.
TextView supports `text_format("markdown"|"html"|"plain")`, rendered UTF-8 byte
selection with `select_text`, and copied `selection_text`. Neither currently
exposes completion providers or clipboard shortcuts through the bridge.

Chart copies `chart_data([["Label", value], ...])` and accepts
`chart_kind("line"|"area"|"bar"|"pie")`. Table contains Item rows with Item text
cells; the first row is the header. DataTable copies `table_data(headers, rows)`
and supports `selection_mode("row"|"column"|"cell")`. Its change event text is
JSON containing `reason`, `row`, `column`, `sortColumn` and `sortDirection`.
The legacy setter assigns one-based row keys. Columns and sort columns are
one-based, and zero means none. Sort events request an order. Replace the
matrix to apply it. Plot copies `plot_points([[x,y], ...])` and draws dots.
Axes, pan/zoom and hit tests are outside its current bridge subset.

Command contains Item actions. `label` changes the native filter query;
`current_text` and `selected` read it back. An Item's `detail` adds a filter
keyword, and its click callback receives native confirmation. Sidebar contains
Item navigation actions and supports `open`, `select`, `selected` and copied
navigation events. RootView can be the sole root and adds `padding(0..256)`.

Questionnaire contains Item questions with Item choices. Before mounting, use
`question(required=True, multiple=False)` in Python or `question(true, false)`
in JavaScript. The first frame freezes the schema. `questionnaire_next`,
`questionnaire_previous`, `questionnaire_submit`, `questionnaire_state` and
`choice_selected` expose progress. Submission enforces native answer rules;
change events contain copied JSON snapshots of answers and navigation.

Dock contains Item panels with child controls. After its first frame,
`dock_activate`, `dock_move_split`, `dock_active` and `dock_panel_bounds` expose
its retained layout. `dock_serialize` returns copied opaque `CUIDOCK 1` text,
not JSON. Panel metadata freezes on the first frame; restoring serialized layouts
and floating OS windows are not exposed yet.

## Settings pages and reset defaults

Settings uses Item pages, nested Item groups, then Switch, Checkbox, Input or
NumberInput fields. Input/NumberInput `detail` supplies the field title;
Input `label` supplies editable text and NumberInput `value` supplies its number. The first successful frame captures reset values
and freezes titles and topology. Only fields support disabled state.

```python
from cui_bridge import App, column, input, item, settings, switch

updates = switch("updates", "Automatic updates").checked()
name = input("name", "Alex").detail("Display name")
general = item("general", "General").children(
    item("account", "Account").children(updates, name))
preferences = settings("preferences").visual_size(760, 400).children(general)
with App(width=900, height=600) as app:
    app.mount(column().children(preferences))
    app.step()  # First frame captures defaults.
    name.label("Changed")
    preferences.settings_reset(general)
    app.dispatch()  # Reset delivers copied field change events.
    assert name.current_text() == "Alex"
```

`settings_query(text)` copies a search string of at most 256 UTF-8 bytes.
`settings_page()` reads the selected Item; `settings_select(page)` navigates and
emits a copied page event. `settings_field_bounds(field)` locates a visible
control for host input. `settings_reset(page)` restores all fields in that page,
including programmatic edits. The same method names work in JavaScript/TypeScript.
Limits are eight pages, 32 total groups and 128 total fields. Persist preferences
in your application; custom setting fields and dropdowns are not mapped yet.

## Theme, clipboard and speech services

`app.theme_presets()` returns native preset names. `app.theme_preset(index)`
changes one app's theme; `app.current_theme_preset()` is empty until an explicit
choice. The bridge does not automatically watch the host's appearance preference.

SDL apps support `clipboard_set(text)` and copied `clipboard_get()` on the owner
thread, outside a frame. Headless mode reports backend unavailable. Clipboard
text is valid UTF-8 without NUL, up to 64 KiB.

Speech exposes `speech_start`, `speech_report(text, final)`, `speech_finish`,
`speech_transcript` and `speech_active`. The host supplies recognition results;
this component does not capture audio or include a speech recognition provider.
Copied lifecycle events distinguish start, transcript and finish.

The [coverage guide](BRIDGE_COMPONENTS.md) documents capacities and unsupported
features for every family. [Validation receipts](validation/component-builders-expanded-78.txt)
record the combined checks and their platform limits.

For native callback loops, live resizing, callback failure containment and the
manual-loop compatibility contract, see [binding native runners](BINDING_NATIVE_RUN.md).

The optional `cui_date_time_picker.h` single civil date/time API is C-only in this revision. Generated source SDKs carry its header/implementation, but Python/JavaScript/TypeScript/Go/Rust builders and bridge records do not expose its typed value/state, assignment, options, helpers or result. Do not pass legacy date-only builder storage to it. See `docs/DATE_TIME_PICKER.md`.

### Stepper layouts

Stepper builders opt into fixed connectors and per-Item RGBA icons with
`stepper_visual(true, false, "md")` (Python uses `True`/`False`; Go uses
`StepperVisual(true, false, cui.StepperMD)`; Rust uses
`stepper_visual(node, true, false, StepperDensity::Md)`). The default remains
legacy rendering. Density is `md/xs/sm/lg`; shared `design_size` also accepts
Stepper.

Use `stepper_fill(vertical, density, width, height)` for growing connectors and
indicator hover and press feedback. Width 0 grows with the containing box.
Height 0 sizes to the steps. Positive dimensions within 4096 logical pixels
set the allocated extent. To distribute a vertical Stepper over a region, pass
that region's height. Set the axis again when your layout calls for a change.

| Language | Capability probe | Apply fill |
| --- | --- | --- |
| Python, JavaScript, TypeScript | `app.supports_stepper_fill()` | `steps.stepper_fill()` for horizontal fill, or pass the four options |
| Go | `app.SupportsStepperFill()` returns `(bool, error)` | `steps.StepperFill(false, cui.StepperMD, 0, 0)` |
| Rust | `app.supports_stepper_fill()` | `app.stepper_fill(steps, false, StepperDensity::Md, 0., 0.)?` or the fluent builder method |

The probe checks the optional `cui_bridge_set_stepper_fill` symbol. Use
`stepper_visual` for fixed connectors on an earlier provider. Calling either
setter replaces the previous layout choice. An explicit Fill request requires
the new provider. Python, JavaScript, TypeScript and Go reject unsupported
settings before mounting the tree. Rust reports the error at the setter on the
already-created node. A failed update preserves the C bridge configuration.

Use Wrapped Fill to keep long Item labels readable inside horizontal lanes or
beside each vertical indicator. It accepts the same four arguments. Leave height
at zero to let rows grow when text wraps. This API is available independently
through the optional `cui_bridge_set_stepper_wrapped_fill_v1` symbol.

| Language | Capability probe | Apply wrapped fill |
| --- | --- | --- |
| Python, JavaScript, TypeScript | `app.supports_stepper_wrapped_fill()` | `steps.stepper_wrapped_fill()` |
| Go | `app.SupportsStepperWrappedFill()` returns `(bool, error)` | `steps.StepperWrappedFill(false, cui.StepperMD, 0, 0)` |
| Rust | `app.supports_stepper_wrapped_fill()` | `app.stepper_wrapped_fill(steps, false, StepperDensity::Md, 0., 0.)?` or the fluent builder method |

Choose Centered Fill when horizontal markers and labels should share a center
line or vertical markers should center beside their labels. Connectors connect
the actual marker edges with gaps. Use the same four parameters and keep
height at zero for content-sized rows. The provider exports
`cui_bridge_set_stepper_centered_fill_v1` independently of the wrapping setter.

| Language | Capability probe | Apply centered fill |
| --- | --- | --- |
| Python, JavaScript, TypeScript | `app.supports_stepper_centered_fill()` | `steps.stepper_centered_fill()` |
| Go | `app.SupportsStepperCenteredFill()` returns `(bool, error)` | `steps.StepperCenteredFill(false, cui.StepperMD, 0, 0)` |
| Rust | `app.supports_stepper_centered_fill()` | `app.stepper_centered_fill(steps, false, StepperDensity::Md, 0., 0.)?` or the fluent builder method |

Switching between the layout setters preserves the selected Item. User
selection still produces a queued CHANGE. See the capability fallback in
[Stepper navigation](STEPPER_NAVIGATION.md#foreign-builder-visual-authoring).

The complete headless examples select the newest layout the provider supports. [Python](../bindings/python/examples/stepper_visual.py) · [JavaScript](../bindings/node/examples/stepper_visual.cjs) · [TypeScript](../bindings/typescript/examples/stepper_visual.ts) · [Go](../bindings/go/examples/stepper_visual/main.go) · [Rust](../bindings/rust/examples/stepper_visual.rs)

Use the existing Item `image_rgba` method (Go `RGBA`, Rust `image_rgba`) to supply
straight-alpha RGBA icons. Builders and bridge copy bytes; use white RGB for
monochrome foreground tint. Only Item children of Stepper accept this image
storage; other Item parents reject it at mount/update. Replacement validates and
allocates before releasing the previous copy; removing the Item releases it.
Missing icons use ordinals, while XS hides icons and ordinals. Existing image
limits remain 2048 per dimension and 16 MiB including stride padding.

The bridge keeps its 64-Item limit, selected Item and aggregate CHANGE phase.
Visual paint uses a selection snapshot; accepted selection appears on the next
submission. Selecting the already selected Item emits no CHANGE; accepted changed
selection queues one existing aggregate CHANGE after rendering. This is not the C
controlled REQUEST callback surface. Legacy mode preserves its prior behavior.
Use matching bridge/addon/SDK revisions for the additive methods. See
[Stepper navigation](STEPPER_NAVIGATION.md) and the
[binding design contract](STEPPER_BINDING_VISUAL_DESIGN.md).

The retained Fill layout uses the theme's indicator colors. Use
`cui_stepper_present` directly from C for custom palettes or synchronous
application-controlled requests.

## Native window visibility

```python
with App(backend="sdl", font_path=font, initially_hidden=True) as app:
    app.mount(row())
    app.step()                 # stays hidden
    app.set_window_visible(True)
    assert app.window_visible()
```

```javascript
const app = new App({backend:'sdl', fontPath:font, initiallyHidden:true});
app.mount(row()); app.step(); // stays hidden
app.set_window_visible(true);
console.assert(app.window_visible());
```

The same JavaScript methods are declared for TypeScript. Defaults remain visible;
there is no show-then-hide fallback. The bridge uses independent version-1
`cui_bridge_window_options` through `cui_bridge_create_with_window_options`;
`cui_bridge_options`, its exact-size check, ABI version and old constructor stay
unchanged. Query returns SDL's hidden flag, not occlusion/minimization. OS mapping
may be asynchronous. Headless hidden requests and visibility operations return
backend unavailable; default headless creation is unchanged. Calls belong on the
native window's owning UI thread, outside an active Core frame. Application event
handlers dispatched after a completed frame by the native callback loop may
show/hide/query; `step` and `run` themselves never show the window. Closed apps
reject visibility operations. Python resolves the new window symbols optionally; default apps do not require
these exports. Hidden creation/control fails explicitly if they are absent.
Missing-export fixtures verify this feature boundary, not compatibility with
every historical ABI1 build. Node addon and native library must match.

Focused Linux private-Xvfb tests cover C, Python, JavaScript, compiled/executed
TypeScript, Go and Rust, including the actual SDL constructor flag and first
presentation flag, explicit show/hide, default visible creation and after-frame
callbacks. This is not Wayland/Windows/macOS or physical GPU validation. Run with
an existing native bridge build, installed tooling and an explicit font:

```sh
python scripts/test_window_visibility_bindings.py --build-dir build --font /absolute/font.ttf
# --tsc /existing/typescript/lib/tsc.js accepts an already installed compiler.
```

The helper downloads nothing, builds only focused dynamic tests/addon and a Linux
test-only SDL interposer, then uses `scripts/test_local.py` for private Xvfb.
Sokol and SDL_GPU renderer-only hosts retain ownership of their own windows.

The dedicated native-binding workflow also runs this focused visibility helper
against its existing bridge build, with pinned TypeScript5.8.3 and Rust1.85.1.
Hosted CI results remain separate from the local private-Xvfb evidence above.

## Bounded typed root shadows

See [typed root shadow authoring](BRIDGE_SHADOWS.md) for the six-kind implementation, owned values, canonical CSS/preset factories and explicit clear/reset behavior. Focused portable runtime tests cover this subset; integration remains pending. This does not expose compound parts or portals.

For bounded semantic ROOT/MAIN/indicator/track/thumb authoring, read
[typed part shadows](BRIDGE_PART_SHADOWS.md). Slider has no thumb; owner topology,
whole-group naming and separate reset are explicit parts of that contract.

## Python controlled Single/Range Slider

The unreleased additive `slider_model(key, config, confirmed)` builder owns frozen
double Single/Range request records and uses explicit receipt acceptance after
step. Read [the exact Python contract and complete authored UI](PYTHON_SLIDER_MODEL.md).
The old `slider()` examples above preserve scalar float/autocommit CHANGE behavior.
Rust controlled builders remain pending.

The matching [Node/TypeScript controlled subset](NODE_SLIDER_MODEL.md) adds
owned frozen records, exact BigInt identities and explicit receipt acceptance.
Its JS and compiled TS examples author actual nested control UIs; close requests
shutdown while dispose destroys the App. Direct C custom Styled state remains
unbound. Rust controlled builders remain pending.

The [Go controlled subset](GO_SLIDER_MODEL.md) adds owned double value copies,
uint64 identities and private receipt authority. Its complete authored UI accepts
or rejects whole pairs after Dispatch; existing Step/Close/RequestClose OS-thread
semantics remain. Rust controlled authoring remains pending.
