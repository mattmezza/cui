# C bridge component coverage

The bridge ABI remains version 1. Existing numeric enum values, option layout,
event layout, and functions are unchanged. New kinds are appended.

| Kind | Native CUI renderer | State and events | Current limit |
| --- | --- | --- | --- |
| Row, Column | `cui_begin_box` | gap | One root; depth 128 |
| Label | `cui_text` | copied UTF-8 text | Text only |
| Button | `cui_button_ex` | copied text, click | Existing options |
| Checkbox | `cui_checkbox` | checked, change, disabled | Boolean only |
| Radio | `cui_radio` | checked, change, disabled | Standalone radio; no group exclusivity |
| Switch | `cui_switch_ex` | checked, change, disabled | Boolean only |
| Toggle | `cui_toggle` | checked, change, disabled | Boolean only |
| Slider | `cui_slider` | value, range, step, change, disabled | Float native precision |
| SliderModel (kind81, C/Python/Node/TypeScript/Go) | `cui_styled_slider_model_events_v1` | owned double Single/Range, separate copied request FIFO, explicit receipt acceptance | [Exact contract](BRIDGE_SLIDER_MODEL.md); [Python builder](PYTHON_SLIDER_MODEL.md); [Node/TypeScript builders](NODE_SLIDER_MODEL.md); [Go builder](GO_SLIDER_MODEL.md); Rust typed builders, native numeric AT, V2 adapter and typed shadows pending |
| Progress | `cui_progress` | value in [0,1] | Width fixed at 160 |
| Badge | `cui_badge`, optional `cui_badge_ex` and `cui_badge_with_icon` | Legacy text pill, or a configured count, dot or artwork indicator around retained children | Configured Badge preserves ordinary child actions and reports current indicator geometry. [Retained C guide](BRIDGE_BADGE.md). |
| Input | `cui_input_ex` | per-node edit model, text, change, disabled | 4096 UTF-8 bytes |
| Textarea | `cui_textarea` | per-node multiline edit model, text, change, disabled | 4096 UTF-8 bytes, 512 layout lines |
| NumberInput | `cui_number_input` | per-node numeric editor, value, range, step, change, disabled | Finite values; 127-byte draft |
| Collapsible | `cui_collapsible` | open state, change, disabled | Child subtree is panel |
| Alert | `cui_alert` | copied title/message | Static informational alert |
| RadioGroup | `cui_radio_group` | selected Item, change, disabled | Up to 64 Items |
| Tabs | `cui_tabs` | selected Item, change, disabled | Up to 64 Items; Item subtree is panel |
| Accordion | `cui_accordion` | per-Item open state, change, disabled | Up to 64 Items; multiple expansion |
| Item | Composite descriptor | label, disabled, optional panel children | Must belong to group |
| Select | `cui_select` | selected Item, change, disabled | Up to 64 Items; viewport is app surface |
| ComboBox | `cui_combobox` | query edit model, selected Item, change, disabled | Up to 64 Items, 4096 UTF-8 query bytes |
| DropdownButton | `cui_dropdown_button` | action click, selected menu Item, change, disabled | Up to 64 leaf menu actions |
| Dialog | `cui_surface_begin` | open, close result, child body | Native modal surface |
| AlertDialog | `cui_alert_dialog` | open, accept/cancel/close result | Native confirm/cancel content |
| Sheet | `cui_surface_begin` | open, close result, child body | Default right edge |
| Popover | `cui_surface_begin` | open, close result, anchor, child body | Anchor must be a live node |
| Tooltip | `cui_tooltip` | native hover lifecycle, open/close change, disabled | Anchor must be a live node |
| HoverCard | `cui_begin_hover_card` | native hover lifecycle, open/close change, disabled, child body | Anchor must be a live node |
| Avatar | `cui_avatar` | copied name, size | Initials avatar |
| Bubble | `cui_bubble` | copied message | Default variant |
| Empty | `cui_empty` | copied title/description, width | Static state |
| GroupBox | `cui_begin_group_box` | copied title, width, child tree | Native group container |
| Kbd | `cui_kbd` | copied chord | Default platform-neutral text |
| Skeleton | `cui_skeleton`, optional retained presentation | Legacy fixed dimensions and responsive sizing, styling and wake hints in [Python](PYTHON_SKELETON.md), [JS/TS](NODE_SKELETON.md), [Go](GO_SKELETON.md) and [Rust](RUST_SKELETON.md) | Direct C palette lookup, custom easing and full Core box descriptor |
| Spinner | `cui_spinner` | size, frame-time motion | Default style |
| Tag | `cui_tag` | copied text | Default style |
| Marker | `cui_marker` | copied text, retained state | Single text child |
| Shimmer | `cui_shimmer_text` | copied text, retained state | Single line |
| List | `cui_list` | selected Item, change, disabled | Up to 64 Items |
| Menu | `cui_menu` | menu Item action, change, disabled | Up to 64 leaf actions |
| Pagination | `cui_pagination` | current page, total, change, disabled | One-based page, total <= 1,000,000 |
| Rating | `cui_rating` | value, maximum, change, disabled | Maximum <= 10 |
| Stepper | `cui_stepper` / opt-in `cui_stepper_visual_navigation` | selected Item, status, aggregate change, disabled; opt-in density/orientation/shared size and copied RGBA Item icons | Up to 64 Items; visual selection paints on next submission; no foreign controlled REQUEST handlers |
| DescriptionList | `cui_description_list` | Item label/value pairs | Up to 64 Items, default columns |
| Scrollable | `cui_scrollable_begin/end_ex` | child tree, viewport/extents, wheel change | Vertical viewport |
| StatusBar | `cui_status_bar` | Item labels in left/center/right regions | Set Item step status to 0/1/2 for region |
| Toolbar | `cui_toolbar` | Item child controls, inherited disabled state | Up to 64 Items; child controls own actions |
| TitleBar | `cui_title_bar` | Item text labels, retained title state | No native window host controls |
| Icon | `cui_icon_named`, optional `cui_icon` | Legacy catalog name and dimensions, or a shared owned resource and full copied Icon options | An unknown legacy name fails the frame. A configured Icon reports its own artwork error separately. [C presentation guide](BRIDGE_ICON_PRESENTATION.md). |
| Message | `cui_message` | Copied body/header, width | Incoming filled bubble, one body |
| Attachment | `cui_attachment` | Copied title/description, status, progress, width | Metadata chip; no host file or remove/retry actions |
| Image | `cui_image` | Owned RGBA8 pixels, revision, description, size | At most 16 MiB; no path, URI, animated loader |
| Tree | `cui_tree` | 64 flat Item records with preorder depth, expansion and selected Item events | Item step status is depth 0..63; no custom row content |
| TimeField | `cui_time_field` | HH:MM:SS state, keyboard change, copied event text, disabled | 24-hour fixed format |
| ColorPicker | `cui_color_picker` | Packed RGBA, hue/plane/alpha input, copied hex event text, disabled | Default picker, no custom palette |
| Calendar | `cui_calendar` | Single selected date, pointer/keyboard change, disabled | Default English labels, one month |
| DatePicker | `cui_date_picker` | Retained popup, single selected date, open state and copied change text | Default English labels, no range mode |
| OtpInput | `cui_otp` | Owned value buffer, target length, text input and copied change text, disabled | 1..256 cells, 4096 UTF-8 bytes; default six cells |
| Form | `cui_form` | Item field labels/descriptions, child controls, inherited disabled state | Up to 64 fields; no built-in validation or nested Form |
| Input Group | `cui_input_group` | Owned editor, Item button addons, alignment, copied text/change, disabled | Up to 64 addons; Item step status 0..3 selects position |
| Notification | `cui_notifications` | Shared retained in-app stack, one card per node, copied title/message, open/dismiss, pointer click and close events | Up to 128 active cards; no OS delivery, action button, or auto-hide |
| Focus Trap | `cui_begin_focus_scope` | Modal scope around child nodes, explicit open state, pointer/focus exclusion outside scope | Child content only; no backdrop or automatic close |
| MessageScroller | `cui_message_scroller` and `cui_message` | Up to 64 Item message records, native virtualized viewport, retained scroll/follow, Item sender/body and direction, copied scroll CHANGE | Fixed default bubble style; no host message store or custom row callback |
| Carousel | `cui_carousel` | Up to 64 Item slides, native horizontal wheel/drag/key navigation, retained selection, copied selected Item CHANGE, disabled | Fixed slide text/child content; no nested carousel, custom controls, or looping |
| Resizable | `cui_resizable` | Up to 64 Item panels with native draggable handles, retained sizes, explicit panel size and copied resize CHANGE, disabled | Horizontal split, default min 100px; no hidden panels or custom handle appearance |
| TextView | `cui_text_view` | Owned Markdown/HTML/plain document parser, native read-only selection, copied selection CHANGE, plain copy, disabled | Source up to 4096 bytes; bounded parser workspace, no image/URL callbacks or host clipboard |
| Editor | `cui_editor` | Owned multiline UTF-8 model, native line-numbered editing, copied text CHANGE, language label and disabled state | 4096 UTF-8 bytes and 512 layout lines; no host syntax-highlighting, completion, search, or clipboard callbacks |
| Chart | `cui_chart` | Copied up to 64 labeled numeric data points, native line/area/bar/pie raster and hover, data readback, disabled | Up to 512×512 pixels; one series, default native tooltip; no custom callbacks, candle, radar, or Sankey |
| Table | `cui_table_begin_ex`, checked row/cell APIs | Item rows and nested Item cells with copied text, native table semantics and layout; first row is header | Up to 64 rows/64 cells per row; fixed equal column widths, text-only cells; no sort, selection, or virtualization |
| DataTable | `cui_data_table` | Copied matrix, virtualized viewport, optional keyed replacement, row/column/cell selection, sortable headers, cell/state/bounds queries, copied CHANGE snapshots, optional near-end polling in C, Python, JavaScript, TypeScript, Go and Rust | 32 rows × 16 columns, 128 UTF-8 bytes per cell, row selection by default, caller-owned sorting and page delivery, no custom cell content |
| Plot | `cui_plot_dots` | Copied up to 256 Cartesian (x,y) samples, native themed point rendering, point readback | Dot plot only; coordinates within ±16384, no path, axis, zoom, or point hit model |
| Command | `cui_command` | Up to 64 Item actions, owned UTF-8 query editor, native keyword filtering, keyboard/pointer selection, copied query/selection CHANGE and Item confirm CLICK | Flat palette; Item detail is one keyword string, no grouped headings, custom row content, or host action callback |
| VirtualList | `cui_virtual_list_draw` | Up to 64 keyed Item rows, native overscanned vertical viewport, wheel/keyboard scroll, copied offset CHANGE, visible Item bounds, disabled | Fixed 32px rows, vertical only; no intrinsic row sizing or host data provider |
| Sidebar | `cui_sidebar`, `cui_sidebar_menu` | Up to 64 Item actions, native collapsible rail, pointer selection, copied selected CHANGE and Item CLICK, open state and disabled | One flat action menu with icon slots; no nested groups, custom callbacks, or persistence |
| Theme | `cui_theme_preset_count`, `cui_theme_preset_name`, `cui_theme_preset_get` | Enumerate native palettes, copy preset names, select a palette per bridge app, and apply it to subsequent component frames | Preset selection only; no custom token import, host preference watcher, or theme-editor widget |
| Root View | `cui_root_init`, `cui_root_render` | Root node owns full viewport background, renders bridge children, supports bounded padding and app theme changes | One app root with no host plugin factories, copy hook, or root-managed surface facilities; dialogs and notifications use their existing bridge paths |
| Clipboard | `cui_sdl3_clipboard_interface` | Explicit UTF-8 OS clipboard write and copied read through the SDL app, with capability failure and output preservation | SDL owner thread only, outside a bridge frame; 64 KiB text limit; headless apps report BACKEND_UNAVAILABLE, no implicit widget copy/paste action |
| Speech | `cui_speech_state`, `cui_speech_button` | Native retained speech lifecycle and button, externally supplied UTF-8 hypothesis/phrase/finish reports, copied transcript and CHANGE event snapshots | Bridge has no microphone or recognizer provider; host supplies reports outside a frame, 4096-byte transcript and bounded queue; no audio waveform input |
| Questionnaire | `cui_questionnaire_state`, composable styled controls | Questionnaire->Item questions->Item choices, native required/single/multiple answers, current/selected queries, next/previous navigation, copied JSON event snapshots | Up to 16 questions×16 choices and 256 choices total; choice-only schema freezes after first render; no freeform input, custom validators, or host content callbacks |
| Dock | `cui_dock`, `cui_dock_view` | Retained native tab and split tree over Item panels and their child content; activate, split, query bounds, serialize and copied selected Item CHANGE | Up to 64 panels; panel topology freezes after first render; center workspace only, no host panel factories or close persistence |
| Settings | `cui_settings` | Settings->Item pages->Item groups->Switch/Checkbox/Input/NumberInput fields; retained native navigation, copied filter query, native field interaction, selected page and field CHANGE snapshots, reset to first-render values | Up to 8 pages, 32 groups, 128 fields; topology and titles freeze after first render; field-level disabled only; no dropdown/custom fields, host persistence, or custom search callbacks |

