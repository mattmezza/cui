# SDL_Renderer V2: source audit and proposed owned-frame route

Design only, after software public 8671fa89; no SDL public symbols, production edits,
compiler run, dependency install or support claim. SDL_GPU, Sokol GLCORE and Canvas
remain required after SDL_Renderer; foreign bindings and all 78 parts remain pending.
The audited SDL blob `aeb9b7d43098605e12c8435f15d34b690ff1551d` equals current
root 479f3908 and this worker. System headers report SDL 3.4.18; CMake also accepts
SDL >= 3.2 or pinned fallback 7f3ae3d57459e59943a4ecfefc8f6277ec6bf540.

## Existing behavior and consequential choices

`backends/sdl3/cui_sdl3.c` owns window/renderer, borrows font and presents in V1
render. `cui_sdl3_size` sets STRETCH logical presentation; `render_density` combines
presentation/render scales, rejects axes > 16 and chooses max-axis glyph density.
Image rounding uses ceil-sized device textures and min-axis radii. Shadows are
prepared before clear, with physical bypass/restoration; ordinary glyph resources
and some validation can occur after clear. Mesh tickets have separate 256-triangle/
8M-texel budgets. V1 glyph/bitmap/shadow caches, table, callbacks and pixels must
remain untouched, including high-DPI behavior. README's final image-command
exclusion is stale: IMAGE is implemented and advertised; correct only with evidence.

Proposed V2 consumes every current kind through the proven CPU pixel policy, then
copies the completed physical RGBA image once through SDL. Replaying V1 SDL draws
with new backgrounds is insufficient: SDL BLEND does not normalize straight RGB
by destination alpha as the CPU painter does. Old float strip/clip/image mapping
also cannot establish the V2 fractional/anisotropic contract. This choice changes
V2's native internal rasterization, never V1; it does not claim universal V1/V2
native unit-pixel equivalence. CPU glyph/shadow/background ordering and coverage
are shared, not five new shaders; native acceleration covers transfer/presentation.

| Alternative | Measurable owned footprint and limitation |
| --- | --- |
| Public software adapter forwarding | Rejected dependency: SDL must not link `cui_software`. Its separate target plus scratch also costs `2P+resources`, rejecting Retina 4K at 32 MiB before useful work. |
| Shared painter into locked, unpublished back texture | CUI scratch can stay `P`; SDL lock pitch/span and nominal front/back textures need separate bounds. Lock failure is visible, unlock is void and can hide native-lock/upload failure; front/back ownership cannot make that failure detectable. Not recommended for a checked-upload contract. |
| Explicit opt-in persistent CPU target | Could move one `P` into separately documented retained storage. Adds a new initialization/resizing/high-water lifetime and up to 32 MiB retained CPU plus transient work/texture; cannot call it a 32 MiB total. Not the smallest ownership model. |
| **Owned prepared frame (recommended)** | One scratch `P` already in the counted transaction; no second destination plane, persistent CPU target or renderer-to-renderer dependency. Needs the precise private extraction below before SDL production. |

`P = 4 * physical_width * physical_height`, checked. At 3840x2160, `P=33,177,600` bytes,
versus `2P=66,355,200`; 32 MiB is 33,554,432. Thus extraction leaves 376,832 counted
CPU bytes for command/resources at that size; it does NOT promise every 4K scene
fits. Existing per-background 4M and aggregate 8M/work/font limits still apply.

## Private extraction checkpoint requiring review before code

Move allocation-free/CPU transaction helpers to `backends/common/`, not Core and
not a dependency on another adapter target. Proposed names `cui_cpu_v2_internal.h`,
`cui_cpu_v2_frame_internal.h`, `cui_cpu_v2_text_internal.h`,
`cui_cpu_v2_shadow_internal.h`; private static identifiers become `cui_cpu_v2_*`.
Pure bitmap/shape/mesh/shadow utility headers currently housed under software must
move mechanically or have explicitly reviewed private forwarding includes. SDL
already includes those pure headers; this is source ownership, not library linkage.
Update canonical SDK discovery to copy `backends/common/`; no hand-edited SDK.
First prove mechanical move/rename with all frozen source/pixel/allocation oracles.

