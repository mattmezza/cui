# Go builders for the CUI bridge

For installable local wheel, npm, Go module and Rust crate artifacts, see the
[packaging guide](PACKAGING.md). Linux alpha packages are tested outside the
checkout; registry publication and other platform packages remain pending.

`bindings/go` is a cgo Go module for the portable CUI bridge ABI v1. Its builders cover the native bridge node kinds through Settings, from layout, controls, and overlays to data views and navigation; the sections below specify each implemented surface and its limits. Builders create a detached tree; `App.Mount` validates it and creates actual CUI nodes. `Step` submits the frame, `QueuePointer` and `QueueKey` send synthetic input, and `Dispatch` invokes Go callbacks after the native frame. Change events carry state and copied text captured when queued. Bridge node coverage does not imply full feature parity with the 78-component manifest.

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

Use the [Go SVG resource guide](GO_ICON_RESOURCES.md) to prepare copied coverage
masks from embedded artwork or app-owned SVG and manage the resource cache.
See the [retained inbox guide](GO_ICON_BADGE.md) for shared artwork and configured
Badge indicators around clickable children.

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

MessageScroller accepts up to 64 `Item` children. Item text is the message body, `Detail` is the sender, and `StepStatus(0/1)` selects incoming or outgoing. `VisualSize` sets the viewport. After the first `Step`, `App.Value(scroller)` reads the live vertical offset and `SetOffset` changes it; `OnChange` receives native offset changes. The initial frame follows the newest message.

Carousel accepts up to 64 `Item` slides with text or child content. `VisualSize` sets the viewport, `Selected(item)` changes the slide, and `App.Selected` reads it. Native wheel, drag, and keyboard navigation emit `OnChange` with a full selected Item handle snapshot. The bridge currently uses horizontal orientation and does not support nested Carousels.

Resizable accepts up to 64 `Item` panels in a horizontal split. Set an Item's initial width with `PanelSize(100..4096)`, read its live width with `App.Value(item)`, and locate a divider with `App.ResizableHandleBounds(group, precedingItem)`. Dragging a divider emits `OnChange` on the resized Item with numeric `Event.Value`. `VisualSize` configures the group and `Disabled(true)` suppresses dragging.

TextView renders owned Markdown (default), HTML, or plain source. Use `Format(Markdown/HTML/Plain)`, `Text` or `SetText`, and `VisualSize`. `App.SelectText(view, anchor, cursor)` selects rendered UTF-8 byte offsets; `App.CopySelection(view)` returns plain selected text. Pointer selection emits `OnChange` with a copied selected-text snapshot. Disabled TextView suppresses pointer selection.

Editor owns a multiline UTF-8 buffer. `Editor(key, source, language)` sets initial text and language metadata; `VisualSize` sets its viewport. `App.Text` reads live content and `OnChange` receives copied text snapshots. The current bridge limits content to 4096 bytes and 512 lines and does not expose completion, syntax callbacks, search, or clipboard actions.

Chart renders caller-supplied `ChartDatum{Label, Value}` data as line, area, bar, or pie via `ChartKind`; `Data` and `SetData` copy at most 64 points into native storage. `App.ChartDatum` reads a copied point back. Labels are valid UTF-8 up to 128 bytes, values are finite and within ±1e9, and pie values must be nonnegative. `VisualSize` sets its dimensions. The bridge draws a native raster with hover feedback and a "No data" empty state; it does not fetch data.

Table is a compositional native table: its `Item` children are rows, and each row's `Item` children are text cells. The first row renders as a header. `VisualSize(width, 0)` sets its width; cell `SetText` updates the next frame. Sorting, selection, and virtualization belong to DataTable rather than this basic Table.

