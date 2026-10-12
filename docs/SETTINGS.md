# Settings

Settings composes searchable pages, groups and editable fields with a resizable sidebar and a scrolling active page. It uses the existing Sidebar, Resizable, Switch, Checkbox, Input, NumberInput and Menu engines (with a legacy Select fallback). Custom callbacks can replace a field or an entire item, and receive the original page/group/item indices, size, layout and disabled state.

```c
bool notifications = true;
cui_setting_item items[] = {{
    .title = {"Notifications", 13}, .kind = CUI_SETTING_SWITCH,
    .boolean = &notifications, .has_default = true,
    .default_boolean = false
}};
cui_setting_group groups[] = {{
    .title = {"General", 7}, .items = items, .count = 1
}};
cui_setting_page pages[] = {{
    .title = {"Preferences", 11}, .icon = {"settings", 8},
    .groups = groups, .count = 1
}};
cui_settings_page_state workspace[1];
cui_settings_state state;
cui_settings_state_init(&state, workspace, 1, search_edit);
cui_settings_options options = cui_settings_default();
options.width = 800;
options.height = 500;
/* Inside a frame: */
cui_settings_result result = cui_settings(ui, id, &theme,
    pages, 1, &state, options);
```

The caller owns descriptors, field values, editor handles and all state. Keep these alive through rendering; callbacks execute synchronously while building the frame. Use separate state for each Settings instance. The search editor is optional and must be single-line. Without it, `options.query` supplies a controlled filter. Search text is reduced before navigation and content, so text queued in the current frame filters both immediately. Editors and popup state must be initialized using their own module APIs. Text fields borrow an Input state; numeric fields borrow a Number state and editor; source dropdown fields borrow an initialized `cui_settings_dropdown_state`, persistent Menu state and scratch arrays, described below. Existing `popup` descriptors keep the Select fallback. Optional Switch state enables its existing retained motion.

Filtering matches item titles, descriptions and keywords. Custom whole-item renderers match only keywords unless their `matches` hook supplies another policy. Page and group titles, group footers and title suffixes do not independently match. The default matcher folds ASCII case and preserves non-ASCII UTF-8 bytes; a host matching callback can provide Unicode lowercase matching. Filtered arrays retain original indices and IDs. A matching selected page/group remains selected; otherwise selection falls back to the first matching page and its page header. No-results filtering keeps selection but renders no stale page. Clearing search preserves the current selection.

The sidebar starts at 250 pixels and resizes within 160–360 by default. Page trees start collapsed unless `default_open` is set. Groups become sidebar entries only when their titles exist and the page has more than one matching group. Page activation selects its header; group activation selects the group and reveals it after measurement. Layout switches to stacked items when the content panel is at most 480 pixels wide. A vertical item override also stacks that item. Text fields use 256 pixels horizontally; NumberInput uses the source 128 pixels. Both grow when stacked. Zero width defaults to 800; negative width grows using the previous measured bounds. Zero height uses a CUI fallback of 500 rather than an unconstrained host viewport.

Medium is the default. Zero-initialized options select a page; set `default_group_selected` to select `default_group` initially. Switch, Checkbox, Input, NumberInput and dropdown triggers honor XS/S/M/L. Input and NumberInput frame heights are 20/24/32/44; the source Button trigger heights are 20/24/32/32. Text glyphs are centered inside their fixed line boxes. Settings forwards its size to custom callbacks. Source item typography uses 14-pixel labels, muted descriptions and centered 20-pixel Label line boxes (plain item descriptions use the inherited golden-ratio line height), scaled by `theme.font_size / 16`. Group surfaces support Normal, Fill and Outline, including a per-group override. Fill uses independently configurable `theme.group_box`; item titles use `theme.group_box_foreground`. Card padding is 16, item gap 16, title gap Normal16/card12 and footer gap 8 at that density. Footer callbacks receive a theme copy with small muted text defaults and render outside the surface. Disabled items paint at half opacity. Built-in controls receive disabled state; custom callbacks must honor `options.disabled` themselves.