Exact private lifecycle sketch (names/types are proposals):

```c
typedef struct { const void *data; size_t bytes; } cui_cpu_v2_exclusion;
typedef struct {
    unsigned width, height;
    cui_cpu_v2_exclusion excluded[3]; /* owner, published pixels, old target view */
    size_t exclusion_count;          /* <= 3, checked; no registry */
    bool *owner_busy;                /* optional; caller clears after all cleanup */
} cui_cpu_v2_frame_request;
/* Private frame embeds workspace, counted text owner/session and an EMPTY /
 * PREPARING / READY / RELEASING state; pixels point INTO its one CPU workspace. */
void cui_cpu_v2_frame_init(cui_cpu_v2_frame *);
cui_error cui_cpu_v2_frame_prepare(cui_cpu_v2_frame *,
    const cui_cpu_v2_frame_request *, const cui_draw_list_v2 *, cui_color,
    const cui_render_v2_options *, cui_font *, alloc_fn, free_fn, void *);
void cui_cpu_v2_frame_release(cui_cpu_v2_frame *);
```

Init allocates nothing. A frame is address-stable/noncopyable private ownership,
not a restriction on portable draw-list copying. Prepare requires EMPTY; READY or
reentrant use rejects untouched. Exclude frame/request records automatically,
plus supplied owner/published-target ranges, before source traversal or record
mutation. Preserve current early no-font/no-Bidi rejection, stop-before-dereference
checks, all checked active resource extents, capacities and current numeric policy.
All capability/geometry/source preflight remains callback-free; claim owner busy
at the current first-allocation boundary. Validate every workspace take.

After exact planning, allocate existing CPU/metadata blocks, record owned text,
prepare resources, clear scratch and paint the complete list in original order.
There is no external target or publication during prepare. READY exposes tight
RGBA rows/stride/span; no post-READY shaping or painting remains. Keep workspace,
text owner and session embedded at their final addresses (session.owner points
inside the same frame); never return pointers into a temporary copied session.
Release on EMPTY is harmless. Release during PREPARING or RELEASING is a no-op:
it neither cancels preparation nor clears the outer owner busy flag. Prepare on
any non-EMPTY state returns LIFECYCLE without callbacks. READY -> RELEASING happens
before the first text/free callback; only the outer release owns teardown.
Internal preparation failure likewise claims RELEASING before cleanup (the public
release entry cannot cancel PREPARING). Teardown ends text session, then frees
workspace, then canonicalizes EMPTY, while the outer adapter keeps owner busy
through its remaining native cleanup. Recursive release/free callbacks observe
RELEASING and cannot double-free; callbacks cannot mutate/retire the frame or
borrowed owner. A release request from an allocation callback is deliberately not
queued: successful preparation still returns READY to its outer caller.
Every failure returns EMPTY, never a partial READY handle or expired resource.
Allocation/provider side effects are not rollbackable.

Frame control is fixed stack storage, separate from the CPU/metadata heap quotas:
`sizeof(frame) = aligned(state/self/ownership fields + workspace + output view +
optional counted_text_owner + text_session)`. The existing 64-face owner table is
embedded once, not duplicated in a temporary later copied into the frame. Require
`sizeof(frame) <= 2048` as an engineering guard (not measured yet), report actual
native/WASM text/no-text size/alignment before code approval, and account one
frame plus request, existing 64-entry clip stacks/validator plan and scalar locals.
Compiler spills/call frames and dependency stacks are additional; no total-stack
byte claim follows. No stack storage scales with viewport or command count.

