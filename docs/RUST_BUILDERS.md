# Rust CUI builders

The `cui-builders` crate is a native Rust authoring layer over `libcui_bridge`. Its `Kind` enum follows the public bridge kind list in [`cui_bridge.h`](../include/cui/cui_bridge.h), including the native components delivered through Settings. This is the current bridge scope, not full parity with all 78 CUI manifest entries. Its node handles are app-scoped; calls reject handles from another app, and the C bridge rejects removed nodes. The `App` contains an `Rc` marker and cannot be sent or shared across threads. C strings are checked for interior NUL and the bridge's 4096-byte limit before FFI. Rust callbacks execute during `dispatch()` after `step()` returns from C; no Rust closure is passed into native frames.

Build from the repository root with CMake, Ninja, Rust 1.85.1 or newer (validated with 1.85.1 and 1.99). The crate uses no external dependencies:

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

`App::headless` is suitable for deterministic bounds and host input fixtures. Queue pointer or keyboard events, call `step`, then `dispatch` (or `frame` for both). `on_change_event` receives each frame's copied checked, numeric, UTF-8 text, and selected Item snapshot. `queue_text` feeds the focused editable node; `editable_text` reads its current text. Button callbacks receive no `App` borrow; update an `Rc<Cell<_>>` or queue a message and apply UI mutations after dispatch. `Size` covers inherit, XS through 9XL. `gap_px` is logical pixels; `gap_units` multiplies by four.

Grouped `Item` handles in RadioGroup, Tabs, Select, and ComboBox can be selected with `select`; `selected` returns `None` before a choice. `detail_text` sets Alert body text, Select placeholder, or DropdownButton menu label; `open` controls disclosure state. `dropdown_part_bounds` locates split action and menu targets for host input.

Surfaces expose `open`/`is_open`, `on_close_event`, and typed `surface_result()`; Popover, Tooltip, and HoverCard require a live anchor via `anchor`. Hover transitions arrive through `on_change_event`.

`visual_size` sets explicit dimensions for Avatar, EmptyView, GroupBox, Skeleton, and Spinner. `total`, `step_status`, and `numeric_part_bounds` expose native pagination, rating, and step interactions. `scroll_extent` sets Scrollable content dimensions and `scroll` queues a host wheel fixture; offset changes arrive through `on_change_event`.

StatusBar, Toolbar, and TitleBar use Item children; StatusBar Item `step_status` chooses left (0), center (1), or right (2). Icon `text` is a catalog name; an unknown name fails the frame. Message `text` is body and `detail_text` is header.

Attachment uses `text` title, `detail_text` description, `value` progress 0..1, and typed `attachment_status`; this bridge component is a metadata chip. `image_rgba` copies RGBA8 pixels into the bridge (maximum 16 MiB, 2048 pixels per dimension), so the caller can release its buffer after the call.

Tree uses flat preorder Item children; Item `step_status` is depth 0..63, `checked` controls expansion, and `select` chooses the active Item. TimeField uses a portable `Time { hour, minute, second }` in 24-hour format; edits deliver HH:MM:SS text snapshots.

ColorPicker uses packed `0xRRGGBBAA` values; `color_part_bounds` exposes hue, plane, and alpha hit targets (parts 1–3), and change events carry copied `#RRGGBBAA` text. Calendar and DatePicker use a portable `Date { year, month, day }`; `calendar_day_bounds` finds a rendered day. The current native mode is English single-date selection. OtpInput uses `total` for 1–256 cells, `text` for its current value, and queued host text for one cell at a time; changes carry copied text snapshots.

Form uses Item fields; each Item may contain native controls. A disabled Form suppresses its controls. The bridge currently has no built-in Form validation or nested Form support.

InputGroup owns editable text and Item button addons; Item `step_status` selects inline start/end or block start/end positions (0–3). Notification is a retained in-app card: `text` title, `detail_text` message, `open` visibility. Its close callback fires when the closing animation finishes; there is no OS delivery. FocusTrap accepts child nodes and uses `open` to enable a native modal focus scope that blocks outside pointer actions.

MessageScroller uses Item messages (`text` body, `detail_text` sender, `step_status` incoming 0/outgoing 1) and `value` for its scroll offset.

Carousel accepts Item slides and uses `select`/`selected`; native wheel, drag, and keyboard navigation emit selected Item snapshots.

Resizable accepts Item panels with pixel widths in `value`; `resizable_handle_bounds` locates a draggable divider and change events carry new sizes.