Reset appears only when the active page permits reset and a matching item is dirty. The icon uses the named Undo2 SVG and the theme asset provider. The reset icon has a delayed hover/focus tooltip using `reset_label`, with state retained in each page workspace; Escape suppresses it until hover/focus leaves. Reset applies to every matching reset-capable item, including disabled items and unchanged matching defaults. `has_default` enables typed defaults; custom dirty/reset hooks take precedence. `on_change` runs after a built-in mutation or successful reset. Number fields keep out-of-range draft text while the setting value is clamped to the configured bounds; blur commits the clamped draft. `cui_settings_reset_page` exposes the same filtered reset policy without rendering. Text reset replaces the whole range atomically, so undo restores the original text and selection.

The state workspace capacity bounds page count to at most 4096. The current sidebar composition supports at most 256 groups per page; groups support at most 65,536 items, each with at most 4096 keywords. Invalid descriptors or nonfinite dimensions return `invalid`. Actual layout is also bounded by the core context capacity. Text edit errors are reported through `edit_error`. `cui_settings_part_id` provides deterministic IDs for headless routing; current navigation parts are page5/group6, field24, reset34 and search4. Resize and scroll controls use their owning modules' part-ID helpers.

## Checked Menu dropdown composition

Use caller-owned, per-field workspace for the source outline Button and checked Menu:

```c
cui_menu_entry menu_entries[3];
bool menu_checks[3];
cui_settings_dropdown_state menu_state;
cui_settings_dropdown_init(&menu_state, popup_id, menu_entries, menu_checks, 3);
/* On a dropdown item with three choices: */
item.dropdown_state = &menu_state;
```

All arrays and state remain alive across frames. Capacity must cover `choice_count`; missing arrays, an uninitialized popup or insufficient capacity returns `invalid` before rendering. Choice IDs remain globally unique Menu row IDs. The adapter reconstructs scratch descriptors and checks from the controlled `choice` each frame, reduces events once, then paints the selected label and ChevronDown SVG. The horizontal trigger fits its contents; a vertical trigger grows. The popup opens below the trigger aligned to its right edge, flips/clamps at the Settings viewport, uses source Medium 26-pixel Menu rows, skips disabled options during navigation and closes after activation. Selecting the current value closes without emitting a changed callback. External changes immediately update label and checks. Set `dropdown_scrollable` for the existing bounded wheel/touch-drag Menu window.

`cui_menu_popup_paint` supports this composition for other custom triggers. Call the drawing-free `cui_base_menu_tree` once, paint the trigger independently, then call the popup painter with its anchor. The painter does not process events, submit a trigger or mutate action/check values; it registers visible popup row semantics and retains layout/window metrics. Settings uses the shared paint-only Button renderer to avoid duplicate registration. For old descriptors with only `item.popup`, the initialized Choice popup still renders the legacy Select. This fallback retains its prior list appearance and is not the source checked Menu.

## Remaining source gaps

Oversized non-scrollable menus still use the bounded existing Menu window rather than the upstream unconstrained-height layout. The scrollable Menu window does not yet paint a source scrollbar. Tooltip overlay styling and slide/shared-tooltip grace motion remain those of the current Tooltip module. Numeric/string external settings are caller-controlled values/editors rather than retained getter/setter closures. String reset is one atomic range edit; undo restores both the original text and its pre-reset selection.

This implementation renders every matching group rather than the upstream virtualized List. Extremely large pages should be bounded by application and core capacities. Grow-width layout and group jumps depend on prior measured geometry and can require another frame; filtering resets page scrolling. Arbitrary style refinements, Markdown descriptions, translated default labels and custom Icon objects are represented by callbacks or explicit strings rather than source object APIs. Named/path icons use the theme provider. These limitations remain explicit; this module does not establish complete upstream parity.
