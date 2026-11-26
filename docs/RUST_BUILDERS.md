# Rust CUI builders

For installable local wheel, npm, Go module and Rust crate artifacts, see the
[packaging guide](PACKAGING.md). Linux alpha packages are tested outside the
checkout; registry publication and other platform packages remain pending.

The `cui-builders` crate is a native Rust authoring layer over `libcui_bridge`. Its `Kind` enum follows the public bridge kind list in [`cui_bridge.h`](../include/cui/cui_bridge.h), including the legacy native components delivered through Settings. The independent kind81 controlled route uses `App::slider_model`, not a new generic Kind variant. This is the current bridge scope, not full parity with all 78 CUI manifest entries. Its node handles are app-scoped; calls reject handles from another app, and the C bridge rejects removed nodes. The `App` contains an `Rc` marker and cannot be sent or shared across threads. C strings are checked for interior NUL and the bridge's 4096-byte limit before FFI. Rust callbacks execute during `dispatch()` after `step()` returns from C; no Rust closure is passed into native frames.

Build from the repository root with CMake, Ninja, Rust 1.85.1 or newer (validated with 1.85.1 and 1.99). The crate uses no external dependencies:

```sh
cmake -S bindings/bridge -B build-bridge -G Ninja
cmake --build build-bridge --target cui_bridge test_bridge_core
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" cargo test --manifest-path bindings/rust/Cargo.toml -- --test-threads=1
```

Matching bundled-source builds pass Cargo's positive `NUM_JOBS` count to the
child CMake build. Use `CARGO_BUILD_JOBS=1` or Cargo `-j1` to keep both phases
serial; missing/invalid standalone NUM_JOBS falls back to1. Setting only
CMAKE_BUILD_PARALLEL_LEVEL does not replace this explicit Cargo-selected count.
`CUI_BRIDGE_LIB_DIR` selects an existing dynamic library and skips that CMake
source build. The offline build-script fixture uses fake CMake/pkg-config and
never builds an SDK or opens a window:

```sh
python bindings/rust/tests/test_build_parallelism.py
```

Serialize all calls across all apps, including apps created on separate threads: the current core uses a process-global Clay context. The crate tests enforce this contract with a process mutex. `build.rs` adds `CUI_BRIDGE_LIB_DIR` to the native linker path and Linux runtime path; use an absolute path. On other platforms, place the shared library on the platform dynamic library search path. The crate checks the bridge ABI version and `cui_bridge_options` size at creation. The C library itself needs its pinned SDL3 dependencies as configured by CMake. Native SDL rendering needs a TTF file. Run the example only on a display you intend to use:

```sh
CUI_BRIDGE_LIB_DIR="$PWD/build-bridge" CUI_TEST_FONT=/path/to/font.ttf cargo run --manifest-path bindings/rust/Cargo.toml --example save_changes
```

The example authors a column, label, row and Button in Rust. A click changes shared Rust state; the label is updated after event dispatch. For a finite SDL smoke under an invisible Xvfb, set `CUI_TEST_FONT` and run `bindings/rust/scripts/test_sdl_smoke.sh`; it fails closed if Xvfb is unavailable. For automated native CTest checks, use `python scripts/test_local.py --build-dir build` from a configured top-level build. It provides an invisible Xvfb on Linux; never use `DISPLAY=:0` or the maintainer's Wayland session. GPU hardware checks remain deferred under Xvfb.

`App::headless` is suitable for deterministic bounds and host input fixtures. Queue pointer or keyboard events, call `step`, then `dispatch` (or `frame` for both). `on_change_event` receives each frame's copied checked, numeric, UTF-8 text, and selected Item snapshot. `queue_text` feeds the focused editable node; `editable_text` reads its current text. Button callbacks receive no `App` borrow; update an `Rc<Cell<_>>` or queue a message and apply UI mutations after dispatch. `Size` covers inherit, XS through 9XL. `gap_px` is logical pixels; `gap_units` multiplies by four.

Grouped `Item` handles in RadioGroup, Tabs, Select, and ComboBox can be selected with `select`; `selected` returns `None` before a choice. `detail_text` sets Alert body text, Select placeholder, or DropdownButton menu label; `open` controls disclosure state. `dropdown_part_bounds` locates split action and menu targets for host input.

