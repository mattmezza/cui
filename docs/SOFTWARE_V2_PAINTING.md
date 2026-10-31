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
map each physical center back to logical coordinates, then use the existing float
shape/UV rules: circular logical corners become elliptical on unequal axes;
border inner radii/edges retain V1 semantics. Mesh XY projects in double per axis,
while source ST stays unchanged; the existing top-left/curve-coverage triangle
raster evaluates derivatives in physical space. Image nearest UV sampling,
grayscale/tint/opacity and straight-byte source-over retain V1 arithmetic.
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