DataTable uses `TableData(headers, rows)` or `SetTable` with 1–16 columns and up to 32 rows; every cell and header is valid UTF-8 up to 128 bytes. Go and the bridge copy the matrix. `SelectionMode(SelectRows/SelectColumns/SelectCells)` chooses native row, column, or cell selection. `App.DataTableCell`, `DataTableCellBounds`, `DataTableHeaderBounds`, and `DataTableState` expose live data and hit targets. Native selection and header presses emit `OnChange`. `DataTableState` reports
the selected row key and one-based column and sort column positions. The
legacy setter assigns one-based positional row keys. To apply a sort
request, reorder the rows and submit the new matrix. See the [keyed DataTable example](#keep-the-visible-datatable-row-during-a-go-update) for live replacements.

DataTable `OnChange` also includes a copied JSON `Event.Text` snapshot with `reason`, `row`, `column`, `sortColumn`, and `sortDirection`. Parse that payload when a callback needs the exact queued transition rather than a later live state.

Plot accepts up to 256 finite `PlotPoint{X, Y}` values within ±16384 via `Points` or `SetPoints`; Go and the bridge copy the points. `App.PlotPoint` reads one point and `VisualSize` sets the viewport. The native plot currently draws themed dots; axes, paths, and hit interactions are not exposed.

Command accepts up to 64 `Item` actions. Item text is the label and `Detail` supplies one filter keyword; `Command.Text` or `SetText` changes the query, and `App.Text` reads it. `App.CommandItemBounds` locates a visible action for pointer input. Native filtering and keyboard navigation update `App.Selected(command)` and emit `OnChange`; an Item's `OnClick` receives confirmation. Grouped/custom rows and host action execution are application responsibilities.

Sidebar accepts up to 64 `Item` navigation actions. `Open` controls the collapsed state, `VisualSize` sets its size, and `App.Selected(sidebar)` reads the active Item. A native press emits a Sidebar `OnChange` event with an exact `Event.Selected` snapshot and an Item `OnClick` event. Disabled Items do not activate.

`App.ThemePresets()` lists names from the loaded bridge. `SetThemePreset(index)` applies one to subsequent frames, and `ThemePreset()` returns the selected index plus a `bool` indicating whether an explicit preset was chosen. The default theme has no selected preset index.

`RootView(key)` can be mounted as the sole root container. It renders child widgets in the native root surface; `RootPadding(0..256)` sets its content padding before or after mounting. Row and Column roots remain available.

SDL apps expose the system clipboard with `SetClipboard(text)` and `Clipboard()`. Text is copied, valid UTF-8 without NUL, and limited to 64 KiB. Headless apps return a backend-unavailable error. The native smoke example checks small, empty, and exact 64 KiB Unicode SDL round trips under invisible Xvfb.

Speech renders a native speech button and retains a transcript. The host supplies recognition results through `StartSpeech`, `ReportSpeech(text, finalPhrase)`, and `FinishSpeech`; `SpeechTranscript` reads the current copied text. `OnChange` snapshots start, report, and finish transitions in `Event.Value` (0, 1, 2), with copied transcript text. The bridge does not capture microphone audio or run speech recognition. Reports are valid UTF-8 up to 4096 bytes.

Questionnaire accepts 1–16 `Item` questions, each with `Item` choices, up to 256 choices total. Configure question flags with `Item.Question(required, multiple)` before the first frame. `QuestionnaireState` returns the current question and completion flag; `ChoiceSelected`, `NextQuestion`, `PreviousQuestion`, and `SubmitQuestionnaire` expose native state/navigation. Submit rejects unanswered required questions, emits one completion event on the first successful call, and emits a submit event on each successful call. `OnChange` carries copied JSON in `Event.Text`: `event`, `item`, `current`, `complete`, and one-based `selected` choice indices. The schema freezes after its first frame; this bridge surface does not support dynamic question/choice edits.

Dock accepts up to 64 `Item` panels, whose child widgets render as panel content. `VisualSize` sets its viewport; after the first frame, `ActiveDockPanel`, `ActivateDockPanel`, `DockPanelBounds`, and `MoveDockSplit(panel, reference, DockHorizontal/DockVertical, ratio, before)` access the native model. `SerializeDock` returns a copied model string for inspection. Activating a panel emits `OnChange` with a full `Event.Selected` handle. Panel schema freezes after the first frame; serialized-model restore and drag docking are not exposed by this bridge API.

Settings contains up to eight `Item` pages, each with up to eight `Item` groups, containing Switch, Checkbox, Input, or NumberInput fields. Totals are limited to 32 groups and 128 fields, with at most 16 fields per group. Page/group text supplies headings; boolean field text supplies its title, while Input and NumberInput use `Detail(title)` so `Text` can hold the editable value. `Query(text)` filters native navigation using up to 256 UTF-8 bytes. After the first frame, `SettingsPage`, `SetSettingsPage`, `SettingsFieldBounds`, and `ResetSettingsPage` access native page and field state; page `OnChange` carries an exact selected handle and field changes use their existing copied event values. Reset restores values captured at first render and emits field changes. Page/group topology and titles freeze after that frame; only individual fields support `Disabled`. The bridge does not provide host persistence or custom fields.

VirtualList accepts up to 64 keyed `Item` rows with fixed 32-pixel height and an overscanned viewport. `VisualSize` sets its viewport, `Value` sets a vertical offset, and `App.Value` reads the live offset. `App.VirtualListItemBounds` returns bounds only for a visible row. Native wheel and keyboard scroll emit `OnChange` with the offset; disabling the list suppresses scrolling.

The composed workspace example lives at `bindings/go/examples/workspace`. From `bindings/go`, run `CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --headless` to validate native hit targets, Account/Preferences tab navigation, name editing, notifications checkbox, an older birthday, save dialog, and status update without opening a window. To use the SDL UI, run `CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --font /absolute/path/to/font.ttf`. Automated SDL runs on Linux must use invisible Xvfb, for example `xvfb-run -a env -u WAYLAND_DISPLAY CUI_BRIDGE_LIBRARY=/absolute/path/to/libcui_bridge.so go run ./examples/workspace --font /absolute/path/to/font.ttf`; native mode remains open until the user closes its window.

## Configure Pagination from Go

`ConfiguredPagination` records copied layout options on a detached node. Set `AvailableWidth` to the current inner width in logical pixels to wrap, or use zero for a single row. Send committed pages to your results view through `OnChange`. For a dynamic provider, check `app.SupportsPaginationOptions()` before opting in. Ordinary `Pagination(key)` and its change callback also work with earlier providers.

```go
pager := cui.ConfiguredPagination("results", cui.PaginationConfig{
    Chevrons: true, JumpMenu: true, VisiblePages: 5, AvailableWidth: 320,
    Group: "Pages de résultats", Previous: "Précédente", Next: "Suivante",
}).Total(24).DesignSize(cui.Base)
pager.OnChange(func(event cui.Event) {
    showResults(int(event.Value))
})
```

Use `pager.ConfigurePagination(config)` on the app's owner thread to replace the options. `pager.PaginationConfig()` returns a copy and a presence flag. `pager.ResetPagination()` clears the options and preserves the current page, total and design size. The app dispatches callbacks after stepping the bridge.

Run the [document results example](../bindings/go/examples/pagination_options/main.go) with Go and CGO. The default Linux build uses the bundled bridge sources.

```sh
cd bindings/go
go run ./examples/pagination_options
```

To open the same example in an SDL window with a font, run `go run -tags cui_native ./examples/pagination_options -font /path/to/font.ttf`. The native build needs SDL3, FreeType and HarfBuzz.

## Keep the visible DataTable row during a Go update

Use `TableDataKeyed` to build and `SetTableKeyed` to replace rows from the
app's owner thread. Pass one distinct nonzero `uint32` per row. Reuse the
same keys across updates so the viewport, selection, and keyboard
navigation follow surviving rows. Go and the bridge copy the input.

```go
headers := []string{"Subject", "Sender"}
rows := [][]string{{"Release notes", "Build team"}, {"Design review", "Morgan"}}
inbox := cui.DataTable("inbox").TableDataKeyed(headers, rows, []uint32{203, 204})

// After mounting, apply the replacement from a UI-thread callback.
updated := append([][]string{{"New message", "Workspace"}}, rows...)
if err := inbox.SetTableKeyed(headers, updated, []uint32{301, 203, 204}); err != nil {
    return err
}
```

`DataTableState.Row` and change events report the selected key. Cell and
bounds reads still use zero-based matrix indices. The limits are 32 rows,
16 columns, and 128 UTF-8 bytes per string. For an empty keyed table, call
`SetTableKeyed(headers, nil, nil)`. Replacing rows does not emit a change event.

`app.SupportsKeyedDataTable()` checks the active bridge. If that provider
lacks keyed replacement, mount and update return
`cui.ErrKeyedDataTableUnavailable` and leave the prior state intact.
`TableData` and `SetTable` continue to use positional keys on those
providers.

Run the [complete inbox](../bindings/go/examples/data_table_live/main.go)
from `bindings/go`. The default opens no window and exercises the button
callback, scroll position, and updated rows.

```sh
go run ./examples/data_table_live
```

The bundled native route on Linux uses
`go run -tags cui_native ./examples/data_table_live --font /path/to/font.ttf`.
For an existing SDL shared bridge, set `CUI_BRIDGE_LIBRARY` to its absolute
path and use `-tags cui_dynamic` instead. This example uses a local arrival.
[Paged inbox](#page-a-go-datatable)

## Page a Go DataTable

`app.SupportsDataTablePaging()` checks for all V1 exports and the expected
record size. Use `LoadMore(threshold)` on a builder with column headers.
To mount the same tree on a provider without paging, clear its opt-in with
`ConfigureLoadMore(false, 0)` before mounting. Ordinary DataTables remain usable.

```go
inbox := cui.DataTable("inbox").
    TableDataKeyed(headers, initialRows, initialKeys).
    LoadMore(0)

// After mounting and stepping, on the UI thread.
request, ok, err := inbox.PollLoadMore()
if err != nil {
    return err
}
if ok {
    if err := inbox.ConfigureLoadMore(false, 0); err != nil {
        return err
    }
    // Queue work using your request generation and service cursor.
    submitPage(request)
}
```

The remaining-row threshold may be 0 through 32. With 0, demand is raised
when the visible window reaches the last row. Start with headers and empty
rows to request the initial page. `PollLoadMore()` consumes one copied
`DataTableLoadMore` or reports no record. The copy includes the `uint64` frame
sequence, row count, half-open visible range, last local row key and capacity.
The regular callback queue is independent.

Keep one request in flight while the current rows remain interactive. Use
your own generation and service cursor to reject stale results, then apply
accepted pages on the UI thread with `SetTableKeyed`. Enable paging again
if more data is available. The ordinary setter holds up to 32 rows.

The synchronous `app.RunWith(.016, afterFrame)` hook receives the app after
regular callbacks for each successful native frame. A request to close or
dispose during dispatch skips the hook. Return an error to stop the loop.
Drain completed work from a channel without blocking the UI thread. Manual
loops can poll after `Step`.

From `bindings/go`, run the [offline inbox](../bindings/go/examples/data_table_paging/main.go).
It verifies selection during loading, retry, stale-response rejection and
scroll retention when appending the final page.

```sh
go run ./examples/data_table_paging
go run -tags cui_native ./examples/data_table_paging --font /path/to/font.ttf
```

### Retain more than 32 rows in Go

Query the active provider on the app's owner thread. A zero limit means the
opt-in API is unavailable. In that case use the ordinary setters for snapshots
of up to 32 rows. With a compatible provider, select a limit that fits the
largest snapshot you intend to retain.

```go
maximum, err := app.DataTableMaxRowCapacity()
if err != nil {
    return err
}
capacity := min(512, maximum)
if capacity < 40 {
    return fmt.Errorf("this inbox needs capacity for 40 rows")
}
headers := []string{"Subject"}
inbox := cui.DataTable("inbox").
    TableDataKeyedCapacity(headers, [][]string{{"Project update"}}, []uint32{101}, capacity).
    VisualSize(360, 160).LoadMore(0)
if err := app.Mount(cui.Column("root").Children(inbox)); err != nil {
    return err
}

// After accepting a page on the UI thread, submit the combined snapshot.
return inbox.SetTableKeyedCapacity(headers,
    [][]string{{"Project update"}, {"Release notes"}},
    []uint32{101, 102}, capacity)
```

The current provider accepts up to 10,000 rows, within its 32 MiB app-wide
native retained budget. The limit is a ceiling and does not preallocate unused
rows. Each header or cell can contain up to 128 UTF-8 bytes. Provide distinct,
nonzero keys that stay stable across replacements, or nil for an empty snapshot.
Successful keyed replacements preserve selection and viewport position when their
row keys survive. If a replacement fails, the current matrix and capacity stay
active. Each paging record reports the capacity of its snapshot.

Run the offline inbox below from `bindings/go`. It grows from 8 to 40 rows,
checks scroll position and selection, and discards a stale response. The second
command adds the SDL window and needs SDL3, FreeType, HarfBuzz and a font.

```sh
go run ./examples/data_table_paging --larger
go run -tags cui_native ./examples/data_table_paging --larger --font /path/to/font.ttf
```

## Packaged build-time CUI bridge

The canonical module path is `github.com/mattmezza/cui/bindings/go`. Linux cgo
builds compile separate CUI translation units from the tracked SDK and statically
link the bridge by default. `go build -tags cui_native` adds SDL3, FreeType and
HarfBuzz through pkg-config; portable builds open no window. A copied executable
needs no CUI checkout or runtime compiler, while native system libraries and a
font remain required.

Explicit `Options.Library` or `CUI_BRIDGE_LIBRARY` still selects a shared bridge.
The optional `cui_dynamic` build tag uses packaged shared-library discovery and
its verified cache. See [local artifact install commands](PACKAGING.md) for
source-only module ZIPs, optional prebuilt modules, and release/tag limitations.

## Hidden native windows

`New(Options{Native:true, FontPath:font, InitiallyHidden:true})` passes hidden
creation to SDL before the first frame. The zero option stays visible. Use
`app.SetWindowVisible(true)` and `app.WindowVisible() (bool, error)` on the owning
UI thread, outside Core stepping; handlers dispatched after a native callback
frame may use them. Headless hidden requests/control return unavailable. The Go
loader resolves the new symbols optionally: default apps still load older ABI1
libraries; feature calls fail explicitly if unavailable. See the [shared contract
and Linux proof](BINDING_BUILDERS.md#native-window-visibility). Existing Go Options
keyed literals remain source compatible; unkeyed literals need the new field.

## Bounded typed root shadows

See [typed root shadow authoring](BRIDGE_SHADOWS.md) for the six-kind implementation, owned values, canonical CSS/preset factories and explicit clear/reset behavior. Focused portable runtime tests cover this subset; integration remains pending. This does not expose compound parts or portals.

For bounded semantic ROOT/MAIN/indicator/track/thumb authoring, read
[typed part shadows](BRIDGE_PART_SHADOWS.md). Slider has no thumb; owner topology,
whole-group naming and separate reset are explicit parts of that contract.

## Responsive loading placeholders

The [responsive Skeleton guide](GO_SKELETON.md) includes a native profile UI
that replaces decorative placeholders with loaded content.