Mechanical namespace proof first freezes the four original8671 private V2 header
bytes/hashes. Normalize only identifier prefixes and include paths; demand exact
remaining source equality. Leave existing frozen V1 source, pixels, ABI, original
Effects/text and `cui_shadow.h` authority hashes unchanged. Old software header
paths forward with exact private type/function/constant aliases, including the
pre-include shadow sample hook. Test new-first/old-first/selective/all-header
orders and real calls, separately prefixed frozen old sizeof/alignment/active
outputs and exact allocation requests. Namespace work adds no frame state or
allocation; extraction must independently retain those compatibility oracles.

The existing software helper becomes prepare -> active-row memcpy -> release,
with existing published target/view exclusions and busy timing preserved. Its
public/old private signatures remain; exact allocation request/count/padding/
callback-order oracles must pass before SDL uses the new seam. SDL omits a CPU
published-target range and supplies its entire backend object exclusion. No
public frame handle, Core renderer architecture or input-dispatch change follows.

## Proposed SDL-specific API, target and admission

Add `cui_sdl3_v2_info { uint32_t size; cui_draw_v2_support draw;
uint32_t text_features; cui_render_v2_options defaults; }` and typed
`cui_sdl3_query_v2(const cui_sdl3 *, cui_sdl3_v2_info *)` /
`cui_sdl3_render_v2(cui_sdl3 *, const cui_draw_list_v2 *, cui_color,
const cui_render_v2_options *)`. SDL-specific A8/CONTEXT/TRACKING constants avoid
including the software public header. V1 function table/capabilities stay intact.

Query actual owned window backbuffer only: non-NULL SDL render targets are
unsupported, not silently presented. Query current logical window size and render
output size without calling mutating `cui_sdl3_size`. Defaults use output/logical
axis ratios and origin 0; explicit options retain independent axes 1/65536..64 and
origins +/-1e6, with physical viewport exactly current output. Do not inherit V1
scale <= 16, clip rounding or cached text density. Requery after size/DPI changes;
explicit options are intentional physical transforms, not a resize reservation.
Query checks positive sides<= 16384, native RGBA32 texture availability/size limit
and sRGB output; reject unsupported HDR/conversion routes before composition.

Use software's actual `min(16384,(B-P)/C,M/D)` command ceiling and background
min(4096,count), defaults B = 32 MiB / M = 1 MiB. Retain common CPU/metadata/work quotas;
final resource plans may reject. Texture and transfer ceilings additionally admit
one physical RGBA upload `P`, not just individual background planes. CPU background
planes remain CPU resources, not additional native uploads. One application-live
SDL texture with nominal bytes `P`; no V2 LRU or pointer/revision cache. Existing
V1 cache/font state is class 1; counted common transaction is class 2; opaque SDL/
FT/HB/Bidi/provider and driver allocations are separate class 3. Driver-deferred
storage is inflight/opaque, not falsely relabeled retained or synchronously freed.

Query/output and original options/list aliases reject before copying; prepared
frame exclusions cover all active known ranges. Entire opaque SDL/font graphs
must remain stable/disjoint, main-thread confined, without same-font recursion.
Guard any SDL query calls that can invoke installed SDL allocation hooks; restore
that temporary guard before pure source validation, and do not read input bytes
while it changes owner state. Prepare then holds busy from allocation through
native cleanup. Busy V2 query/render rejects LIFECYCLE; V1 render/size and native
mutators must refuse before changing state; void destroy is no-op/retry. Poll/wait
must not dispatch nested events during a V2 call. Measure private layout/counts
before adding guard/fault state; no assumed padding or silent V1 allocation changes.

## Native commit, failure and lifetime boundary

Snapshot presentation mode/dimensions, scale, viewport/clip enable and rectangles,
draw blend/color before changing SDL state. Complete CPU preparation, native
RGBA32 texture creation/checked update and property setup before any backbuffer
clear/draw. Require native format rather than hidden conversion fallback; use
NEAREST, BLEND_NONE, white modulation and alpha 1. Submit the entire texture at
integer physical (0, 0, width, height) with presentation disabled, scale 1, no viewport/
clip. This is one-to-one final-byte transport, never IMAGE radius remasking,
opacity multiplication, fractional resampling or another blend of the frame.
Original clips/radii/phase/text alpha were already resolved once on the CPU.

