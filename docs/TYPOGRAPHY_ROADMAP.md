# Typography roadmap grounded in the current implementation

Audit baseline: `integration/density-reconcile`, commit `b96c6844`. This is a
proposal; the declarations below are illustrative and are not shipped APIs.
No font dependency, production code or renderer was changed for this audit.

Subsequent implementation added [body/heading/code tokens](TYPOGRAPHY_TOKENS.md)
and [linear gradient text](TEXT_GRADIENT.md) for software, SDL and Canvas. Those
guides and current public headers define the shipped APIs. The proposed broader
authoring declarations below remain design notes, especially tracking, numeric
weights/stretch and variable axes.

## What applications can use today

| Request | Current support and evidence | Remaining work |
| --- | --- | --- |
| Body, heading, mono families | `cui_text_style.family` is an application `uint32_t` ID. `cui_font_register_style` opens/owns a face; `cui_font_set_style` borrows one. See `include/cui/cui_text.h`, `src/text/cui_text.c:family_find`, `docs/TYPOGRAPHY.md`. | Semantic roles and convenient theme defaults; automatic OS family lookup is absent. |
| Weight, italic, monospace | Four weights: normal, medium, semibold, bold; independent italic/mono flags. Exact face registrations are selected consistently for native measure, metrics and raster. | Arbitrary numeric weights, explicit stretch and variable-font axis authoring. No synthetic bold/italic guarantee. |
| Condensed type | A condensed font file can already be registered under a distinct family ID. | A same-family condensed variant cannot be selected independently of existing style keys; add an explicit stretch key before promising that API. |
| Size and line height | Float logical sizes; explicit line height or zero for natural measurement. Core interns family, size, style and line height. | Named reusable typography tokens and clear defaults across components. |
| Wrap and alignment | `cui_text_ex` wraps within a constrained parent; `cui_text_line` is unwrapped. Paragraph APIs preserve strict UTF-8, hard breaks, grapheme-safe wrapping and optional left/center/right alignment. | One convenient options API, logical start/end alignment and explicit overflow/ellipsis policy. |
| Letter spacing | No public tracking field in `cui_text_style`, measurement callbacks or draw commands. | Shared shaping/advance/caret/wrap contract before a paint-only implementation. |
| Gradient text | Copied linear stops, block/line anchors and real per-pixel glyph masks in software, SDL and Canvas. See `TEXT_GRADIENT.md`. | GPU implementation, radial/conic paints, color-span combinations and broader native acceptance. |

Named-family fallback currently resolves exact family/style, named-family regular,
default-family style, then opened regular. This is variant fallback; missing glyph
fallback is separately configured through the ordered font fallback chain. Neither
mechanism discovers installed OS fonts. Style registration changes require
`cui_text_metrics_changed(ui)` between frames so existing layout caches remeasure.

Core accepts custom styled measurement and metrics callbacks with a shared user
pointer. If the host supplies only legacy measurement, every family can measure
as the same fallback. A convenient API must either require the complete provider
or expose capability failure rather than silently advertise full typography.

The native layout API already retains actual selected faces and exposes positioned
glyphs, visual runs and cluster advances. It accepts paragraph direction and
language at `cui_font_layout_line`; the convenient Core typography entry points do
not expose every one of those shaping inputs. Cluster edges intentionally do not
represent arbitrary caret positions inside ligatures or full editing navigation.
Native contextual behavior also requires the optional bidi build; disabled builds
report unavailable for that functionality. See `include/cui/cui_text.h` and
`tests/packaging/sdk_text_fixture.c` for the existing contract and checks.

Canvas is a separate host implementation: `web/gallery.js:canvasFont` maps IDs to
CSS font stacks and weights 400/500/600/700; measure uses `measureText`, draw uses
`fillText`. Browser system fonts and shaping are distinct from native
FreeType/HarfBuzz. The gallery now consumes the portable linear text paint using
a full contextual line mask; native software/SDL have separate pixel checks.
A browser demo alone cannot prove native text capability.

## Recommended authoring model

First ship semantic tokens on the existing primitives; this is useful without
redesigning native shaping. Keep font resources in the optional text/host layer,
layout contracts in Core, and theme conveniences in base/components. Core must
continue to have no FreeType, HarfBuzz, SDL, browser or Clay types in public APIs.

A proposed public convenience layer can look like this:

```c
/* Proposed API sketch; names and binary layout need review. */
typedef enum { CUI_FONT_BODY, CUI_FONT_HEADING, CUI_FONT_CODE } cui_font_role;
typedef struct {
    cui_text_style face;
    float size;                 /* logical px; finite, positive */
    float line_height;          /* logical px; 0 = selected face natural */
    float letter_spacing;       /* logical px; 0 preserves current shaping */
} cui_typography;
typedef struct {
    cui_typography body, heading, code;
} cui_typography_theme;
/* Resolve role and token first; submit a complete immutable style to Core. */
```

For today's equivalent, applications can keep these token structs locally,
register IDs for each family, and submit `cui_text_ex` with explicit size/height.
Do not add a `letter_spacing` field to that local workaround until the provider
can measure and paint it consistently. A future `cui_text_options` submission
should contain resolved typography, wrap policy, alignment and paint, with a
bool/status result for unsupported combinations. Keep legacy APIs as wrappers
with zero tracking, current wrapping and solid paint. Decide token inheritance
explicitly: role defaults at the theme layer, caller overrides resolved once;
avoid changing every frozen component's metrics by implication.

