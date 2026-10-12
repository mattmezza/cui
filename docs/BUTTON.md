# Buttons

`cui_button` keeps the default primary style and controlled disabled flag.
`cui_button_ex` adds primary, secondary, outline, ghost, destructive and link
variants; small/medium/large density; loading and selected presentation; explicit
or growing width; leading/trailing text; and a presentation-content callback.
Every variant uses `cui_base_button` for capture, keyboard activation and focus.
The content callback should draw presentation without nested interactive controls.

```c
if (cui_button_ex(ui, save_id, &theme, cui_string("Save changes"),
    (cui_button_options){.variant = CUI_BUTTON_OUTLINE,
        .size = CUI_CONTROL_SMALL, .loading = saving})) {
    begin_save();
}
```

Loading suppresses activation and focus, draws a textual loading mark, and exposes
an accessible description. Disabled text/background preserve theme contrast;
hover/pressed states tint enabled surfaces and keyboard focus has a distinct border.
Selected is app-owned presentation and semantic pressed state, not an internal
toggle model. For value toggling use `cui_toggle` or the controlled toggle group.

Headless tests exercise all six variants, three sizes and four themes, measured
bounds, adornments, content callbacks, semantic names/selected state and keyboard
suppression while loading. SVG icons, custom palettes and animation are separate
pending capabilities; a text adornment does not implement the Icon catalog family.