`cui_bridge_poll_event_ex` copies queued CHANGE snapshots, including editable
text. A capacity failure preserves the event for retry. The legacy event poll
returns CLICK events and skips CHANGE events.

The table covers all 78 manifest families plus four bridge-only structural kinds
(Row, Column, RadioGroup, Item). Each row describes the supported subset; this
does not imply full source component parity.

Settings uses Item page and group nodes, with Switch, Checkbox, Input, or
NumberInput leaves. Set page/group and boolean-field titles with `set_text`;
set Input/NumberInput titles with `set_detail_text`. Set Input content with
`set_text` and NumberInput values with `set_value`. The first successful frame
captures reset defaults and freezes
field topology and titles. `settings_set_query` copies up to 256 UTF-8 bytes;
`settings_get_page` and `settings_field_bounds` require an initialized frame.
Page selection CHANGE events copy the selected handle. Field CHANGE events
copy the field's value or text; native reset emits one event for each field in
the page, including fields already at their default. Programmatic input and
number changes remain available after initialization, and reset restores the
captured defaults. Only individual fields accept `set_disabled`.

DataTable CHANGE events include copied JSON text with `reason` (`selection` or `sort`),
`row`, `column`, `sortColumn`, and `sortDirection`. The numeric event value
and `row` in the JSON and typed snapshot report the selected row key. Zero
means no selected row. The legacy setter assigns one-based row keys, so its
event values keep their original meaning. Columns and sort columns stay
one-based. The application can supply a reordered matrix after a step to apply
a sort request.