Use an additive versioned descriptor/provider for extended font selection rather
than reinterpreting the four existing weight enum values as CSS numbers. Suggested
extended selection is numeric weight 1..1000, finite positive width percentage
(100 normal), italic flag and family ID, with documented exact/fallback policy.
A font registry can initially map those requests to explicit static files.
Variable axes and optical sizing should follow only after axis identity,
clamping, ownership and cache keys are specified. Registering a condensed static
face under another family is the reliable interim path.

## Tracking must change layout and paint together

The same resolved shape parameters must reach styled measurement, contextual
geometry, wrap candidate widths, selection/hit testing, retained layouts and draw.
The current callbacks cannot carry tracking or stretch; introduce an extended
provider contract with capability bits and an explicit unavailable result.
Do not let Core compute `character_count * spacing` independently of the shaper.

Define tracking between eligible shaped clusters in visual advance order, with
no extra gap after the final eligible cluster. Combining marks must stay attached;
ligatures, Arabic joining, Indic shaping, default-ignorables, tabs and hard breaks
need explicit rules and fixtures. Retain natural shaping for zero tracking.
Choose whether nonzero tracking disables optional ligatures and expose/document
that policy; splitting UTF-8 text into independent character draws destroys
contextual shaping. Negative tracking requires an explicit overlap policy and
must not invalidate finite geometry or hit-test ordering assumptions.

Line boxes, ink bounds and advance bounds are distinct. Explicit line height
sets baseline spacing and may be smaller than ink height; it must not stretch
raster glyphs. A gradient anchor must not be inferred from cropped glyph bitmaps.
Cache keys must include all shape parameters, selected face generation, direction,
language and relevant feature settings. Paint changes should reuse geometry.
Font-load/registration changes invalidate metrics; gradient-stop changes do not.

## Gradient glyph paint

Start with solid and linear gradients; defer radial/conic gradients and CSS string
parsing. A proposed portable `cui_text_paint` is a tagged value containing solid
color or finite start/end points, a bounded copied stop array and an explicit
coordinate space. Use resolved logical coordinates with a documented anchor:
whole paragraph block or each visual line. Make the default paragraph-wide so a
multiline heading has one continuous gradient. Resolve final box/alignment before
publishing commands, and preserve the same anchor on every emitted line.

Specify stop ordering, duplicate-stop hard edges, out-of-range rejection or clamp,
zero-length axis behavior and interpolation/color space. Reuse a documented CUI
color convention rather than claiming exact Tailwind/CSS fidelity by appearance.
Recommend premultiplied-alpha interpolation with explicit color-space selection;
CSS matching must name and test the matching space. Gradient alpha multiplies
coverage and cumulative ancestor opacity exactly once. Current draw colors and
bitmap opacity already absorb ancestor opacity in Core; extending paint requires
one consistent policy for stop alpha and span alpha, not a second backend factor.

Native software currently multiplies glyph coverage by `r->ink.a`; SDL3 caches
coverage textures and applies a single RGB/alpha modulation per glyph
(`backends/software/cui_software.c:glyph`, `backends/sdl3/cui_sdl3.c:glyph`). A
single modulation cannot express a continuous gradient across a glyph. Keep the
coverage cache independent of paint and sample the gradient at each output pixel
in software. For SDL, add a bounded temporary RGBA mask/composite path first or a
proper masked gradient rendering path; changing tint once per glyph is not
sufficient. Cache any painted result by paint, anchor, density and mask revision,
or do not cache it. SDL_GPU/Sokol need equivalent explicit support or observable
unsupported status; do not silently flatten to one color.

Canvas should render a full shaped line mask and apply a gradient in the same
resolved coordinate space using a bounded offscreen surface and compositing.
Preserve context state, clips, logical-to-device scale, baseline and opacity.
Avoid one `fillText` call per letter. If browser and native shaping differ, test
semantic anchor/alpha behavior independently and state the raster difference.
All paths must include italic overhang and fallback glyph ink beyond advance
bounds, respect ancestor clips, bound temporary allocations and handle empty
text/zero bounds without NaNs or accidental giant surfaces.

## Implementation sequence and acceptance evidence

1. Add a typography example using existing body/heading/mono IDs, registered
   static variants and size/line-height tokens. Cover missing variant/family
   fallback and dynamic registration invalidation. This can deliver immediate
   usability without waiting for tracking or gradients.
2. Add versioned extended shape/provider contracts and shared capability handling.
   Establish real static condensed-face selection before variable-font work.
   Extend cache keys and binding generators together.
3. Implement tracking in one retained native layout path and route measurement,
   geometry, wrapping and paint through it. Add RTL, combining, ligature, Arabic,
   fallback-face, negative/zero tracking and fractional-size tests. Assert caret,
   selection and measured/drawn advances agree.
4. Add paint descriptors and Core lifetime/opacity/anchor tests; implement software
   as the deterministic reference, then SDL3 and headless Canvas. Test a gradient
   inside a single wide glyph, transparent stops, multiline alignment, clipping,
   italic overhang, nested opacity and density 1/1.5/2. Unsupported adapters must
   report capability limitations until implemented.
5. Publish semantic theme roles and ergonomic overrides across components, then
   update generated SDKs/packages. Re-run actual installed library fixtures and
   compare wrappers as well as native text. Preserve all 78 manifest entries.

Native tests use `python scripts/test_local.py --build-dir build` on Linux;
browser tests remain headless. GPU hardware parity remains explicitly deferred
when unavailable under the virtual display. This audit ran no UI or native tests
because it changes proposal documents only.
