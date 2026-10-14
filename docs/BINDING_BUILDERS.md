# Build a native UI in Python, JavaScript or TypeScript

For installable local wheel, npm, Go module and Rust crate artifacts, see the
[packaging guide](PACKAGING.md). Linux alpha packages are tested outside the
checkout; registry publication and other platform packages remain pending.

The component builders let you author a UI tree in your language. CUI performs
layout and input handling in C; the optional SDL adapter draws a native window.
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
Disposed or stale handles are rejected. Keep each app on one thread and serialize **all calls across all apps**: the
core uses a process-global Clay context. Explicitly release each app, and drain its bounded event queue. Most strings must be valid UTF-8 without
embedded NUL, at most 4096 bytes. Clipboard accepts 64 KiB; Settings search accepts
256 bytes; chart labels and table cells accept 128 bytes. Trees support up to 128 levels including their
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

## Dialogs, trees, images and date entry

Surface builders use `open()` and `on_close()`; popovers and hover surfaces
require a live `anchor(node)`. Dialog, Sheet and Popover accept child components.
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
Notification is an in-app card. It uses title/detail, `open`, `is_open`, click and
close callbacks. Cards share one stack; there is no OS notification delivery,
autohide or action-button API in this bridge subset. The close callback arrives after the close animation completes;
`is_open()` becomes false when dismissal starts. Set the title/detail before opening.

FocusTrap accepts children and `open()`/`is_open()`. Its open modal focus scope
blocks pointer actions outside the subtree; closing restores outside interaction.

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
`selected`. Resizable contains Item panels with `panel_size(100..4096)` widths;
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
Indices are one-based, with zero meaning none. Sort events request an order;
replace the matrix yourself to apply it. Plot copies `plot_points([[x,y], ...])`;
its bridge subset draws dots and does not yet expose axes, pan/zoom or hit tests.

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
