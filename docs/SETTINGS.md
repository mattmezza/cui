# Settings

Settings composes searchable pages, groups and editable fields with a resizable sidebar and a scrolling active page. It uses the existing Sidebar, Resizable, Switch, Checkbox, Input, NumberInput and Select engines. Custom callbacks can replace a field or an entire item, and receive the original page/group/item indices, size, layout and disabled state.

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

The caller owns descriptors, field values, editor handles and all state. Keep these alive through rendering; callbacks execute synchronously while building the frame. Use separate state for each Settings instance. The search editor is optional and must be single-line. Without it, `options.query` supplies a controlled filter. Search text is reduced before navigation and content, so text queued in the current frame filters both immediately. Editors and popup state must be initialized using their own module APIs. Text fields borrow an Input state; numeric fields borrow a Number state and editor; dropdown fields borrow a Choice popup initialized with a unique popup ID. Optional Switch state enables its existing retained motion.

Filtering matches item titles, descriptions and keywords. Custom whole-item renderers match only keywords unless their `matches` hook supplies another policy. Page and group titles, group footers and title suffixes do not independently match. The default matcher folds ASCII case and preserves non-ASCII UTF-8 bytes; a host matching callback can provide Unicode lowercase matching. Filtered arrays retain original indices and IDs. A matching selected page/group remains selected; otherwise selection falls back to the first matching page and its page header. No-results filtering keeps selection but renders no stale page. Clearing search preserves the current selection.

The sidebar starts at 250 pixels and resizes within 160–360 by default. Page trees start collapsed unless `default_open` is set. Groups become sidebar entries only when their titles exist and the page has more than one matching group. Page activation selects its header; group activation selects the group and reveals it after measurement. Layout switches to stacked items when the content panel is at most 480 pixels wide. A vertical item override also stacks that item. Text and numeric fields use 256 pixels horizontally and grow when stacked. Zero width defaults to 800; negative width grows using the previous measured bounds. Zero height uses a CUI fallback of 500 rather than an unconstrained host viewport.

Medium is the default. Zero-initialized options select a page; set `default_group_selected` to select `default_group` initially. Switch and Checkbox honor XS/S/M/L. Settings forwards its size to custom callbacks. Source item typography uses 14-pixel labels, muted descriptions and centered 20-pixel Label line boxes (plain item descriptions use the inherited golden-ratio line height), scaled by `theme.font_size / 16`. Group surfaces support Normal, Fill and Outline, including a per-group override. Fill uses independently configurable `theme.group_box`; item titles use `theme.group_box_foreground`. Card padding is 16, item gap 16, title gap Normal16/card12 and footer gap 8 at that density. Footer callbacks receive a theme copy with small muted text defaults and render outside the surface. Disabled items paint at half opacity. Built-in controls receive disabled state; custom callbacks must honor `options.disabled` themselves.

Reset appears only when the active page permits reset and a matching item is dirty. The icon uses the named Undo2 SVG and the theme asset provider. Reset applies to every matching reset-capable item, including disabled items and unchanged matching defaults. `has_default` enables typed defaults; custom dirty/reset hooks take precedence. `on_change` runs after a built-in mutation or successful reset. Number fields keep out-of-range draft text while the setting value is clamped to the configured bounds; blur commits the clamped draft. `cui_settings_reset_page` exposes the same filtered reset policy without rendering.

The state workspace capacity bounds page count to at most 4096. The current sidebar composition supports at most 256 groups per page; groups support at most 65,536 items, each with at most 4096 keywords. Invalid descriptors or nonfinite dimensions return `invalid`. Actual layout is also bounded by the core context capacity. Text edit errors are reported through `edit_error`. `cui_settings_part_id` provides deterministic IDs for headless routing; current navigation parts are page5/group6, field24, reset34 and search4. Resize and scroll controls use their owning modules' part-ID helpers.

## Remaining source gaps

Input and NumberInput currently have no public size option, so those adapters use their existing renderer metrics rather than claiming all-four-size source fidelity. Dropdown uses the existing Select engine; the source's outline Button plus checked, optionally non-scrolling Menu requires a separate Menu popup paint contract. Reset has the correct SVG and accessible name but no reset tooltip yet. Numeric/string external settings are caller-controlled values/editors rather than retained getter/setter closures. String reset is one atomic range edit; undo restores both the original text and its pre-reset selection.

This implementation renders every matching group rather than the upstream virtualized List. Extremely large pages should be bounded by application and core capacities. Grow-width layout and group jumps depend on prior measured geometry and can require another frame; filtering resets page scrolling. Arbitrary style refinements, Markdown descriptions, translated default labels and custom Icon objects are represented by callbacks or explicit strings rather than source object APIs. Named/path icons use the theme provider. These limitations remain explicit; this module does not establish complete upstream parity.
