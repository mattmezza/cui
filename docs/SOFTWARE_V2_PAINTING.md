# Private transactional software painting

`backends/software/cui_software_v2_paint_internal.h` is an incomplete maintainer
building block. It has **no public support query/render entry or V1 call site**.
It paints RECT, BORDER, IMAGE, MESH, explicit clips and owned BACKGROUND commands
in array order at independent physical scales/origins. DROP/INSET shadows use
bounded target-pixel AREA reconstruction of the unchanged source profile. The
private with-font entry adds owned text recording and per-axis bilinear glyph
reconstruction; the no-font wrapper still rejects TEXT before allocation. This does not complete the
software checkpoint, any of the other four adapters, bindings or all78 parts.

The private `cui_software_v2_paint(target,list,clear,options,allocate,release,user)`
returns `cui_error`. The explicit target view supplies writable RGBA8 pixels,
width/height, actual row stride/capacity and an initialized-false busy guard.
Viewport must match nonzero target dimensions<=16384. Checked span is
`(height-1)*stride+width*4`; rows/padding beyond those active bytes are untouched.
Target pixels/view must be disjoint from list/options/command records and active
bitmap/mesh resources and active text/paint/span bytes. Borrowed inputs, options and target fields stay stable
through return, including allocation/release callbacks; callbacks cannot write
published pixels. Reentrant paint on the same view rejects, including during free.
A future initialized software adapter must own this guard and its allocator;
callers cannot fabricate public adapter support from this private helper.

Whole-list active-field/clip/resource validation precedes allocation. A second
bounded pass computes exact double logical clip intersections and outward float
covers for shared background queries. The cover is culling only; each emitted
physical center is checked against the exact original logical intersection.
Copied command values, tightly packed scratch, background RGBA and shadow A8 planes charge
CPU<=32MiB; aligned typed descriptors charge metadata<=1MiB. Aggregate prepared
background plus shadow and glyph rectangle pixels<=8M (shadows additionally<=1M), per background<=4M, and packed background bytes also fit
texture/transfer ceilings. Command copies do not own bitmap/mesh data. Exact
nontext heap allocations are two distinct blocks (one for an empty list), with
no retained capacity/cache or hidden V1 shadow allocation. Fixed bounded control
stack (64 clips plus validator stack and scalar locals) is separate from these
heap-allocation quotas; there is no total-stack/process/time guarantee.

All background and shadow resources are prepared before clearing scratch. Shapes/images
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

Shadows use q=max(1,sx,sy) with reduced float source phases, the reviewed copied
source geometry/kernel, and exact rectangular target-footprint overlap with
byte-quantized source cells. The full footprint area remains the denominator
when support clips source work. Layer alpha is rounded once after averaging
source bytes; Core has already applied ancestor opacity. Hard ancestor clips
use inverse pixel-center membership and do not shrink that footprint. Existing
source radii/SDF/Gaussian/empty-hole rules are preserved, not fitted to an
unsnapped reference or claimed CSS-exact. Output blends straight original RGB
and prepared A8 once; outside-support cells skip, while inside alpha0 follows
the existing canonicalizing blend. Source order remains unchanged.

The entire transaction admits at most1,048,576 shadow rectangle cells and
1,048,576 source-cell evaluations, with no q reduction/fallback/cache. All plans
are checked before kernel calls/allocations/pixel writes. CPU request is command
copies + scratch RGBA + background RGBA + shadow A8. Metadata is existing
per-command descriptors + checked alignment padding + copied shadow descriptors.
Both exact requests fit the caller limits. Empty/transparent shadows are validated
before skipping. The structural validator's shadow_bytes=0 is explicitly not a
completed resource plan; the painter adds exact clipped A8/tap accounting. V1
raster-query allocation limits are not used to admit valid clipped large-q work.
See [the transaction contract](design/SOFTWARE_V2_SHADOW_TRANSACTION.md).

The private `cui_software_v2_paint_with_font` additionally takes a borrowed font
before allocator callbacks. It requires the actual text build; no-text builds
reject TEXT without linking font dependencies. Actual contextual availability
is checked separately from letter-spacing support. No-Bidi builds reject context
(including empty non-NULL paragraphs), spans and tracking before borrowed reads.
Font size<=4096 and line origins |x|,|y|<=1e6 are explicit V2 text limits.

Text chooses q=max(1,sx,sy)<=64, shapes at logical size, and owns copied GRAY256
coverage at q. Each target center inverse-maps both axes independently; four
source neighbors with zero outside the bitmap form bilinear coverage, rounded
once. Positive-weight occupancy distinguishes zero coverage from a zero-weight
halo. Solid uses selected command/cluster ink; resolved gradients sample logical
coordinates. Alpha is ink.a*coverage/255 truncated once, then original source-over.
Original command and callback glyph order stay intact; clips never alter weights.

An aligned per-TEXT {first,last} range charges base metadata. A counted text owner
receives the remaining CPU/metadata envelopes after exact base reservations.
CUI shaping/layout/Bidi payloads, prefixes and glyph copies charge CPU; glyph
records/prefixes charge metadata. Text frames have additional counted allocations,
not the nontext two-block guarantee. Exact peaks are baseCPU+owner.cpu_peak and
baseMetadata+owner.metadata_peak. Existing font runtime and newly opaque
FT/HB/Sheen/provider work remain separate; no joint process-memory guarantee.
All records, transformed covers and joint glyph-cell quotas are prepared before
scratch clear. Session records release before base workspace, with busy held
through every callback. Borrowed fonts, entire graphs/FT/HB storage and paint stops must stay stable
and disjoint from target/workspace through return. No nested/concurrent rendering
on the same font is allowed. The pointer-byte font overlap sentinel does not
validate opaque graph allocation ranges; those are caller preconditions, unlike
known active text/span/stop/image ranges checked explicitly. failure does not roll back dependency/provider side effects. Intrinsic
RGBA/color-font glyph formats remain unsupported, distinct from span ink+A8.

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

Private shadow checkpoint receipt: actual helper compilation links the integrator's
current Effects-profile archive SHA256
`0535906e874ffda405e9c7b3cc0b0b2063b5bd0ef76e88294c236a38ada68f97`.
Strict O3 and private-display checks passed3/3 in0.53s after correcting the mixed
fixture to retain IMAGE alongside all other ordinary commands. The corrected
14-command fixture checks48 complete RGBA frames, plus8 large positive/negative
source positions and3 exact/nextafter clip boundaries. An independent slow
source-cell overlap oracle uses reviewed geometry and public pure Effects
samples, without production plane/planning code. The actual private painter also
matches4,608 frozen original-software unit pixels with layer alpha1/127/173,
nonzero clear alpha, sigma0/positive and square/asymmetric corners. The separate
512-pixel legacy profile gate remains exact. No expected fixture was recaptured.

Tests assert both allocation requests exactly for every interleaved background/A8
frame. A two-shadow example requests1,699 CPU bytes and720 metadata bytes for163
A8 cells/source evaluations. Exact/below tap, cell, joint prepared-pixel, CPU and
metadata limits; first/second allocation failures; first/late private preparation
failure; empty/transparent/invalid shadows; large-q visible work; reentrancy and
retry all preserve the target/padding as specified. The preparation failure hook
exists only as a private test instrumentation macro, with no public API/callback
or default runtime behavior. See `docs/validation/software-v2-shadow-worker.json`
for hashes/provenance. Public software support and full transformed text remain
pending, as do all four other adapters.
