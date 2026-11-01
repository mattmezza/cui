# Historical four-gather software V2 shadow proposal (rejected)

This historical proposal is superseded by the approved [AREA transaction](SOFTWARE_V2_SHADOW_TRANSACTION.md). Its four-gather policy was rejected; it is not the implemented coverage contract.

At the time of this proposal, the private shape/image/mesh/background painter rejected both shadow kinds before
allocation. The measured four-gather policy is NOT approved; see
[quality report and per-axis proposal](SOFTWARE_V2_SHADOW_QUALITY.md). Full software/all-five support still requires them. This checkpoint
needs review before shadow implementation; no approximation is silently enabled.

Evidence: `src/effects/cui_effects.c:153-380` supplies allocation-free query,
sample and caller-buffer raster APIs. Geometry uses one scalar density, snaps the
covered caster/hole/element to that source grid, preserves asymmetric corner
selection, uses a four-midpoint Gaussian integral for sigma>0, and a one-source-
pixel SDF transition at sigma0. Inset additionally multiplies element-edge
coverage. The old backend cache (`cui_shadow.h`) allocates pixel/command caches
outside the V2 workspace and accepts only approximately equal axis scales. Reuse
would break counting and cannot implement unequal density faithfully.

Recommended next policy to review: explicit scalar source-grid reconstruction,
matching the approved text approach but source q=max(1,sx,sy)<=64 (effects permit64).
Evaluate source shadow coverage at four neighboring grid centers using the existing
pure sample API with layer.a=255 and quantize `255*coverage` with existing
lroundf. The old backend cache normalizes alpha to255 (`cui_shadow.h:141`) and
applies layer alpha later through bitmap opacity (`:230`); directly rounding
layer.a*coverage would differ. Bilinearly reconstruct this byte **coverage only**,
then alpha=lroundf(float(reconstructed_byte)*(layer.a/255.0f)), preserving the old
unit-case second rounding. RGB remains layer byte RGB; blend once in original
command order. A zero-alpha layer is a complete no-op, matching old cache skip;
otherwise covered source-support cells with quantized alpha0 still use legacy
source-over canonicalization, while outside support never paints. Source
phase is independently `phase_x=float(frac(q*origin_x/sx))` and likewise Y,
with frac(v)=v-floor(v) computed in double. Use this same stored phase in geometry
and point mapping: X=q*((physical_x+.5-origin_x)/sx)+phase_x. Gather centers
floor(X-.5)+.5 and its successor, with linear weight X-(floor(X-.5)+.5), and
likewise Y. A rounded phase1 is a harmless equivalent integer grid translation;
never pass the huge unreduced origin to the existing 1,048,576 bound. At isotropic
q=sx=sy the gathers coincide with source centers, preserving integer-phase legacy
coverage. Four direct pure samples avoid an entire source raster/cache.

Extract the existing pure source geometry/domain preparation into a private
explicit-value effects helper, preserving old calls/output exactly. This permits
source sample-domain validation and physical clipped destination planning without
allocating or imposing the old full-source raster query's 16K-side/16M-pixel
allocation bound on a small clipped destination. Map the original source support
(including its q-grid snapping and 3*sigma tail) back to logical coordinates and
then to each physical axis, intersect clip/viewport, and outward-cover cells.
The current validator's uniform-max-density raster reservation must be replaced
by this reviewed physical plan when shadows become supported. No Core/public API
or legacy sampler behavior changes are required. The admitted Effects caster limit is16384, not1e6. Measured active source support
at q<=64 preserves adjacent/half-integer centers; offscreen points can collapse
without contributing coverage. The quality report corrects the original concern
and measures real minification/support defects instead.

This is a declared reconstruction/quality policy, not exact anisotropic Gaussian
integration. It preserves the existing isotropic unit/integer-phase result when
source centers coincide. At extreme anisotropy/minification it can alias existing
source transitions; compare it with direct independent-axis Gaussian quadrature
before approval. The alternative would require physical elliptical-corner spans,
separate sigma_x/y, a defined sigma0 antialias distance and independent old-unit
proof; it is a larger effects seam and must not alter legacy sampler output.
Neither option should be called CSS-exact blur. Do not reuse uniform max-density
bounds as final physical bounds or silently reduce sigma/source quality to fit.

Count a clipped physical shadow span plus any typed descriptor in the same
transaction arenas as scratch/commands/backgrounds/text. Prefer direct gather,
so no second source texture, unbounded cache or effects malloc exists. Define
per-shadow<=4M covered cells, aggregate<=8M shadow cells (distinct from background
pixel quota), side<=16384 and four gathers/cell; charge exact RGBA bytes to CPU
before allocation. Existing background texture/transfer quotas need an explicit
shadow policy for eventual GPU adapters rather than pretending software uploads.
All geometry/phase/sample-domain checks precede scratch clear; all span admission
precedes allocation. Empty clipped work is canonical zero; preserve original
caster/hole geometry and do not crop meaningful fill to fit a quota.

Required independent tests: frozen original unit pixels for drop/inset, sigma0
and blurred, offsets/spread/negative spread/asymmetric corners, near-degenerate
hole, source-grid fractional phase, min/max/unequal density, clip edges and portals,
transparent layer/destination and actual drop→fill→inset→border order. Compare
reconstruction to an independent scalar reference and document its quality limits.
Failure injection/late invalid/budget/retry must preserve target bytes; no new V1
cache insertion/allocation, input replay or advertised public support.
