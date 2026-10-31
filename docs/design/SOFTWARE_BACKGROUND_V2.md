# Software V2 rendering checkpoint — proposal, not implemented

Based on approved `BACKGROUND_V2_RECORDS.md` through `638fc225`, Core V2
`3e18bbba`, and private preparation `34a499a4`. This checkpoint needs review before
production. All five adapters remain required; this specifies software only.
Slice(a) supplies private structural validation/planning and counted arenas.
A private [partial painter](../SOFTWARE_V2_PAINTING.md) now stages shapes, images,
meshes and backgrounds; it rejects TEXT and both SHADOW kinds. A counted source-
glyph recorder exists separately, but no full text/shadow painter or public
render/support entry exists. All-five completion remains pending.

## Exact additive entry points

In `cui_software.h`, include `cui_draw_v2.h` and add:

```c
bool cui_software_support_v2(const cui_software *,
                             const cui_render_v2_options *, cui_draw_v2_support *);
bool cui_software_render_v2(cui_software *, const cui_draw_list_v2 *,
                            cui_color clear, const cui_render_v2_options *);
```

No changes to V1 structs, enum, renderer interface or routes. Query the actual
initialized software target BEFORE Core begin; options viewport must equal its
current pixel dimensions. Query validates exact size/output tag, finite density
and origin, limits and target dimensions, writes support only on success. Report
ABI2, CENTER_V1, backgrounds (both models/spaces), max_commands=16384,
max_backgrounds=4096, and the explicitly supplied bounded pixel/byte ceilings.
These are upper admission limits, not a promise every admitted frame fits.
Render rechecks target/options; resize after query can fail without Core replay.
No fabricated support, V1 fallback or successful incomplete list. Bool failure
leaves pixels unchanged; no new error-state/public status API in this slice.

## Existing seams and decisions requiring review

`backends/software/cui_software.c` currently prepares shadows and checks meshes
before clear, but checks ordinary geometry, clip stack and text during painting.
`bitmap()` resamples logical bounds and masks shape; it cannot submit prepared
background pixels correctly. `blend()` already implements straight-byte RGBA
source-over with destination alpha and integer round-to-nearest; reuse its exact
arithmetic rather than introducing premultiplied target storage.

V2 must not call V1 render then discover an invalid later command. Extract small
private painters only where their V1 call sites retain identical input/output.
V1's shadow cache and default allocations stay unchanged. New V2 workspace is
optional and owned by the opaque software renderer; V1 creates/uses none of it.

Two implementation decisions are explicitly proposed:

1. Transactional scratch target: paint the complete validated/prepared frame into
   bounded private RGBA storage, then memcpy into the existing published target
   only after every operation succeeds. Thus even font/provider failure leaves
   the published target unmodified. Clearing scratch is resource preparation,
   never clearing the caller-visible target. This is stronger than existing V1.
2. Full physical transform for ordinary commands, not just backgrounds. Existing
   unit-coordinate painters are insufficient for arbitrary density/origin,
   particularly anisotropic glyph coverage. Do not silently accept these options
   and leave ordinary commands unscaled. A temporary unit-density-only adapter
   would be an explicitly incomplete milestone requiring separate approval, not
   completion of this checkpoint.

## Owned workspace, bounds and transaction

Use one V2-only transaction owner containing scratch target, normalized command
metadata/clip descriptors, prepared background spans, and shadow/font preparation
allocations. No raw caller pointers retained after render returns. Brush copies
are owned; ordinary text/image/mesh/font input remains borrowed through the call.
No whole-list snapshot ownership claim. Reject reentrant render/resize/destroy.

CPU accounting includes scratch target + all prepared background spans + shadow
pixels + glyph copies + counted CUI text temporaries + command copies, including old retained
capacity while replacement allocation is live. Metadata separately includes all
aligned descriptor/clip tables; use checked actual sizeof/alignment. Both counts
must fit options CPU<=32MiB and metadata<=1MiB respectively. Packed background
pixels also charge the negotiated texture/transfer ceilings as the common policy
requires, though software performs no GPU upload. Aggregate background work fits
max_pixels<=8M, each background<=4M; each prepared side<=16384. Scratch target
bytes are additionally charged to CPU, not background pixel count. This can reject
large existing V1 targets; it does not silently enlarge V1's memory contract.

Reserve final required capacities before publication. Free expired transaction
storage before growth; failed growth leaves old visible pixels and renderer usable.
Retained capacity plus transient allocations must stay within the same ceilings;
shrinking budgets cannot hide retained memory. Simplest first implementation frees
transaction allocations after each call (no cache); destroy releases any remaining
workspace. No GPU lifetime, retirement or driver-memory claim is involved.

## Complete preflight and painting

1. Validate exact list header/version/stride/error/output/features before traversal;
   then count<=16384, pointer/count contract, finite dimensions and every active
   field. Legacy kind must match embedded type. Unknown kinds/features reject.
