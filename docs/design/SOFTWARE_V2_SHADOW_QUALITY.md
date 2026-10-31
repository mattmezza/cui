# Shadow reconstruction measurements — test only, policy rejected

No shadow renderer/API is implemented or approved here. The broad four-gather
proposal in SOFTWARE_V2_SHADOW_PREPARATION.md is **not approved**. The evidence is
[per-case CSV](../../tests/fixtures/shadow_v2_quality/report.csv),
[receipt/source hashes](../../tests/fixtures/shadow_v2_quality/receipt.json) and
`tests/unit/test_shadow_v2_quality.c`. Timings are observations, not a benchmark
claim or performance gate. Successful CTest means the measurements/legacy checks
ran, not acceptance of the proposed rendering policy.

## Candidate and independent diagnostic reference

Candidate calls actual `cui_effect_{drop,inset}_shadow_sample` at four q-grid
centers, q=max(1,sx,sy), reduced per-axis phase, byte coverage=lroundf(255*sample),
bilinear byte reconstruction, then lroundf(float(byte)*(layer.a/255.f)). This is
the old cache's two-stage rounding, not rounding layer.a*coverage directly. The
reference independently transforms unsnapped logical geometry per axis, integrates
Gaussian X with erf and Y with2048 midpoint rows (512-row convergence comparison).
Sigma>0 is a **center convolution** with independent sigma_x/y, tails at±8sigma;
sigma0 is **pixel-square area**, integrating clipped horizontal ellipse spans.
Inset reference=(1-hole coverage)*element pixel area. It treats empty holes as
full inset element coverage. These are explicit diagnostic definitions, not a
CSS-exact reference or a proved uniform numerical error bound; the sigma0 policy
is intentionally different from the positive-sigma center model.

Logical offset/spread retain source f32 materialization; clamp radii to original
caster min dimension/2, then project independently. Hole radii subtract spread.
The independent reference does not source-grid-cover the bounds, use the source
SDF edge ramp, approximate erf or use four midpoint kernel samples. Hence error
includes **intentional model differences already present at unit density**;
it cannot all be attributed to q reconstruction. Fine/coarse reference maximum
change was0.001882 alpha byte; this is empirical convergence on this matrix only.

Base caster=(2.25,2.5,4.5,3.25), radii=(.5,1.5,1,.25), offset=(.375,-.25),
spread=.25, RGBA=(40,80,120,173). Every scenario runs DROP and INSET on8x8 cells.

| Scenario(s) | sx,sy | origin | sigma / change |
|---|---|---|---|
| unit-crisp/blur | 1,1 | 0,0 | 0 /1.5 |
| fractional-crisp/blur | 1,1 | .375,-.625 | 0 /1.5 |
| wide-crisp/blur | 2,.75 | -3,.125 | 0 /1.5 |
| tall-crisp/blur | .5,3 | .25,-6 | 0 /1.5 |
| min-axis-crisp/blur | 1/65536,4 | .5-4.5/65536,-8 | 0 /1.5 |
| max-min-crisp/blur | 64,1/65536 | -144,.5-4/65536 | 0 /1.5 |
| max-both | 64,64 | -144,-160 | 1.5 |
| large-visible / crisp | nextafter(1/65536,+∞),64 | .5-sx*16386,-960000 | 1.5 /0; caster=(16384,15000,4,4) |
| negative-spread | 1.25,.625 | .5,1 | 1.5; spread=-1.5 |
| transparent | 2,.75 | -3,.125 | 1.5; alpha0 |
| offscreen-phase | nextafter(1/65536,+∞),64 | 1000000,.375 | 1.5; wholly offscreen |
| collapsed-hole | 1,1 | 0,0 | 1.5; spread2 |
| subpixel-thin | 1,1 | 0,0 | 0; caster width=.125,height=.25 |

## Results and attribution

Errors below are alpha-byte units (layer alpha173), not normalized fractions.

