# Go builders for the CUI bridge

`bindings/go` is a cgo Go module for the portable CUI bridge ABI v1. It mounts row, column, label, Button, Checkbox, Radio, Switch, Toggle, Slider, Progress, Badge, Input, Textarea, NumberInput, Collapsible, Alert, RadioGroup, Tabs, Accordion, Item, Select, ComboBox, DropdownButton, Dialog, AlertDialog, Sheet, Popover, Tooltip, HoverCard, Avatar, Bubble, EmptyView, GroupBox, Kbd, Skeleton, Spinner, Tag, Marker, Shimmer, List, Menu, Pagination, Rating, Stepper, DescriptionList, Scrollable, StatusBar, Toolbar, TitleBar, Icon, Message, and Attachment nodes. Builders create a detached tree; `App.Mount` validates it and creates actual CUI nodes. `Step` submits the frame, `QueuePointer` and `QueueKey` send synthetic input, and `Dispatch` invokes Go click and change callbacks after the native frame. Change events carry the state and copied text captured when each event was queued. `App.Run` provides a native event loop. The API covers the listed bridge components, not the 78-component manifest.

Requires Go 1.23 or newer, a C11 compiler, CMake, Ninja, `CGO_ENABLED=1`, and a built CUI bridge shared library. On Linux, build and test from the repository root:

```sh
cmake -S bindings/bridge -B build-go-bridge -G Ninja -DCMAKE_C_FLAGS='-Wall -Wextra -Werror'
cmake --build build-go-bridge --target cui_bridge test_bridge_core test_bridge_components
python scripts/test_local.py --build-dir build-go-bridge -- -R '^bridge_'
cd bindings/go
CUI_BRIDGE_LIBRARY="$PWD/../../build-go-bridge/libcui_bridge.so" go test ./...
```

The Go package loads `CUI_BRIDGE_LIBRARY` at runtime with `dlopen` on POSIX or UTF-8 decoding and `LoadLibraryW` on Windows, verifies the ABI version and options size, and calls the C bridge through a small cgo shim. `go test` itself uses the headless backend and opens no window. The library path must be absolute (`.so` on Linux, `.dylib` on macOS, `.dll` on Windows). The first loaded bridge remains active for the process lifetime. Linux has been validated. The loader has POSIX and Windows branches; Windows cross compilation with Zig/MinGW succeeds; Windows runtime and macOS remain unvalidated.

Example native application, from `bindings/go`:

```sh
CUI_BRIDGE_LIBRARY="$PWD/../../build-go-bridge/libcui_bridge.so" \
CUI_TEST_FONT=/absolute/path/to/font.ttf \
go run ./examples/save_changes
```

The native example opens an SDL window and uses an existing TTF font. A finite native smoke check is available as `go run ./examples/native_smoke` inside Xvfb. For automated native checks on Linux, run it only inside an invisible Xvfb display; never point tests at the desktop display. For example: `xvfb-run -a env -u WAYLAND_DISPLAY CUI_TEST_FONT=/absolute/path/to/font.ttf CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/native_smoke`. The bridge reports unavailable SDL or font failures directly.

For SDL glyph coverage, `Options.FontFallbacks` takes up to 32 absolute UTF-8 TTF paths, or call `App.SetFontFallbacks(paths)` to replace or clear the chain. Font loading is transactional: a failed replacement leaves the previous chain active. Font fallbacks require the native SDL backend. The finite smoke example installs its primary font as a fallback to exercise this API.

```go
app, err := cui.New(cui.Options{})
if err != nil { panic(err) }
defer app.Close()
status := cui.Label("Ready").Key("status")
button := cui.Button("save").Text("Save").Size(cui.MD)
button.OnClick(func(cui.Event) { _ = status.SetText("Saved ✓") })
err = app.Mount(cui.Column("root").GapUnits(2).Children(status, button))
if err != nil { panic(err) }
_ = app.Step(1.0/60)
```

`New` locks its goroutine to an OS thread. Call all methods for that app, including `Close`, on that same goroutine; `Close` unlocks it. Until the core's process-global Clay context is synchronized, callers must also serialize calls across separate apps, even when they use different OS threads. Go callbacks execute during `Dispatch`, outside C. Callback panics propagate as ordinary Go panics. Keep a reference to every app and close it explicitly. Strings are copied into C and limited to valid UTF-8, no NUL, and 4096 bytes. Handles are app scoped and rejected after removal or disposal. Gap values are logical pixels; `GapUnits(2)` equals eight pixels. Button design sizes span `Inherit`, `XS` through `Size9XL`. `Checked`, `Value`, `Range`, `Open`, and `Selected` configure selection, disclosure, and numeric components; `QueueText` sends host text input to the focused Input or Textarea. Hover colors use the bridge's existing palette mapping, and ghost custom hover remains unsupported by the bridge.

Grouped components use `Item` children. RadioGroup, Tabs, Select, and ComboBox expose a selected Item; Tabs and Accordion Item children form their panels. `Event.Selected` is a copied full handle snapshot for group changes. `Alert` takes title and message. Collapsible and Accordion Item state is available through `App.Checked`.

Select uses a name and placeholder; ComboBox uses a label and editable query. Their options are `Item` children. `App.Text(combo)` reads the current query.

DropdownButton has separate action and menu areas. Use `App.DropdownPartBounds(button, false)` for the action and `true` for the menu. `OnClick` receives action presses; `OnChange` receives selected `Item` snapshots from menu choices.

Dialog, AlertDialog, Sheet, and Popover have `Open` state and `OnClose` callbacks. `Event.CloseResult` reports accepted, cancelled, or closed. Popover requires `Anchor` with a live node before opening. Dialog, Sheet, and Popover children render inside their surfaces; AlertDialog uses its title and description.

