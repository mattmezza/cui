# Slider palette values: implementation contract

Design checkpoint only, following `fec63936`. This slice adds owned, portable
palette values and pure resolvers; no presenter, reducer, Core command, renderer,
bridge, binding or public `cui_theme` layout change. It cannot close the Slider
family or disabled/theme presentation ledger. All78 entries remain.

## Proposed source-specific API

Authoritative new header: `include/cui/cui_slider_palette.h`; implementation in
`src/theme/cui_slider_palette.c`. These names are deliberately Slider-specific:
the shared background ABI/transport proposed in `e286d3ed` is still unapproved.
A future private conversion may map these values into it without a second renderer.

```c
typedef struct { float r, g, b, a; } cui_slider_palette_color;
typedef struct {
    float position;
    cui_slider_palette_color color;
} cui_slider_palette_stop;
typedef enum {
    CUI_SLIDER_BRUSH_SOLID = 0, CUI_SLIDER_BRUSH_LINEAR = 1
} cui_slider_brush_kind;
typedef enum {
    CUI_SLIDER_BRUSH_SRGB = 0, CUI_SLIDER_BRUSH_OKLAB = 1
} cui_slider_brush_space;
typedef enum { CUI_SLIDER_BRUSH_SOURCE_038 = 0 } cui_slider_brush_projection;
typedef struct {
    cui_slider_brush_kind kind;
    cui_slider_palette_color solid;
    cui_slider_brush_space space;
    cui_slider_brush_projection projection;
    float angle_degrees;
    cui_slider_palette_stop stops[2];
} cui_slider_palette_brush;
typedef struct {
    cui_slider_palette_color bar, ring;
    cui_slider_palette_brush thumb;
} cui_slider_palette;
typedef enum {
    CUI_SLIDER_PALETTE_LIGHT = 0, CUI_SLIDER_PALETTE_DARK = 1,
    CUI_SLIDER_PALETTE_HIGH_CONTRAST = 2, CUI_SLIDER_PALETTE_TERMINAL = 3
} cui_slider_palette_builtin;

bool cui_slider_palette_color_from_srgba8(cui_color, cui_slider_palette_color *out);
bool cui_slider_palette_color_from_source_hsla(float h, float s, float l, float a,
                                               cui_slider_palette_color *out);
bool cui_slider_palette_valid(const cui_slider_palette *);
bool cui_slider_palette_from_theme(const cui_theme *, cui_slider_palette *out);
bool cui_slider_palette_builtin_get(cui_slider_palette_builtin, cui_slider_palette *out);
size_t cui_slider_palette_preset_count(void);
cui_str cui_slider_palette_preset_name(size_t index);
bool cui_slider_palette_preset_get(size_t index, cui_slider_palette *out);
```

## Color, brush and inactive-field meaning

Colors are finite straight **linear RGB** and straight alpha in [0,1]; never
premultiplied or quantized to bytes. Hidden RGB at alpha0 survives. The sRGBA8
constructor decodes the standard sRGB transfer for RGB; alpha is byte/255. The
source-HSLA constructor accepts finite normalized [0,1] inputs, uses pinned
GPUI0.3.8 shader HSL-to-RGB math and treats its result as linear, as that shader
does. It does NOT apply another transfer decode. Hue1 follows the source shader's
actual branch behavior, not an invented CSS wrap. Invalid floats/null output
return false unchanged. Exact source shader bytes and hash must be recorded in
the implementation evidence; no renderer/shader bit-identity is promised.

This intentional distinction matters: a caller's custom CUI byte colors are
encoded sRGBA8, while pinned source theme values originate through source HSLA.
Do not manufacture a thumb brush from CUI's already flattened representative
byte. Values preserve enough information for a future explicit source renderer.

SOLID validates only its color and ignores every gradient field, including
invalid inactive enum/float/stop data. LINEAR ignores `solid`, validates both
owned stops, space/projection enums, finite angle [0,360] and positions [0,1].
360 is an angle alias of0. Reversed/equal positions are retained as authored,
not sorted. Alpha is interpolated separately from straight color components;
premultiplication belongs after final source interpolation, not in storage.
For SRGB, convert stored linear RGB to encoded sRGB, mix straight RGB and alpha,
then decode RGB. For OKLAB, convert linear RGB to Oklab, mix straight components
and alpha, then convert back. No sampling/quantization/output-gamut policy is
implemented by this resolver slice. Future consumers must handle out-of-gamut
intermediate results before claiming rendering parity.

Projection is explicitly source GPUI038, not CSS: for positive logical W/H,
angle0 points up and90 right. Starting d=(sin A,-cos A), scale d.y by H/W when
W>H, otherwise d.x by W/H; t=.5+dot(P-center,d)/(length(d)*(abs(d.x)>abs(d.y)?W:H)).
Then map through the two positions and clamp. Equal stops need a documented
future sampling extension (second color at/after boundary); zero-sized boxes
paint empty. These rules describe the value's semantic intent, not a new paint
transport or an implemented sampler. No hidden pixels, caches or device scale
are retained.

All records own their fixed scalar/stop values, contain no pointers, allocations,
platform types or callbacks. Copying/reusing them has no borrowed lifetime; source
resolvers perform no I/O. Constructors zero inactive fields deterministically;
validation does not mutate or normalize caller data. No caller struct ABI grows.

