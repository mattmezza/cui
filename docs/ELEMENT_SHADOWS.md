# Styling named elements with shadows

For a custom open box, continue using `cui_set_box_shadows` or
`cui_set_shadow_preset`. To style a box that a composition already closed, call
`cui_element_set_box_shadows` or `cui_element_set_shadow_preset` after submission
and before `cui_end_frame`. These APIs address a named paint box, including a
portal, in the current frame. They do not imply that every component's public ID
is its rectangular paint root or expose private anonymous subparts.

```c
cui_begin_box(ui, card_id, (cui_box){.width = 160, .padding = 16,
                                    .radius = 8, .background = theme->background});
/* Submit application content here. */
cui_end_box(ui);
bool styled = cui_element_set_shadow_preset(
    ui, card_id, CUI_SHADOW_MD, (cui_corner_radii){8, 8, 8, 8});
```

A successful call copies at most eight total DROP/INSET layers and replaces the
entire list. Last successful assignment wins. Zero count or `CUI_SHADOW_NONE`
clears both kinds without removing underlay, outline or other custom paint.
Caller stack layer storage can be reused immediately. No style persists into a
later frame. Existing components retain their default appearance unless styled.

A missing, zero, previous-frame-only or layout-only target rejects with
`CUI_ERROR_INVALID_ACTION`. A frame with a submitted duplicate/folded-colliding
box ID rejects element styling to avoid ambiguous ownership. Call only during
composition, outside diagnostic, measurement and prepaint callbacks; layout-only
submission cannot mutate another painted target. Invalid lists, kinds, radii or
numeric values reject before replacing decoration. Zero-count radii have no paint
and follow the existing empty-list validation policy. Outside an active frame the
call returns false. `true` means accepted style values, not completed layout or
renderer resource success; check frame and renderer results normally.

Custom CSS-inspired values use an explicit constructor:

```c
cui_box_shadow layer;
if (cui_box_shadow_from_css(0, 8, 24, -4, (cui_color){0, 0, 0, 48},
                             CUI_BOX_SHADOW_DROP, &layer)) {
    styled = cui_element_set_box_shadows(
        ui, card_id, &layer, 1, (cui_corner_radii){8, 8, 8, 8});
}
```

`blur_radius` converts to Gaussian `sigma = blur_radius / 2`; radius must be finite
in `[0,512]`. Offset/spread must be finite in `[-4096,4096]`. Invalid input leaves
the constructor output unchanged. Direct `cui_box_shadow.layer.sigma` and named
presets keep their existing meanings. The CSS spec describes the half-radius
Gaussian relationship: [CSS Backgrounds §6.1.2](https://www.w3.org/TR/css-backgrounds-3/#shadow-blur).
This convenience does not claim browser pixel identity: CUI retains its bounded
kernel, source spread/radius and mask-culling behavior, and RGBA8 quantization.

DROP paints before background; INSET after background, before local content and
children. Each phase retains supplied order. Ancestor clipping/opacity and portal
ordering remain the existing box-shadow rules; local child-overflow clipping does
not remove the box's outer shadow. Decoration changes no layout, hit target,
semantics or input behavior. See [DROP_SHADOW.md](DROP_SHADOW.md),
[INSET_SHADOW.md](INSET_SHADOW.md) and [SHADOW_PRESETS.md](SHADOW_PRESETS.md) for
renderer limits, masks, owned cache budgets and failure boundaries.

The generic setter is a C API. The additive [bridge V1 authoring slice](BRIDGE_SHADOWS.md)
implements six explicitly listed builder roots with focused C/Python/JS/Go/Rust
runtime and strict TypeScript checks. Combined release validation remains pending;
bundled C headers alone do not establish other typed language support.
Popup/trigger/backdrop and anonymous or repeated component parts need explicit
owner-specific public addressing and tests. Full 78-family component-part and
binding coverage remains planned in `design/ELEMENT_SHADOW_STYLING.md`. The
[78-family ownership audit](design/ELEMENT_SHADOW_COVERAGE.md) identifies actual
paint targets and remaining addressing gaps; source eligibility is not per-family
test or binding acceptance.

## Component paint ownership

Apply styling after the component submits its boxes. These C entry points have
specific paint targets; IDs do not imply that every nested part is addressable.

| Component | Surface ID | Notes |
| --- | --- | --- |
| Button | caller ID | Includes custom/styled content; use resolved corner radii. |
| Input | caller ID | Framed root; `cui_input_viewport_id` is the inner text viewport. |
| Avatar | caller ID | Outer circular surface/ring, including the photo variant. |
| Legacy `cui_badge` | caller ID | Standalone pill. |
| `cui_badge_ex` | `cui_badge_indicator_id(owner)` | Caller ID wraps content; count0 omits the indicator, dot/icon still show. |
| Switch | `cui_switch_track_id(owner)`, `cui_switch_thumb_id(owner)` | Caller ID includes label; thumb shadow remains clipped by track. |
| Slider | `cui_slider_track_id(owner)` | Caller ID includes label/framing. This entry has no separate thumb. |
| Popover / Dialog | `state.overlay.id` | Panel portal, independent of body, arrow, backdrop and trigger. |
| Tooltip | caller tooltip ID | Only after tooltip submission returns true; separate from trigger ID. |

Badge/Slider getters expose the IDs already used by their painters. They return0
for owner0 or a derived zero ID; zero cannot name an element style target. They
perform no presence check. A hidden count badge or inactive tooltip has no panel
to style that frame. Do not reconstruct internal ID salts or treat another part's
semantic ID as the surface.

Checkbox and Radio caller IDs denote whole rows including labels. Their anonymous
indicator boxes remain unexposed. Single Radio and RadioGroup are separate paint
owners. Their future named-parts route needs explicit opt-in and capacity/collision
contracts; this slice does not change legacy anonymous IDs or claim indicator
coverage. These part contracts remain C-only until matching bridge and typed
language authoring is implemented.