Tooltip and HoverCard follow their anchor on pointer hover. Both require `Anchor`; HoverCard may contain child content. `App.Open` reads their hover state, and `OnChange` receives open/close snapshots.

Visual builders map to native CUI renderers. `VisualSize(width, height)` applies to Avatar, EmptyView, GroupBox, Skeleton, and Spinner, within 0–4096 logical pixels. Avatar and Spinner currently use the width; their height remains renderer-defined. GroupBox accepts children; EmptyView takes a title and description.

List, Menu, and Stepper use `Item` children and selected Item snapshots. Stepper Items accept `StepStatus(0..3)` and a detail description. Pagination and Rating expose `Total` and `Value`; `NumericPartBounds` locates a numbered native interaction part for host input and tests.

DescriptionList uses `Item(label).Detail(value)` children. Scrollable accepts children, `VisualSize` for its viewport, and `ScrollExtent` for content dimensions. `QueueScroll` sends host wheel input; `App.Value(scroll)` returns the vertical offset, with each change captured in `OnChange`.

StatusBar, Toolbar, and TitleBar use `Item` children. StatusBar Item `StepStatus(0/1/2)` chooses left/center/right. Toolbar Items may contain controls; disabling the Toolbar suppresses their actions. TitleBar Items render text labels and do not expose host window controls.

Icon takes a catalog name such as `check` and optional `VisualSize`; an unknown name is reported at `Step`. Message renders one filled message body with an optional header and configurable width. A failed mount removes its partial native tree, so a corrected tree can be mounted on the same app.

Attachment renders native status and progress metadata from a title, description, `AttachmentStatus`, `Value(0..1)`, and optional visual width. This bridge surface does not expose file transfer, remove, or retry actions.

Image accepts decoded RGBA8 pixels with `RGBA(pixels, width, height, stride)` and optional display `VisualSize`. Go copies the input immediately; the bridge copies it again for rendering, so callers may reuse their byte slice. Dimensions must be 1–2048 pixels, stride at least four bytes per pixel, and the exact buffer at most 16 MiB. `SetRGBA` replaces pixels on a mounted image. File decoding remains the application's responsibility.

Tree accepts up to 64 flat `Item` children in preorder. `Item.Depth(0..63)` sets indentation and `Item.Checked(true)` starts an item expanded. `Tree.Selected(item)` chooses an Item; `App.Selected(tree)` and `App.Checked(item)` read live state. `Tree.OnChange` receives selection snapshots from pointer or keyboard input. `VisualSize` sets the tree viewport; disabling Tree suppresses interaction.

TimeField renders an editable 24-hour `HH:MM:SS` value. Configure it with `Time(Time{Hour, Minute, Second})`, update it with `SetTime`, and read it with `App.Time`. Each native edit emits `OnChange` with a copied eight-character `Event.Text` snapshot; disabling the field suppresses edits.

ColorPicker accepts packed `0xRRGGBBAA` with `ColorRGBA`, updates via `SetColorRGBA`, and reads via `App.ColorRGBA`. `App.ColorPartBounds(picker, 1/2/3)` locates the hue, color plane, or alpha part for host input. Native changes emit `OnChange` with the exact packed value in `Event.Value` and a copied `#RRGGBBAA` string in `Event.Text`.

Calendar and DatePicker use a validated `Date{Year, Month, Day}`. Set one with `Date`, update with `SetDate`, and query with `App.Date`. `App.CalendarDayBounds` locates a visible day for pointer input. DatePicker can be opened or closed with `Open` and sized by width with `VisualSize`. Native selections emit `OnChange` with numeric `YYYYMMDD` in `Event.Value` and copied `YYYY-MM-DD` in `Event.Text`. The current bridge uses English single-date mode.

OTPInput uses `Total(1..256)` for its target cell count (default six), `Text` or `SetText` for its current value, and `App.Text` to read live edits. Queued text input to a focused OTPInput emits `OnChange` with a copied text snapshot. Configure `Total` before `Text`; changing `Total` on a mounted input resets its value.

Form accepts up to 64 `Item` children. Each Item's text and detail become a field label and description, and its children render as the field controls. `Form.VisualSize(width, 0)` sets its width. Disabling either the Form or an Item suppresses descendant control input. The bridge does not supply form validation or nested Forms.

InputGroup combines an editable value with `Item` button addons. Set its placeholder with `Detail`, width with `VisualSize`, initial value with `Text`, and read edits with `App.Text` or copied `OnChange` snapshots. Each addon may use `StepStatus(0..3)` for inline start/end or block start/end and `OnClick` for its native action. Disabling the group suppresses editor and addon input.

Notification creates one retained in-app card from a title and message. `Open(true)` presents it; `App.Open` reads its state. `OnClick` receives a card press, and `OnClose` receives its completed close transition. The bridge does not deliver OS notifications or provide action buttons or automatic hiding.

FocusTrap contains arbitrary child nodes. `Open(true)` activates a native modal focus scope that blocks pointer interaction outside its subtree; `Open(false)` restores ordinary input. `App.Open` reads its live state. It does not create a visual dialog surface by itself.

The composed workspace example lives at `bindings/go/examples/workspace`. From `bindings/go`, run `CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --headless` to validate native hit targets, Account/Preferences tab navigation, name editing, notifications checkbox, an older birthday, save dialog, and status update without opening a window. To use the SDL UI, run `CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --font /absolute/path/to/font.ttf`. Automated SDL runs on Linux must use invisible Xvfb, for example `xvfb-run -a env -u WAYLAND_DISPLAY CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --font /absolute/path/to/font.ttf`; native mode remains open until the user closes its window.
