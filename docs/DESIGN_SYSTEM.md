# Sizes, spacing and visual styling

Use a shared scale to keep a form, toolbar and application layout consistent.
CUI values are logical pixels: a renderer's display scale maps them to physical
pixels. The component gallery lets you compare sizes, corner radii and spacing
without changing your operating system's display scale.

## Control and typography sizes

Set `design_size` on Button, Checkbox, Radio, Switch, Toggle, Input, Textarea,
List, Select, Menu or Combobox options. Zero (`CUI_DESIGN_INHERIT`) preserves the
existing GPUI size enum and its metrics. The shared scale is opt-in; existing
applications keep their current dimensions.

| Scale | Font | Line height | Single-line control height |
| --- | ---: | ---: | ---: |
| xs | 12 | 16 | 20 |
| sm | 14 | 20 | 24 |
| base / md | 16 | 24 | 32 |
| lg | 18 | 28 | 40 |
| xl | 20 | 28 | 48 |
| 2xl | 24 | 32 | 56 |
| 3xl | 30 | 36 | 64 |
| 4xl | 36 | 40 | 80 |
| 5xl | 48 | 48 | 104 |
| 6xl | 60 | 60 | 128 |
| 7xl | 72 | 72 | 152 |
| 8xl | 96 | 96 | 200 |
| 9xl | 128 | 128 | 264 |

These are CUI's explicit logical-pixel metrics, inspired by utility design
scales. `md` is an alias of `base` in this scale. Metrics multiply by the theme's
root font density (`theme.font_size / 16`). The legacy GPUI Button Large remains
32 pixels; select `CUI_DESIGN_LG` for the new 40-pixel control.

```c
cui_button_ex(ui, save_id, &theme, cui_string("Save changes"),
    (cui_button_options){.design_size = CUI_DESIGN_LG});
cui_input_ex(ui, email_id, &theme, cui_string("Email"), edit, &input_state,
    (cui_input_options){.width = 320, .design_size = CUI_DESIGN_LG});
```

Single-line controls use the listed height. Checkbox/Radio indicators use the
font size; Switch tracks add four logical pixels to that indicator size.
Textarea grows by its row count using the shared line height and padding, plus
its border. Popup query fields and option rows share the same scale.
Explicit row-height overrides still take precedence. Supply enough width for
large text; a larger size does not increase an explicitly fixed container width.

`cui_design_measure(size, density, &metrics)` returns font, line height, control
height, horizontal/vertical padding, gap and indicator size. Invalid inputs return
false without writing the output. A base Textarea receives its metrics from the
caller; its legacy `cui_textarea_measure` helper continues to measure legacy sizes.
The styled Textarea applies `design_size` before submitting its base behavior.

## Spacing and corners

`cui_space(units)` maps one unit to four logical pixels. Fractions work:
`cui_space(0.5f)` is two pixels. Use negative units for offsets, and nonnegative
units for padding and gaps.

```c
cui_begin_box(ui, card_id, (cui_box){
    .width = 360, .padding = cui_space(6), .gap = cui_space(3),
    .radius = cui_radius(CUI_RADIUS_XL, 1), .background = theme.surface
});
/* Submit card children here. */
cui_end_box(ui);
```

The radius scale is none=0, xs=2, sm=4, md=6, lg=8, xl=12, 2xl=16,
3xl=24 and full=9999 logical pixels, multiplied by the supplied density.
Renderers constrain full rounding to each shape's bounds. Set the theme's
`radius` and `radius_lg` where appropriate, or a particular box's
`radius`. Comfortable/compact theme choices remain useful starting points;
these units give you finer control over an individual layout.

## Typography

`cui_text_ex` supports normal, medium, semibold and bold weights, italic and
monospace styles, explicit font size and line height. For example:

```c
cui_text_ex(ui, cui_string("Workspace"), 24, theme.text, 32,
    (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD});
cui_text_ex(ui, cui_string("cui_begin_frame(...)"), 14, theme.muted, 20,
    (cui_text_style){.monospace = true});
```

The host owns font selection and measurement. Configure styled measurement and
font metrics alongside drawing, so the same font faces and styles determine both
layout and pixels. See [text adapter](TEXT.md) and
[editing](EDITING.md). The website's browser font registry is a host policy;
it does not install those fonts for a native application.

## Backgrounds, opacity and motion

A box's `background` accepts RGBA color. `custom_opacity=true` and `opacity`
apply inherited draw opacity to its children as well as the box. Use
`cui_paint_bitmap` for a generated gradient or other image fill; keep its pixels
alive until rendering completes and increment its revision after changing them.
The current generic box API has no backdrop-blur or gradient-stop command.
A CSS blur on website chrome would not establish native CUI blur support.

Progress supports indeterminate animation with host time. Sheet supports timed
entry/exit motion through `sheet_animation_seconds`, including reduced-motion
behavior. Tabs expose an application-owned spring indicator. See
[Progress](PROGRESS.md), [surfaces](SURFACES.md) and [choices](CHOICE.md) for their
actual APIs. Animate your own state using frame time and request another frame
with `cui_invalidate`; respect `cui_reduced_motion(ui)` in the host's motion policy.
