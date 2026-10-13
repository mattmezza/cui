# Complete bounded inset-shadow proposal

Read-only baseline `c866950`; upstream component 0.7.1 is pinned to `87d10ae5e1299d1be18670a37570c5402f952e34`. The dependency is gpui-pre 0.3.8, including its WGPU and Apple renderer crates. Companion JSON records exact hashes and source locations. This is a production proposal, not an implementation or a BoxShadow completion claim.

## Exact source behavior

Public `BoxShadow` (`style.rs:349–393`) contains color, offset, blur radius, signed spread and an inset boolean. Its constructor requires offset/color and defaults sigma/spread to zero and inset to false. Style defaults to an empty list. There is no implicit inset layer and the component popover factory remains drop-only. No pinned component consumer was found requiring an invented inset Theme preset.

Style clamps each original radius to half the original minimum dimension, then paints filtered DROP layers, background, filtered INSET layers, children, and ordinary borders (`style.rs:706–740` and following border branch). Relative input order survives within each phase; a mixed list is not emitted in unconditional input order. Element opacity encloses Style paint; its own overflow clip is installed inside the child continuation (`elements/div.rs:2543–2553`). Both phases therefore use the current ancestor mask before child overflow. Existing CUI mesh/underlay/prepaint content are extensions: inset emission must precede these local content streams without changing their legacy order or moving the existing outline.

`Window::paint_inset_shadows` (`window.rs:4460–4501`) first covers the original element. For each inset layer it computes `hole = (bounds + offset).dilate(-spread)`. Materialize the offset origin, dilated origin and size in source f32 order, then derive far from stored origin plus stored size. Covering floors near, ceils far and clamps far to near (`window.rs:3196–3205`, `geometry.rs:1067–1073,1323`). Do not reject zero/negative logical hole size before covering: the previous drop correction applies equally to holes.

Hole radii are `max(original_clamped_radius - spread, 0) * device_scale`; element radii are the original clamped radii times scale. Do not reclamp hole radii to the derived hole size. Offset and asymmetric corners require independent hole and element quadrant selection.

The shader vertex domain is the covered element, with no Gaussian tail enlargement. Fragment alpha is `(1 - hole_coverage) * element_coverage`. Sigma zero evaluates each rounded-rectangle SDF with `saturate(.5 - distance)`, including the source unrounded fast path `max(qx,qy)`, not Euclidean outside-corner distance. Positive sigma uses the same f32 erf approximation and four midpoint Y samples as DROP. Crucially, hole convolution has no DROP rectangular three-sigma early return. Only its Y integration interval is bounded to ±3 sigma. Preserve the source per-endpoint expression `.5 + .5*erf(...)` before subtracting the two X integrals, rather than silently substituting DROP's algebraic `.5*(erf(b)-erf(a))` expression. Do not reuse DROP's intermediate coverage clamp without proving equivalence: source complements the raw convolution. Final stored RGBA8 coverage has an explicit [0,1] quantization/clamp boundary; native shader/target floating-point identity is separate.

Scene insertion intersects `Shadow.bounds` with the covered ancestor mask (`scene.rs:88–94,235–238`). For INSET, that bounds field is the covered hole, not element. Empty or mask-disjoint holes are culled even if the element overlaps. Once inserted, raster/draw clipping is against element and current mask, not their intersection with the hole. A mathematical full-fill complement for an empty hole would contradict this pinned renderer behavior. Source WGPU and Metal agree on these contracts (`shaders.wgsl:360–379,976–1045`; `shaders.metal:462–555`).

## Proposed portable public declarations

All declarations are additive; existing struct fields, enum values and DROP APIs remain unchanged:

```c
typedef enum {
    CUI_BOX_SHADOW_DROP = 0,
    CUI_BOX_SHADOW_INSET = 1
} cui_box_shadow_kind;
typedef struct {
    cui_drop_shadow layer;
    cui_box_shadow_kind kind;
} cui_box_shadow;
enum { CUI_BOX_SHADOW_MAX = CUI_DROP_SHADOW_MAX };
typedef cui_drop_shadow_paint cui_inset_shadow_paint;

bool cui_set_box_shadows(cui_context *, const cui_box_shadow *, size_t count,
                         cui_corner_radii);
bool cui_prepaint_inset_shadow(cui_prepaint_writer *,
                              const cui_inset_shadow_paint *, bool deferred);
/* Append CUI_DRAW_INSET_SHADOW; its existing command.shadow stores this value. */
/* Append CUI_RENDER_INSET_SHADOW = 128 to renderer capabilities. */

typedef struct {
    cui_rect bounds;          /* Logical placement of covered ELEMENT raster. */
    cui_rect covered_hole;    /* Physical integer bounds; insertion/cull only. */
    cui_rect covered_element; /* Physical integer bounds; raster/draw domain. */
    unsigned width, height;
    size_t bytes;
} cui_inset_shadow_raster;
bool cui_effect_inset_shadow_query(cui_inset_shadow_paint, float scale,
                                   float origin_x, float origin_y,
                                   cui_inset_shadow_raster *out);
bool cui_effect_inset_shadow_sample(cui_inset_shadow_paint, float scale,
                                    float origin_x, float origin_y,
                                    float x, float y, float *out);
bool cui_effect_inset_shadow_raster(cui_inset_shadow_paint, float scale,
                                    float origin_x, float origin_y,
                                    cui_effect_buffer output);
```

