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
| Collapsible | `cui_collapsible`, optional `cui_collapsible_present` | open state, change, disabled | Child subtree is panel. [Retained C presentation](#style-retained-accordion-and-collapsible-from-c). |
| Alert | `cui_alert` | copied title/message | Static informational alert |
| RadioGroup | `cui_radio_group_present_with_content` | selected Item, description, change, disabled, optional animated selection and decorative children | Up to 64 Items |
| Tabs | `cui_tabs` | selected Item, change, disabled | Up to 64 Items; Item subtree is panel |
| Accordion | `cui_accordion`, optional `cui_accordion_present` | per-Item open state, change, disabled | Up to 64 Items. [Retained C presentation and opening policy](#style-retained-accordion-and-collapsible-from-c). |
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
| Rating | `cui_rating`, `cui_rating_present` | Value, maximum, queued change and disabled. Opt in to half steps, shared sizing, color and SVG masks. | Maximum 10. [Retained C example](#configure-a-retained-rating-from-c). |
| Stepper | `cui_stepper`, `cui_stepper_visual_navigation`, `cui_stepper_present` | Selected Item, status, aggregate change, disabled, shared size and copied RGBA Item icons. Choose fixed connectors or fill with an explicit axis. | Up to 64 Items. Visual selection paints on the next submission. Synchronous controlled requests and custom indicator palettes use the immediate C API. |
| DescriptionList | `cui_description_list` | Item label/value pairs | Up to 64 Items, default columns |
| Scrollable | `cui_scrollable_begin/end_ex` | child tree, viewport/extents, wheel change | Vertical viewport |
| StatusBar | `cui_status_bar` | Item labels in left/center/right regions | Set Item step status to 0/1/2 for region |
| Toolbar | `cui_toolbar` | Item child controls, inherited disabled state | Up to 64 Items; child controls own actions |
| TitleBar | `cui_title_bar` | Item text labels, retained title state | No native window host controls |
| Icon | `cui_icon_named`, optional `cui_icon` | Legacy catalog name and dimensions, or a shared owned resource and full copied Icon options | An unknown legacy name fails the frame. A configured Icon reports its own artwork error separately. [C presentation guide](BRIDGE_ICON_PRESENTATION.md). |
| Message | `cui_message` | Copied body/header, width | Incoming filled bubble, one body |
| Attachment | `cui_attachment` | Copied labels, transfer status and fractional progress. Optional named sizes, metadata styles, axis and decoded previews in C and the language builders | Retained C can queue built-in Open, Remove and Retry. The application owns file operations. [Presentation and examples](#configure-a-retained-attachment-from-c) · [Actions](ATTACHMENT_EVENTS.md#retained-actions) |
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

JavaScript and TypeScript use the same capability probe and take the optional
delay as the second positional argument. Pass `null` or an empty string to
clear the hint.

```js
if (app.supports_radio_tooltip()) {
  priority.radio_tooltip('Track your parcel online', 350);
}
```

[JavaScript delivery form](../bindings/node/examples/radio-delivery.cjs) ·
[TypeScript delivery form](../bindings/typescript/examples/radio_delivery.ts)

Rust exposes `Builder::radio_tooltip(text, delay_ms)` and
`app.radio_tooltip(option, text, delay_ms)`. Supply `Some(text)` to set a
hint and `None` to clear it. Probe `app.supports_radio_tooltip()` before
opt-in for older providers. [Rust delivery form](RUST_BUILDERS.md#add-radio-tooltips-from-rust)

Go offers `option.RadioTooltip(text, delayMS...)` with a 350 ms default.
Use `option.SetRadioTooltip(text, delayMS...)` to handle a mounted update
error, and pass an empty string to clear. Check `app.SupportsRadioTooltip()`
before opting in. [Go delivery form](GO_BUILDERS.md#add-radio-tooltips-from-go)

### Set signed Radio tab order

Give a standalone Radio or a RadioGroup a signed index. Lower values come
first. Equal values follow submission order, with zero as the default. The
group has one Tab stop, which follows the focused or selected Item. Arrow keys
still follow Item order. Setting or clearing an index preserves selection and
the existing queued callbacks.

```c
#include <cui/cui_bridge.h>

cui_bridge_status prioritize_delivery(cui_bridge_app *app, cui_bridge_node delivery) {
    return cui_bridge_radio_set_tab_index_v1(app, delivery, 1, -2);
}
```

Call between steps on the app's owner thread. Pass `enabled = 0` to restore
default order. The setter stores the value on the owner and the next frame
submits it. A direct Item inherits the group's order. Disabled targets remain
ineligible. If you load the bridge dynamically, probe for this optional symbol.

In Python, call `delivery.tab_index(-2)` on a builder or a mounted widget
and `delivery.tab_index(None)` to clear. Use `app.supports_radio_tab_index()`
before opting in when using a dynamically loaded provider.

```python
if app.supports_radio_tab_index():
    delivery.tab_index(-2)
```

JavaScript and TypeScript use the same probe and `delivery.tab_index(-2)`.
Pass `null` to restore default order. All three builders validate the
signed 32-bit range and check support before attaching an opted-in tree.

The delivery forms put **Reset selection** above the group. With the opt-in,
Tab enters the group first, the arrows choose a service, and the next Tab
reaches Reset. [Python](../bindings/python/examples/radio_delivery.py) ·
[JavaScript](../bindings/node/examples/radio-delivery.cjs) ·
[TypeScript](../bindings/typescript/examples/radio_delivery.ts) ·
[Go](GO_BUILDERS.md#set-radio-tab-order-from-go) ·
[Rust](RUST_BUILDERS.md#set-radio-tab-order-from-rust)

### Style retained Radio states from C

Use `cui_bridge_radio_set_paint_v1` on a Radio, a RadioGroup, or one of the
group's direct Items. The bridge copies the supplied record. A dynamically
loaded provider must export this optional symbol. Python, JavaScript,
TypeScript, Go and Rust use the same copied layers.

This delivery form gives the group an inset frame and highlights the selected
Priority option. Pass the RadioGroup handle as `group` and its Priority Item
handle as `priority` after creating both with `cui_bridge_node_create`.

```c
#include <cui/cui_bridge.h>

cui_bridge_status style_delivery(cui_bridge_app *app, cui_bridge_node group,
                                 cui_bridge_node priority) {
    cui_bridge_radio_paint_v1 frame = {
        .struct_size = sizeof frame,
        .schema_version = 1,
        .fields = CUI_BRIDGE_RADIO_ROOT_BG | CUI_BRIDGE_RADIO_ROOT_RADIUS |
                  CUI_BRIDGE_RADIO_ROOT_PADDING | CUI_BRIDGE_RADIO_ROOT_GAP,
        .root_background_rgba = 0x1a2330ff,
        .root_radius = 10,
        .root_padding = 12,
        .root_gap = 8,
    };
    cui_bridge_status status = cui_bridge_radio_set_paint_v1(
        app, group, CUI_BRIDGE_RADIO_PAINT_BASE, &frame);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_bridge_radio_paint_v1 base = {
        .struct_size = sizeof base,
        .schema_version = 1,
        .fields = CUI_BRIDGE_RADIO_ROOT_RADIUS | CUI_BRIDGE_RADIO_ROOT_PADDING,
        .root_radius = 6,
        .root_padding = 8,
    };
    status = cui_bridge_radio_set_paint_v1(app, priority, CUI_BRIDGE_RADIO_PAINT_BASE, &base);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_bridge_radio_paint_v1 checked = {
        .struct_size = sizeof checked,
        .schema_version = 1,
        .fields = CUI_BRIDGE_RADIO_ROOT_BG | CUI_BRIDGE_RADIO_LABEL_COLOR,
        .root_background_rgba = 0x284664ff,
        .label_rgba = 0xedf6ffff,
    };
    return cui_bridge_radio_set_paint_v1(
        app, priority, CUI_BRIDGE_RADIO_PAINT_CHECKED, &checked);
}
```

Each call replaces one layer and leaves the other layers in place. Fields
selected by the mask override the resolved theme, and all others inherit the
previous layer. Color values use numeric `0xRRGGBBAA`, so zero explicitly clears
a color. Pass NULL or a valid record with no fields to remove a layer.

Enabled options apply base, checked, hovered, pressed and focused in that
order. Disabled options apply base, checked and disabled. A RadioGroup applies
base and disabled to its frame. Set individual option layers on its direct
Items. The base layer owns sizing, padding, radii, text size and weight. Layers
for changing states can edit colors and border widths. Root and indicator
border widths range from 0–64. Radii, padding and gaps range from 0–256.
Indicator extent ranges from 4–128 and text size from 1–128, in logical
pixels. Item descriptions keep their existing accessible text.

Motion still controls mark opacity. An explicit indicator fill overrides the
theme's fill interpolation for that layer. Shared part shadows, option
tooltips, keyboard order and queued selection notifications continue to work.
Update styles between steps on the owner thread. Invalid records and
allocation failure preserve the previous settings. Each styled node uses
528 bytes of paint storage, freed when its final layer is cleared or the
node is removed.

### Style Radio states in language builders

The delivery examples style a RadioGroup frame and one or more option rows.
Selecting a service still dispatches the existing change event and updates the
estimated arrival. Use the builder version from this checkout with a matching
bridge. With an older bridge, skip optional styles using the capability probe.

**Python**

```python
from cui_bridge import RadioPaint, RadioPaintLayer

if app.supports_radio_paint():
    delivery.radio_paint(RadioPaintLayer.BASE, RadioPaint(root_padding=8))
    priority.radio_paint(RadioPaintLayer.CHECKED,
                         RadioPaint(root_background_rgba=0x365C85FF, label_rgba=0xEAF4FFFF))
```

Pass `None` or an empty `RadioPaint()` to clear that layer. The same
method works on detached builders and mounted widgets.
[Runnable Python delivery form](../bindings/python/examples/radio_delivery.py)

**JavaScript and TypeScript**

```js
const {RadioPaintLayer} = require('@cui/native');

if (app.supports_radio_paint()) {
  delivery.radio_paint(RadioPaintLayer.BASE, {rootPadding: 8});
  priority.radio_paint(RadioPaintLayer.CHECKED, {rootBackground: 0xeaf1ffff});
}
```

TypeScript exposes the same methods and the `RadioPaint` type. Pass
`null` or an empty object to clear a layer.
[JavaScript delivery form](../bindings/node/examples/radio-delivery.cjs) ·
[TypeScript delivery form](../bindings/typescript/examples/radio_delivery.ts)

**Go**

```go
supported, err := app.SupportsRadioPaint()
if err != nil {
    return err
}
if supported {
    return priority.SetRadioPaint(cui.RadioPaintChecked, cui.RadioPaint{
        Fields: cui.RadioPaintRootBackground,
        RootBackground: 0xeaf1ffff,
    })
}
return nil
```

Use `RadioPaint(layer, paint)` to chain on a builder and `ClearRadioPaint(layer)`
to clear a layer. The fields mask distinguishes explicit zero from inheritance.
[Runnable Go delivery form](../bindings/go/examples/radio_delivery/main.go)

**Rust**

```rust
use cui_builders::{RadioPaint, RadioPaintLayer};

if app.supports_radio_paint() {
    app.radio_paint(priority, RadioPaintLayer::Checked, Some(&RadioPaint {
        root_background_rgba: Some(0xeaf1ffff),
        ..RadioPaint::default()
    }))?;
}
```

Rust accepts `Some(0)` as explicit zero and `None` as inheritance in each
field. Pass `None` as the paint argument to clear the layer. Rust creates
builder nodes immediately, so check support before adding paint if the
application can load older providers.
[Runnable Rust delivery form](../bindings/rust/examples/radio_delivery.rs)

Run each example from the repository root. Use the setup commands under
[Animate retained Radio selection from C](#animate-retained-radio-selection-from-c)
to build the matching provider. Omit the font argument to run headlessly.
The TypeScript example uses Node's TypeScript execution support.

### Configure a retained Attachment from C

Set the filename, detail, width, transfer status and fractional progress with
the ordinary bridge setters. Call `cui_bridge_attachment_configure_v1` to opt
into one-line metadata with ellipsis and full semantic labels. An empty title
or detail is omitted. All nonempty labels must be single lines while the
presentation is configured. Use `cui_bridge_set_design_size` with the shared
`XS` through `9XL` scale or `INHERIT` for the original density. Width set
through `cui_bridge_set_visual_size` overrides the default width.

```c
cui_bridge_status configure_preview(cui_bridge_app *app, cui_bridge_node file,
                                    const uint8_t *rgba, uint32_t width, uint32_t height,
                                    uint32_t stride, uint32_t bytes) {
    const cui_bridge_attachment_config_v1 presentation = {
        .struct_size = sizeof presentation,
        .schema_version = 1,
        .flags = CUI_BRIDGE_ATTACHMENT_VERTICAL_V1 | CUI_BRIDGE_ATTACHMENT_MEDIA_V1,
        .media_aspect = 16.0f / 9.0f,
        .title = {
            .flags = CUI_BRIDGE_ATTACHMENT_TEXT_STYLE_V1,
            .weight = 2, /* Semibold. */
        },
    };
    cui_bridge_status status = cui_bridge_attachment_configure_v1(app, file, &presentation);
    if (status != CUI_BRIDGE_OK)
        return status;
    return cui_bridge_attachment_set_preview_rgba_v1(
        app, file, rgba, width, height, stride, bytes);
}
```

The host provides the decoded RGBA8 rows. The bridge copies at most 16 MiB
per preview. Replacing or clearing it releases the old copy. Pass `NULL` and
four zero sizes to clear the pixels. With `MEDIA` set, the slot stays visible
for lifecycle feedback before pixels arrive. Vertical media uses the supplied
aspect or defaults to square. At very narrow widths the preview may collapse
while metadata labels remain available to assistive technology.

A value of `0.4` represents 40% during Uploading. Set `INDETERMINATE` to omit
the percentage and show an indeterminate media indicator. Use the child status
flags to let the title or description reflect a different stage from the card.
A text record can specify a size or color independently of its font style.
Color includes explicit transparency. Nonzero letter spacing needs a capable
text host. A NULL configuration restores the earlier metadata renderer and
orientation. Preview ownership and named size are independent.

Probe `cui_bridge_attachment_configure_v1` and
`cui_bridge_attachment_set_preview_rgba_v1` separately when loading a bridge
dynamically. Call the setters between steps on the owner thread. The
application owns file selection, decoding, transfer state and retry.
[Immediate C Attachment](MEDIA.md#fit-file-names-and-descriptions)

### Compose a retained Attachment from C

Call `cui_bridge_attachment_slot_create_v1` between steps. Create one
container per role and give each a unique key. Use ordinary node creation for
the children. The card can use any combination of these roles.

| Role | Layout and visibility |
| --- | --- |
| `CUI_BRIDGE_ATTACHMENT_SLOT_MEDIA_V1` | Centered fallback in the preview. An installed image or lifecycle feedback can take its place. |
| `CUI_BRIDGE_ATTACHMENT_SLOT_OVERLAY_V1` | Centered content over the preview, including when an image is installed. Built-in lifecycle feedback keeps its foreground layer. |
| `CUI_BRIDGE_ATTACHMENT_SLOT_METADATA_V1` | A column after the typed title and description. |
| `CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1` | A row at the trailing end or in the upper-right corner of a vertical card. The lane fits within the card. |

Give the action row a preferred width in logical pixels, up to 4096. Pass 0
for the other roles. Use `cui_bridge_set_gap` on the slot and choose child
sizes that fit the lane. A slot with no room can be omitted. Slots inherit
ancestor clipping. App-wide Notifications belong outside the slot tree.

```c
cui_bridge_node actions, inspect;
cui_bridge_status status = cui_bridge_attachment_slot_create_v1(
    app, card, CUI_BRIDGE_ATTACHMENT_SLOT_ACTIONS_V1, "report-actions", 100, &actions);
if (status == CUI_BRIDGE_OK)
    status = cui_bridge_node_create(app, actions, CUI_BRIDGE_BUTTON, "inspect-report", &inspect);
if (status == CUI_BRIDGE_OK)
    status = cui_bridge_set_text(app, inspect, "Inspect");
```

A click on Inspect arrives with its Button handle and the usual activation
payload. The card can also opt into [built-in file actions](ATTACHMENT_EVENTS.md#retained-actions).
If both fire in a frame, child events can precede file actions. Keep handling
them through a single chosen queue reader. Disabling a slot or its card makes
the contents inert and preserves their settings. Removing a slot or its card
also removes the descendants and retires their unread events. For dynamically
loaded providers, probe the factory symbol independently. The language
builders currently expose presentation, previews and built-in actions.

Build and run the [project files example](../bindings/bridge/examples/attachment_slots.c)
to verify that Inspect updates the summary from an ordinary Button event.

```sh
cmake -S . -B build-bridge -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build-bridge --target bridge_attachment_slots_example
./build-bridge/bridge_attachment_slots_example
```

Run that example with a font path and an SDL-enabled bridge to open the
interactive native window. See [native setup](NATIVE_APP_STARTER.md) for prerequisites.

### Show an attachment preview in a language builder

Python, JavaScript and TypeScript use `attachment_options` to apply the copied
presentation and `attachment_preview_rgba` to install a decoded image. Set a
logical width with `visual_size` and choose a named size with `design_size`.
The bridge keeps the full labels for accessibility even when it paints an
ellipsis. With an older provider, probe `supports_attachment_options()`
before applying the new presentation or named size and probe
`supports_attachment_preview()` separately before setting pixels.

```python
from cui_bridge import App, AttachmentOptions, AttachmentTextStyle, attachment, column

with App(width=440, height=360) as app:
    upload = (attachment("hero", "Workspace landing page.png")
              .detail("Uploading").attachment_status("uploading")
              .value(.4).visual_size(240, 0))
    if app.supports_attachment_options():
        upload.design_size("lg").attachment_options(AttachmentOptions(
            vertical=True, media=True, media_aspect=16 / 9,
            title=AttachmentTextStyle(weight=2),
        ))
    if app.supports_attachment_preview():
        pixels = bytes([38, 84, 166, 255, 87, 187, 170, 255])
        upload.attachment_preview_rgba(pixels, 2, 1)
    app.mount(column().children(upload))
    app.step()
```

Use `attachment_options(None)` in Python or `attachment_options(null)` in
JavaScript and TypeScript to restore the earlier metadata and orientation.
`clear_attachment_preview()` releases the pixels while keeping the
presentation. `media=True` or `media: true` reserves a slot even when the
pixels are cleared. Reset and replacement preserve progress and status.

Go exposes the fluent `AttachmentOptions` and `AttachmentPreviewRGBA` methods.
Use `SetAttachmentOptions(nil)` to reset and `ClearAttachmentPreview` to clear.
Rust offers `attachment_options` and `attachment_preview_rgba` on the mounted
builder and `App::configure_attachment` and `App::clear_attachment_preview`
for updates. The Go and Rust upload examples include an application-owned
button that completes the transfer. Applications using the C bridge can opt
into built-in Open, Remove and Retry and read them from the notification queue.
Language builders cover presentation and previews. Immediate and retained C
support custom slots. [Retained C composition](#compose-a-retained-attachment-from-c)

From the repository root, build the headless C bridge and Node addon,
then run the examples:

```sh
cmake -S . -B build-bridge -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build-bridge --target cui_bridge
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build-node --target cui_node_bindings

export CUI_BRIDGE_LIBRARY="$PWD/build-bridge/libcui_bridge.so"
export CUI_BRIDGE_LIB_DIR="$PWD/build-bridge"
export CUI_NODE_ADDON="$PWD/build-node/cui.node"
PYTHONPATH=bindings/python python bindings/python/examples/attachment_options.py
node bindings/node/examples/attachment-options.cjs
node bindings/typescript/examples/attachment_options.ts
(cd bindings/go && go run ./examples/attachment_upload)
cargo run --manifest-path bindings/rust/Cargo.toml --example attachment_upload
```

The TypeScript command uses Node 25's built-in type stripping. The shared
library paths above are for Linux. See the [builder setup](BINDING_BUILDERS.md)
for prerequisites and platform-specific paths.
Omit the C bridge environment variables to use the bundled Go or Rust provider
on supported hosts. For a native SDL window, use a bridge built with SDL support
and pass `--font /path/to/font.ttf` to the Go example or
`-- /path/to/font.ttf` after the Rust example name.

See [retained Attachment actions](ATTACHMENT_EVENTS.md#retained-actions-from-language-builders)
for optional Open, Remove and Retry builders and runnable event examples.

### Style retained Accordion and Collapsible from C

Create an Accordion with direct Item children and place each panel's content
under its Item. A Collapsible owns its panel children directly. Set the
initial expansion of each Item or Collapsible with `cui_bridge_set_checked`.

```c
#include <cui/cui_bridge.h>

cui_bridge_status style_disclosure(cui_bridge_app *app, cui_bridge_node accordion,
                                  cui_bridge_node item) {
    const cui_bridge_disclosure_config_v1 card = {
        .struct_size = sizeof card,
        .schema_version = 1,
        .flags = CUI_BRIDGE_DISCLOSURE_JOINED_V1 | CUI_BRIDGE_DISCLOSURE_BORDERED_V1 |
                 CUI_BRIDGE_DISCLOSURE_SINGLE_V1 | CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1,
        .design_size = CUI_BRIDGE_SIZE_LG,
        .width = -1,
        .collapsed_icon = "plus",
        .expanded_icon = "minus",
    };
    const cui_bridge_disclosure_trigger_style_v1 style = {
        .struct_size = sizeof style,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HOVER_V1 | CUI_BRIDGE_DISCLOSURE_CURSOR_V1,
        .hover_rgba = 0x4879e02bu,
        .cursor = CUI_BRIDGE_DISCLOSURE_CURSOR_POINTER,
    };
    cui_bridge_status status = cui_bridge_disclosure_configure_v1(app, accordion, &card);
    return status == CUI_BRIDGE_OK
        ? cui_bridge_disclosure_set_trigger_style_v1(app, item, &style) : status;
}
```

Widths are `-1` to fill, `0` to fit, or positive logical pixels. The
`CUI_BRIDGE_SIZE_*` scale sets trigger and icon dimensions, and each empty
icon name uses the default chevron. Supply a name or path from the
provider's embedded catalog. Each wire field has room for 63 bytes and one
NUL. The `CUSTOM_CARD` flag accepts the outer background, border, radius,
padding and gap together, and `CLIP_CARD` clips its children. For a
standalone Collapsible, use `BORDERLESS` to remove its default trigger outline.

Set hover, press, expanded, disabled and focus colors or the cursor on any
direct Accordion Item or on a Collapsible. A named field may explicitly use
zero, including a transparent color. Style and configuration are copied
and can be cleared independently by passing NULL to the corresponding
setter. Invalid records leave installed values unchanged.

Accordion's default policy permits multiple open Items. An open Item can
close on activation with `ALLOW_COLLAPSE`. With `SINGLE`, opening an Item
closes its siblings. Explicit `set_checked` writes stay silent, even with
`SINGLE`. Configuration and paint also leave the event queue unchanged.
After stepping, use `cui_bridge_poll_event_ex` for queued CHANGE records.
Accordion emits one record per Item whose final expansion differs from
its previous value, in child order. Collapsible keeps its existing
frame-level change record. The records carry the resulting `checked`
value. Panel children follow expansion and share normal retained lifetimes.

#### Style retained headings and dividers

Call `cui_bridge_disclosure_set_heading_v1` on an Accordion to set shared
heading styles. Use the same setter on a direct Item for overrides or on a
Collapsible for its standalone heading. Each layer can supply typography,
a vertically centered row box, and an explicit title color. Normal applies
first, then Expanded when open, then Disabled when disabled. At each layer
the Item takes precedence per field. The trigger feedback setter continues
to control hover, press and the focus outline.

```c
cui_bridge_status style_preferences_headings(cui_bridge_app *app, cui_bridge_node accordion) {
    const cui_bridge_disclosure_heading_v1 heading = {
        .struct_size = sizeof heading,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY,
        .font_size = 17,
        .line_height = 24,
        .weight = 1, /* Medium. */
        .text_flags = CUI_BRIDGE_DISCLOSURE_TITLE_WRAP,
    };
    const cui_bridge_disclosure_divider_v1 divider = {
        .struct_size = sizeof divider,
        .schema_version = 1,
        .thickness = 2,
        .inset_left = 12,
        .inset_right = 12,
        .color_rgba = 0x4d80c0aau,
    };
    cui_bridge_status status = cui_bridge_disclosure_set_heading_v1(
        app, accordion, CUI_BRIDGE_DISCLOSURE_HEADING_NORMAL, &heading);
    return status == CUI_BRIDGE_OK
        ? cui_bridge_disclosure_set_divider_v1(app, accordion, &divider) : status;
}
```

Give the group a bounded width to wrap titles. Set the divider once on the
Accordion and enable `JOINED` in its display configuration to show it after
each complete section. Zero thickness removes the divider and its layout
space. NULL restores the theme divider, or clears the specified heading layer.

Both setters copy their input and preserve the existing settings on errors.
The bridge allocates heading storage only on configured nodes and releases it
after the last layer is cleared or the node is removed. Divider storage follows
the Accordion lifetime and can be cleared independently. Setters are silent
and run between steps. The wire box covers width, height, padding, gap,
corner radius, background and border. Typography uses the host default or
monospace face. Nonzero letter spacing requires host support. Application-defined
font-family IDs and arbitrary `cui_box` properties are available in immediate C.

Build and run the [workspace preferences example](../bindings/bridge/examples/disclosure_preferences.c)
from the repository root. It applies shared heading typography, title feedback
and inset dividers. Its headless mode activates Notifications, closes the Account panel
and updates a status label.

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_disclosure_preferences_example
./build/bridge_disclosure_preferences_example
```

For a native window, use a native-capable build with `CUI_BUILD_NATIVE=ON`
and pass a font file as the first argument. [Native prerequisites](NATIVE_APP_STARTER.md)

When dynamically loading providers, look up `cui_bridge_disclosure_configure_v1`
and `cui_bridge_disclosure_set_trigger_style_v1` for the original presentation
settings. Probe `cui_bridge_disclosure_set_heading_v1` and
`cui_bridge_disclosure_set_divider_v1` separately for headings and dividers.
Older providers still offer ordinary Accordion and Collapsible nodes.

#### Style retained titles for pointer and keyboard

Use `cui_bridge_disclosure_set_title_state_v1` with the same heading record to
set hover, keyboard-focus or press typography and title color. The record may
include either or both fields. Configure row geometry separately through a
normal, expanded or disabled heading. Clear a state by passing `NULL` or a
valid record with zero fields.

```c
cui_bridge_status style_preferences_focus(cui_bridge_app *app, cui_bridge_node accordion) {
    const cui_bridge_disclosure_heading_v1 focus = {
        .struct_size = sizeof focus,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_COLOR,
        .title_rgba = 0x275dccff,
    };
    return cui_bridge_disclosure_set_title_state_v1(
        app, accordion, CUI_BRIDGE_DISCLOSURE_TITLE_FOCUS_VISIBLE, &focus);
}
```

Configure an Accordion for shared styles, a direct Item for per-field
overrides or a Collapsible for its own title. States resolve in the same
order as the immediate C presenter. The bridge copies the record between
steps and releases the storage when the last state clears or the node is
removed. Expansion continues to enqueue its usual change records. Probe
`cui_bridge_disclosure_set_title_state_v1` independently of the ordinary
heading setter when loading a provider dynamically.
[Immediate C state order and sizing](DISCLOSURE.md#style-titles-for-pointer-and-keyboard)

### Style disclosure from language builders

Python, JavaScript, TypeScript, Go and Rust expose the same retained options.
Configure the Accordion as a group, or configure a Collapsible directly.
Set trigger style on a Collapsible or on an Item directly under an Accordion.
Read expansion from the ordinary checked/open state or from the copied
`checked` value delivered to your change callback.

| Language | Probe on the app | Apply or reset |
| --- | --- | --- |
| Python | `supports_disclosure_presentation()` | `widget.disclosure_configure(DisclosureConfig(...))` and `item.set_trigger_style(DisclosureTriggerStyle(...))`. Pass `None` to reset either. |
| JavaScript and TypeScript | `supports_disclosure_presentation()` | `node.disclosure_configure({...})` and `item.set_trigger_style({...})`. Pass `null` to reset either. |
| Go | `SupportsDisclosurePresentation()` | `group.ConfigureDisclosure(&config)` and `item.SetDisclosureTriggerStyle(&style)`. Pass `nil` to reset either. Draft builders also provide `DisclosureOptions(config)` and `DisclosureStyle(style)`. |
| Rust | `supports_disclosure_presentation()` | `app.configure_disclosure(group, Some(&config))` and `app.set_disclosure_trigger_style(item, Some(&style))`. Pass `None` to reset either. A mounted-node builder also provides `disclosure_options(&config)` and `disclosure_style(&style)`. |

```python
from cui_bridge import DisclosureConfig, DisclosureTriggerStyle, accordion, item, label

editor = item("editor", "Editor").children(label("Auto-save is enabled."))
sections = accordion("preferences").children(editor)
if app.supports_disclosure_presentation():
    sections.disclosure_configure(DisclosureConfig(
        width=-1, design_size="lg", joined=True, bordered=True,
        single=True, allow_collapse=True, collapsed_icon="plus", expanded_icon="minus",
    ))
    editor.set_trigger_style(DisclosureTriggerStyle(hover_rgba=0x4879E02B, cursor="pointer"))
editor.on_change(lambda event: print("Editor expanded:", event.checked))
```

Omit a trigger field to inherit. Set a field to zero for transparent paint.
Go uses pointers and Rust uses options to distinguish those values. Supply
`Card: &cui.DisclosureCard{...}` in Go, `card: Some(DisclosureCard { ... })`
in Rust, or the flat card fields with `custom_card=True` in Python and
`customCard: true` in JavaScript and TypeScript. The bridge copies the
settings and icon names. Configuration and styles reset independently and
do not change expansion or replay notifications. Older providers load and
run ordinary controls. Requesting an extended setter on an unsupported
provider reports an error.

#### Style heading layers and dividers from language builders

Python, JavaScript, TypeScript, Go and Rust support normal, expanded and
disabled heading layers, and optional hover, keyboard-focus and press title
styles. [Retained C title styles](#style-retained-titles-for-pointer-and-keyboard)
Set them on an Accordion for shared styles, its direct Item for per-field
overrides, or a Collapsible for its own heading. Probe heading and divider
support independently before configuring a provider. A divider belongs to
an Accordion and appears when its presentation uses `joined`.

| Language | App probes | Apply or reset |
| --- | --- | --- |
| Python | `supports_disclosure_heading()`, `supports_disclosure_divider()` | `widget.set_disclosure_heading(DisclosureHeadingLayer.NORMAL, heading)`, `group.set_disclosure_divider(divider)`. Pass `None` to clear. |
| JavaScript and TypeScript | `supports_disclosure_heading()`, `supports_disclosure_divider()` | `node.set_disclosure_heading("normal", heading)`, `group.set_disclosure_divider(divider)`. Pass `null` to clear. |
| Go | `SupportsDisclosureHeading()`, `SupportsDisclosureDivider()` | `node.SetDisclosureHeading(cui.DisclosureHeadingNormal, &heading)`, `group.SetDisclosureDivider(&divider)`. Pass `nil` to clear. Draft builders also provide `DisclosureHeading(layer, heading)` and `DisclosureDivider(divider)`. |
| Rust | `supports_disclosure_heading()`, `supports_disclosure_divider()` | `app.set_disclosure_heading(node, DisclosureHeadingLayer::Normal, Some(&heading))`, `app.set_disclosure_divider(group, Some(&divider))`. Pass `None` to clear. Mounted builders also provide `disclosure_heading(layer, &heading)` and `disclosure_divider(&divider)`. |

A heading can carry a `DisclosureTypography`, a `DisclosureHeadingBox`,
and a title color. Include a positive font size when supplying typography.
Wrap needs a bounded width. Set the font size and line height independently
for each layer, or omit typography to inherit it. An explicit zero title
color is transparent. Pass a divider with zero thickness to remove its
layout space. Clear the divider to restore the themed one-pixel line.

Probe title-state support separately from the heading and divider capabilities.
A `DisclosureTitleStyle` supplies title typography, an explicit color, or both.
Set a stable box through a normal heading when the title needs to keep its
hit geometry as text metrics change.

| Language | App probe | Apply or clear a title state |
| --- | --- | --- |
| Python | `supports_disclosure_title_state()` | `widget.set_disclosure_title_state(DisclosureTitleState.HOVERED, style)`. Pass `None` to clear. |
| JavaScript and TypeScript | `supports_disclosure_title_state()` | `node.set_disclosure_title_state("hovered", style)`. Pass `null` to clear. |
| Go | `SupportsDisclosureTitleState()` | `node.SetDisclosureTitleState(cui.DisclosureTitleHovered, &style)`. Pass `nil` to clear. Draft builders also provide `DisclosureTitleState(state, style)`. |
| Rust | `supports_disclosure_title_state()` | `app.set_disclosure_title_state(node, DisclosureTitleState::Hovered, Some(&style))`. Pass `None` to clear. Mounted builders also provide `disclosure_title_state(state, &style)`. |

```python
from cui_bridge import (
    DisclosureDivider, DisclosureHeading, DisclosureHeadingLayer,
    DisclosureTitleState, DisclosureTitleStyle, DisclosureTypography,
)

# Apply to the joined sections from the previous example.
if app.supports_disclosure_heading():
    sections.set_disclosure_heading(
        DisclosureHeadingLayer.NORMAL,
        DisclosureHeading(typography=DisclosureTypography(font_size=17, line_height=24, wrap=True)),
    )
    sections.set_disclosure_heading(
        DisclosureHeadingLayer.EXPANDED,
        DisclosureHeading(typography=DisclosureTypography(font_size=19, line_height=26, weight=3, wrap=True)),
    )
if app.supports_disclosure_title_state():
    sections.set_disclosure_title_state(
        DisclosureTitleState.HOVERED,
        DisclosureTitleStyle(typography=DisclosureTypography(font_size=17, line_height=24, weight=2, wrap=True)),
    )
    sections.set_disclosure_title_state(
        DisclosureTitleState.FOCUS_VISIBLE,
        DisclosureTitleStyle(title_rgba=0x275DCCFF),
    )
if app.supports_disclosure_divider():
    sections.set_disclosure_divider(DisclosureDivider(
        thickness=2, inset_left=12, inset_right=12, color_rgba=0x4D80C0AA,
    ))
```

Build your provider and follow the language setup guides to set the Python
import path, bridge library, or Node addon location. Go and Rust use bundled
sources by default. Run from the repository root without a font for a
headless click and an in-app status update. The five examples apply the heading
styles, title interaction styles and divider when supported by the provider.

```sh
PYTHONPATH=bindings/python python bindings/python/examples/workspace_preferences.py
node bindings/node/examples/workspace-preferences.cjs
node bindings/typescript/examples/workspace_preferences.ts
(cd bindings/go && go run ./examples/disclosure_preferences)
(cd bindings/rust && cargo run --example disclosure_preferences)
```

The TypeScript command needs Node with type stripping. With a native-capable
provider, pass `--font /path/to/font.ttf` in Python, a font path in JavaScript,
`-font /path/to/font.ttf` in Go or `-- /path/to/font.ttf` to the Cargo command.
Add `--features native` before the Cargo example name when building the
bundled native provider. [Python, JS and TS setup](BINDING_BUILDERS.md) ·
[Go setup](GO_BUILDERS.md) · [Rust setup](RUST_BUILDERS.md)

Immediate C also provides custom headers, measured reveal, application-defined
font families and synchronous observers. [Immediate C guide](DISCLOSURE.md)

### Configure a retained Rating from C

Create a `CUI_BRIDGE_RATING` node and set its label with `cui_bridge_set_text`.
Use the existing value and total setters. The default maximum is five, and
retained Rating accepts at most ten marks.

```c
#include <cui/cui_bridge.h>

cui_bridge_status configure_delivery_rating(cui_bridge_app *app, cui_bridge_node rating) {
    const cui_bridge_rating_config_v1 options = {
        .struct_size = sizeof options,
        .schema_version = 1,
        .flags = CUI_BRIDGE_RATING_HALF_STEPS_V1 |
                 CUI_BRIDGE_RATING_HOVER_PREVIEW_V1 |
                 CUI_BRIDGE_RATING_CLEARABLE_V1 |
                 CUI_BRIDGE_RATING_CUSTOM_FILL_V1,
        .design_size = CUI_BRIDGE_SIZE_SM,
        .width = 280,
        .fill_rgba = 0xbe3155ffu
    };
    cui_bridge_status status = cui_bridge_rating_configure_v1(app, rating, &options);
    return status == CUI_BRIDGE_OK ? cui_bridge_set_value(app, rating, 2.5) : status;
}
```

Width 0 uses the natural size. Set `outline_icon` and `fill_icon` to existing Icon
resource handles from the same app to replace either star mask. Use white
artwork for a monochrome tint. The bridge retains one reference per handle field and releases
it on option replacement, reset or node removal. Shared resources may be
updated between frames. An empty or unpreparable resource uses the default star
for that mask. The Icon resource budget and provider requirements still apply.
[Icon resource API](BRIDGE_ICON_RESOURCES.md)

The setter copies the configuration and leaves any queued change intact.
`cui_bridge_rating_get_config_v1` fills a size- and version-initialized record.
Pass NULL to the setter to restore the legacy presenter. The next step
normalizes the current numeric value to the active grid. Normalization can
queue a change as before. Hover preview leaves the value and event queue
untouched. Read-only and disabled prevent user changes.

When loading a shared provider, require the configure, get and size symbols and
verify that the reported size equals your record size before offering these
options.

### Build a feedback form with Rating

The same retained Rating options are available in Python, JavaScript,
TypeScript, Go and Rust. Use the existing change callback to update an app
label or persist the committed score. A hover preview produces no change.
With half steps enabled the callback carries a number such as `3.5`.
The examples probe support and keep whole scores on earlier providers.

| Language | Probe | Configure, read and reset |
| --- | --- | --- |
| Python | `app.supports_rating_options()` | `rating.configure(RatingConfig(...))`, `rating.get_config()`, `rating.reset_config()` |
| JavaScript / TypeScript | `app.supports_rating_options()` | `rating.configure({...})`, `rating.get_config()`, `rating.reset_config()` |
| Go | `app.SupportsRatingOptions()` | `cui.ConfiguredRating(key, config)` or `rating.ConfigureRating(config)`, `rating.RatingConfig()`, `rating.ResetRating()` |
| Rust | `app.supports_rating_options()` | `app.configured_rating(parent, key, &config)` or `app.configure_rating(node, &config)`, `app.rating_config(node)`, `app.reset_rating(node)` |

Set up Python and the Node addon with the [builder guide](BINDING_BUILDERS.md).
Run the feedback examples from the repository root. The commands exercise the
UI, commit a rating, and update the visible feedback label in headless mode.
Set `CUI_BRIDGE_LIBRARY` and `CUI_NODE_ADDON` to your provider and addon paths
when running directly from a checkout. An installed package can supply them.

```sh
PYTHONPATH=bindings/python python bindings/python/examples/rating_options.py
node bindings/node/examples/rating-options.cjs
node bindings/typescript/examples/rating_options.ts
(cd bindings/go && go run ./examples/rating_options)
(cd bindings/rust && cargo run --example rating_options)
```

The TypeScript command needs Node with TypeScript execution support. To open a
native window, use an SDL-enabled provider and pass a TrueType font path.
Python takes `--font /path/to/font.ttf`. JavaScript takes the font as its first
argument. For a bundled Linux build use `go run -tags cui_native
./examples/rating_options -font /path/to/font.ttf` or `cargo run --features
native --example rating_options -- /path/to/font.ttf` from the respective
binding directory. Use the Go or Rust example to see how to create heart
masks as shared Icon resources. Release a mask after resetting or removing
the Rating. A config snapshot reuses those resources.

[Python](../bindings/python/examples/rating_options.py) ·
[JavaScript](../bindings/node/examples/rating-options.cjs) ·
[TypeScript](../bindings/typescript/examples/rating_options.ts) ·
[Go](../bindings/go/examples/rating_options/main.go) ·
[Rust](../bindings/rust/examples/rating_options.rs)

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