The [Rust workspace preferences example](BRIDGE_COMPONENTS.md#style-disclosure-from-language-builders) shows optional Accordion and Collapsible presentation and how to take changes into the next frame.

Surfaces expose `open`/`is_open`, `on_close_event`, and typed `surface_result()`; Popover, Tooltip, and HoverCard require a live anchor via `anchor`. Hover transitions arrive through `on_change_event`.

`visual_size` sets explicit dimensions for Avatar, EmptyView, GroupBox, Skeleton, and Spinner. `total`, `step_status`, and `numeric_part_bounds` expose native pagination, rating, and step interactions. `scroll_extent` sets Scrollable content dimensions and `scroll` queues a host wheel fixture; offset changes arrive through `on_change_event`.

StatusBar, Toolbar, and TitleBar use Item children; StatusBar Item `step_status` chooses left (0), center (1), or right (2). Icon `text` is a catalog name; an unknown name fails the frame. Message `text` is body and `detail_text` is header.
Rust can prepare and copy custom artwork, or attach it to retained Icons and
configured Badge containers. [SVG resources](RUST_ICON_RESOURCES.md) ·
[Retained inbox example](RUST_ICON_BADGE.md).

Attachment uses `text` for the title, `detail_text` for the description, `value` for fractional progress, and `attachment_status` for lifecycle state. Use the scoped RGBA8 preview setter or mount child widgets with `App::create_attachment_slot`. Removing a slot or card releases callback captures owned by its removed children. [Attachment examples](BRIDGE_COMPONENTS.md#compose-a-retained-attachment-in-a-language-builder)

Tree uses flat preorder Item children; Item `step_status` is depth 0..63, `checked` controls expansion, and `select` chooses the active Item. TimeField uses a portable `Time { hour, minute, second }` in 24-hour format; edits deliver HH:MM:SS text snapshots.

ColorPicker uses packed `0xRRGGBBAA` values; `color_part_bounds` exposes hue, plane, and alpha hit targets (parts 1–3), and change events carry copied `#RRGGBBAA` text. Calendar and DatePicker use a portable `Date { year, month, day }`; `calendar_day_bounds` finds a rendered day. The current native mode is English single-date selection. OtpInput uses `total` for 1–256 cells, `text` for its current value, and queued host text for one cell at a time; changes carry copied text snapshots.

Form uses Item fields; each Item may contain native controls. A disabled Form suppresses its controls. The bridge currently has no built-in Form validation or nested Form support.

InputGroup owns editable text and Item button addons; Item `step_status` selects inline start/end or block start/end positions (0–3). Notification is a retained in-app card: `text` title, `detail_text` message, `open` visibility. Its close callback fires when the closing animation finishes; there is no OS delivery. FocusTrap accepts child nodes and uses `open` to enable a native modal focus scope that blocks outside pointer actions.

MessageScroller uses Item messages (`text` body, `detail_text` sender, `step_status` incoming 0/outgoing 1) and `value` for its scroll offset.

Carousel accepts Item slides and uses `select`/`selected`; native wheel, drag, and keyboard navigation emit selected Item snapshots.

Use `app.supports_carousel_basis()` before setting a fraction. For example,
`app.build_child(carousel, Kind::Item, "activity")?.text("Activity")?.carousel_basis(0.5)?.finish()`
creates a slide that takes half of the viewport. Use
`app.carousel_basis(item, 0.)?` to restore the full primary extent. After
checking `app.supports_carousel_axis()`, call `carousel_axis(CarouselAxis::Vertical)?`
on the Carousel builder or use `app.carousel_axis(carousel, CarouselAxis::Vertical)?`.
Vertical fractions use the visual height. See the
[retained Carousel guide](CAROUSEL.md#retained-slides). When
`app.supports_carousel_looping()` reports support, use `carousel_looping(true)?`
on the builder or `app.carousel_looping(carousel, true)?` to enable wrapping.

Resizable accepts Item panels with pixel widths in `value`; `resizable_handle_bounds` locates a draggable divider and change events carry new sizes.

TextView owns Markdown, HTML, or plain source in `text`; `text_view_format` selects its parser. `text_view_select` uses rendered UTF-8 byte offsets, and `text_view_selection` copies plain selected text. Native pointer selection delivers a copied text snapshot through `on_change_event`.

Editor owns a multiline UTF-8 buffer; `text` supplies initial source, `detail_text` sets the language name, `editable_text` reads the current document, and queued host text generates copied change snapshots. The bridge currently supports up to 4096 bytes and 512 lines, without completion, syntax callbacks, search, or clipboard APIs.

Chart accepts up to 64 caller-supplied label/value pairs through `chart_data`; the bridge copies them. `chart_kind` selects line, area, bar, or pie rendering, and `chart_datum` reads one retained pair. This native chart is a basic raster/hover view, with no streaming data API.

Table is compositional: Table children are Item rows and each row contains Item text cells. The first row is the header. This bridge Table has no sorting, selection, or virtualization.

DataTable copies UTF-8 headers and row-major cells through `data_table`. Its bounds helpers locate cells and headers for host input. `data_table_state` reports the selected row key, one-based column, and requested sort column and direction. The legacy setter assigns positional keys starting at one. To apply a sort request, supply rows in the desired order. Change callbacks expose a copied JSON snapshot containing `reason`, `row`, `column`, `sortColumn`, and `sortDirection`. Use the [keyed DataTable example](#keep-the-visible-datatable-row-during-a-rust-update) for live replacements.

Plot accepts up to 256 copied finite `PlotPoint { x, y }` values through `plot_points` and supports readback via `plot_point`. The current native Plot draws themed dots without axes, paths, or hit-testing.

Command accepts up to 64 Item actions; each Item uses `text` as label and `detail_text` as one filter keyword. Command owns the query through `text`/`editable_text`; `command_item_bounds` locates a filtered action for pointer input. Native selection produces CHANGE events and confirmation produces an Item CLICK callback. There are no grouped or custom result rows.

VirtualList accepts up to 64 keyed Item rows of 32 pixels by default, renders a vertical viewport with overscan, and exposes its scroll offset through `value`/`current_value`. `virtual_list_item_bounds` succeeds only for currently rendered rows. Native wheel and key scrolling emit offset CHANGE events. Enable
`preserve_virtual_list_anchor` and send `VirtualListRow` values with
`set_virtual_list_rows` to retain the first visible Item during edits.
[Run the live inbox](VIRTUAL_LIST.md#live-updates-from-the-language-builders).

Sidebar accepts Item navigation actions; `open` controls disclosure, `selected` reads the active Item, and native activation emits a selected CHANGE plus Item CLICK. DataTable also supports typed row, column, or cell `data_table_selection_mode`; cell selection snapshots preserve columns beyond the first in copied event JSON.

RootView is a sole root node that composes regular child widgets and accepts `root_padding` from 0 to 256 logical pixels. `App::theme_presets` lists native preset names; `set_theme_preset` changes the theme for one app and `theme_preset` returns `None` before an explicit selection.

SDL apps also expose `clipboard_set` and `clipboard_get` on the owner thread outside a frame. Clipboard text is UTF-8 and limited to 64 KiB; headless apps return backend unavailable.

Speech is a host-reported lifecycle view: `speech_start`, partial or final `speech_report`, `speech_finish`, and copied `speech_transcript`. It does not capture microphone audio or run recognition; CHANGE callbacks copy lifecycle and transcript snapshots.

Questionnaire composes Item questions and nested Item choices. `questionnaire_question` configures required and multiple selection before the first frame, which freezes the schema; `questionnaire_next` enforces required answers, `questionnaire_previous` navigates back, and state/choice getters read current progress. Its CHANGE callbacks contain copied JSON snapshots of choices, navigation, completion, and repeated `questionnaire_submit` actions.

Dock composes Item panels with optional child content; `dock_activate`, `dock_move_split`, `dock_active`, `dock_panel_bounds`, and `dock_serialize` expose the native panel model. Dock panel schema freezes after the first frame. `DockAxis` selects horizontal or vertical splits; serialized layout is copied UTF-8.

Settings composes Item pages, nested Item groups, and Switch/Checkbox/Input/NumberInput fields. `settings_query` filters pages (up to 256 bytes), `settings_page`/`settings_set_page` control navigation, `settings_field_bounds` locates visible fields, and `settings_reset_page` restores first-frame defaults with copied field events. The page/group schema freezes after the first frame; Input and NumberInput titles use `detail_text`. All fallible setters return the bridge status as `Error(code)`.

## Animate Radio selection from Rust

Use `app.supports_radio_motion()` to read the provider capability. Pass that
flag to `Builder::animate_selection(enabled)` on a Radio or RadioGroup child.
On a mounted node, use `app.animate_selection(node, enabled)` to change the
setting. A RadioGroup applies it across its options. Pass false to preserve
immediate presentation, including when loading an earlier provider. An
explicit opt-in with an earlier provider returns the backend-unavailable
error.

`app.selected(group)` reflects the current choice after `step`. The
`on_change_event` handler receives the selected Item during `dispatch` or
the native runner. Standalone Radio reports a checked boolean through the
same callback API. The delivery form applies the callback's selected Item
on the UI thread in both its interactive and headless modes.
[Complete delivery example](../bindings/rust/examples/radio_delivery.rs)

Run from the repository root with Rust and a C11 toolchain. Native mode also
uses SDL3, FreeType and HarfBuzz.

```sh
cargo run --locked --manifest-path bindings/rust/Cargo.toml --example radio_delivery
cargo run --locked --manifest-path bindings/rust/Cargo.toml --features native \
  --example radio_delivery -- --font third_party/inter/Inter-Regular.otf
```

## Compose Radio options from Rust

Use `Builder::decorate_content(enabled)` on an Item owned by a RadioGroup.
Probe `app.supports_radio_item_content()` when your application also loads
earlier providers. The delivery example uses the probe so its ordinary
option remains selectable on those providers.
[Complete delivery example](../bindings/rust/examples/radio_delivery.rs)

```rust
let decorate = app.supports_radio_item_content();
let priority = app
    .build_child(group, Kind::Item, "priority")?
    .text("Priority")?
    .detail_text("Tracked delivery. Recommended.")?
    .decorate_content(decorate)?
    .finish();
app.build_child(priority, Kind::Tag, "recommended")?
    .text("Recommended")?
    .finish();
```

Use `app.decorate_content(priority, false)` to hide the children and keep
their handles. The option keeps one focus stop and the group delivers
the usual selection event. See the allowed decorative kinds in the
[bridge composition guide](BRIDGE_COMPONENTS.md#compose-retained-radio-option-content).

## Add Radio tooltips from Rust

Set a hint on a standalone Radio or one Item in a RadioGroup. Pass the
opening delay in milliseconds and use the provider probe when supporting
earlier bridge releases.

```rust
let hint = app.supports_radio_tooltip().then_some("Track your parcel online");
let priority = app.build_child(group, Kind::Item, "priority")?
    .text("Priority")?
    .radio_tooltip(hint, 350)?
    .finish();
```

Call `app.radio_tooltip(priority, None, 0)?` to clear an existing hint.
The mounted setter can also replace the text or delay between frames.
The complete delivery example enables the hint and updates an arrival estimate
in the regular selection callback. [Run the delivery form](#animate-radio-selection-from-rust)

## Set Radio tab order from Rust

Pass `Some(index)` to `Builder::tab_index` for a standalone Radio or
RadioGroup. Lower signed indices come first and a group keeps one Tab stop.
Probe the provider before building a child when supporting earlier releases.

```rust
let order = app.supports_radio_tab_index().then_some(-2);
let delivery = app.build_child(root, Kind::RadioGroup, "delivery")?
    .tab_index(order)?
    .finish();
```

An explicit index requires provider support. This builder creates its node in
`build_child` before applying options. Use `app.radio_tab_index(delivery, None)?`
to restore default order on a mounted node. Keyboard ordering preserves
selection and queued callbacks. [Run the delivery form](#animate-radio-selection-from-rust)


## Style Radio states from Rust

Probe `app.supports_radio_paint()` and set one layer with `app.radio_paint` or the builder method.
Use the delivery example to see group geometry and checked paint on live options.
[Builder syntax and runnable form](BRIDGE_COMPONENTS.md#style-radio-states-in-language-builders)

## Configure Pagination from Rust

Build a detached `PaginationOptions` when you want the bridge capability and record size checked before creating a retained child. Ordinary `Kind::Pagination` still works with earlier providers. `app.supports_pagination_options()` reports whether the extended API is available.

```rust
use cui_builders::{DesignSize, PaginationConfig, PaginationOptions};

let pager = PaginationOptions::default()
    .configure(PaginationConfig {
        chevrons: true,
        jump_menu: true,
        visible_pages: 5,
        available_width: 320.0,
        group: "Pages de résultats".into(),
        previous: "Précédente".into(),
        next: "Suivante".into(),
        ..Default::default()
    })?
    .design_size(DesignSize::Base)
    .custom_size(52.5)?
    .mount(&mut app, root, "results")?;
app.total(pager, 24)?;
```

Use `app.configure_pagination(pager, &config)` to update a mounted pager as its inner width changes. `app.pagination_config(pager)` returns an owned snapshot. `app.reset_pagination(pager)` clears navigation options and preserves the page, total and size. Use `app.set_pagination_custom_size(pager, height)` and `app.pagination_custom_size(pager)` on a mounted node. Heights from 1 through 512 accept fractional logical pixels. Set zero to return to the named size. Check `app.supports_pagination_custom_size()` independently of navigation options for a varying provider. Use `on_change_event` to capture the committed page, then update your app after dispatch. The [document results example](../bindings/rust/examples/pagination_options.rs) applies the page update to three document labels.

```sh
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example pagination_options
```

For the native UI, add `--features native` before `--example` and `-- /path/to/font.ttf` at the end. The native build needs SDL3, FreeType and HarfBuzz.

## Keep the visible DataTable row during a Rust update

Use `Builder::data_table_keyed` on creation and `App::data_table_keyed` on
the owner thread to replace rows. Pass one distinct nonzero `u32` per row.
Reusing keys preserves the visible row and its offset where scroll bounds
allow. Surviving selection and keyboard navigation follow the keys. The
bridge copies the input.

```rust
use cui_builders::{App, Kind, Result};

fn inbox(app: &mut App) -> Result<()> {
    let root = app.root(Kind::Column, "root")?;
    let headers = ["Subject", "Sender"];
    let rows: &[&[&str]] = &[&["Design review", "Morgan"], &["Build status", "Alex"]];
    let table = app.build_child(root, Kind::DataTable, "inbox")?
        .data_table_keyed(&headers, rows, &[203, 204])?
        .visual_size(360., 184.)?
        .finish();

    // Apply a replacement on the UI thread after dispatching a page arrival.
    let mut updated: Vec<&[&str]> = vec![&["Release notes", "Build team"]];
    updated.extend_from_slice(rows);
    app.data_table_keyed(table, &headers, &updated, &[301, 203, 204])?;
    Ok(())
}
```

`DataTableState.row` and the change payloads report the key. Cell text and
bounds reads use zero-based matrix indices. The limits are 32 rows, 16
columns, and 128 UTF-8 bytes per header or cell. Pass empty rows and keys
to clear a keyed table. A replacement queues no change event.

`app.supports_keyed_data_table()` checks the linked bridge. With alpha.4,
an explicit keyed setter returns `Error(9)` and preserves existing data.
`data_table` remains available and assigns positional keys.

The complete [live inbox](../bindings/rust/examples/data_table_live.rs)
drives the load button and verifies the viewport without opening a window.
From the repository root, run:

```sh
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example data_table_live
```

For the native window, add `-- --font /absolute/path/to/font.ttf` and use an
SDL-enabled shared bridge via `CUI_BRIDGE_LIB_DIR`, or add `--features native`
for the bundled Linux build. The inbox uses a local page arrival.
[Paged inbox](#page-a-rust-datatable)

## Page a Rust DataTable

Check `app.supports_data_table_paging()` before opting in. Providers without
the complete V1 group still support ordinary tables. An explicit paging call
returns `Error(9)` on those providers and keeps the existing table intact.

```rust
let table = app.build_child(root, Kind::DataTable, "inbox")?
    .data_table_keyed(headers, initial_rows, initial_keys)?
    .data_table_load_more(0)?
    .finish();

// After a successful frame, on the UI thread.
if let Some(request) = app.poll_data_table_load_more(table)? {
    app.configure_data_table_load_more(table, false, 0)?;
    // Enqueue work using this observation and your own request identity.
    submit_page(request);
}
```

The remaining-row threshold is 0 through 32. With 0, demand is raised when the
visible window reaches the last row. For an initial page, bind column headers
and empty rows before opting in. `poll_data_table_load_more` consumes one
copied `DataTableLoadMore` with the `u64` frame sequence, row count, half-open
visible range, last local row key and capacity. The callback queue is
independent. The copy remains valid after removing the table or dropping the app.

Keep the service cursor and one in-flight request in application state.
Rows stay interactive while paging is off. Check that a response still belongs
to the current request before using `data_table_keyed` to append it. Enable
paging again if the source has more rows. The ordinary setter holds up to 32 rows.

Use `App::run_with` to poll demand and ready completions in a synchronous
after-frame hook, or poll after `step` in a manual loop. Apply responses on
the app's creating thread. The example uses a local source and demonstrates
selection during loading, retry, stale-response rejection and anchored append.
[Runnable paged inbox](../bindings/rust/examples/data_table_paging.rs)

```sh
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example data_table_paging
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --features native --example data_table_paging -- --font /path/to/font.ttf
```

### Retain more than 32 rows in Rust

Query `app.data_table_max_row_capacity()` before opting in. It returns `None`
when the linked provider lacks the capacity API. The ordinary positional and
keyed methods keep their 32-row limit. On a compatible provider, choose a limit
that fits the full snapshot you intend to keep.

```rust
use cui_builders::{App, Error, Kind, Result};

fn inbox(app: &mut App) -> Result<()> {
    let capacity = app.data_table_max_row_capacity().ok_or(Error(9))?.min(128);
    if capacity < 40 {
        return Err(Error(3));
    }
    let headers = ["Subject"];
    let root = app.root(Kind::Column, "root")?;
    let table = app.build_child(root, Kind::DataTable, "inbox")?
        .data_table_keyed_capacity(&headers, &[&["Project update"]], &[101], capacity)?
        .visual_size(360., 160.)?
        .data_table_load_more(0)?
        .finish();

    // On the UI thread, replace the full snapshot after accepting a page.
    app.data_table_keyed_capacity(
        table, &headers, &[&["Project update"], &["Release notes"]], &[101, 102], capacity,
    )?;
    Ok(())
}
```

The current provider accepts up to 10,000 rows within its 32 MiB app-wide
native retained budget. The limit leaves unfilled rows unallocated. Headers
and cells support up to 128 UTF-8 bytes each. Use a distinct nonzero key for
each row, or empty slices for rows and keys when the snapshot is empty.
Keyed replacement preserves the viewport and selection for surviving keys.
Failed replacement keeps the active snapshot and capacity. Each paging record
reports its snapshot's capacity.

The offline inbox grows from 8 to 40 rows and checks scroll position,
selection and stale-response rejection. Run it from the repository root.

```sh
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example data_table_paging -- --larger
cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --features native --example data_table_paging -- --larger --font /path/to/font.ttf
```

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

## Hidden native windows

`App::sdl_with_window_options(width, height, title, font,
WindowOptions { initially_hidden: true })` requests hidden creation before the
first frame. `App::sdl` remains visible; `WindowOptions::default()` also stays
visible. Use `app.set_window_visible(true)` and `app.window_visible()` (Results)
on the owning UI thread, outside Core stepping; `run_with` after-frame callbacks
may use them. Headless visibility queries return unavailable. This is an additive
Rust API and independent C window-options record, preserving the old C options
ABI. Rebuild with matching native exports for the new methods. See the [shared
contract and Linux proof](BINDING_BUILDERS.md#native-window-visibility).

## Bounded typed root shadows

See [typed root shadow authoring](BRIDGE_SHADOWS.md) for the six-kind implementation, owned values, canonical CSS/preset factories and explicit clear/reset behavior. Focused portable runtime tests cover this subset; integration remains pending. This does not expose compound parts or portals.

For bounded semantic ROOT/MAIN/indicator/track/thumb authoring, read
[typed part shadows](BRIDGE_PART_SHADOWS.md). Slider has no thumb; owner topology,
whole-group naming and separate reset are explicit parts of that contract.

## Controlled Single/Range Slider

The unreleased [controlled Rust subset](RUST_SLIDER_MODEL.md) supplies owned
f64 Single/Range requests, u64 identities and explicit issued-receipt acceptance.
It uses a restricted borrowed `SliderRequestContext`, preserving legacy callback
signatures and scalar Slider autoacceptance. Its complete windowless authored UI
is `bindings/rust/examples/slider_model.rs`. Linux dynamic current/old/provider, UI and matching bundled-static source
checks passed on1.99.1; MSRV1.85.1 validation remains a required follow-up. Direct C custom style/motion and native numeric AT
are not exposed by this subset.

## Responsive loading placeholders

The [responsive Skeleton guide](RUST_SKELETON.md) includes a native profile UI
that replaces decorative placeholders with loaded content.


## Display a file preview in an upload card

The upload example uses the retained Attachment builder, a decoded RGBA
preview and an application button that completes the transfer. Headless
mode verifies the button callback. [Build and run the upload example](BRIDGE_COMPONENTS.md#show-an-attachment-preview-in-a-language-builder)
