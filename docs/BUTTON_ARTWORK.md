# Typed Button artwork

Button can borrow an Icon, Spinner or ProgressCircle descriptor through
`leading_artwork`. `loading_artwork` accepts an Icon descriptor. Existing string
icons and zero-initialized options retain their behavior. `leading_content`
takes priority over both typed slots.

The rules come from pinned v0.7.1 `button/button_icon.rs` (`ButtonIconVariant`,
`ButtonIcon::render`): loading replaces an ordinary Icon with a Spinner, using
its custom loading Icon when provided. An explicit Spinner or ProgressCircle
remains that artwork while loading, and ignores the loading replacement. A
custom loading Icon rotates with the existing 800ms quadratic Spinner cycle;
reduced motion leaves it static. This API does not add menus, progress ownership,
or a complete Button source parity claim.

```c
static bool save_button(cui_context *ui, cui_id id, const cui_theme *theme,
                        cui_icon_state *mark) {
    cui_button_artwork leading = {
        .kind = CUI_BUTTON_ARTWORK_ICON,
        .icon = mark,
        .icon_options = {.custom_color = true, .color = {25, 160, 80, 255}}
    };
    return cui_button_ex(ui, id, theme, cui_string("Save"),
                         (cui_button_options){.leading_artwork = &leading});
}
```

Initialize the caller-owned `mark` with `cui_icon_state_init`, then check
`cui_icon_set_name(mark, cui_string("check"), NULL, NULL)` before composition.
Destroy it with `cui_icon_state_destroy` only after all rendering has completed.
The caller owns Icon state and its bounded retained masks. Use
`cui_icon_set_limits` to constrain memory, and `cui_icon_state_collect` only
when prior draw lists have finished rendering. Button does not collect or
release caller resources. ProgressCircle borrows caller-provided RGBA storage,
its state, and any optional motion state. No Button artwork heap allocation is
introduced. Underlying Icon preparation can allocate within its configured
limits; the default Spinner uses the existing bounded context resource cache.

Button's resolved artwork size wins over descriptor dimensions: legacy/design
size, numeric `pixel_size`, and then explicit `icon_size` determine the slot.
Custom Icon color, opacity and nonloading transform are retained. Spinner
rotation replaces the transform while animated. ProgressCircle diameter is
resolved by Button; its fraction, color, clock, motion and raster scale remain
caller-controlled. A Spinner's default Loader is selected when its `icon` is
null. An ordinary Icon requires an initialized caller-owned Icon state.

A standalone Button copies descriptor values before accepted click callbacks.
Group copies its child option structures but borrows artwork descriptors:
callbacks must not mutate those descriptors until `cui_button_group_ex` returns.
Referenced render-resource states remain mutable for painting and must live
through renderer completion. Descriptor/result/resource objects must not alias;
known overlapping result ranges are rejected. Contained pointer targets are
not copied. Neither API freezes arbitrary caller mutations to resource storage.

Optional result outputs report local Icon preparation failures (including the
exact `cui_icon_error`) or ProgressCircle raster/argument/capacity failure. The
failed slot remains the resolved size and paints no stale image. Invalid
structural descriptors reject the Button without callbacks or result writes.
A zero-progress fraction is valid. Only the selected artwork descriptor receives
a result; ignored descriptors retain their previous output. Result outputs do not certify Core command,
layout, frame-arena, renderer, or GPU success; check those APIs separately.

Inert composition still paints and can update visible artwork resources or
request animation redraws, while preserving Button interaction state and
suppressing callbacks, hover notifications and tooltips. Layout-only composition
reserves the same slot but does not prepare artwork, change resource/result
state, or request redraw. Measurement and prepaint callbacks cannot compose
Buttons. Loading still disables activation independently of artwork kind.

The focused fixtures cover all 14 design sizes plus four legacy sizes, both
densities and both joined axes, custom color, computed/explicit size, replacement
and retained artwork, passive state/resource invariants, and bounded resource
failures. Native Noto and Chromium WASM fixtures exercise actual font/layout and
Canvas pixels; injected CDP touch is not physical-device validation.

Failure fixtures exercise configured Icon limits, ProgressCircle storage capacity,
and a full context resource table. They do not force a physical allocator failure.
