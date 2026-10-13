# Portable source drop-shadow contract proposal

Read-only baseline 911e0e9, pinned gpui-component 0.7.1 commit 87d10ae5e1299d1be18670a37570c5402f952e34. No production/renderer changes. Tooltip sigma-zero correction 626648e separately fixes a confirmed source mismatch; this audit does not register or merge it.

## Verified dependency evidence

Downloaded crates.io exact 0.3.8 archives into `/home/matteo/.cache/cui-validation/gpui-shadow-next30` (and prior gpui-pre cache). SHA256 matches pinned Cargo.lock for every inspected archive:

| Package | SHA256 |
| --- | --- |
| gpui-pre | d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360 |
| gpui-pre-platform | e200b7b78836f1a0fc70767d971768e88f29d8b91b697285c15fb8e373635a3e |
| gpui-pre-wgpu | 439c5421cafe1a7cf68d0c0ae5cc82b26fad6b0e6345ac98d20acaf1f2463914 |
| gpui-pre-macos | 0d756813ff1cd741a502af60fd18869e0677795936d7349642b41c49f43120c0 |
| gpui-pre-apple | bfdd914311cd12894367c8b9908947bb0c27fd42f8b9a5ff7a0730551a83fa4c |

macOS delegates to gpui-pre-apple Metal renderer. Exact shader files are apple/src/shaders.metal and wgpu/src/shaders.wgsl (storage/webgl/subpixel shader files also ship; their complete renderer combinations have not been executed here).

## Public source contract and actual kernel

`gpui-pre/src/style.rs:349–389` publicly exposes BoxShadow color, pixel offset, blur_radius, signed spread_radius, inset. new(offset_x,offset_y,color) sets sigma 0,spread 0,inset false. `scene.rs:574–590` Shadow is renderer primitive data with no Default constructor, not a required independent portable public API; Window initializes order 0/inset 0/pad 0 before Scene assigns draw order. Insets exist publicly but theme popovers require drop shadows only; an initial CUI drop-only API must name that scope rather than expose unused inset fields.

Component `styled.rs:59–75` popover list is ordered ring then offset 4 shadow then offset 2 shadow: ring Theme.foreground RGB alpha .1, offset(0,0),sigma 0,spread +1; black alpha .1 offset(0,4),sigma 3,spread -1; black alpha .1 offset(0,2),sigma 2,spread -2. Strength clamps[0,1] and multiplies each alpha; popover passes 1. `popover_style:186–195` uses Theme.popover/popover_foreground/radius, no ordinary border. All three are painted before background; translucent surface/progress exposes shadow beneath its interior, not only outside it.

`gpui-pre/style.rs:700–740` clamps original corner radii individually to min(original width,height)/2, paints drop shadows before background, then inset shadows. `window.rs:4424–4455` iterates supplied order, skips inset, offsets then dilates bounds by signed spread; IMPORTANT: corner radii remain the ORIGINAL clamped radii, not radius+spread. Bounds are covered on the physical pixel grid (floor near edge,ceil far edge,window.rs:3196–3205), sigma/radii scale by device scale, color gets current element opacity, and current content mask is also covered with floor/ceil before attachment (window.rs:3209–3212). Empty collapsed covered bounds are possible and produce no primitive. A sigma-zero layer is a FILLED rounded rectangle beneath the background. A border band or expanded radius changes both corner geometry and transparent compositing (confirmed/fixed by626648e).

`wgpu/shaders.wgsl:319–339,977–1047` and `apple/shaders.metal:462–553,1093–1114` agree on kernel: geometry extends3 * sigma beyond covered shadow bounds; sigma 0 coverage=saturate(.5-rounded-quad-SDF). Positive sigma computes analytic horizontal Gaussian integral with rational erf approximation (coefficients .278393,.230389,.000972,.078108), then FOUR midpoint samples of vertical Gaussian across the intersection of[-3 sigma,+3 sigma] with the caster's vertical span. This is source's bounded Gaussian approximation, not a mathematically exact infinite convolution. Select corner radius by fragment quadrant. Tail has a hard rectangular3 sigma support cutoff. Do not substitute separable box blur, generic CSS shadowBlur, CSS blur radius or many expanded translucent rectangles.

Metal clip distances apply current mask; WGPU explicitly zeros samples with any negative clip distance. Shadow fragment multiplies source alpha by computed coverage. Metal uses straight RGB/source-alpha blend; WGPU chooses straight or premultiplied output/blend according to target alpha mode (`blend_color:391–395`,wgpu_renderer:710–712). sRGB/target format, sampling, f32 exp/sqrt and shader AA mean native bit identity needs actual platform proof. Source Scene.insert_primitive (`scene.rs:88–94`) culls if UNBLURRED shadow bounds do not intersect content mask; tail-only intersection therefore does not survive insertion. This observed source behavior must be tested before promising source-equivalent clipping/culling; do not silently assume infinite visible Gaussian tails.

