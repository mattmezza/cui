# Private transactional software painting

`backends/software/cui_software_v2_paint_internal.h` is an incomplete maintainer
building block. It has **no public support query/render entry or V1 call site**.
It paints RECT, BORDER, IMAGE, MESH, explicit clips and owned BACKGROUND commands
in array order at independent physical scales/origins. TEXT, DROP_SHADOW and
INSET_SHADOW reject before allocation/publication. This does not complete the
software checkpoint, any of the other four adapters, bindings or all78 parts.

The private `cui_software_v2_paint(target,list,clear,options,allocate,release,user)`
returns `cui_error`. The explicit target view supplies writable RGBA8 pixels,
width/height, actual row stride/capacity and an initialized-false busy guard.
Viewport must match nonzero target dimensions<=16384. Checked span is
`(height-1)*stride+width*4`; rows/padding beyond those active bytes are untouched.
Target pixels/view must be disjoint from list/options/command records and active
bitmap/mesh resources. Borrowed inputs, options and target fields stay stable
through return, including allocation/release callbacks; callbacks cannot write
published pixels. Reentrant paint on the same view rejects, including during free.
A future initialized software adapter must own this guard and its allocator;
callers cannot fabricate public adapter support from this private helper.

Whole-list active-field/clip/resource validation precedes allocation. A second
bounded pass computes exact double logical clip intersections and outward float
covers for shared background queries. The cover is culling only; each emitted
physical center is checked against the exact original logical intersection.
Copied command values, tightly packed scratch and prepared backgrounds charge
CPU<=32MiB; aligned typed descriptors charge metadata<=1MiB. Aggregate prepared
background pixels<=8M, per background<=4M, and packed background bytes also fit
texture/transfer ceilings. Command copies do not own bitmap/mesh data. Exact
per-call heap allocations are two distinct blocks (one for an empty list), with
no retained capacity/cache or hidden V1 shadow allocation. Fixed bounded control
stack (64 clips plus validator stack and scalar locals) is separate from these
heap-allocation quotas; there is no total-stack/process/time guarantee.

All background resources are prepared before clearing scratch. Shapes/images
map each physical center back to logical coordinates, retaining original absolute
binary64 near/far edges. Border far insets evaluate `(x+w)-right_edge` and
`(y+h)-bottom_edge`; they never subtract a small inset from a huge float width.
Clamped radius/edge values retain CUI rules, with double rationalized elliptical
row arcs and left-inclusive/right-open horizontal spans. Logical circular corners
become elliptical on unequal axes. Nearest image UVs use the closer absolute
edge and compensated source-end addition before integer selection, preserving
open source endpoints even for giant bounds or tiny source offsets/widths. Mesh XY projects in double per axis,
while source ST stays unchanged; the existing top-left/curve-coverage triangle
raster evaluates derivatives in physical space. Grayscale/tint/opacity and straight-byte source-over retain V1 byte arithmetic.
V2 corrects admitted relative-float cancellation and uses the stated binary64
coverage/UV policy, so boundary pixels can intentionally differ from legacy float
roundoff; this is not universal unit-scale pixel identity. V1 remains unchanged.
The painter never groups/reorders commands or goes through logical-ceil images.

Background covered cells blend their prepared CENTER_V1 bytes once, with no
resampling or additional radius/opacity multiplication. The shared binary
`cui_background_prepare_inside_internal` predicate skips uncovered cells at the
same inverse center. This occupancy test distinguishes an uncovered corner from
a covered alpha0 fill: uncovered pixels keep even hidden clear RGB; covered
alpha0 onto alpha0 follows V1 canonical RGB0. Ancestor clip is applied once.

After complete preparation/painting, active scratch rows publish by memcpy once.
Validation, budget or allocation failure leaves published bytes unchanged,
releases acquired blocks and permits valid retry. This is pixel atomicity under
the documented stable-input/allocator contract, not rollback of callback side
effects. V1's structs, renderer object, allocations and output route are unchanged.

Tests use independent scalar physical-center containment/UV/mesh/source-over
formulas and the frozen original background sampler, plus the separately compiled
immutable V1 software source for exact ordinary-only unit pixels. V1-only inactive
bounds are canonicalized in its reference input, without changing active fields.
Tests cover both gradient models/spaces, reversed SOURCE_038 stops, transparent
stops/destination/hidden RGB, fractional unequal/extreme densities, nested clips,
source order, padded targets, offscreen large extents, quotas, both allocation
failures, malformed late commands, rejection/retry and callback reentrancy.

Local receipt: focused test compiled strict C11 Release `-O3 -DNDEBUG -UNDEBUG
-Wall -Wextra -Wpedantic -Werror` on CPU0/nice19, linking the preserved existing
`build-shadow/libcui.a` SHA256
`96d40727c65d39de0accedf45edba34098a209ec41747bd05800d96362e1a137`.
No engine archive was rebuilt or immutable expected output recaptured. New helper
code is compiled into the focused test, with independently compiled frozen V1
software/sampler source. The first oracle run rejected inactive NaN bounds in
V1; canonicalizing those inactive reference-only fields resolved it. Final private
Xvfb checks passed4/4: new painter, existing validator, pure background preparation
and frozen V1 current comparison. Installed-skill tests passed18/18; reference
hashes2/2, clang-format23.1.2 and generated freshness passed. Integrator actual
current-library CMake execution remains an independent gate; no native GPU proof.

Precision regression receipt: the original075a60d4 helper SHA256 was
`bb8c1d8c3b777207cff95cfa7f4cfcc5b73293092a5d10c974504bb598d9e55d`.
Before modifying it, a strict O3 probe linked that helper and returned normal
exit2 at the first hand-enumerated4x2 square case (x0,y0). The corrected helper
passes eight tiny-target cases: negative1e8 X/Y, rounded corners, asymmetric
right/bottom border insets, negative1e30 X and three source-cutout endpoint cases.
Each case requests<2KiB per allocation; no giant raster is created. Additional
nextafter left/right edges, inner-border endpoints and exact3-4-5 circle points
verify horizontal half-open semantics. The expanded fixture, old-negative probe,
validator, pure background preparation and preserved V1 compare passed5/5 under
private Xvfb. All prior40 physical frames and frozen ordinary-unit fixture bytes
still pass; an added tiny-offset UV expectation was corrected analytically to
red at distance1.5/width and blue at .5/width around source integer1.
The preserved old-negative executable SHA256 is
`4fc87c0ba822e7d841295a7ca1dc51eea1c7cbfdeadc7a6e6387c60b0ef4dc5b`.
Precision followup installed-skill tests passed18/18 in38.344s; final source format
and canonical SDK/amalgamation/skill freshness checks passed.
