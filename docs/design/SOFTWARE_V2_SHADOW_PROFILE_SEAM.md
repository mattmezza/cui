# Private source-shadow profile seam (proposal)

Status: design checkpoint only. No extraction, V2 coverage-policy approval, public API or adapter capability is implemented here. `SOFTWARE_V2_SHADOW_BOX.md` records the test-only footprint experiments; neither reference replaces the legacy source profile.

## Actual source and ownership

`src/effects/cui_effects.c` currently owns `shadow_geometry`, `inset_geometry`, `shadow_sample`, `inset_sample`, `shadow_clamp`, `shadow_erf` and `inset_sdf`. Public Effects query/sample/raster functions call these static helpers. The existing private `src/core/cui_drop_shadow_bounds_internal.h` provides logical float materialization and value validation, shared with Core. Core does not need the proposed physical sampler and must not depend on Effects.

Extract those seven helper bodies, without algebraic changes, into a new private `src/effects/cui_shadow_profile_internal.h`. Use C11 `static inline` definitions and CUI value types supplied by the including translation unit, plus standard math/float/integer headers. Effects and the future software V2 preparer include it; no public symbol, public header inclusion, platform type, allocator, context lookup or global cache is added. Keep logical validation in its current Core-owned header. Update the canonical amalgamation inliner for this exact private include; never edit generated output.

Proposed exact private records and operations (names are proposals):

```c
typedef struct {
    cui_drop_shadow_paint paint;
    float scale;
    cui_rect covered;
    cui_corner_radii radii;
} cui_shadow_drop_profile_internal;
typedef struct {
    cui_inset_shadow_paint paint;
    float scale;
    cui_rect hole, element;
    cui_corner_radii hole_radii, element_radii;
} cui_shadow_inset_profile_internal;
typedef struct { int64_t left, top, right, bottom; }
    cui_shadow_profile_support_internal;

static inline bool cui_shadow_drop_prepare_internal(
    cui_drop_shadow_paint, float scale, float ox, float oy,
    cui_shadow_drop_profile_internal *out);
static inline bool cui_shadow_inset_prepare_internal(
    cui_inset_shadow_paint, float scale, float ox, float oy,
    cui_shadow_inset_profile_internal *out);
static inline float cui_shadow_drop_eval_internal(
    const cui_shadow_drop_profile_internal *, float x, float y);
static inline float cui_shadow_inset_eval_internal(
    const cui_shadow_inset_profile_internal *, float x, float y);
static inline bool cui_shadow_drop_support_internal(
    const cui_shadow_drop_profile_internal *,
    cui_shadow_profile_support_internal *out);
static inline bool cui_shadow_inset_support_internal(
    const cui_shadow_inset_profile_internal *,
    cui_shadow_profile_support_internal *out);
```

Records contain only copied values. Their owner is the calling stack or adapter's already-counted transaction metadata. No borrowed memory, retained arena, raster dimensions or resource lifetime attaches to them. They are private constructed records: `eval`/`support` require a successful `prepare` result, no external mutation, finite sample coordinates and a nonnull record. No public untrusted-record validation protocol is introduced. The existing public sample wrappers retain their finite-point/null-output checks and failure behavior.

## Validation and exact compatibility

`prepare` copies into a local record, calls the original geometry logic, then publishes once. Invalid input/null output leaves the caller record unchanged. Validation preserves exactly the current Effects domain: caster coordinates/dimensions and radii bounded by16384 (dimensions/radii nonnegative), offset/spread magnitude4096, sigma0..256; finite scale1/65536..64; positive sigma*scale at least4*sqrtf(FLT_MIN); finite origins of magnitude at most1048576. Color remains unrestricted byte data and is not reinterpreted. V2's stricter scale/origin options remain the adapter's separate admission rules.

Retain every float intermediate and operation order, notably logical stored origin plus stored size for far edges, floor/ceil covering, caster-min-half radius clamp before scale, inset spread negation and hole-radius shrink before scale. Do not replace float arithmetic with double, factor `sigma*scale`, fuse expressions, normalize the erf approximation, change the four Gaussian samples or replace the SDF ramp. Preserve the distinct DROP and INSET coordinate expressions, including their current rounding. Keep negative logical sizes that cover a physical pixel valid. An empty covered hole still produces zero INSET, not full element coverage. Alpha is absent from kernel coverage; source-byte quantization and layer-alpha rounding remain the caller's unchanged two stages.