No extra public radius metadata, mutable state, borrowed layers, platform types, arbitrary shader callback or unused Theme field is needed. Core can use private checked validation/element-support metadata; physical Effects query remains authoritative, including fractional zero-size covering. A standalone Effects implementation must not require linking Core implementation. The shared private geometry/validation helper must be inlined into both generated headers by the integrator.

The new setter validates every kind/layer/radius and total capacity before mutation and copies at most eight TOTAL mixed layers. Count zero clears both phases. Existing `cui_set_drop_shadows` replaces the same entire list with DROP entries, so it clears any previous insets; its existing API and behavior for drop-only callers remain exact. Unknown enums, invalid pointer/count and finite-domain violations fail atomically. Changing box shadows adds no hit regions, activation, focus or semantics. Core counts both phases before emitting anything. Generic paint-only prepaint emission recognizes the new command and copies its paint value; NULL helper input follows existing latched writer failure behavior. Caller payload/layers may be overwritten after registration. Manual prepaint callbacks control their own ordering around surfaces; they do not magically become an automatic Style stack.

Pure query/sample/raster have no ancestor-mask input. Query succeeds empty when covered hole or covered element has zero area. Sample is zero outside covered element or for an intrinsically empty hole; otherwise it evaluates the complete complement, including positions outside the hole. Mask-dependent Scene culling is the prepared command consumer's responsibility. Query validates complete geometry and resource limits before publishing output. Raster validates dimensions/stride/size and the whole paint before touching caller storage. No implicit allocation or pointer retention.

## Finite domain and preparation contract

Retain the established bounded DROP input domain: caster coordinates/dimensions ≤16384 (dimensions nonnegative), absolute offset/spread ≤4096, sigma in [0,256], input radii in [0,16384], finite values; uniform scale [1/65536,64], origin magnitude ≤1048576. Sigma zero is separate; positive physical sigma below `4*sqrt(FLT_MIN)` is explicitly rejected atomically. The hole may be larger than the element: do not apply raster-area limits to hole geometry merely because only the element is rasterized. Check intermediate representability and final covered coordinates without silently clamping source geometry. Supported input must never publish NaN/Inf coverage. Any newly necessary narrower numerical support must be an explicit checked rejection with evidence, not a hidden approximation.

Raster limits remain ≤16384 per side and ≤16M texels; empty results require no pixels. DROP and INSET share eight layers/node, command/clip capacities, 64 cache entries and the combined ≤64MiB owned CPU-plus-upload cache budget. Pin all prepared resources through submission; retire only unpinned entries. Preparing inset resources participates in the same transaction as other shadows: no target resize, clear, submission or GPU swapchain acquisition before successful frame preflight. Preserve the legal post-acquisition SDL GPU fallback submission and atlas publication fixes. Existing no-shadow anisotropic paths remain unchanged; any frame containing either shadow kind rejects genuinely nonuniform transforms before clear.

Cache keys include kind and all paint values (RGB, not presentation alpha), uniform transform, and actual f32 phase. Reuse normalized translation only when BOTH hole and element checked dimensions/near edges and raster placement match the actual query; inset can fail normalization differently for these two rectangles. Query hole/element independently from materialized source values. Never reuse a DROP raster for an INSET or infer physical scale from density/backing rounding. Alpha-only changes may reuse coverage; opacity applies once during composition.

## All five consumers and host bridge

- **Software:** use existing physical geometry scale 1. Shared preparation obtains full covered-element raster, culls using covered hole, crops against physically covered current mask and target. Consume the resulting bitmap once, with supplied color/ancestor alpha. No extra rounded clipping pass: element AA is already in raster coverage.
- **SDL Renderer:** derive actual scale/origin from renderer presentation/viewport; reject unsupported nonuniform shadow transforms before resource/clear side effects. Preallocate/upload all mixed-kind textures, apply shadow-specific covered physical mask and restore clip afterward. Preserve ordinary draw behavior. Test readback before real RenderPresent through the existing test-only wrapper.
- **SDL GPU:** use actual target/list projection and existing uniform tolerance. Mixed-kind upload/mesh resources are prepared before swapchain acquisition. The shadow bitmap quad covers cropped element raster; scissor uses covered physical mask with actual-target clamp. Preserve previous offscreen surface and dirty atlas on failures.
- **Sokol:** use actual target/list transform, preflight all raster/upload resources and quad budget before pass. Use the same covered-element crop and physical mask, no additional element-radius shader approximation. Actual GL readback is required, not mocked calls.
- **Canvas:** extend the reusable module to recognize `type: 'insetShadow'` beside existing `'shadow'`, copying the same sixteen paint values. Preserve existing callback calls by adding optional fifth `kind` argument to rasterizer/query, default DROP. INSET query returns `coveredHole`, `coveredElement`, raster `bounds`, width/height. Existing DROP shape remains exact. Add C bridge inset query/prepare exports using existing sixteen-value paint staging, plus checked hole/element bound getters. Root owns Gallery command decoding/export registration: it dispatches the appended command kind without changing old numeric values. Use actual `ctx.getTransform()` uniform scale/origin, prepare before backing-dimension assignment and clear, and draw with ordinary clip stack suspended/reapplied because raster crops already represent physically covered masks. Prepared generation lifetime and copied WASM pixels remain explicit.

