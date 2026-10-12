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
forking keyboard behavior. Tabs expose tab role and selection through checked;
ToggleGroup exposes button role and pressed state through checked. Semantic group
relationships and OS accessibility bridges require additional core integration.

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
