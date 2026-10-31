# V2 shadow discrete-profile box experiment

Status: TEST ONLY; no renderer capability, shadow painting, public API, or production geometry change. This follows the rejected four-gather proposal and its measurements in `SOFTWARE_V2_SHADOW_QUALITY.md`.

The prototype integrates the existing byte-quantized q-profile over each complete inverse-transformed target pixel footprint. Each source cell contributes its exact rectangular overlap area. The denominator remains the complete footprint area when source support crops the work; cropped support is zero, not a renormalized filter. Original layer alpha applies after averaging, retaining the legacy second byte rounding. It neither lowers q nor substitutes four gathers when work exceeds the budget.

`tests/unit/shadow_v2_box_internal.h` is deliberately test-only. It duplicates existing float geometry materialization for the experiment, then uses the actual pure Effects sampler. It does not allocate a full q raster or use the raster query's 16K-side/16M-pixel allocation admission as a geometry query. One separate Effects domain probe precedes integration; reported source taps exclude that probe.

The fixed experiment admits 64 output cells and at most 1,048,576 intersecting source-cell evaluations. It preflights every output cell before reading profile cells or publishing means. Reader failure preserves the entire output. This cap is an engineering limit, not an optimality or performance claim. No allocation or target rendering occurs.

Independent small table oracles cover unit-cell identity, weighted 2D overlap, negative phase, shifted support, full-footprint normalization, empty support, cap rejection before reads, late reader failure and retry. A large-q case distinguishes valid clipped work from the old full-raster allocation limit. The comparison retains the original point candidate and independent Gaussian/pixel-area reference unchanged. Source discretization, support truncation and empty-hole semantics still differ from that reference; this experiment cannot establish CSS-exact rendering.

## Production prerequisite, still unapproved

If results justify this policy, first extract a private value-only geometry helper from the existing Effects `shadow_geometry`/inset geometry functions. Both old callers and the new planner must use the same float operation order, snapping, q grid, radii, sigma and empty-hole rules without changing legacy pixels. Its record would describe integer potential support and the existing sampled shape; it would not allocate a bitmap or inherit bitmap allocation limits. Keep this seam private and review it separately before production changes.

A future adapter must independently preflight aggregate cell work, metadata, temporary storage and transaction memory before target publication. It must retain the full denominator, checked coordinate arithmetic, bounded source reads and explicit failure/retry behavior. Actual profile calls include any validation probe in addition to integration taps. No cache, public shadow redesign or silent fallback is proposed here. Text, complete shadow painting and all five adapter implementations remain mandatory follow-on work.

## Measured checkpoint

The strict GCC16.2 O3 two-TU executable passed the private-display wrapper (1/1). `tests/fixtures/shadow_v2_quality/box_report.csv` records every case; `box_receipt.json` pins sources, unchanged reference/archive hashes and flags. Across 40 rows/2,560 cells: 25,714 source taps plus 40 separate domain probes; box work used 0.004893 process-CPU seconds and the complete comparison 0.329634 seconds in one low-priority CPU0 run. These are observations, not stable performance guarantees. All 512 legacy-unit full-RGBA comparisons and uniform-scale point-profile comparisons matched exactly.

| Case / kind | Box max byte difference | Prior point max | Interpretation |
|---|---:|---:|---|
| min-axis crisp / drop | 0.013199 | 172.989785 | footprint integration removes narrow-axis point alias |
| max-min crisp / inset | 0.006708 | 172.994851 | same improvement on the other axis |
| large-visible crisp / drop | 0.011121 | 172.989111 | negative phase and large admitted position remain stable |
| min-axis blur / drop | 120.382533 | 4.694492 | area footprint differs strongly from the center reference |
| max-min blur / inset | 0.005184 | 136.995934 | area averaging removes point overcoverage |
| large-visible blur / drop | 94.581377 | 3.624770 | area versus center reference; eight missing-support cells |
| collapsed hole / inset | 173 | 173 | unchanged legacy empty-hole semantic difference |

Overall RMS against the mixed reference is 24.918362 bytes (prior point 31.504076). That aggregate does not prove a quality bound. Crucially, the unchanged reference uses **center** Gaussian convolution for nonzero sigma but pixel-square **area** for sigma zero. The proposed filter uses area for both. At extreme minification a center may have substantial blur alpha while its enormous pixel footprint has negligible average alpha. Thus the nonzero-sigma DROP increases above are a real semantic difference in the comparison, not evidence of an integration arithmetic failure or an established production regression. The worst nonzero-sigma maximum is fractional-blur INSET 132.246296, unchanged from point sampling; source snapping/legacy kernel and reference differences remain. No universal center-reference error threshold is proposed.

The analytical overlap tests establish the implemented weighted-discrete-profile operation independently of the Gaussian reference. They also prove cap rejection before reads/writes, full normalization after support cropping, late-reader output atomicity and successful retry. A 16,384-logical-wide caster at q64 fails the existing full-raster query while the tiny visible box operation succeeds with exactly 1,024 source taps; cap1,023 rejects atomically. The rotated case exceeds the 1M budget and rejects without sampling/output. This establishes bounded planning, not permission to bypass sampler geometry validation.

Recommendation: review target-pixel area integration as an explicit V2 coverage policy, preserving exact V1 behavior. The experiment supports its arithmetic and alias reduction, while its quality relative to a center-sampled blur is intentionally different. Production remains unapproved pending that semantic decision and the private shared-geometry review above.