Effects public query/raster bodies retain their existing allocation checks, origin reconstruction and RGB/alpha writes. Only obtain their existing geometry fields through the prepared record and invoke the identical extracted kernel. In particular, public query still refuses sides above16384 or area above16777216; it must not become the large-q planner. Public sample retains its broader allocation-free valid domain. Do not replace a legacy raster's sample-origin calculation with integer support coordinates, even if it appears equivalent.

## Integer support and bounded V2 work

Support describes potential nonzero source cells, not a raster allocation. DROP uses the original float expressions `margin=3*(paint.layer.sigma*scale)`, near=`covered.x-margin`, far=`covered.x+covered.w+margin` (and Y), with outward floor/ceil. INSET uses covered element edges only when both hole and element have positive covered dimensions. Otherwise return canonical all-zero support. Do not silently erase support based on layer alpha; an adapter may skip transparent work after successful input validation. Preserve half-open support. Use checked finite integral-to-int64 conversions with explicit range checks before casts; publish the entire support only on success. Current admitted geometry is far below int64 limits, but do not rely on unchecked casts.

A future box preparer maps the original target pixel footprint to this source grid with the approved q/phase equations. It preflights all visible output cells and full-footprint denominators, intersects support solely to omit zero cells, and checks every addition/product against the transaction's remaining tap and metadata quotas. The1,048,576-tap cap from the experiment is an explicit proposed engineering budget, still subject to policy review. No full-raster allocation, down-q, four-gather fallback, renormalized crop or sampler-domain probe is needed after successful `prepare`. Count actual kernel evaluations, including retries only in diagnostics; reject capacity before kernel calls, allocations dependent on the plan, or pixel writes. Aggregate per-render quotas remain separate from per-profile geometry validity.

Integer support itself does not guarantee distinct float sample centers. Before any reads, the V2 planner checks the actual intersected first/last half-centers are representable and adjacent centers remain distinguishable; reject rather than sample collapsed coordinates. The supported V2 q<=64, phase in[0,1), current geometry/offset bounds and3sigma margin keep active support below2^22, where half-centers are distinguishable. Tests must prove this bound at all sign/extreme combinations rather than infer it from offscreen viewport points. Do not narrow public Effects sampling or impose this adapter-only rule on legacy callers.

## Verification and implementation sequence

1. Freeze the current original Effects source in a prefixed test-only reference TU, pin its source revision/SHA/flags, and construct separate old/new profile values. Compare public query active fields, finite samples and complete raster bytes; never compare padding or recapture expected outputs from the extraction. Preserve the existing frozen software512 unit-RGBA gate and V1 normalized/pixel fixtures.
2. Extract only geometry/kernel bodies and route public Effects callers through them. Compare DROP/INSET across sigma0/positive, asymmetric radii, negative spread, fractional positive/negative origins, empty hole, negative-size covering, tiny sigma rejection and exact boundaries. Check nextafter on caster/radii/sigma/scale/origin bounds, all nonfinite fields, null outputs and unchanged sentinels after failure. Raster wrong size/stride/capacity behavior remains untouched.
3. Test private prepared evaluation equals the old public sample at every active cell of representative small profiles, including source-byte and final-alpha rounding. Test support edges independently with hand-enumerated square/sigma0 and clipped Gaussian bounds; samples omitted by support must be zero. Test the current16384-wide q64 profile whose old raster query rejects while a clipped1024-tap plan succeeds; cap1023 must reject before eval/write, retry succeeds, rotated over-cap case remains atomic.
4. Test large admitted positive/negative geometry/support corners, checked cast rejection via the private conversion helper, transparent layers, canonical empty support and active half-center distinctness. Retain both center and area reference reports as diagnostics, not new legacy acceptance thresholds. Compile strict C11 selective/umbrella and actual copied amalgamation with symbol use; verify no duplicated private definitions when implementation include order varies.

Submit the extraction as its own reviewed compatibility commit before adding any V2 shadow painter. It must preserve V1 pixels/ABI/default routes exactly. Box-area policy, aggregate workspace accounting, transactional shadow painting, full text and all five adapter implementations remain required separate decisions/slices.
