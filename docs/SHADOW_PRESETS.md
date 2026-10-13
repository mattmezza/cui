# Named shadow presets

`cui_shadow_preset_layers` copies the pinned public Styled DROP presets into caller storage. `cui_set_shadow_preset` applies the same list to the open box and replaces its entire mixed DROP/INSET list. `CUI_SHADOW_NONE` clears both kinds. A zero/default style has no implicit shadow.

```c
cui_begin_box(ui, card_id, card_box);
if (!cui_set_shadow_preset(ui, CUI_SHADOW_MD, card_radii)) {
    /* Handle rejected layout state or invalid radii. */
}
/* Card content. */
cui_end_box(ui);
```

The factory needs capacity for the selected list, at most `CUI_SHADOW_PRESET_MAX` (two). It writes the count and exactly the selected layer span; unused elements remain untouched. Unknown enums, insufficient capacity, capacity multiplication overflow, NULL required pointers, and count-output overlap with the written layer span reject before writing either output. NONE accepts a NULL layer buffer with capacity zero and writes only count zero. The caller supplies valid, suitably aligned storage. The factory retains no pointers and allocates no memory.

```c
cui_drop_shadow layers[CUI_SHADOW_PRESET_MAX];
size_t count;
if (cui_shadow_preset_layers(CUI_SHADOW_SM, layers, CUI_SHADOW_PRESET_MAX, &count)) {
    cui_box_shadow mixed[CUI_SHADOW_PRESET_MAX];
    for (size_t i = 0; i < count; ++i)
        mixed[i] = (cui_box_shadow){.layer = layers[i], .kind = CUI_BOX_SHADOW_DROP};
    cui_set_box_shadows(ui, mixed, count, card_radii);
}
```

Both existing setters copy their lists; caller stack storage may be reused after a successful setter. The render commands retain their existing value ownership and renderer lifetime contracts. NONE follows the existing zero-count setter: radii do not affect empty paint. An invalid preset leaves an already assigned list unchanged.

`cui_box_shadow_init(offset_x, offset_y, color, &out)` reproduces the explicit source constructor: DROP, zero sigma, zero spread, and the supplied offsets/color. It accepts finite offsets in [-4096,4096] and leaves output unchanged on rejection. Callers can then set bounded sigma/spread or choose INSET through the existing mixed-list API. This is an explicit constructor, not a default style layer.

| Preset | Ordered (offset_y, sigma, spread, black alpha byte) |
| --- | --- |
| NONE | empty |
| 2XS | (1, 0, 0, 13) |
| XS | (1, 2, 0, 13) |
| SM | (1, 3, 0, 26), (1, 2, -1, 26) |
| MD | (4, 6, -1, 26), (2, 4, -2, 26) |
| LG | (10, 15, -3, 26), (4, 6, -4, 26) |
| XL | (20, 25, -5, 26), (8, 10, -6, 26) |
| 2XL | (25, 50, -12, 64) |

Offsets on x are zero, RGB is black, all layers are DROP, and all distances are logical pixels. The sigma values are the source public macro values, without the component-private CSS blur halving. CUI rounds source Hsla alphas .05/.1/.25 to nearest 8-bit bytes 13/26/64. Canvas global-alpha composition has renderer-specific byte rounding; browser probes compare two-layer paint with independent opaque Canvas images and bound the one-layer difference from ideal arithmetic. Native software assertions retain exact quantized pixels. It therefore does not claim floating Hsla or native GPUI shader pixel identity. Ancestor opacity uses the existing Core command alpha conversion; interaction-disabled semantics alone do not remove the assigned decoration.

Source: pinned gpui-component0.7.1 `87d10ae5e1299d1be18670a37570c5402f952e34`, dependency `gpui-pre-macros0.3.8/src/styles.rs:384–490`; explicit constructor `gpui-pre0.3.8/src/style.rs:349–393`. Source fixture records the exact macro file hash. These are public named presets; private popover, toast, and raised-tab consumers are separate appearance work.

The focused native test checks all ordered source tuples, constructor bounds, output transactions, mixed-list replacement, software pixels below a translucent surface, ancestor opacity, and unchanged decoration under disabled semantics. The standalone browser fixture exercises all eight presets with the actual pure-C WASM raster and Canvas at DPR1/2, including literal one/two-layer composition and opacity. Existing renderer capability, budgets, mask culling, cache preparation, and DROP/INSET rendering contracts remain applicable. This convenience slice does not certify the full shadow family, native GPUI antialiasing/color identity, or the appearances of unrelated component owners.
