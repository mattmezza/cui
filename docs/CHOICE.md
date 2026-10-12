# Tabs and toggle groups

`cui_choice.h` contains allocation-free, caller-controlled Tabs and ToggleGroup
behavior, plus themed composable renderers. Every item has a stable nonzero ID,
label and disabled flag. Tabs selection uses an ID so reordering the caller's
array does not select a different panel.

```c
cui_choice_item choices[] = {
    {21, cui_string("Account"), false},
    {22, cui_string("Profile"), false},
};
cui_id selected = 21; /* persistent application state */
cui_tabs(ui, 20, &theme, choices, 2, &selected,
    (cui_tabs_options){.variant = CUI_TAB_UNDERLINE}, draw_panel, application);
```

`draw_panel(cui_context *, size_t index, void *)` runs synchronously only for the
selected enabled tab. Invalid or disabled selected IDs are retained as caller
state and display no panel. Applications decide how to recover when removing a
tab. Hidden panels and their children are not submitted, so they cannot remain
in the focus order.

Tabs support horizontal Left/Right or vertical Up/Down navigation, wrapping,
Home/End, disabled-item skipping, click/Enter/released Space selection and automatic
or manual activation. Manual mode moves focus using arrows and requires activation
to select a panel. Default, underline, pill, outline and segmented styles derive
colors and metrics from the supplied theme. The underline variant draws a bottom
line independently of the focus outline.

ToggleGroup accepts a caller-owned boolean per item. Options choose single or
multiple selection, whether the final selected item can be cleared, orientation,
and group disabled state. Arrow/Home/End traversal moves focus without selecting;
activation applies the selected-state rule. Return values report selection
changes. Callers should initialize state consistent with their selected options;
externally supplied multiple selections are not silently normalized.

The base functions paint nothing. Styled functions call those base functions and
submit theme-dependent boxes, which permits an independent design system without
forking keyboard behavior. Tabs expose tab, tab-list and panel roles with selected state and bidirectional
trigger/panel relationships. The focused (or selected, or first enabled) tab is
the group's single Tab stop; arrow traversal updates it. ToggleGroup exposes a
group and buttons with pressed state through checked. OS accessibility bridges
remain unfinished.

The tab list ID is the container ID XOR `0xcda0312561f50cab`; a panel ID is its
tab ID XOR `0x691a17db80e22f45`. These IDs must not overlap application children.
Labels are frame-copied by the core, and callbacks must not retain draw-list strings.

`tests/unit/test_choice.c` covers actual pointer activation, keyboard modes,
orientation, wrapping, disabled controls, ID-stable reorder, invalid selection,
toggle selection rules, and all five tab variants in all four themes. This is a
portable C11 vertical slice. GPUI Kit's overflow menu, drag reordering, close
buttons, maximum label width/ellipsis, icon/custom child tabs, prefix/suffix hooks,
and gallery stories remain pending. Composing those controls beside this tab list
is possible, but is not claimed as equivalent built-in support. Full family parity
is not yet verified.

## List, Select and Menu

The same header also supplies flat selectable lists and anchored popup controls:

```c
cui_choice_popup popup; /* persistent application state */
cui_choice_popup_init(&popup, 80); /* once; unique popup/scope ID */
cui_select(ui, 79, &theme, cui_string("Account"), cui_string("Choose account"),
    choices, count, &selected, &popup,
    (cui_choice_popup_options){.viewport = {0, 0, width, height}, .dt = dt});
```

`cui_list` accepts an application-owned `cui_list_state`. Activation updates the
selected item ID; moving the active cursor does not commit selection. Up/Down,
Home/End wrap through enabled options. Incremental typeahead matches label prefixes
with ASCII case folding and byte-exact matching for other characters; a repeated
single letter cycles matching labels. Pass elapsed seconds as `dt` to reset search
after 750 ms of inactivity. No locale-dependent global state is changed.

Select and Menu use the same base list behavior with distinct semantic roles.
The trigger has an accessible name, expanded state and a controlled popup ID;
Select displays the committed choice label or the supplied placeholder. Menu
returns true for every action activation, including repeated activation of the
same command. Select returns true only when the selected ID changes. Applications
dispatch menu commands themselves.

Opening with click, Enter, released Space or Up/Down moves focus into the popup.
Selection closes it synchronously and restores the enabled trigger. Escape and
outside clicks dismiss without changing selection. The popup is a floating portal,
so a clipping parent does not cut it off. Placement flips/clamps against the
explicit logical viewport; the width defaults to 240 and fits that viewport.
The modal focus scope prevents Tab from reaching background controls while open.
A flat empty popup renders an explicit empty state and remains dismissible.

For long lists, only a bounded window of options is submitted and painted. Arrow
navigation, Home/End and typeahead bring the active item into that window in the
same frame. This provides keyboard paging without allocating or registering
thousands of offscreen controls. It is separate from the retained variable-height
VirtualList model. Callers rendering a standalone list can set `visible_count` and
`visible_begin`; `state.first_visible` returns the window after cursor navigation.

The popup ID is reserved for its scope and floating root; its internal list ID is
that ID XOR `0xd1bc905ee4216f11`. Item IDs must be globally unique and not overlap
trigger, popup or internal IDs. The API borrows all arrays and state for the call;
labels follow the core's frame-copy lifetime. Initialize state once, not per frame.

The headless choice test verifies selection, repeated menu dispatch, clipping
escape, synchronous focus restoration, outside/Escape dismissal, empty lists,
ASCII typeahead and keyboard paging over forty items with disabled endpoints.
This flat subset does not include nested submenus, separators or check/radio menu
items, wheel/touch scrolling, arbitrary item delegates, searchable/multiple Select,
Unicode normalization/case folding, Combobox text editing, or OS accessibility
adapters. Tab currently stays in the open popup rather than dismissing and
advancing to the next background control. These are tracked parity gaps; rendering
and keyboard selection of the implemented subset are real.