| Case | Maximum / RMS | Meaning |
|---|---|---|
| unit-crisp DROP |126.664 /34.979| Covered-grid inflation/SDF versus unsnapped pixel area exists in legacy too. |
| unit-blur INSET |59.584 /18.614| Source kernel/geometry/element mask versus diagnostic model. |
| fractional-blur INSET |132.246 /52.308| Fractional grid covering strongly changes source geometry; phase arithmetic error is0. |
| min-axis-crisp DROP |172.990 /61.160| Actual footprint alias: thin minified shape reconstructed near opaque instead of its tiny pixel area;8 extra support cells. |
| max-min-crisp INSET |172.995 /61.163| Minified element mask has8 extra support cells. |
| max-min-blur INSET |136.996 /47.818| Worst **nondegenerate nonzero-sigma** error;8 extra support cells. |
| collapsed-hole INSET |173 /71.724| Worst overall/nonzero-sigma: source empty-hole=>empty paint versus reference full inset;20 missing cells, intentional semantic difference. |
| negative-spread DROP |18.870 /4.622|28 extra support cells versus thresholded reference. |

Worst nonzero-sigma DROP is fractional-blur46.812/27.832. Matrix-wide RMS is
31.504 alpha bytes. Thresholded support uses candidate alpha>0 and reference>=.5.
All512 full-RGBA **legacy unit** comparisons match exactly (unit crisp/blur,
collapsed-hole, subpixel-thin, both kinds). This establishes incumbent compatibility
on those cases, not anisotropic quality. Transparent cases are exact no-ops.

Source authority: `cui_drop_shadow_bounds_internal.h` admits caster coordinates/
dimensions<=16384, offsets/spread<=4096 and sigma<=256. Thus the earlier proposal's
q*1e6 visible-source example was outside that domain. Active support at q<=64 stays
below2^22 source coordinates, retaining half-integer centers. There were **zero
active collapsed gathers**. Some offscreen viewport cells (including in the
large-visible scenario) inverse-map far outside support and do collapse, with
coverage0; the wholly offscreen phase case has64 such cells/kind. Its phase error
is3.433e-5 source pixel versus exact rational f32 operands; visible phase error
max2.981e-8. These are not manufactured supported-domain precision failures.
The real API rejects caster x=1e6 and preserves output, separately asserted.

Measured work:2560 destination cells,9728 pure source sample calls,6348800 reference
row evaluations,0.334463s Linux process CPU including both references and old-unit
checks. Single CPU0/nice19, no concurrent CUI heavy task; no retained cache/new
render resources. Strict C11/O3 warnings-as-errors +private-Xvfb1/1 passed.

## Smallest compatible per-axis alternative — design first

Recommend replacing four point taps with **exact axis-aligned box integration of
the existing discrete q-grid coverage bytes**, not a new shader or new Gaussian
renderer. A source byte at(n+.5,m+.5) represents constant coverage over cell
[n,n+1)×[m,m+1). Map the entire physical target pixel square to q coordinates using
the same phase; average source-cell bytes weighted by exact rectangular overlap,
normalized by the full mapped pixel area, then apply layer alpha once with the
existing second rounding. At unit/isotropic q=sx=sy>=1, pixel squares align with
one source cell, preserving the demonstrated unit path. At minification, integrate
all touched source cells; never replace a large footprint with four samples.

Intersect traversal with finite source support, keeping the full denominator;
outside contributions are zero, not a cropped/renormalized fill. Use pure shared
private source geometry to plan integer source ranges (old raster query's16K/full-
raster allocation limit is inappropriate). No source image/cache allocation is
required for a first direct traversal. Preflight exact tap counts with checked
int64/size_t math and a proposed1M source-evaluation/frame ceiling, separately
from destination pixel/CPU/metadata budgets; reject before publication if exceeded.
Do not silently lower q, kernel precision, or sampling density to fit. Target
spans/descriptor bytes stay in the existing bounded transaction workspace.

This preserves the source's grid inflation, four-midpoint/SDF/empty-hole semantics;
it promises filter consistency of that **declared discrete profile**, not a small
error versus the unsnapped Gaussian/area reference. It should address the admitted
minification/support defects, but needs the same test-only comparison before
approval. A direct independent-axis Gaussian/area implementation would be a
separate semantic choice and intentionally differ from existing unit pixels;
do not claim both those incompatible promises. V1 renderer/cache/sampler remain
unchanged either way. No production shadow, support query, adapter or binding
approval follows from this report; full text/shadows and all five adapters remain
mandatory.