## Current CUI contracts

`cui_effects.h`/`src/effects/cui_effects.c:96–143` blur is separable BOX blur with replicated edge samples, premultiplied internal RGBA, caller scratch width*height*4 floats, no allocation and invalid-output atomicity. It is not the source shadow kernel and its image-edge behavior differs from a shadow's transparent outside support.

Core draw list has RECT/BORDER/TEXT/CLIP_PUSH/CLIP_POP/IMAGE/MESH only; Core outline emits expanded outside border geometry but cannot supply filled Gaussian shadows. Correction626648e uses filled sigma-zero RECT/originalradius and two separate legacy portals for shadow/surface. Core prepaint copies value commands, rejects unknown types/clip commands, reserves command budgets including local/deferred clip wrappers, copies text and publishes owner state after successful callback. Local paint inherits owner/ancestor clip; deferred paint uses viewport escape and its own opacity. A new drop-shadow primitive must preserve these contracts, not mutate layout/input/semantics during prepaint.

| Adapter | Existing path | Shadow requirement |
| --- | --- | --- |
| Software | pixel-center shapes, integer straight-alpha source-over; nearest bitmap sampling, clip stack | shared source-kernel sampler/raster, physical pixel mask and source-over |
| SDL Renderer | bounded64 MiB bitmap cache, SDL textures/clip; draw images via cached raster | same raster into existing texture path; preflight resources before clear |
| SDL GPU | analytic rounded SDF shader, texture atlas/cache, straight-alpha image blend, linear sampler/scissor | shared raster texture initially; no generic blur shader substitution |
| Sokol | analogous rounded coverage shader, straight-alpha images, linear sampler/scissor,64 MiB bitmap budget | shared raster texture initially, explicit scale/crop alignment |
| Canvas | Canvas rounded paths/evenodd borders, RGBA bitmap cache, DPR transform, native source-over | transfer shared WASM-generated raster, drawImage at aligned logical extent; avoid browser shadowBlur |

All five implement IMAGE. Their native unknown-command branches return false (GPU/Sokol attach errors); current Canvas dispatcher must gain an explicit shadow branch rather than treating a new type as a rectangle. Renderer capability flags for SDL Renderer/SDL GPU omit IMAGE despite implementations; update/report capability honestly when adding SHADOW. Shared bitmap keys include source pointer/revision/crop/radii/tint/size; opacity/placement do not invalidate pixels. A shadow cache must not retain caller stack layer data, use color/opacity as accidental unbounded cache keys, or leave one adapter silently ignoring a new type.

## Smallest complete implementation proposal (approval required)

Prefer one append-only Core `CUI_DRAW_DROP_SHADOW` plus value-only paint data, a shared pure Effects reference kernel/rasterizer and bounded fallback consumption by ALL five adapters. It avoids new GPU shaders initially while preserving source kernel rather than a box-blur approximation. An effects-only unused raster helper is not a completed capability. Deliver Theme popover projection and a real tooltip owner consuming all THREE layers in the same slice; replace its dedicated sigma-zero command so no duplicated ring is painted. Append renderer capability CUI_RENDER_DROP_SHADOW=64 only when actual consumption is present. No Base→Theme dependency.

Illustrative exact declaration direction, to finalize with Core/adapter owners before implementation:

```c
typedef struct {
    float offset_x, offset_y, sigma, spread;
    cui_color color;
} cui_drop_shadow;
typedef struct {
    cui_rect caster;
    cui_corner_radii radii; /* original caster radii, clamped before spread */
    cui_drop_shadow layer;
} cui_drop_shadow_paint;
/* append value paint to cui_draw_command; bounds is conservative PAINT extent */
bool cui_set_drop_shadows(cui_context *, const cui_drop_shadow *, size_t count,
                          cui_corner_radii); /* count 0 clears, copies <=8 */
bool cui_prepaint_drop_shadow(cui_prepaint_writer *, const cui_drop_shadow_paint *,
                              bool deferred);
```

Shared checked query/raster API should take explicit finite positive logical→physical scale, caller-owned effect buffer and output logical placement; no allocated/borrowed shader/platform data. Query computes original-radius clamp (NEVER reclamp to the smaller spread rectangle), offset/spread, physical floor/ceil, support cutoff, raster dimensions/bytes and conservative logical paint extent, atomically. Core's paint extent can conservatively include one logical pixel beyond3 sigma for physical rounding; actual sampling uses adapter scale. Final declaration must settle scale ownership: software units currently map to pixels, Canvas has explicit DPR; GPU/SDL need actual drawable/list mapping, including logical presentation. Do not infer it from design density or pretend arbitrary nonuniform scaling is GPUI's uniform device scale. This is an explicit unresolved integration contract, not a reason to hardcode all adapters to1.