2. Simulate the complete clip stack (maximum64, no underflow/unclosed stack),
   retaining logical intersections and transformed physical clips. Validate all
   shapes, image source/stride/extent/opacity/tint, borrowed mesh topology,
   text paint/tracking/span combinations and font availability before target use.
   Read only active payload fields, not union padding or inactive pointers.
3. Plan all background queries with original logical bounds and active clip;
   sum checked pixel/span/descriptor charges before allocation. Prepare shadows
   and text/glyph resources with bounded allocation/failure reporting. Existing
   unbounded shadow/font helpers need audited bounded wrappers or must fail before
   publication; do not count only the new gradient buffers.
4. Raster preparation may fail into private staging; original target is untouched.
   Execute commands in original array order, including drop/fill/inset/underlay,
   portals and explicit clip operations; never sort/group fills. All resources and
   geometry must be prepared before clearing even the scratch drawing surface.
5. Clear scratch, replay prepared operations, and publish once. If an external
   font provider can still fail during replay, scratch is discarded; no claim that
   its callback side effects can be rolled back. No callbacks/input are replayed.

Background spans are exact physical cells at query physical_x/y, stride from raster
output, sampled at inverse physical pixel centers. Blend each cell directly: no
logical-ceil bitmap cache, image resampling, grayscale/tint, opacity multiplication
or additional radius/opacity multiplication. A binary occupancy SKIP using the
same shared predicate/same inverse center prevents writes outside rounded shape;
covered alpha0 and uncovered cells must remain distinct. Apply ancestor clip ONCE with the physical center test
against transformed original clip; preparation's outward cover is only culling.
Corner masking and ancestor opacity already occur once in the shared helper.
Keep hidden RGB/alpha quantization exactly CENTER_V1. Integer placement is derived
from fractional original geometry, never by rounding the original box first.

Ordinary shape/image/mesh painters must evaluate the corresponding logical point
from each physical center, preserving source UVs and clip semantics. Text requires
an explicit density-aware glyph preparation/coverage mapping, including unequal
scales, colored glyphs and text gradients; rescaling font_size alone is insufficient.
`cui_font_draw_context_line_scaled` and its colored counterpart already emit
physical glyph coordinates/bitmaps, but accept ONE positive scale <=16, whereas
V2 permits independent scales through64. `cui_text_layout_draw_scaled` has the
same scalar shape. These are useful for the supported isotropic subset, not proof
of the complete transform. Mesh CPU preparation likewise uses a scalar mesh paint
scale and origin before triangle rasterization; composing anisotropic target
scales needs private per-axis transformed vertices, not overwriting that scalar.
Full text mapping is an implementation-review prerequisite: specify an explicit
coverage resampling policy for anisotropic/extreme density and source selection
before advertising it. No existing generic transform API solves that boundary.

Independently reviewable software slices: (a) complete list validator and bounded
transaction workspace with no advertised background support; (b) physical ordinary
shape/image/mesh mapping plus backgrounds and independent pixels; (c) complete
scaled/colored/gradient text preparation, fault tests and final support query.
An intermediate test-only renderer may reject unsupported text/density before
painting; public support must not imply that these remaining cases are complete.

## Required independent proof before support is advertised

Keep frozen V1 ABI, all24 normalized/pixel fixtures and text/Plot/effects oracles
unchanged. New V2 tests interleave every legacy kind with backgrounds, nested clips,
portal output, drop/fill/inset/border/underlay/prepaint and real text resources.
Compare background and source-over bytes to independent scalar references including
transparent destinations; compare ordinary-only V2 unit-scale output to immutable
V1 pixels. Text command checks do not substitute for font raster tests.

Use independent expected physical-center pixels for fractional origin, unequal
scales, corners, offscreen bounds and clipped phase. Exercise every model/space,
transparent stops, opacity once, image UV/tint, mesh and colored/gradient glyphs.
Test late invalid commands, stack errors, borrowed-resource rejection, wrong tags,
budget/overflow boundaries and injected allocation/provider failures: target bytes
and query outputs remain unchanged, then a valid retry succeeds. Include resize
between query/render, repeated render, destroy and no-opt-in allocation checks.
Run portable strict C11 selective/umbrella and software private-Xvfb tests; no GPU
claims follow. SDL_Renderer, SDL_GPU, Sokol GLCORE, Canvas/wire and foreign authoring
remain later mandatory checkpoints, as do all78 component-part coverage gaps.

Approved text-accounting clarification: existing font state and new opaque
FT/HB/Sheen/provider temporaries are explicitly separate from `cpu_bytes`; no joint
32MiB/process guarantee. The private counted session and exact admission ceilings
are specified in [SOFTWARE_V2_FONT_SESSION.md](SOFTWARE_V2_FONT_SESSION.md). Dependency
operations may allocate before an output-dependent rejection. V1 is unchanged.
