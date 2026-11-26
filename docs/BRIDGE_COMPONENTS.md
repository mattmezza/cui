# C bridge component coverage

The bridge ABI remains version 1. Existing numeric enum values, option layout,
event layout, and functions are unchanged. New kinds are appended.

| Kind | Native CUI renderer | State and events | Current limit |
| --- | --- | --- | --- |
| Row, Column | `cui_begin_box` | gap | One root; depth 128 |
| Label | `cui_text` | copied UTF-8 text | Text only |
| Button | `cui_button_ex` | copied text, click | Existing options |
| Checkbox | `cui_checkbox` | checked, change, disabled | Boolean only |
| Radio | `cui_radio_present` | checked, change, disabled, optional animated selection | Standalone radio; use RadioGroup for exclusivity |
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
| RadioGroup | `cui_radio_group_present_with_content` | selected Item, description, change, disabled, optional animated selection and decorative children | Up to 64 Items |
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
| Pagination | `cui_pagination` | current page, total, change, disabled | One-based page, total <= 1,000,000. The retained bridge and language bindings add sizing, wrapping, page budgets, jump menus and localized navigation. [C](#configure-a-retained-pager-from-c) · [Python](BINDING_BUILDERS.md#configure-pagination-from-python) · [JS/TS](BINDING_BUILDERS.md#configure-pagination-from-javascript-and-typescript) · [Go](GO_BUILDERS.md#configure-pagination-from-go) · [Rust](RUST_BUILDERS.md#configure-pagination-from-rust). |
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
| DataTable | `cui_data_table` | Copied matrix, virtualized viewport, optional keyed replacement, row/column/cell selection, sortable headers, cell/state/bounds queries, copied CHANGE snapshots and optional near-end polling | Up to 32 rows through ordinary setters or 10,000 via opt-in C, Python and JS/TS APIs, 16 columns and 128 UTF-8 bytes per header or cell, caller-owned sorting and page delivery |
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

### Animate retained Radio selection from C

Pass a Radio or RadioGroup handle to `cui_bridge_radio_set_motion_v1` to enable
the theme spring for the selection mark and circular fill. RadioGroup owns the
motion for all of its options. The selected handle and its queued change event
are available on the activation frame. The fade never queues additional events.

```c
#include <cui/cui_bridge.h>

cui_bridge_status animate_delivery(cui_bridge_app *app, cui_bridge_node radio_group) {
    return cui_bridge_radio_set_motion_v1(app, radio_group, 1);
}
```

The first frame after enabling snaps to the selected value. Repeated enable
calls preserve active motion. Passing `0` snaps the next frame to the current
value and frees the node's motion state. Removing the node also frees that
state. The bridge allocates a slot index on first use and one bounded state per
enabled node. In an application that calls `cui_bridge_step` directly, keep
passing elapsed time on successive frames. A dynamically loaded provider must
export this optional symbol for the setting. Native hosts can pass `0` when
their own preference handling calls for reduced motion.

Python, JavaScript and TypeScript expose `.animate_selection()` on `radio()` and
`radio_group()` before or after mounting. If your app supports older bridge
releases, pass `app.supports_radio_motion()` as the argument to opt in only
when supported. Pass false to honor your app's reduced-motion setting. Trying
to enable motion against an older provider fails before mounting any nodes,
and the same tree can be mounted after turning the option off.

The delivery form updates the estimated arrival in its existing `on_change`
callback. The Python example runs with an installed binding and matching bridge.
Omit the font argument for a headless check.
[Python delivery form](../bindings/python/examples/radio_delivery.py)

```sh
python bindings/python/examples/radio_delivery.py
python bindings/python/examples/radio_delivery.py --font third_party/inter/Inter-Regular.otf
```

The JavaScript checkout example uses the SDL3-backed Node addon to open the same
form. Build with CMake, Ninja, Node development headers and the native SDL3/text
dependencies in the [native starter](NATIVE_APP_STARTER.md). Omit the font argument
for a headless check. Node 25 also runs the TypeScript version headlessly.
[JavaScript delivery form](../bindings/node/examples/radio-delivery.cjs) ·
[TypeScript delivery form](../bindings/typescript/examples/radio_delivery.ts)

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=ON
cmake --build build-node --target cui_node_bindings
node bindings/node/examples/radio-delivery.cjs third_party/inter/Inter-Regular.otf
node bindings/typescript/examples/radio_delivery.ts
```

Go builders opt in with `AnimateSelection(enabled)` after checking
`app.SupportsRadioMotion()`. Use `SetRadioMotion(enabled)` to handle a mounted
update or a fallback directly. The Go delivery form can run headlessly or open
an SDL window. [Go guide and runnable form](GO_BUILDERS.md#animate-radio-selection-from-go)

Rust uses `app.supports_radio_motion()` and `Builder::animate_selection(enabled)`.
Use `app.animate_selection(node, enabled)` to update a mounted control. Its
delivery form also supports headless and native execution.
[Rust guide and runnable form](RUST_BUILDERS.md#animate-radio-selection-from-rust)

### Describe retained Radio options

Set copied supporting text on a RadioGroup's direct Item with
`cui_bridge_set_detail_text(app, item, text)`. It appears under the label and
is published as the option's semantic description. Pass an empty string to
remove it. The whole row remains one radio target and selection produces the
same queued change event.

Use `item.detail(text)` in Python, JavaScript or TypeScript, `item.Detail(text)`
in Go, and `app.detail_text(item, text)` or `Builder::detail_text(text)` in
Rust. The delivery form examples above include a described option. Earlier
bridge releases accept the same Item detail setter and keep ordinary
RadioGroup labels.

### Compose retained Radio option content

Enable decorative children on a direct RadioGroup Item from C with
`cui_bridge_radio_item_set_content_v1`. This example adds a tag below an
existing delivery option. Set the Item's label and detail for the option's
accessible name and description.

```c
#include <cui/cui_bridge.h>

cui_bridge_status add_recommendation(cui_bridge_app *app, cui_bridge_node item) {
    cui_bridge_node tag;
    cui_bridge_status status =
        cui_bridge_node_create(app, item, CUI_BRIDGE_TAG, "recommended", &tag);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = cui_bridge_set_text(app, tag, "Recommended");
    if (status == CUI_BRIDGE_OK)
        status = cui_bridge_radio_item_set_content_v1(app, item, 1);
    if (status != CUI_BRIDGE_OK)
        cui_bridge_node_remove(app, tag);
    return status;
}
```

Use Row and Column to lay out Label, plain Badge, Avatar, Kbd, Tag, Icon and
Image nodes. The entire rendered row is a single radio target with one focus
stop and the group's usual selection event. Decorative children only paint
and contribute layout. Leave interactive controls outside the Item and put
meaningful decoration text in the Item's label or detail.

Children are hidden by default. Enabling with an unsupported descendant
returns `CUI_BRIDGE_INVALID_STATE` and leaves the Item unchanged. An enabled
Item rejects attempts to create an unsupported descendant. Passing `0` hides
the children while keeping their handles valid. Use `cui_bridge_node_bounds`
after a step to measure a visible decoration.

This is an optional C bridge symbol. Probe its availability when loading a
provider dynamically.

Python, JavaScript and TypeScript use `item.decorate_content()`. Check
`app.supports_radio_item_content()` for compatibility with earlier providers.
The delivery forms attach a recommendation tag to the Priority item and
update the estimate through the group's existing `on_change` callback.
[Python](../bindings/python/examples/radio_delivery.py) ·
[JavaScript](../bindings/node/examples/radio-delivery.cjs) ·
[TypeScript](../bindings/typescript/examples/radio_delivery.ts)

```python
priority = (
    item("priority", "Priority")
    .detail("Tracked delivery. Recommended.")
    .children(tag("recommended", "Recommended"))
    .decorate_content(app.supports_radio_item_content())
)
```

Rust supports the same capability probe and provides
`Builder::decorate_content(enabled)` and `app.decorate_content(item, enabled)`.
[Runnable Rust delivery form](RUST_BUILDERS.md#compose-radio-options-from-rust)

Go uses `item.DecorateContent(enabled)` and
`item.SetRadioItemContent(enabled)`. Check `app.SupportsRadioItemContent()`
for the provider capability. [Runnable Go delivery form](GO_BUILDERS.md#compose-radio-options-from-go)

For arbitrary frame-local decorative drawing, use the
[immediate C presenter](RADIO_GROUP.md#add-descriptions-and-custom-content).

### Add tooltips to retained radio options

Pass a standalone Radio or a direct RadioGroup Item to
`cui_bridge_radio_set_tooltip_v1`. The bridge copies the UTF-8 hint and keeps
its hover state. A short hint about the Priority delivery option can be added
after creating the Item.

```c
#include <cui/cui_bridge.h>

cui_bridge_status add_priority_hint(cui_bridge_app *app, cui_bridge_node priority) {
    return cui_bridge_radio_set_tooltip_v1(app, priority, "Track your parcel online", 350);
}
```

The text limit is 256 UTF-8 bytes. Use a brief label that fits in a compact
tooltip. The delay is in milliseconds, with zero opening on the first eligible
frame and 10,000 the maximum. Pass NULL or empty text to clear the hint.
Setting new text preserves the current open state and updates any pending
delay. Errors leave the previous hint in place. Call from the app's owner
thread between steps. The bridge frees the hint when cleared or when its
option is removed.

The tooltip follows hover or keyboard focus. A tap can also open it when the
host focuses the option. Escape and disabled state dismiss it. Placement uses
the current window bounds. An Item's explicit detail text takes precedence as
its accessible description. Tooltip visibility adds no queued notifications
and leaves the existing selection callback and payload intact.

For a dynamically loaded bridge, check for the optional
`cui_bridge_radio_set_tooltip_v1` symbol.

In Python, check `app.supports_radio_tooltip()` and call
`priority.radio_tooltip("Track your parcel online", delay_ms=350)`. You can
set or clear the hint before mounting or between steps. Pass `None` or an
empty string to clear it. On earlier providers, skip the optional hint so the
same form mounts normally. The runnable delivery example uses this pattern.
[Python delivery form](../bindings/python/examples/radio_delivery.py)

JavaScript, TypeScript, Go and Rust do not yet expose this setter.

### Configure a retained pager from C

The bridge keeps the page on a `CUI_BRIDGE_PAGINATION` node. Set its total and initial value through `cui_bridge_set_total` and `cui_bridge_set_value`. Use the shared design-size setter for XS through 9XL and the V1 configuration for navigation and labels. For a dynamically loaded provider, probe the optional Pagination V1 symbols and check the version bit and exact layout size before using the extended options. A configured pager or one with an explicit design size or custom height uses the extended theme paint, including the current-page outline and pointer feedback. The default retained pager keeps the original paint.

```c
#include <cui/cui_bridge.h>

cui_bridge_status configure_results_pages(cui_bridge_app *app, cui_bridge_node pager,
                                          float content_width) {
    cui_bridge_pagination_config_v1 options = {
        .struct_size = sizeof options,
        .schema_version = CUI_BRIDGE_PAGINATION_V1,
        .flags = CUI_BRIDGE_PAGINATION_CHEVRONS | CUI_BRIDGE_PAGINATION_JUMP_MENU,
        .visible_pages = 5,
        .available_width = content_width,
        .group = "Pages de résultats",
        .previous = "Précédente",
        .next = "Suivante",
        .earlier_pages = "Pages précédentes",
        .later_pages = "Pages suivantes",
    };
    cui_bridge_status status = cui_bridge_pagination_configure_v1(app, pager, &options);
    if (status == CUI_BRIDGE_OK)
        status = cui_bridge_set_design_size(app, pager, CUI_BRIDGE_SIZE_BASE);
    return status;
}
```

Run configuration on the bridge owner thread outside `cui_bridge_step`. Supply the current container's inner width in logical pixels if it should wrap, and reconfigure when that width changes. A zero width uses one row. A control wider than the width keeps its natural size. The jump menu places itself in the current host viewport and is available in numbered mode. `CUI_BRIDGE_PAGINATION_COUNTER` selects a page count. `CUI_BRIDGE_PAGINATION_ICON_ONLY` selects just Previous and Next SVG controls with accessible names and takes priority over the other flags.

The bridge copies each label, up to 127 UTF-8 bytes plus NUL. Empty fields use English defaults. Labels are single-line and control characters are rejected. Configuration storage is allocated only on use, with one 916-byte configuration per configured node plus a single pointer table bounded by the app's node capacity. Invalid updates and allocation failures preserve the prior configuration. Read it back with a size- and version-initialized `cui_bridge_pagination_get_config_v1` output. Passing NULL to the configure call clears the override. Size, value and total use their independent setters. Both chevron layouts use the current bridge viewport for delayed mouse and pen hover labels. Touch and keyboard can still navigate using the same accessible names.

Set a custom height with `cui_bridge_pagination_set_custom_size(app, pager, 52.5f)` and read it back with `cui_bridge_pagination_get_custom_size`. These optional provider exports take a plain C `float`. Passing zero restores the named size. The V1 copied configuration and named size setter are independent and can be changed while the custom height is active. The Python, JS/TS, Go and Rust builders discover the custom height exports independently of V1 navigation. [Sizing rules](NUMERIC.md#pagination)

The existing scalar `CUI_BRIDGE_EVENT_CHANGE` delivers the final page via the normal bridge queue. Configuring, opening or dismissing a jump menu does not add an event. A committed jump uses the same queue and value as a Next action. The frame API's synchronous typed callback remains available through [immediate C submission](NUMERIC_SELECTION_EVENTS.md). Python, JS/TS, Go and Rust expose the V1 configuration with their existing page-change callbacks.

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
The ordinary setter accepts up to 32 rows, 16 columns and 128 UTF-8 bytes per
header or cell.
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

### Retain more than 32 DataTable rows in C

Query `cui_bridge_data_table_max_row_capacity_v1()` and choose a nonzero
row capacity at least as large as the submitted row count. Pass the capacity
along with each row-major snapshot to
`cui_bridge_data_table_set_keyed_capacity_v1`. Both V1 symbols must be available
when loading the bridge dynamically. The C provider supports capacities up to
10,000 rows. See the corresponding language APIs for
[Python](BINDING_BUILDERS.md#retain-more-than-32-rows-in-python),
[JavaScript and TypeScript](BINDING_BUILDERS.md#retain-more-than-32-rows-in-javascript-or-typescript),
[Go](GO_BUILDERS.md#retain-more-than-32-rows-in-go), and
[Rust](RUST_BUILDERS.md#retain-more-than-32-rows-in-rust).

The setter copies headers and cells of up to 128 UTF-8 bytes each, with up to
16 columns. Pass zero rows and NULL cells and keys for an empty table. The
bridge allocates for the submitted rows even when capacity is much larger.
Call on the owner thread between steps, just as with the ordinary keyed setter.

The app shares a 32 MiB budget across opted-in matrices. This covers CUI's
allocation requests for owned matrix metadata, row and cell views, copied
UTF-8 with terminators, and Base indexes. Allocator overhead, application input
arrays and ordinary 32-row matrices sit outside that budget. While staging a
replacement the old allocation stays live. The opted-in allocations can
therefore briefly total up to 64 MiB. Replacing a table credits its old charge
when checking the retained budget. Removing a table, switching to an ordinary
setter, or destroying the app releases its charge.

An invalid count, key or string returns `CUI_BRIDGE_INVALID_ARGUMENT`. A
request beyond the provider's maximum or available app budget returns
`CUI_BRIDGE_CAPACITY`. An allocator failure returns `CUI_BRIDGE_OUT_OF_MEMORY`.
Each failure leaves the old matrix, selection, scroll anchor, sort and paging
edge intact. A successful keyed update preserves the surviving selections and
the viewport under the ordinary keyed rules, and clears stale paging demand.

Run `bridge_data_table_paging_example --larger` for an offline inbox that
grows from 8 to 40 rows and keeps its selection and position. The command
below runs that scenario through the private-display test wrapper.

~~~sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_data_table_paging_example
python scripts/test_local.py --build-dir build -R '^bridge_data_table_capacity_example$'
~~~

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
row count, visible index range, last row key, and active row capacity describe
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
that step. Demand stops at the active capacity. The application owns page
delivery.
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
