# Go builders for the CUI bridge

`bindings/go` is a cgo Go module for the portable CUI bridge ABI v1. It mounts row, column, label, Button, Checkbox, Radio, Switch, Toggle, Slider, Progress, Badge, Input, Textarea, NumberInput, Collapsible, Alert, RadioGroup, Tabs, Accordion, Item, Select, ComboBox, and DropdownButton nodes. Builders create a detached tree; `App.Mount` validates it and creates actual CUI nodes. `Step` submits the frame, `QueuePointer` and `QueueKey` send synthetic input, and `Dispatch` invokes Go click and change callbacks after the native frame. Change events carry the state and copied text captured when each event was queued. `App.Run` provides a native event loop. The API covers the listed bridge components, not the 78-component manifest.

Requires Go 1.23 or newer, a C11 compiler, CMake, Ninja, `CGO_ENABLED=1`, and a built CUI bridge shared library. On Linux, build and test from the repository root:

```sh
cmake -S bindings/bridge -B build-go-bridge -G Ninja -DCMAKE_C_FLAGS='-Wall -Wextra -Werror'
cmake --build build-go-bridge --target cui_bridge test_bridge_core test_bridge_components
python scripts/test_local.py --build-dir build-go-bridge -- -R '^bridge_'
cd bindings/go
CUI_BRIDGE_LIBRARY="$PWD/../../build-go-bridge/libcui_bridge.so" go test ./...
```

The Go package loads `CUI_BRIDGE_LIBRARY` at runtime with `dlopen` on POSIX or `LoadLibraryA` on Windows, verifies the ABI version and options size, and calls the C bridge through a small cgo shim. `go test` itself uses the headless backend and opens no window. The library path must be absolute (`.so` on Linux, `.dylib` on macOS, `.dll` on Windows). The first loaded bridge remains active for the process lifetime. Linux has been validated. The loader has POSIX and Windows branches; macOS and Windows builds and native behavior remain unvalidated.

Example native application, from `bindings/go`:

```sh
CUI_BRIDGE_LIBRARY="$PWD/../../build-go-bridge/libcui_bridge.so" \
CUI_TEST_FONT=/absolute/path/to/font.ttf \
go run ./examples/save_changes
```

The native example opens an SDL window and uses an existing TTF font. A finite native smoke check is available as `go run ./examples/native_smoke` inside Xvfb. For automated native checks on Linux, run it only inside an invisible Xvfb display; never point tests at the desktop display. For example: `xvfb-run -a env -u WAYLAND_DISPLAY CUI_TEST_FONT=/absolute/path/to/font.ttf CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/native_smoke`. The bridge reports unavailable SDL or font failures directly.

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

`New` locks its goroutine to an OS thread. Call all methods for that app, including `Close`, on that same goroutine; `Close` unlocks it. Go callbacks execute during `Dispatch`, outside C. Callback panics propagate as ordinary Go panics. Keep a reference to every app and close it explicitly. Strings are copied into C and limited to valid UTF-8, no NUL, and 4096 bytes. Handles are app scoped and rejected after removal or disposal. Gap values are logical pixels; `GapUnits(2)` equals eight pixels. Button design sizes span `Inherit`, `XS` through `Size9XL`. `Checked`, `Value`, `Range`, `Open`, and `Selected` configure selection, disclosure, and numeric components; `QueueText` sends host text input to the focused Input or Textarea. Hover colors use the bridge's existing palette mapping, and ghost custom hover remains unsupported by the bridge.

Grouped components use `Item` children. RadioGroup, Tabs, Select, and ComboBox expose a selected Item; Tabs and Accordion Item children form their panels. `Event.Selected` is a copied full handle snapshot for group changes. `Alert` takes title and message. Collapsible and Accordion Item state is available through `App.Checked`.

Select uses a name and placeholder; ComboBox uses a label and editable query. Their options are `Item` children. `App.Text(combo)` reads the current query.

DropdownButton has separate action and menu areas. Use `App.DropdownPartBounds(button, false)` for the action and `true` for the menu. `OnClick` receives action presses; `OnChange` receives selected `Item` snapshots from menu choices.