### Replace DataTable rows without losing the viewport

Use `cui_bridge_data_table_set_keyed` for a retained DataTable with changing
rows. Assign each logical row a distinct nonzero `uint32_t` key and reuse that
key wherever the row appears in later updates. Call the setter on the bridge's
UI thread between steps. Headers, cells and keys are copied during the call.

```c
#include <cui/cui_bridge.h>

cui_bridge_status update_inbox(cui_bridge_app *app, cui_bridge_node table,
                               const char *const *cells, const uint32_t *keys,
                               uint32_t row_count) {
    const char *headers[] = {"Subject", "Sender"};
    return cui_bridge_data_table_set_keyed(app, table, headers, 2, cells, keys,
                                          row_count);
}
```

Pass a flat row-major array of two strings per row to the example and one
key per row. For an empty table, pass zero rows with NULL cells and keys.
The limits are 32 rows, 16 columns and 128 UTF-8 bytes per header or cell.
Invalid keys, invalid data or allocation failure preserve the previous table.

A successful replacement keeps the first visible row at its previous offset
where scroll bounds allow. If its key disappears, the former row index is
clamped to the new data. Selection and keyboard navigation follow surviving
keys and column positions. A removed selection clears, and the existing sort
request remains if its column position survives. The setter queues no change
event. Cell text and bounds lookups use zero-based indices in the current
matrix. Step the bridge before reading the new layout bounds.