## Exact resolver precedence and scope

`from_theme` is the explicit **CUI theme adaptation**: current primary -> bar,
on_primary -> solid thumb, focus -> ring, using the sRGBA8 constructor. It reads
no other field, infers no preset from `dark`, colors, identity or luminance, and
accepts arbitrary caller edits. Builtin get calls this rule on each of the four
existing builtin themes. These four are CUI skins, not replacements for GPUI's
named Default Light/Dark values; the distinction must remain in docs/tests.

Named get follows current `cui_theme_preset_get` index/name order for all38 entries,
but resolves from pinned raw source tokens independently of adapted byte themes:
bar = recognized `slider.background`, otherwise representative color of resolved
primary; thumb = recognized `slider.thumb.background`, otherwise resolved primary
foreground as SOLID; ring = recognized ring, otherwise source base.blue.
`slider.bar.background` is NOT an alias. The exact pinned schema's background
parser supports two-stop linear brushes; representative source color is first
stop. Preserve Aurora Light thumb as owned LINEAR, angle180, stop positions0/1,
SRGB, GPUI038, source colors #FFFFFF and #F8FAFC. Never flatten it.

All pinned themes explicitly provide primary/primary.foreground/base.blue, so no
invented missing-token fallback is necessary in the static named table. Tests
must prove that inventory assumption against pinned data. Future source refreshes
with unsupported brush syntax, missing required fallback tokens or changed preset
order fail generation/check rather than silently approximate. Runtime receives
no strings/theme files to parse and adds no network dependency.

Null output, invalid builtin and every out-of-range preset index (including
SIZE_MAX) leave output bytes untouched. Invalid preset names return empty string;
count exactly matches current preset count. Source getters are pure, reentrant
and context/frame independent. `valid(NULL)` is false and mutates no diagnostics.
A valid palette is not evidence of any current-frame painted box or support in a
presenter/binding.

## Independent proof and integration plan

Use strict C11 unit tests for conversion distinctions, source-HSLA primary sectors,
hue1, alpha0 hidden RGB, SOLID inactive junk, LINEAR invalid active fields,
reversed/equal positions and transaction failure preserving sentinel output.
All4 builtins must match current theme byte adaptation after explicit transfer
conversion; custom themes with equal/different colors/dark flags must never gain
Aurora brush identity. No frame/context/window is required.

For all38 named entries, compare the C runtime output against an **independent
pinned oracle**, not the same generated C table or the same parser function that
creates it. Record source hashes, exact palette names/order, raw recognized and
ignored keys, solid bar/ring values and owned thumb fields. Use frozen expected
source tokens plus independent test conversion, checking Aurora endpoints,
positions/angle/space/model and the default misspelled-key fallback. Validate
generator freshness against canonical raw source/preset order separately.
No tests may silently install GPUI, parse unpinned live themes or download data.

Wire the new module/header into canonical source metadata, generated standalone
headers, minimal Go/Rust source SDKs and agent-skill resources; keep all78 manifest
entries and mark this C resolver subset only. Public consumer docs explain encoded
custom themes versus source values, brush ownership and missing presentation.
Strict selective/umbrella header/source checks and focused headless CTest run
through scripts/test_local.py after the integrator grants a compiler slot.

Signed: `/root/hidden_windows`, 2026-10-31. Contract review requested before
production commit. Shared brush ABI/transport remains the separate Stepper/root
approval gate. No merge/push, no compiler used at this checkpoint.

Implementation clarification: the public projection identifier is
`CUI_SLIDER_BRUSH_SOURCE_038`; upstream names remain provenance only. The
source-HSLA constructor bounds floating roundoff at mathematical unit-gamut
edges, without quantization/transfer conversion; GPU-f32 bit identity is not
claimed. Focused pure C, independent all38 runtime oracle, generation freshness
and selective/umbrella distribution checks passed through the private-display
wrapper on 2026-10-31. Rendering, presenter integration and bindings remain out
of scope and whole Slider disabled/theme coverage remains incomplete.

Additional implementation evidence (cached primary sources only, no dependency
installation):

- [Pinned source color parser](https://raw.githubusercontent.com/longbridge/gpui-kit/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/theme/color.rs),
  SHA256 `d3c81fb0fdca3fce797c312addc5afd9767d5985dc56bbf73afecdd0019c4e01`: representative
  color chooses first stop; background parser owns exactly two stops.
- [GPUI0.3.8 WGPU shader](https://docs.rs/crate/gpui-pre-wgpu/0.3.8/source/src/shaders.wgsl),
  extracted raw source SHA256 `1d91f2c8a1958a241f1b659a4137c160592c8076b9be09049db4c9fb9a344529`;
  crate checksum `439c5421cafe1a7cf68d0c0ae5cc82b26fad6b0e6345ac98d20acaf1f2463914`.
  `prepare_gradient_color` (404) and `gradient_color` (430) establish source
  HSLA-as-linear values, SRGB encode/mix/decode, Oklab mixing and projection.
  Raw shader source is retained externally under the shared bounded reference
  cache, not copied into the production library.
