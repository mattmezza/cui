# Slider style values (unreleased)

Include `cui/cui_slider_style.h` for the pure C-only owned resolver. It has no
context, allocation, callback or renderer dependency at runtime. Initialize the
exact version1 record and choose a palette explicitly:

```c
cui_slider_style_options_v1 options = {
    .version = 1, .struct_size = sizeof(cui_slider_style_options_v1),
    .layout = CUI_SLIDER_STYLE_PARENT, .rem = 16, .full_radius = 999
};
cui_slider_style_plan_v1 plan;
if (cui_slider_palette_preset_get(0, &options.palette) &&
    cui_slider_style_resolve_v1(&options, &plan)) {
    /* Owned values only: retain/copy plan, no frame/source lifetime required. */
}
```

`FIXED` requires positive finite length; `PARENT` ignores it and writes zero to
`plan.length`. Parent sizing is a future presenter's composition policy, not a
promise that every enclosing layout accepts it. The private source geometry path
has ordinary-flow horizontal/vertical proof, including first-frame parent fill
and resize; the registered Y-scroll validator still rejects the vertical path.
The resolver does not validate a particular frame's layout admission.

Rem sets hit cross extent1.5R, track .375R, outer thumb R and inner max(0,R−2).
Padding1, axis offset−8, cross offset−5 and maximum pointer ring width3 are literal
logical pixels. Do not replace the source offsets with half-thumb centering.
The source vertical default120px must be selected explicitly for FIXED mode.
Full radius0 expresses a square theme. Each active track-corner mask independently
overrides its radius; fill/outer/inner keep full radius. Track uses bar alpha×.2
or×.4 when ACTIVE, outer uses×.5 and fill preserves bar alpha. DISABLED records
visual omission policy while preserving these values and semantic geometry.
There is no live activity detection or animation in this pure function.

The entire input palette must be valid. Color override masks distinguish a
transparent authored override from absence. Active overrides require finite
linear RGB/alpha in[0,1]; inactive override bytes/corners are ignored. Unknown
masks/modes/flags, nonzero reserved words, invalid version/size and invalid active
fields reject without changing any output byte. The output is owned and contains
no pointers; neither input nor output retains a palette/theme reference.

The thumb is an owned `cui_background`, copied fieldwise without byte quantization.
It preserves straight-linear hidden RGB at alpha0, source projection, interpolation
space, angle, stop order/positions and independent alpha. Aurora Light stays a
linear two-stop brush. A transparent solid override is deliberate and works even
when the underlying preset thumb is a gradient. No byte representative, preset
identity inference or gradient-to-solid fallback occurs.

This is step2 toward a typed Styled presenter. The private Base seam changes no
public Base ABI and public calls select the frozen NULL path. There is no public
Styled widget, presentation state, spring, named decoration API, V1 conversion
helper or new renderer/host support here. Current adapters do not advertise V2
background rendering; the value resolver does not make them support Aurora.
Native numeric hosts and all five typed language builders also remain pending.
For actual controlled Base interaction use the complete
[example](../examples/slider_model_controlled.c) and [guide](BASE_SLIDER_MODEL.md).
