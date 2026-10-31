# Owned Slider palette values

`include/cui/cui_slider_palette.h` supplies context-free C values. It does not
change existing Slider rendering, input, state, semantics or theme ABI. There is
no presenter setter, brush sampler, renderer transport or Python/JS/TS/Go/Rust
builder authoring in this slice. Disabled/theme presentation remains incomplete.

```c
#include <cui/cui_slider_palette.h>
cui_slider_palette palette;
if (cui_slider_palette_preset_get(0, &palette)) {
    /* Owned values for application inspection; no implicit paint mutation. */
    bool valid = cui_slider_palette_valid(&palette);
    (void)valid;
}
```

## Sources and identity

`cui_slider_palette_builtin_get` resolves four CUI skins (Light, Dark, High
Contrast, Terminal). `cui_slider_palette_from_theme` uses any caller theme's
`primary`, `on_primary`, `focus` as bar, solid thumb and ring. These are encoded
sRGBA8 bytes: RGB uses the sRGB transfer decode, alpha remains straight. Other
theme fields, including `dark`, do not infer source-preset identity. A custom
Aurora-colored theme still produces a solid thumb.

The 38 named presets use the same index/name order as `cui_theme_preset_get`, but
resolve their original tokens independently of that byte-valued theme adaptation.
Recognized `slider.background` falls back to resolved primary; the differently
spelled `slider.bar.background` is ignored by the pinned source schema. Thumb
falls back to primary foreground, ring to source base.blue. All pinned entries
explicitly supply these fallback tokens. Source representative colors select the
first stop when the token is a gradient. Aurora Light's thumb remains an owned
LINEAR brush: 180 degrees, source projection, SRGB interpolation, stop positions
0/1, colors #FFFFFF and #F8FAFC. It is never flattened into a fallback byte color.

## Color and brush contract

Colors are straight linear RGB and alpha in [0,1]. Hidden RGB survives alpha0.
The source-HSLA constructor applies the pinned shader's normalized HSL formula,
without sRGB transfer decoding or byte quantization. It bounds floating-point
roundoff at unit-gamut edges; GPU-f32 bit identity is not promised. This is a
different conversion from the encoded custom-theme byte constructor.

SOLID validates only `solid`; inactive gradient fields may contain arbitrary
values. LINEAR ignores `solid` and validates both owned stops, positions [0,1],
angle [0,360], interpolation space and `CUI_SLIDER_BRUSH_SOURCE_038` projection.
Equal/reversed positions retain their authored order; 360 aliases 0. SRGB means
linear RGB to encoded sRGB, straight component interpolation, then transfer
decode back to linear RGB; OKLAB means linear RGB to Oklab interpolation and
back. Alpha interpolates independently; premultiplication follows evaluation.
This differs from CSS premultiplied-stop interpolation.

For a rectangle W,H, source projection uses d=(sin(angle),-cos(angle)), scales
its Y by H/W when W>H, otherwise X by W/H, and computes
`t=.5+dot(P-center,d)/(length(d)*(abs(dx)>abs(dy)?W:H))`. Stop positions then map
and clamp that coordinate. This describes owned semantics for a future consumer;
this slice implements no sampling, zero-size geometry handling or rendering.

Constructed records zero inactive fields. Values contain no pointers, allocation,
context, frame state or borrowed lifetime. Copies remain usable independently of
the source theme. A valid value does not prove a currently painted part exists.
Null outputs, invalid builtin/index (including SIZE_MAX) leave output untouched;
invalid preset names are empty strings. Invalid source-HSLA inputs fail atomically.

## Evidence and scope

Pinned source/schema/default/shader URLs, hashes and exact interpretation are in
[the reviewed design](design/SLIDER_PALETTE_VALUES.md) and
[the whole-feature audit](design/SLIDER_THEME_COMPLETION.md). The canonical raw
palette SHA256 is `03de5a483daab83ed983495963a560d70f8ea7db3fd079bf4a69a4df0e2f77c0`.
`generate_slider_palettes.py --check` verifies source pin, preset order and output
freshness; unreviewed brush syntax fails rather than approximating it.

`tests/unit/test_slider_palette.c` checks conversions, four builtins, custom
identity, active/inactive validation and atomic failures. An independent oracle
in `tests/test_slider_palette_oracle.py --probe PATH` parses pinned raw tokens
without importing the generator and compares actual C values for all 38 entries,
including every owned Aurora brush field. Selective and umbrella distribution
fixtures check public-header implementation/linkage. These checks require no
native window. All five focused tests passed under the private-display wrapper: pure C values,
independent 38-preset runtime oracle, generation freshness, selective distribution
and umbrella distribution. The build uses C11 with warnings treated as errors.