Use source's f32 erf/four-sample model as the documented kernel; sigma 0 uses its1px SDF coverage. Every layer/color field is consumed. No inset/inner-hole fields in a drop-only first API; public source inset support remains pending. Bounded design limits proposed: <=8 layers/node, source coordinates/dimensions <=16384 magnitude/extent, sigma 0..256, abs(spread/offset)<=4096, finite computed extents, raster sides<=16384 and max16 M texels/64 MiB across active shadow cache budget. Validate all layer lists/count multiplication before changing node/frame data; collapsed negative spread succeeds as no paint. Query/raster invalid/capacity failure leaves outputs/pixels untouched. Exact limits require profiling/approval; popover's three tiny layers fit comfortably.

Setter copies caller layers into frame storage immediately; writer receives only values and Core copies them. Renderer-local mask caches own buffers/textures until retirement/destruction and reference no caller stack. Separate coverage mask from RGB/progress where possible: immutable geometry key includes covered caster dimensions/origin phase, original scaled radii,sigma,spread,device scale and support extent. Integer translation and changing opacity should reuse it; physical snapping can change dimensions with fractional translation, so excluding every position component is wrong. Theme RGB can tint the coverage mask; source/ancestor opacity is applied exactly once per layer. In a simpler colored raster fallback, RGB changes invalidate revision but opacity remains a bitmap paint multiplier, not a pixel cache key.

Prepare all required shadow rasters/texture resources before clearing/submitting the frame; fail explicitly on exhausted budget instead of dropping shadows. Core emission failure follows existing owner rollback. Borrowed raster/external host effects/later global renderer failure are outside component rollback; do not promise whole-frame restoration without renderer preflight. Never allocate unbounded work in prepaint. Each shadow consumes one draw command plus existing wrapper costs, independently of kernel work; raster work is bounded4 samples/texel, no sigma-sized convolution scratch.

Maintain supplied layer order and paint all shadows before background; do not batch shadows across intervening owners. Inherit current ancestor opacity and clip for ordinary Core boxes, before applying a box's child overflow clip. Deferred tooltip shadow/content escape owner clip together and carry their own fade under viewport/modal policy. For source mode, mirror observed unblurred-bounds mask culling or explicitly identify/test any intentional CUI extension; tail clipping must not silently differ. No interaction/focus/hitbox is added for shadow pixels. Radius/spread query must avoid the former radius+1 mistake.

## Proof required before completion

Headless pure reference: independent horizontal Gaussian CDF comparison (with expected source erf error), symmetry/edge/corner samples, exact4sample support cutoff, sigma 0 SDF ramps, signed spread/offset, radius unchanged/clamped original, empty collapse, source-over translucent surface, three-layer order/alpha, opacity exactly once, NaN/infinity/overflow and capacity atomicity, overwritten input lists, cache resize/scale/phase/revision eviction and no unbounded frame work. Distinguish independent numerical reference checks from tests merely copying shader implementation.

Core integration: actual current-frame resize/scroll stationary pointer, parent clip vs child overflow, passive/modal occlusion, deferred portal fade, bounded command/memory failure, copied payload lifetime, no shadow semantics or redraw stall. Pixel tests need each actual adapter: Software reference pixels, real SDL Renderer readback, SDL GPU offscreen/readback, actual Sokol context readback, real Chromium WASM at DPR1/2 with fixed device-pixel probes. Existing GPU null/mock tests prove plumbing only. Gaussian tails must differ meaningfully from existing BOX blur; compare all3 theme layers on light/dark and translucent surfaces. Exact native pinned GPUI Metal/WGPU image identity remains unverified until matching source-renderer captures, identical scale/target color format and shader AA are compared. Portable source-kernel/geometry proof is useful but not native identity or full public BoxShadow parity.

## Audit validation

This documentation-only branch is based on 911e0e9b3ae38eb2a7d11fa1dcb5565fcc90f0f8. Strict-warning configure/build completed with exit 0 (410 build steps); completed CTest exited 8: 126/127 passed, only `distribution` failed because generated distribution is stale on this isolated baseline. No production change or new runtime test was introduced by this read-only audit. Logs: `/home/matteo/.cache/cui-validation/shadow-contract-{config,build,ctest}.log`. Dependency archives were verified against pinned checksums; native source shader execution and production shadow capability remain unverified.
