# Owned element backgrounds: semantic and transport checkpoint

Design only, 2026-10-31, integration `f724017c`; no API below exists or is approved
for production. Preserve all 78 entries and existing box/style/draw ABI. Button's
bounded shadow facade and Slider's Aurora thumb gradient motivate a shared Core
paint primitive, followed by explicit component/part/theme and binding ownership.
This is a semantic-background proposal, not a claim of complete CSS/source parity.

## Evidence and reusable mechanisms

Original GPUI Kit [87d10ae5](https://github.com/longbridge/gpui-kit/tree/87d10ae5e1299d1be18670a37570c5402f952e34)
pins `gpui-pre=0.3.8`. Its [color representation](https://docs.rs/crate/gpui-pre/0.3.8/source/src/color.rs)
has solid/two-stop-linear plus slash/checkerboard. The exact published
[WGPU shader](https://docs.rs/crate/gpui-pre-wgpu/0.3.8/source/src/shaders.wgsl)
prepares linear colors from Hsla, interpolates RGB/Lab and alpha independently,
and rescales the short axis before dominant-axis projection. Its extracted-source
SHA256 is `1d91f2c8a1958a241f1b659a4137c160592c8076b9be09049db4c9fb9a344529`.
Pinned `crates/component/src/slider.rs` consumes a Background-valued thumb token;
`theme/theme_color.rs` keeps that brush separate from its representative Hsla.
`theme/schema.rs:test_aurora_theme_parses_gradient_backgrounds` verifies the
`themes/aurora.json` brushes; flattening them to representative colors loses paint.

| Existing CUI authority | Actual capability / limitation |
| --- | --- |
| `include/cui/cui_plot_view.h`, `src/components/cui_plot.c`, `docs/PLOT_MODEL.md` | Source projection, reversed/equal stops, sRGB/Oklab, patterns; separate byte and source-Hsla→linear-premultiplied samplers. Reuse audited equations/tests; Core must not depend on components/Plot types. |
| `cui_effect_linear_gradient`, `docs/BACKGROUNDS.md` | Caller RGBA8 raster, pixel-center diagonal, angle 0 right, premultiplied encoded-sRGB interpolation. Not source/CSS geometry or Oklab. |
| `cui_text_paint`, `docs/TEXT_GRADIENT.md` | Copied 2..8 stops; explicit endpoints, premultiplied encoded-sRGB glyph paint. Not rectangular background; prepaint text-gradient emission is rejected. |
| `cui_paint_bitmap`, bitmap underlay, prepaint | Borrowed pixels through renderer completion; underlay is above background/inset shadows, prepaint above that. Neither replaces the background at its proper paint position. |
| `cui_mesh_paint`, `backends/software/cui_mesh.h` | Borrowed coverage vertices, one flat premultiplied linear color. No per-vertex/background gradient. |
| Core resources / frame storage | Existing keyed resource size/callback identity, collection and next-begin lifetimes; no gradient cache, device scale or arbitrary retained resource graph may be assumed. |

All adapters currently dispatch RECT/IMAGE/MESH, not a semantic rectangle brush:
software (`cui_software.c`) rasterizes; SDL_Renderer (`cui_sdl3.c`) has bounded
bitmap/shape paths; SDL_GPU (`cui_sdl_gpu.c`) and Sokol (`cui_sokol.c`) use private
quad uniforms/textures; Canvas (`web/gallery.js`) reads explicit WASM fields and
uses roundRect/image paths. `cui_bitmap.h` caches source pointer/revision/crop/
shape at logical ceil dimensions; `cui_shadow.h` has its OWN 64-slot/32MiB cache.
Neither is an approved background cache or a DPI-correct gradient implementation.

## Proposed value contract and API sketch

```c
typedef enum { CUI_BACKGROUND_SOLID, CUI_BACKGROUND_LINEAR } cui_background_kind;
typedef enum { CUI_BACKGROUND_SRGB, CUI_BACKGROUND_OKLAB } cui_background_space;
typedef enum { CUI_BACKGROUND_CSS, CUI_BACKGROUND_GPUI_038 } cui_background_model;
typedef struct { float r, g, b, a; } cui_background_color; /* straight LINEAR RGB */
typedef struct { float position; cui_background_color color; } cui_background_stop;
typedef struct {
    cui_background_kind kind;
    cui_background_space space;
    cui_background_model model;
    cui_background_color solid;
    float angle_degrees;
    cui_background_stop stops[2]; /* fixed owned values, no count or pointers */
} cui_background;
cui_background_color cui_background_color_from_srgba8(cui_color);
bool cui_background_color_from_source_hsla(float h, float s, float l, float a,
                                            cui_background_color *out);
bool cui_background_valid(const cui_background *);
bool cui_set_background(cui_context *, const cui_background *); /* open box */
bool cui_element_set_background(cui_context *, cui_id, const cui_background *);
/* NEW transport decision; field names and entry points require Core review. */
typedef struct { size_t command_index; cui_background paint; float opacity; }
    cui_background_binding;
typedef struct {
    cui_draw_list commands;
    const cui_background_binding *backgrounds;
    size_t background_count;
} cui_background_draw_list;
cui_background_draw_list cui_end_frame_backgrounds(cui_context *);
```

Colors are finite straight linear RGB/alpha in [0,1], preserving hidden RGB at
alpha zero. The sRGBA8 constructor decodes RGB transfer; source-Hsla explicitly
uses the pinned shader conversion, not CSS-HSL/byte reinterpretation. Interpolation
space is mandatory policy, not a guessed Tailwind/browser default. No premultiplied
`cui_linear_color` stop: it would erase transparent endpoint RGB needed by source.
SOLID ignores inactive gradient fields; LINEAR validates both stops, known enums,
finite angle [0,360] (360 aliases zero), and finite positions [0,1]. CSS requires
ordered stops; GPUI preserves reversed order. Equal stops select the second color
at/after the boundary: a deterministic extension of source's undefined division.

For logical border-box W,H>0 and center C, angle zero points up, 90 right.
CSS uses d=(sin A,-cos A), L=abs(W*d.x)+abs(H*d.y), t=.5+dot(P-C,d)/L;
GPUI scales d.y by H/W when W>H, otherwise d.x by W/H, then uses
 t=.5+dot(P-C,d)/(length(d)*(abs(d.x)>abs(d.y)?W:H)).
Both map t through the two stop positions and clamp; zero-sized boxes paint empty.
CSS premultiplies components in the selected interpolation space, mixes with
alpha, unpremultiplies (zero alpha gives zero RGB), then returns to linear RGB;
GPUI mixes straight components and alpha, THEN premultiplies the final fragment.
These differences are intentional. CSS geometry/alpha references:
[Images 3](https://www.w3.org/TR/2023/CRD-css-images-3-20231218/#linear-gradients),
[Color 4](https://www.w3.org/TR/2026/CRD-css-color-4-20261029/#interpolation-alpha).
Use standard Oklab matrices; retain computed out-of-gamut float RGB until explicit
SDR output clamp/encode/round. That output policy is bounded CUI paint, not a full
CSS gamut-mapping/HDR or GPU-f32 bit-identity claim. Quantize straight encoded
RGB/alpha to RGBA8 once; apply inherited opacity with existing alpha truncation,
then existing shape coverage, with no duplicate image opacity. Old Plot/effects/
text helpers retain their semantics; extract pure math without changing them.

## Composition, ownership and bounded failure

Set copies the complete active value now; last successful assignment wins. NULL
removes the override and restores the submitted box background; transparent SOLID
explicitly suppresses it. No inheritance/list merging/pending-next-widget style.
The open setter can address an anonymous current box; named setter uses existing
current-frame owned-node/map/config guards, rejecting missing/previous-frame/
layout-only/ambiguous-folded IDs and measurement/prepaint/diagnostic mutation.
Validation/capacity preflight publishes no partial replacement; preserve other
shadow/outline/mesh/underlay/prepaint owners on reset. No default custom paint is
attached to unstyled nodes. Geometry/input/semantics/callback timing remain intact.
Replace the original fill exactly between DROP and INSET, retain existing outline/
underlay/content/border order; never paint a gradient over an existing fill.
Anchor to full current border-box bounds before clips, not visible/previous bounds.
Corners use existing CUI individual clamp to min(W,H)/2, not CSS overlap scaling;
ancestor clips/portals/layers and existing opacity apply once after interpolation.
Keep each adapter's existing RECT coverage policy; cross-adapter edge AA is not
currently byte-identical. No new CSS group-opacity/offscreen compositing claim.

Proposed hard ceilings: <=4096 styled boxes and existing max_elements/frame_bytes;
no persistent context raster allocation. Raster fallback <=16384 per physical side,
<=4M pixels per brush and <=8M total pixels/32MiB CPU per prepared frame; streamed
uploads also obey a separate aggregate <=32MiB budget. These are NEW limits for
review, not existing resource guarantees. Scale axes finite [1/65536,64], origin
within +/-1e6; checked physical covering/inverse sampling preserves logical angle
under DPI/nonuniform axis scale. Clip work without changing the anchor. Reject
unsupported transforms, overflow, allocation/upload or aggregate budget failure
explicitly; no flat-color fallback. Final conversion/preparation failure publishes
an empty view and an error. Copied frame descriptors survive renderer completion
until next begin_frame; no new in-flight-frame/thread lifetime is implied.

## Transport alternatives and approval gate

1. Caller-owned raster + existing IMAGE is usable today, with explicit size/revision/
   lifetime management. It is NOT this owned semantic API: Core lacks device scale,
   and underlay/prepaint ordering is wrong for inset-shadow backgrounds.
2. Core-owned raster lowered to IMAGE avoids draw ABI changes but requires a NEW
   pixel owner/budget/DPI contract; logical-resolution raster is a quality compromise.
3. New draw kind/value payload is clean but enlarges public command stride and
   breaks old ABI/adapters/bindings; do not hide data in bitmap/mesh/text fields.
4. Recommend reviewing the sketched explicit sidecar view: old structs stay exact;
   Core records a transparent RECT placeholder at the original fill position and
   owned indexed brush/opacity entries (including originally transparent boxes).
   Reserve one fill slot per owner; validate unique in-range RECT indexes and clip
   balance before painting. Replacements reuse storage; old max_commands still
   bounds conversion. New render entry points replace placeholders. Legacy
   end_frame rejects outstanding overrides; old render
   APIs must never silently consume the sidecar's incomplete base list.

The sidecar/renderer extension is a NEW ownership/transport protocol requiring
approval BEFORE production, not an existing capability. It needs explicit new
render entry points/capability negotiation for software, SDL_Renderer, SDL_GPU,
Sokol and Canvas/WASM field export. Software reuses rounded-RECT traversal;
GPU/Sokol add private brush shader parameters; SDL_Renderer and Canvas may use
bounded caller/adapter-owned per-frame rasters. Native Canvas gradients alone
cannot implement source straight-alpha/Oklab policy. Specify scratch ownership,
release/upload completion and failure before writing those fallbacks; changed
raster bytes require a fresh bitmap revision, never an overwritten live upload.
Initially use no retained cache. Any later cache needs its own key/eviction/invalidation
review (brush, shape, DPI, fractional placement/phase; opacity if baked).

Required proof: independent source/CSS sampler goldens (cardinals/obliques,
non-square projection, reversed/equal/transparent stops, both spaces, Aurora
thumb); old untouched draw-list/pixel oracles; set/replace/reset/collision/open/
closed/portal/current-frame/clip/corners/opacity/order/resize/DPI/suppression;
transactional invalid and exact-budget/overflow/upload failures; all five adapter
interior samples and their existing edge-coverage tolerances; headless browser
and private-Xvfb native tests, no desktop windows. No compiler ran for this audit.
Only then map theme brushes and Button/Slider parts, preserve representative
accessibility colors, and add owned typed foreign authoring/version checks.
Theme brush storage/access must be additive too: do not enlarge `cui_theme` or
silently replace its existing scalar palette. Missing brush support stays an
explicit importer limitation, not a first-stop color presented as full support.
No all-78/complete-Background claim: source patterns, multistop/radial/repeating
CSS, arbitrary image backgrounds, font/text/focus and remaining component parts
stay explicit follow-on work; no default palette/pixels change is authorized.