The legacy `cui_bridge_data_table_set` uses keys 1 through row count and resets
the retained table state. Switching to keyed updates can retain matching old
keys. Keyed updates are available in the
[Python](BINDING_BUILDERS.md#keep-a-python-datatables-scroll-position-during-updates),
[JavaScript/TypeScript](BINDING_BUILDERS.md#keep-a-javascript-or-typescript-datatables-scroll-position-during-updates),
[Go](GO_BUILDERS.md#keep-the-visible-datatable-row-during-a-go-update),
and [Rust](RUST_BUILDERS.md#keep-the-visible-datatable-row-during-a-rust-update)
builders.

### Load another DataTable page

Bind the initial rows, or an empty matrix with column headers, then call
`cui_bridge_data_table_configure_load_more_v1(app, table, 1, threshold)`.
Use a threshold of 0 through 32 remaining rows. With 0, the table raises
an edge when the visible window reaches the end. Paging defaults to off.

After a successful step, call `cui_bridge_data_table_poll_load_more_v1`
with a `cui_bridge_data_table_load_more_v1` output and its exact size.
`CUI_BRIDGE_OK` yields one copied record. `CUI_BRIDGE_EMPTY` means there
is no request to claim. Neither polling nor replacing rows dispatches an
event. Drain the regular event queue separately. The frame number,
row count, visible index range, last row key, and bridge capacity describe
the table when it raised the edge.

Keep one in-flight request per data source. Use your own generation and
service cursor to identify its response. Set paging off during loading to
suppress subsequent edges while keeping the existing rows interactive.
On an accepted completion, use keyed replacement to retain the visible row
and selection. Turn paging on again for another page, or leave it off when
the source is exhausted. To retry, cancel the old application request and
enable paging again. This cancellation invalidates the logical response.
How to stop network I/O belongs to the transport.

A successful replacement clears any unclaimed edge and keeps the policy.
Changing the policy or disabling the table also clears the edge. Removing
the table retires its handle. A failed step discards edges first raised in
that step. A full 32-row table cannot request another page. The application
owns page delivery.
[Python paging example](BINDING_BUILDERS.md#page-a-python-datatable) ·
[JavaScript and TypeScript paging examples](BINDING_BUILDERS.md#page-a-javascript-or-typescript-datatable) ·
[Go paging example](GO_BUILDERS.md#page-a-go-datatable) ·
[Rust paging example](RUST_BUILDERS.md#page-a-rust-datatable)

The [offline inbox](../bindings/bridge/examples/data_table_paging.c) scrolls
to the end, drives row selection while loading, retries, ignores the old
response, and appends a final page without moving the selected row. From the
repository root:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_data_table_paging_example
python scripts/test_local.py --build-dir build -R '^bridge_data_table_paging_example$'
```

The visual families above expose their documented default variants. Avatar is
an initials avatar with no image source. Marker is a single text child; Shimmer
is a single line. The bridge has no arbitrary C content callback entrypoint.
Scrollable currently uses a vertical viewport and caller-supplied extents.
All strings are copied and all event payloads are snapshots; none of these
limited variants implies full source component parity.

SDL applications can call `cui_bridge_set_font_fallbacks` with up to 32 explicit
UTF-8 font paths. The native font loader opens an owned replacement chain
transactionally; an invalid path leaves the old chain intact. A render failure
after backend creation returns `CUI_BRIDGE_CORE_ERROR` rather than reporting
the backend unavailable. The optional native regression test uses
`CUI_TEST_FONT_PATH` for a primary font without ✓ and
`CUI_TEST_FALLBACK_FONT_PATH` for a font with ✓; run it under isolated Xvfb.

## Bounded typed root shadows

See [typed root shadow authoring](BRIDGE_SHADOWS.md) for the six-kind implementation, owned values, canonical CSS/preset factories and explicit clear/reset behavior. Focused portable runtime tests cover this subset; integration remains pending. This does not expose compound parts or portals.

For bounded semantic ROOT/MAIN/indicator/track/thumb authoring, read
[typed part shadows](BRIDGE_PART_SHADOWS.md). Slider has no thumb; owner topology,
whole-group naming and separate reset are explicit parts of that contract.