An unpublished new texture is the back resource. Failure before native drawing
never replaces the old displayed frame; release it and READY frame. After clear/
draw, failure may have partially changed the backbuffer: no GPU rollback claim.
Explicitly check `SDL_FlushRenderer` before the single presentation attempt;
restore every snapshotted property even after another restoration fails. Failed
restoration poisons further render/query until backend destruction/recreation,
rather than claiming a safe retry on unknown state. Other failures with successful
restoration allow fresh preparation/retry; never replay Core/input callbacks.
No extra retained front texture is needed for this full-frame path; the previous
visible presentation is not an owned texture or a rollback guarantee.

CUI allocator/work failures are CAPACITY; unavailable target/busy is LIFECYCLE;
malformed/unsupported data and checked SDL API failures are INVALID_ACTION with
`cui_sdl3_error()` detail. Success means checked preparation/upload/draw/flush and
**presentation attempted**. It cannot promise physical presentation or detect
all native presentation failures in the pinned SDL. Destroy new texture through
SDL after last queued use and release frame through the same busy scope; no fake
GPU wait/fence. Destruction does not prove immediate driver storage reclamation.
Window stored alpha/precision can differ from the exact CPU RGBA image; document
native output-format limits and test an RGBA target transport seam independently.

## Evidence and independently reviewable production gates

1. Mechanical common-helper relocation, then owned-frame extraction only: immutable
   original V1 software/Effects/text, public software 12 checks and exact allocation
   request formulas; prepare/release/reentry/allocation failures, READY lifetime,
   aliases, negative strides/overflow and repeated EMPTY cleanup. No SDL query yet.
2. Freeze actual old SDL V1 outputs/cache/allocation/ABI and high-DPI state; measure
   guard bytes. Implement full SDL query/all-kind route, not a background-only
   success. Use real query -> Core begin/end -> SDL render; include both brush
   models/spaces, DROP/INSET, images/meshes, contextual/styled/span/gradient text,
   transparent/opaque clears, fractional/unequal/min/max scales and huge origins.
3. Compare CPU READY bytes to existing independent references; native BLEND_NONE
   transport to separately generated whole-frame SDL textures, exact RGB and all
   RGBA channels where the actual target preserves alpha. Do not substitute SDL
   point primitives as the blend oracle. Native format limitations stay explicit.
4. Hidden-at-creation private-Xvfb software/available accelerated-driver readback
   before Present, plus native-format rejection, query/output aliases, stale DPI/
   resize, budgets, late invalid command, every CUI allocation and SDL create/update/
   setup/draw/flush/present/restore failure. Assert no early clear, no replacement
   on preparation failure, cleanup/known-state retry and poisoned-state refusal.
   Test masked pinned Present failure as submission-only, not false detection.
5. Root actual CMake/selective/C++/SDK/skill freshness and no adapter cross-link.
   Hardware GPU, real mixed-DPI monitor migration and untested OS/Wayland remain
   explicit deferrals; SDL_GPU/Sokol/Canvas and foreign bindings stay unfinished.

Primary SDL evidence (pinned source, independently inspected; no copied code):
[renderer implementation](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/render/SDL_render.c)
`SDL_UpdateTexture`1921-1955, unlock2277-2315, checked flush310-316,
present4675-4708, destruction4710-4761: conversion can hide errors, unlock has no
status, Present masks backend/flush failure, destroy flushes queued texture use.
[public renderer header](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/include/SDL3/SDL_render.h)
properties364-371 and readback/presentation2205-2279 describe format/size/colorspace,
main-thread use and invalidated post-Present backbuffer. Selected installed/CI
SDL versions must be verified during production, not inferred from their numbers.
