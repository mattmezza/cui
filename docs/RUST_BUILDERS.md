# Rust CUI builders

The `cui-builders` crate is a native Rust authoring layer over `libcui_bridge`. It currently exposes the bridge's row, column, label, Button, Checkbox, Radio, Switch, Toggle, Slider, Progress, Badge, Input, Textarea, NumberInput, Collapsible, Alert, RadioGroup, Tabs, Accordion, Item, Select, ComboBox, and DropdownButton nodes. This is the currently delivered bridge scope, not all 78 CUI manifest components. Its node handles are app-scoped; calls reject handles from another app, and the C bridge rejects removed nodes. The `App` contains an `Rc` marker and cannot be sent or shared across threads. C strings are checked for interior NUL and the bridge's 4096-byte limit before FFI. Rust callbacks execute during `dispatch()` after `step()` returns from C; no Rust closure is passed into native frames.

Build from the repository root with CMake, Ninja, Rust 1.99/Cargo 1.99 as tested here. The crate uses no external dependencies:

```sh
cmake -S bindings/bridge -B build-bridge -G Ninja
cmake --build build-bridge --target cui_bridge test_bridge_core
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" cargo test --manifest-path bindings/rust/Cargo.toml -- --test-threads=1
```

The crate tests serialize native calls with a process mutex because the current CUI core has shared process state. `build.rs` adds `CUI_BRIDGE_LIB_DIR` to the native linker path and Linux runtime path; use an absolute path. On other platforms, place the shared library on the platform dynamic library search path. The crate checks the bridge ABI version and `cui_bridge_options` size at creation. The C library itself needs its pinned SDL3 dependencies as configured by CMake. Native SDL rendering needs a TTF file. Run the example only on a display you intend to use:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf cargo run --manifest-path bindings/rust/Cargo.toml --example save_changes
```

The example authors a column, label, row and Button in Rust. A click changes shared Rust state; the label is updated after event dispatch. For a finite SDL smoke under an invisible Xvfb, set `CUI_TEST_FONT` and run `bindings/rust/scripts/test_sdl_smoke.sh`; it fails closed if Xvfb is unavailable. For automated native CTest checks, use `python scripts/test_local.py --build-dir build` from a configured top-level build. It provides an invisible Xvfb on Linux; never use `DISPLAY=:0` or the maintainer's Wayland session. GPU hardware checks remain deferred under Xvfb.

`App::headless` is suitable for deterministic bounds and host input fixtures. Queue pointer or keyboard events, call `step`, then `dispatch` (or `frame` for both). `on_change_event` receives each frame's copied checked, numeric, UTF-8 text, and selected Item snapshot. `queue_text` feeds the focused editable node; `editable_text` reads its current text. Button callbacks receive no `App` borrow; update an `Rc<Cell<_>>` or queue a message and apply UI mutations after dispatch. `Size` covers inherit, XS through 9XL. `gap_px` is logical pixels; `gap_units` multiplies by four. Grouped `Item` handles in RadioGroup, Tabs, Select, and ComboBox can be selected with `select`; `selected` returns `None` before a choice. `detail_text` sets Alert body text, Select placeholder, or DropdownButton menu label; `open` controls disclosure state. `dropdown_part_bounds` locates split action and menu targets for host input. All fallible setters return the bridge status as `Error(code)`.
