# Buttons

`cui_button` keeps the default primary style and controlled disabled flag.
`cui_button_ex` adds primary, secondary, outline, ghost, destructive and link
variants; XSmall/Small/Medium/Large sizes; loading and selected presentation; explicit
or growing width; leading/trailing text; and a presentation-content callback.
Every variant uses `cui_base_button` for capture, keyboard activation and focus.
The content callback should draw presentation without nested interactive controls.

At the default 16px theme font, XS/S/M/L heights are 20/24/32/32px,
with 12/14/14/16px labels and horizontal padding of 4/8/10/12px.
Medium and Large intentionally share a height in the frozen upstream source.
These metrics scale with `theme.font_size / 16`; the legacy generic
`theme.control_height` and `theme.padding` do not override button size metrics.
Text uses a 1.25 line height and the content row is centered vertically without
vertical padding reducing the available label space.

```c
if (cui_button_ex(ui, save_id, &theme, cui_string("Save changes"),
    (cui_button_options){.variant = CUI_BUTTON_OUTLINE,
        .size = CUI_CONTROL_SMALL, .loading = saving})) {
    begin_save();
}
```

Loading suppresses activation and focus, draws an animated SVG loader, and exposes
an accessible description. Disabled text/background preserve theme contrast;
hover/pressed states tint enabled surfaces and keyboard focus has a distinct border.
Selected is application-owned presentation; it does not imply a semantic pressed state. For value toggling use `cui_toggle` or the controlled toggle group.

Headless tests exercise all nine variants, four sizes and four themes, measured
bounds, adornments, content callbacks, semantic names and ordinary push-button state and keyboard
suppression while loading. SVG icons, custom palettes and animation are separate
pending capabilities; a text adornment does not implement the Icon catalog family.

Buttons additionally support warning, success, and info intent variants and
extra-small density (`CUI_CONTROL_XSMALL`). The `outline` flag combines with an
intent to use its theme color for the border and label on the surface background.
The original standalone `CUI_BUTTON_OUTLINE` remains available. Semantic intent
foreground tokens are used for filled labels, including `on_danger`.

`selected` controls presentation only, matching the frozen reference. It no
longer mislabels an ordinary push button as checked; use the separate toggle
control for toggle semantics. Full upstream button parity, including independent
explicit toggle metadata and richer icon/tooltip/animation options, is still
under audit. Tests exercise all variants/densities across four themes plus
colored-outline painting, pointer activation and disabled keyboard suppression.