TextView owns Markdown, HTML, or plain source in `text`; `text_view_format` selects its parser. `text_view_select` uses rendered UTF-8 byte offsets, and `text_view_selection` copies plain selected text. Native pointer selection delivers a copied text snapshot through `on_change_event`.

Editor owns a multiline UTF-8 buffer; `text` supplies initial source, `detail_text` sets the language name, `editable_text` reads the current document, and queued host text generates copied change snapshots. The bridge currently supports up to 4096 bytes and 512 lines, without completion, syntax callbacks, search, or clipboard APIs.

Chart accepts up to 64 caller-supplied label/value pairs through `chart_data`; the bridge copies them. `chart_kind` selects line, area, bar, or pie rendering, and `chart_datum` reads one retained pair. This native chart is a basic raster/hover view, with no streaming data API.

Table is compositional: Table children are Item rows and each row contains Item text cells. The first row is the header. This bridge Table has no sorting, selection, or virtualization.

DataTable accepts copied UTF-8 headers and row-major cells (up to 16 columns and 32 rows) through `data_table`; its bounds helpers locate cells and headers for host input. `data_table_state` reports selected one-based row/column and requested sort column/direction. Sort is a request: re-supply rows in the desired order. Change callbacks expose the raw native `checked_raw` field as well as its Boolean convenience form; DataTable CHANGE callbacks include a copied JSON text snapshot with reason, row, column, sortColumn, and sortDirection; `data_table_state` reads current state.

Plot accepts up to 256 copied finite `PlotPoint { x, y }` values through `plot_points` and supports readback via `plot_point`. The current native Plot draws themed dots without axes, paths, or hit-testing.

Command accepts up to 64 Item actions; each Item uses `text` as label and `detail_text` as one filter keyword. Command owns the query through `text`/`editable_text`; `command_item_bounds` locates a filtered action for pointer input. Native selection produces CHANGE events and confirmation produces an Item CLICK callback. There are no grouped or custom result rows.

VirtualList accepts up to 64 keyed Item rows, renders a fixed 32-pixel vertical viewport with overscan, and exposes its scroll offset through `value`/`current_value`. `virtual_list_item_bounds` succeeds only for currently rendered rows. Native wheel and key scrolling emit offset CHANGE events; there is no unbounded data source callback.

Sidebar accepts Item navigation actions; `open` controls disclosure, `selected` reads the active Item, and native activation emits a selected CHANGE plus Item CLICK. DataTable also supports typed row, column, or cell `data_table_selection_mode`; cell selection snapshots preserve columns beyond the first in copied event JSON.

RootView is a sole root node that composes regular child widgets and accepts `root_padding` from 0 to 256 logical pixels. `App::theme_presets` lists native preset names; `set_theme_preset` changes the theme for one app and `theme_preset` returns `None` before an explicit selection.

SDL apps also expose `clipboard_set` and `clipboard_get` on the owner thread outside a frame. Clipboard text is UTF-8 and limited to 64 KiB; headless apps return backend unavailable.

Speech is a host-reported lifecycle view: `speech_start`, partial or final `speech_report`, `speech_finish`, and copied `speech_transcript`. It does not capture microphone audio or run recognition; CHANGE callbacks copy lifecycle and transcript snapshots.

Questionnaire composes Item questions and nested Item choices. `questionnaire_question` configures required and multiple selection before the first frame, which freezes the schema; `questionnaire_next` enforces required answers, `questionnaire_previous` navigates back, and state/choice getters read current progress. Its CHANGE callbacks contain copied JSON snapshots of choices, navigation, completion, and repeated `questionnaire_submit` actions.

Dock composes Item panels with optional child content; `dock_activate`, `dock_move_split`, `dock_active`, `dock_panel_bounds`, and `dock_serialize` expose the native panel model. Dock panel schema freezes after the first frame. `DockAxis` selects horizontal or vertical splits; serialized layout is copied UTF-8.

Settings composes Item pages, nested Item groups, and Switch/Checkbox/Input/NumberInput fields. `settings_query` filters pages (up to 256 bytes), `settings_page`/`settings_set_page` control navigation, `settings_field_bounds` locates visible fields, and `settings_reset_page` restores first-frame defaults with copied field events. The page/group schema freezes after the first frame; Input and NumberInput titles use `detail_text`. All fallible setters return the bridge status as `Error(code)`.

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

For a Rust SDL clipboard round trip under an invisible Linux display, use an explicit font and the virtual-display marker:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf env -u DISPLAY -u WAYLAND_DISPLAY xvfb-run -a env CUI_TEST_VIRTUAL_DISPLAY=1 cargo run --manifest-path bindings/rust/Cargo.toml --example clipboard_smoke
```
