# Rust CUI builders

The `cui-builders` crate is a native Rust authoring layer over `libcui_bridge`. It currently exposes the bridge's row, column, label, Button, Checkbox, Radio, Switch, Toggle, Slider, Progress, Badge, Input, Textarea, NumberInput, Collapsible, Alert, RadioGroup, Tabs, Accordion, Item, Select, ComboBox, DropdownButton, Dialog, AlertDialog, Sheet, Popover, Tooltip, HoverCard, Avatar, Bubble, EmptyView, GroupBox, Kbd, Skeleton, Spinner, Tag, Marker, Shimmer, List, Menu, Pagination, Rating, Stepper, DescriptionList, Scrollable, StatusBar, Toolbar, TitleBar, Icon, Message, Attachment, Image, Tree, TimeField, ColorPicker, Calendar, DatePicker, OtpInput, Form, InputGroup, Notification, and FocusTrap nodes. This is the currently delivered bridge scope, not all 78 CUI manifest components. Its node handles are app-scoped; calls reject handles from another app, and the C bridge rejects removed nodes. The `App` contains an `Rc` marker and cannot be sent or shared across threads. C strings are checked for interior NUL and the bridge's 4096-byte limit before FFI. Rust callbacks execute during `dispatch()` after `step()` returns from C; no Rust closure is passed into native frames.

Build from the repository root with CMake, Ninja, Rust 1.99/Cargo 1.99 as tested here. The crate uses no external dependencies:

```sh
cmake -S bindings/bridge -B build-bridge -G Ninja
cmake --build build-bridge --target cui_bridge test_bridge_core
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" cargo test --manifest-path bindings/rust/Cargo.toml -- --test-threads=1
```

Serialize all calls across all apps, including apps created on separate threads: the current core uses a process-global Clay context. The crate tests enforce this contract with a process mutex. `build.rs` adds `CUI_BRIDGE_LIB_DIR` to the native linker path and Linux runtime path; use an absolute path. On other platforms, place the shared library on the platform dynamic library search path. The crate checks the bridge ABI version and `cui_bridge_options` size at creation. The C library itself needs its pinned SDL3 dependencies as configured by CMake. Native SDL rendering needs a TTF file. Run the example only on a display you intend to use:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf cargo run --manifest-path bindings/rust/Cargo.toml --example save_changes
```

The example authors a column, label, row and Button in Rust. A click changes shared Rust state; the label is updated after event dispatch. For a finite SDL smoke under an invisible Xvfb, set `CUI_TEST_FONT` and run `bindings/rust/scripts/test_sdl_smoke.sh`; it fails closed if Xvfb is unavailable. For automated native CTest checks, use `python scripts/test_local.py --build-dir build` from a configured top-level build. It provides an invisible Xvfb on Linux; never use `DISPLAY=:0` or the maintainer's Wayland session. GPU hardware checks remain deferred under Xvfb.

`App::headless` is suitable for deterministic bounds and host input fixtures. Queue pointer or keyboard events, call `step`, then `dispatch` (or `frame` for both). `on_change_event` receives each frame's copied checked, numeric, UTF-8 text, and selected Item snapshot. `queue_text` feeds the focused editable node; `editable_text` reads its current text. Button callbacks receive no `App` borrow; update an `Rc<Cell<_>>` or queue a message and apply UI mutations after dispatch. `Size` covers inherit, XS through 9XL. `gap_px` is logical pixels; `gap_units` multiplies by four. Grouped `Item` handles in RadioGroup, Tabs, Select, and ComboBox can be selected with `select`; `selected` returns `None` before a choice. `detail_text` sets Alert body text, Select placeholder, or DropdownButton menu label; `open` controls disclosure state. `dropdown_part_bounds` locates split action and menu targets for host input. Surfaces expose `open`/`is_open`, `on_close_event`, and typed `surface_result()`; Popover, Tooltip, and HoverCard require a live anchor via `anchor`. Hover transitions arrive through `on_change_event`. `visual_size` sets explicit dimensions for Avatar, EmptyView, GroupBox, Skeleton, and Spinner. `total`, `step_status`, and `numeric_part_bounds` expose native pagination, rating, and step interactions. `scroll_extent` sets Scrollable content dimensions and `scroll` queues a host wheel fixture; offset changes arrive through `on_change_event`. StatusBar, Toolbar, and TitleBar use Item children; StatusBar Item `step_status` chooses left (0), center (1), or right (2). Icon `text` is a catalog name; an unknown name fails the frame. Message `text` is body and `detail_text` is header. Attachment uses `text` title, `detail_text` description, `value` progress 0..1, and typed `attachment_status`; this bridge component is a metadata chip. `image_rgba` copies RGBA8 pixels into the bridge (maximum 16 MiB, 2048 pixels per dimension), so the caller can release its buffer after the call. Tree uses flat preorder Item children; Item `step_status` is depth 0..63, `checked` controls expansion, and `select` chooses the active Item. TimeField uses a portable `Time { hour, minute, second }` in 24-hour format; edits deliver HH:MM:SS text snapshots. ColorPicker uses packed `0xRRGGBBAA` values; `color_part_bounds` exposes hue, plane, and alpha hit targets (parts 1–3), and change events carry copied `#RRGGBBAA` text. Calendar and DatePicker use a portable `Date { year, month, day }`; `calendar_day_bounds` finds a rendered day. The current native mode is English single-date selection. OtpInput uses `total` for 1–256 cells, `text` for its current value, and queued host text for one cell at a time; changes carry copied text snapshots. Form uses Item fields; each Item may contain native controls. A disabled Form suppresses its controls. The bridge currently has no built-in Form validation or nested Form support. InputGroup owns editable text and Item button addons; Item `step_status` selects inline start/end or block start/end positions (0–3). Notification is a retained in-app card: `text` title, `detail_text` message, `open` visibility. Its close callback fires when the closing animation finishes; there is no OS delivery. FocusTrap accepts child nodes and uses `open` to enable a native modal focus scope that blocks outside pointer actions. All fallible setters return the bridge status as `Error(code)`.

## Composed workspace example

`bindings/rust/examples/workspace.rs` authors a navigable Account/Preferences workspace entirely in Rust. Account has an editable name and a DatePicker initialized to an older birthday; Preferences has a notifications Checkbox. The Save Button callback records shared Rust state; after dispatch, the loop changes the status label and opens a Dialog. `--headless` drives native bounds and pointer input, edits the name, switches tabs, toggles the Checkbox, and verifies the Dialog without opening a window:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" cargo run --manifest-path bindings/rust/Cargo.toml --example workspace -- --headless
```

For a desktop SDL session, set an explicit font and omit `--headless`:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf cargo run --manifest-path bindings/rust/Cargo.toml --example workspace
```

A finite SDL smoke must use an invisible display on Linux. From the repository root with `xvfb-run` available:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf CUI_RUST_SMOKE_FRAMES=3 env -u DISPLAY -u WAYLAND_DISPLAY xvfb-run -a env CUI_TEST_VIRTUAL_DISPLAY=1 cargo run --manifest-path bindings/rust/Cargo.toml --example workspace
```

The SDL workspace also accepts `CUI_FONT_FALLBACK=/path/to/another.ttf`, which calls `App::font_fallbacks` with an explicit fallback font path. The smoke path rejects a missing virtual-display marker. It exercises SDL construction and rendering for three frames; the deterministic `--headless` path verifies interaction.
