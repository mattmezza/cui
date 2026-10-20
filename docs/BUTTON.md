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

Set `.icon = cui_string("close")` to add a real SVG icon. With `.icon_only = true`,
the button is square and hides the visible label while retaining `name` for
accessibility. For example, a Small close button uses a 24px target and a 14px
icon. Dialog and sheet close controls use this path. The theme's `icon_loader`
can replace that artwork; custom content callbacks remain available for other
compositions.

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

With `custom_colors`, `pressed || selected || open` selects `colors.active`
(or `outline_active`); otherwise hover selects `hover`, then `normal` is the
fallback. A selected/open button therefore keeps its active background while
hovered or pressed. Disabled painting takes precedence over those states. Use a
custom interactive row when selected, hovered and pressed need independent
colors; see [Interaction](INTERACTION.md#custom-clickable-rows).

Component variants and theme palettes are different enums. Primary maps to
`CUI_THEME_BUTTON_PRIMARY`; secondary/danger/warning/success/info map to their
namesakes; default/outline map to `CUI_THEME_BUTTON_DEFAULT`. Ghost uses
secondary active, accent hover and a transparent normal background; its pressed
background comes from default active. Link uses `link`, `link_hover` and
`link_active`. There is no `CUI_THEME_BUTTON_GHOST` palette. Set `custom_colors`
on a component for a local override instead of copying the theme.

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

## Bounded paint handlers

Use [Button styles](BUTTON_STYLES.md) for additive instance, hover, active, selected, disabled and focus paint refinements. Explicit masks preserve zero values; callbacks are borrowed pure calculations and cannot change geometry or dispatch input. Legacy submission remains available.