Renderer capability bit is advertised only when the corresponding real INSET path is consumed. The Theme popover factory and Chart tooltip shadow settings do not need new inset fields for this slice: their pinned source layers are DROP. No new user-facing Gallery controls are mandatory for implementation; a dedicated browser probe provides the complete actual path, with root adding an example after approval.

## Required shared fixtures and failures

Pin archive/file hashes in a versioned fixture. Separate literal geometry/byte expectations from a source-formula reference, and independent numerical convolution from a replica of the four-sample code:

1. Square caster [20,20,20,20], sigma0, spread2, offset0: hole [22,22,16,16]. Black opaque inset over white: center (21.5,30.5) black, (22.5,30.5) white, exterior (19.5,30.5) unchanged. DPR2 uses the same logical design and actual projected pixels. This catches missing inset, wrong spread sign and wrong phase.
2. Offset hole, asymmetric original radii, positive/negative spread and independent quadrant choices. Assert original clamp and `max(r-spread,0)` with no second hole clamp. Direct SDF point expectations include unrounded diagonal outside-hole points where max and Euclidean distance differ, and independent element AA.
3. Nonzero sigma: compare rounded/square complement against high-accuracy double convolution with a documented error bound, then locked source four-midpoint fixtures. Include a point beyond hole's three-sigma X extent INSIDE the element: it must retain inset complement instead of DROP's hard cutoff logic.
4. Source materialized f32 far edges in both axes, high origins/offsets, fractional logically collapsed hole producing one pixel, zero original dimensions, genuinely empty covered hole, and large negative spread. Empty or ancestor-mask-disjoint HOLE suppresses all paint even with element overlap; intersecting hole permits complement in element areas outside hole. This distinguishes insertion culling from raster clipping.
5. Mixed DROP/INSET input order, opaque and translucent backgrounds, colored layers and progress/ancestor alpha: phase order must be DROP→surface→INSET→child/border. Assert under/over-surface numeric composition and child-overflow versus ancestor mask. Bare/no-style Chart behavior remains unchanged.
6. Copied setter and copied prepaint payload destruction/relocation; setter replacement via legacy DROP clears stale insets. Invalid enum/count/NaN/Inf/tiny sigma/capacity leave owner/output/target unchanged; no acquisition/clear/draw/present on predictable failure. Fault injection preserves submitted-frame resources.
7. Cache cross-kind separation; opacity-only reuse; translation/resize/scrolled fractional phases; both geometry normalization fallbacks; bounded multi-layer eviction and budget failure without dangling resources. Odd physical targets, fractional masks at DPR1/2 and near/far target edges.
8. Run the same literal composed-pixel fixtures on Software, actual SDL Renderer OpenGL and software drivers, actual SDL GPU Vulkan, actual Sokol GL and Canvas WASM at DPR1/2. Compare full frame bytes before/after rejected preflight, including a pending resize. Record device/driver evidence and preserve first failures; mock GPU tests alone do not certify renderer paths.

Native GPUI Metal/WGPU target-format, Hsla conversion, AA and transcendental rounding/image identity remain separate pending proof. Named public shadow convenience factories and other component owners' appearance are separate source contracts. A complete bounded INSET implementation would close the public inset capability within the declared portable domain; it would not justify full BoxShadow, all adapters' source-native identity or Chart-family completion.

## Production ownership proposal

A shadow owner needs narrow Core shadow structs/node copied storage/validation/emission/prepaint regions, private shared bounds helper, Effects kernel/query/raster, shared backend shadow cache plus shadow-specific paths/capability flags in Software/SDL Renderer/SDL GPU/Sokol, reusable Canvas module/C bridge, and new scoped unit/integration/browser fixtures plus shadow guide. Avoid unrelated Core accepted-event/frame-guard/focused-text regions. Root alone owns CMake/WASI/Pages/scripts/distributions, Gallery serialization/export wiring and final manifest/ledger registration. No Base→Theme dependency or Theme/source preset addition is required. Exact API approval and disjoint ownership assignment must precede production edits.

Strict-warning portable build completed, exit 0 (409 steps). Complete CTest terminated with 132/133 passing, exit 8: the sole distribution freshness check reports existing root-owned `cui.h`, `cui_effects.h`, `cui_edit.h`, `cui_base.h`, and `cui_components.h` stale. Source-only CTest passed 132/132, exit 0. Exact cached crate hashes and inspected extracted source bytes were reverified against their archives in this audit. This audit reruns no native adapter or browser rendering and does not present earlier DROP evidence as new INSET proof.
