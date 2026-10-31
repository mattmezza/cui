# Versioned background commands and five-adapter preparation

Proposed records/contracts only, following approved transport B. Values/math are
implemented by `2a435a5f` + `cf72b10e`; nothing below is implemented or approved.
V1 `cui_draw_command`, `cui_draw_list`, `cui_config`, `cui_renderer`, their enums,
functions and default output remain unchanged; V1-only contexts allocate no V2
buffers. No context lookup,
new V1 tag, transparent placeholder, incomplete V1 list or V1/V2 array cast.

## Concrete portable records and frame handshake

New supplemental header `cui_draw_v2.h` includes the existing public headers
(including `cui_background.h`):

```c
#define CUI_DRAW_ABI_V2 2u
/* Constants 0..8 explicitly equal existing V1 kinds; do NOT extend cui_draw_type. */
#define CUI_DRAW_V2_BACKGROUND 9u
#define CUI_DRAW_V2_FEATURE_BACKGROUNDS UINT64_C(1)
#define CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 1u

typedef struct {
    cui_rect bounds;
    cui_corner_radii radii;
    cui_background brush;              /* OWNED fixed-two-stop value. */
    float opacity;                     /* Already resolved ancestor product. */
} cui_background_draw_v2;
typedef struct {
    uint32_t kind;
    union {
        cui_draw_command legacy;        /* Active for kinds 0..8. */
        cui_background_draw_v2 background;
    } payload;
} cui_draw_command_v2;
typedef struct {
    uint32_t header_size, abi_version;
    size_t command_stride;
    const cui_draw_command_v2 *commands;
    size_t count;
    float width, height;
    uint64_t required_features;
    uint32_t background_output;
    cui_error error;                    /* Non-OK list MUST NOT render/clear. */
} cui_draw_list_v2;
typedef struct {
    uint32_t size, abi_version, background_output;
    uint64_t features;
    size_t max_commands, max_backgrounds;
    size_t max_pixels, max_pixels_per_background;
    size_t cpu_bytes, texture_bytes, transfer_bytes, metadata_bytes;
} cui_draw_v2_support;
typedef struct {
    uint32_t size;
    cui_draw_v2_support support;        /* Actual consumer query, before begin. */
    uint64_t required_features;
    size_t command_bytes, background_bytes;
} cui_frame_v2_options;
bool cui_begin_frame_v2(cui_context *, float w, float h, float dt,
                        const cui_frame_v2_options *);
bool cui_end_frame_v2(cui_context *, cui_draw_list_v2 *out);
bool cui_set_background(cui_context *, const cui_background *); /* Open box. */
bool cui_element_set_background(cui_context *, cui_id, const cui_background *);
```

Version, header size and stride must equal this exact record version; do not guess
prefix compatibility. No serialized representation is the raw struct/union/padding.
V1/V2 pointers are incompatible C types; an explicit unsafe cast is outside the
contract, not something runtime negotiation can make safe.
For legacy kinds, `payload.legacy.type` must equal `kind`; all existing payload
semantics remain intact. Inactive union bytes are ignored. Unknown kind, feature,
policy, version or stride fails before any array traversal/target mutation.
The background feature initially means BOTH models and BOTH interpolation spaces;
no misleading partial-support bit. New policy versions require new negotiation.

All five consumers expose a separate V2 support query and V2 render entry point;
there is no extension to V1 renderer structs. Query actual initialized consumer,
font/target policy and resource limits. Host queries BEFORE composition and passes
support by value to begin; Core copies values, never an adapter/context association.
A copied V2 array can consequently be rendered by any compatible V2 consumer.
Missing symbol/old adapter/unsupported host means do not start a V2 background
frame. Capabilities are an ordinary caller contract, not protection from fabricated
C records. A changed/lost target after query can still fail render without replay.

Zero byte-budget options select the documented defaults; null options, wrong
record sizes/versions/unsupported features use INVALID_ACTION, allocation/budget
failures CAPACITY. Existing lifecycle errors remain LIFECYCLE.
Begin validates sizes/feature subset/limits and reserves optional storage BEFORE
opening a frame or consuming queued input. Failure leaves no frame active and
input queued. Existing begin starts V1; new begin starts V2. Background setters
reject V1/outside-frame/measurement/prepaint/diagnostic reentrancy before modifying
paint, with lifecycle diagnostics. A V2 frame must also have requested the
background feature; otherwise setters reject before mutation. They never enable
features implicitly. A legacy-only V2 frame needs no background map/storage.
Wrong end function reports LIFECYCLE, returns an empty error result, and leaves
the frame open without layout finalization or dispatch. Caller may clear the
error and call the matching end exactly once; callbacks are not replayed. Matching
end closes the frame once even on layout/conversion/capacity failure. V2 failure
sets out to an ABI2 empty list with non-OK error and returns false; V1 wrong-end
returns its existing zero list and sticky context error. Null V2 out is rejected
before closing. Reentrant/multiple begin/end follows existing lifecycle guards.

## Ownership and bounded retained storage

The brush is owned by value, NOT the entire command's resources. A Core-produced
array lasts until next begin (either mode) or context destruction.
Legacy strings, paragraph data, spans and text stops retain their existing frame
lifetimes; bitmap pixels, mesh vertices, fonts/provider backing retain their
existing borrowed-through-render-completion contracts. Copying/reordering a V2
command array copies brushes without lookup restrictions, but does NOT deep-copy
those borrowed resources. A full snapshot/export must explicitly copy all payloads
and rebase typed references. No full snapshot helper is implied by this slice.

Optional V2 storage is reserved during explicit V2 begin, before opening it;
retain its bounded capacity for reuse until context destruction, not another copy
per frame. Default proposed ceilings: command_bytes=16MiB, background_bytes=512KiB;
hard maxima16MiB each, checked against SIZE_MAX/PTRDIFF_MAX before allocation.
Command capacity is min(config.max_commands,16384,support.max_commands,
command_bytes/sizeof(cui_draw_command_v2)); count all clips and expanded decoration.
Background capacity is min(config.max_elements,4096,support.max_backgrounds),
subject to actual sizeof records plus config.max_elements*sizeof(uint32_t) optional
node-slot map fitting background_bytes. Default4096-node map costs16KiB;4096
approximately88-byte owner/value records cost352KiB on a common64-bit ABI. Use
actual sizeof/alignment, never those estimates, for budgeting. Large element
configs may require larger explicit background_bytes or fail begin before input.
Retained capacities are high-water marks bounded by those hard maxima (at most
32MiB additional Core payload storage); lowering a later frame's logical budget does not
silently shrink retained storage or allocate another copy. V1 existing
arrays/prepaint arrays/frame_bytes/resource_bytes remain separate;
these ceilings are ADDITIONAL memory, not a claim of total context footprint.
No raster/pixel/upload storage lives in Core. No unconditional V1 node growth or
pre-attached custom paint: optional owner map indexes already owned current nodes.

## Setters, identity and precise Core seams

NULL clears the override and restores the original box background; transparent
SOLID replaces it with transparent fill. Last valid setter wins atomically per
owner. Copy/validate the value and reserve a new slot before mutating node/custom
configuration. Replacement reuses its slot; clear of an existing slot never
allocates; cleared slots remain reusable by that owner until next frame. Budget
counts distinct touched owners, not calls. Bad values/capacity/identity preserve
the previous override and all shadows/underlays/outline/prepaint ownership.

Open-box setter supports the actual open anonymous or named paint node. Named
setter requires nonzero ID already submitted in THIS frame, open/closed/portal;
reject missing/layout-only/suppressed targets and ambiguous duplicate/folded-ID
collision frames. Reuse the guards in `cui_element_set_box_shadows`: current node
ownership, matching internal map/config userData, no text/exiting/previous node.
Do not invent anonymous IDs or expose the private layout engine. Existing late
collision detection remains authoritative: failed frames publish no valid V2 list.
Neither setter changes nodes, layout, semantics, event targets or callback timing.
Clearing must retain other custom-paint owners; an installed V2 custom shell may
remain through this frame only if ordinary output/order still matches exactly.
No-setter V2 frames wrap exactly the same ordinary command sequence as V1.

Refactor `src/core/cui.c:cui_end_frame` into one guarded finalizer with an internal
V1-or-V2 emitter. Keep V1 emission code behavior; V2 wraps ordinary local commands
and emits its background payload directly. NEVER produce/return an incomplete V1
list and repair it later. Existing `cui_prepaint_emit` continues accepting only
valid V1 commands, which the V2 emitter wraps; no new tag sneaks through prepaint.
Core's CUSTOM branch currently orders DROP list, outside outline, box fill, INSET
list, mesh, locally clipped underlay, prepaint, children; deferred prepaint and
portal/layer sorting retain their current positions. Replace ONLY box fill;
background is emitted even when the original byte alpha was zero. Keep existing
ancestor clips, separate outline/shadow radii and all shadow phases. Underlay's
local clip must not become an ancestor clip for DROP. No extra clip wrapper that
clips children/portals differently. Resolve bounds/radii from current final layout;
individual corner radii clamp to half min(width,height), matching existing shapes.
Store inherited opacity as one float, leave brush endpoints unchanged, apply that
opacity once during preparation. Legacy opacity truncation remains unchanged.

## Shared CPU preparation, physical coverage and output policy

New private allocation-free query/raster helper uses the approved common sampler.
Proposed portable records, with checked size/version rather than appended V1 fields:

```c
typedef struct {
    uint32_t size, background_output;
    unsigned viewport_width, viewport_height;
    float scale_x, scale_y, origin_x, origin_y;
    size_t cpu_bytes, texture_bytes, transfer_bytes, metadata_bytes;
    size_t max_pixels, max_pixels_per_background;
} cui_render_v2_options;
/* Private result; raster recomputes/checks plan rather than trusting caller edits. */
typedef struct {
    int32_t physical_x, physical_y;
    unsigned width, height;
    size_t stride, bytes;
    cui_rect logical_covered;
} cui_background_prepared_query;
```

Private query/raster take draw-by-value, render options, conservative active clip
and caller buffer/stride/capacity. Query leaves output unchanged on failure;
raster validates/recomputes its whole plan before writing. Output byte budget is
not a request for this helper to allocate. Place it below adapters, depending on
Core values/math, never Core depending on an adapter/component.
Query receives a copied background draw, viewport and explicit positive sx/sy in
[1/65536,64], origin x/y in[-1e6,1e6]; checks finite geometry/overflow/work/byte limits
and returns integer physical covering, logical placement, width/height/stride/bytes.
Raster writes a caller-owned prepared buffer only after full validation/capacity
checks. No retained cache or five new gradient shaders. Sampling frame always
remains original logical bounds, never cropped bounds or a logical-ceil surrogate.

Physical covering uses floor(left/top),ceil(right/bottom); sample target pixels at
physical centers, inverse-transform to logical points, preserve fractional phase.
Intersect with viewport/active clip OUTWARD integer covering for conservative
work culling only, without changing gradient origin/extent or baking ancestor
clip alpha into the raster. Initial explicit CENTER_V1 policy uses logical center-in-shape
coverage and existing independently clamped corner circles (ellipses under unequal
scale); transparent outside. It is a consistent discrete mask, not an analytic-AA
or source-GPU edge parity claim. No radius remask later. Future AA needs an explicit
policy/version and new tests, not silently changed pixels. Existing adapter
ancestor clipping applies ONCE at submission: software center tests, native
scissor conventions and Canvas clip edges retain their existing rules. Do not
pre-mask those clips and apply them again. This does not promise identical
ancestor-edge AA across adapters. Clip push/pop remain in the original stream.

Use common sample_srgba8 semantics for color: preserve Oklab excursions until
straight-linear clipping/encoding, alpha after resolved opacity/coverage, one final
byte rounding. Implementation needs a shared output-conversion helper so opacity
is applied BEFORE alpha quantization, not byte alpha multiplied a second time.
Prepared output is straight encoded RGBA8; target source-over follows established
IMAGE blending on encoded UNORM targets. No claim of linear-light destination
compositing or complete source framebuffer color-space parity. Nonmatching target
formats/color-management policies must reject negotiation, not approximate them.
Draw uploaded pixels at the inverse-transformed INTEGER covering rectangle, using
one-to-one physical sampling; drawing again into original fractional box bounds
would stretch/shift phase. Existing IMAGE helper's logical-ceil raster/mask path
MUST be bypassed via private prepared-image submission, not revision-bit tags,
synthetic shadow kinds, changes to old IMAGE or generic second radius/opacity.

## Five consumers, staging and failure lifetime

Proposed common `cui_render_v2_options` is a sized portable value containing output
policy, actual physical viewport width/height, sx/sy/origin and separate CPU,
texture, transfer and metadata byte budgets. Query validates actual target against
it; render revalidates after resize. No platform pointer is in Core records.
Each native opaque CUI adapter adds bool `*_support_v2(adapter,options,out_support)`
and bool `*_render_v2(adapter,list,clear,options)` for prefixes cui_software,
cui_sdl3, cui_sdl_gpu and cui_sokol; Canvas has the equivalent wire handshake.
Adapter-specific diagnostics must identify validation/preparation/submission failure;
existing error accessors are reused where available, new V2 diagnostics reviewed
where absent.
V1 functions remain available and cannot accept the V2 list type. No per-context
adapter binding and no new input dispatch architecture.

| Consumer/source seam | Required implementation, beyond shared query/raster |
| --- | --- |
| Software `backends/software/cui_software.c` | Full V2 validation before clearing pixels; prepared center-masked bytes source-over at exact physical cells; wrap ordinary drawing without routing backgrounds through `cui_bitmap_key_make`. |
| SDL_Renderer `backends/sdl3/cui_sdl3.c` | Query current target/DPI; stage RGBA textures and exact physical destination/source rectangle, neutral texture tint/opacity, preserve clip/order; explicit completion of queued SDL work before releasing prepared textures. |
| SDL_GPU `backends/sdl_gpu/cui_sdl_gpu.c` | New V2 collector branch feeds existing textured quads, direct prepared physical upload; no logical reraster. Retain texture/transfer resources through fence completion, including failed submissions and safe cancellation. |
| Sokol `backends/sokol/cui_sokol.c` | Reserved-slot `sg_init_image`/view preparation, exact physical quads/UVs, bounded ownership and explicit GLCORE fence retirement. See borrowed-runtime boundary below. |
| Canvas `web/gallery.c`, `web/gallery.js`, `scripts/build_web.py` | Negotiate V2 wire BEFORE compose; typed field exports and common C raster query/write exports; JS preflight all commands, owned ImageData/offscreen upload, strict tag/version checks and exact physical placement. Refresh views after WASM memory growth; no Canvas-gradient approximation. |

Every V2 renderer first validates header/stride/features/count overflow, all active
payloads, finite geometry and complete bounded clip-stack balance (depth<=256).
Then queries all work/allocations and stages needed backgrounds plus existing
image/mesh/shadow/glyph resources before clearing or beginning a target pass.
Some current legacy paths validate/upload while drawing; those need real V2
preparation refactors, not merely wrapping the old renderer and hoping. Borrowed
pointer validity is the caller's C contract; a validator cannot prove arbitrary
pointer safety. Rendering returns false with adapter error on failure, never a
representative-color or silently omitted background. Existing V1 stays exact.

Limits per V2 render: <=4M pixels/background, <=8M pixels/frame, <=16384 each physical
side, <=4096 backgrounds,32MiB CPU pixels,32MiB texture payload,32MiB transfer staging,
plus<=1MiB prepared-command/handle metadata, all constrained further by explicit
adapter limits. If actual metadata exceeds its limit, reject before target mutation.
Conservative additional background peak is97MiB when these copies coexist; it is
NOT total process/driver memory. Existing text/image/shadow caches, target/offscreen
surfaces, allocator alignment and opaque driver allocation add separately. Query
must expose/reject needed capacity; incremental release may lower peak only with
a proven lifetime ledger. Do not claim the earlier64MiB CPU+texture shorthand as
a complete upload bound. No retention cache initially.

One application-owned background batch may be in flight per adapter. Render
success means accepted/submitted, NOT GPU completion. Before returning, consume
or copy every needed frame-owned CPU payload into accounted adapter staging;
GPU retirement cannot extend the Core array/text/stop lifetime or leave a deferred
raw Core pointer. This is not full snapshot ownership of a copied V2 list.
Query must refuse another
V2 frame while an unretired batch occupies its resource envelope. CPU input may
be released only after its actual upload API has consumed/copied it; private
textures/transfer resources remain owned until safe retirement. Fresh handles/
revisions prevent pointer reuse from matching legacy caches; no cache tag tricks.
The GLCORE-specific correction below replaces the unverified Sokol wait/hook
proposal; other native consumers also need concrete API retirement proofs, not
an assumption that render return or a flush means GPU completion. Canvas's
synchronous byte/offscreen path may release C/JS staging after copying; any
asynchronous createImageBitmap path must retain it until completion and fit the
same one-batch/byte limits.

Validation/allocation/preparation failures leave the previous target untouched and
clean partial staging. After target mutation, unexpected provider errors, driver
submission failure or device loss report failure; there is NO GPU rollback promise.
Reset/recreate a lost consumer and re-query before a later application frame.
Actions may already have run during composition: never replay input, rerun accepted
callbacks, silently recompose or downgrade to V1 in the failed frame. CPU commands
and borrowed resources remain valid only under their documented lifetimes.

## Bridge, serialization and implementation checkpoints

`src/bridge/cui_bridge.c:cui_bridge_step` currently composes then ends a V1 list;
`cui_bridge_sdl.c` forwards it to `cui_sdl3_render`. Add a distinct versioned bridge
mode/symbol negotiation, query initialized actual backend before first V2 begin,
and choose ONE matching begin/end/render route throughout a step. Unsupported or
headless/no-consumer mode cannot advertise rendering; pure values stay usable.
Keep bridge ABI1 entry points/ordinary facade defaults intact. The future typed
Python/JS/TS/Go/Rust layer must detect symbols/features, copy brushes, validate
numbers, and surface failure; ABI1 alone is not a background feature test.

Gallery V1 `gallery_command` exports a property table, not binary command memory.
Add explicit V2 host/wire version and typed accessors for kind, brush kind/model/
space/angle, both color/position stops, bounds/radii/opacity and required policy.
Pointers/size_t/union bytes are never serialized. Legacy text/bitmap/mesh exports
need equivalent lifetime/deep-copy rules and bounded lengths; use checked wire
integer widths, not lossy float fields for 64-bit identity/revisions. Host snapshots
must own resource bytes explicitly. No old JS unknown-tag fill fallback in V2.
Intermediate forwarders/captures either deliberately negotiate V2 or stay V1;
they cannot receive an apparently successful incomplete V1 substitute.

Independent implementation checkpoints, each reviewed/tested before the next:

1. Freeze V1 ABI/command/software oracles and review these exact records, output
   policy, budgets and Sokol GLCORE retirement/pool-reservation contract; no renderer claimed yet.
2. Core V2 mode/storage/emitter and setters with source metadata/distribution/skill
   updates. No-setter V2 normalized legacy commands/pixels equal V1; reject old
   consumers before composition. Test wrong ends with real queued actions exactly
   once, bad-capability begin preserving queue, setter invalid/capacity/collision
   atomicity, anonymous/current named/portal geometry, clear/owners and lifetimes.
3. Shared physical query/raster plus software V2. Independent CSS/SOURCE_038 and
   sRGB/Oklab goldens; tiny HSLA, out-of-gamut/alpha quantization, fractional moves,
   unequal scales, clip corners, opacity once, DROP/outline/fill/INSET/mesh/underlay/
   prepaint order. Fault-inject all bounds/allocation/preparation paths and next-frame
   recovery; byte guards, borrowed-resource copy tests and no old-cache collisions.
4. SDL_Renderer, SDL_GPU and Sokol V2 preparation/ownership/output, then Canvas/WASM
   wire. Keep all five in scope. Compare prepared pixel oracle exactly; compare
   target pixels under each declared encoded-UNORM policy with explicit readback
   tolerance, including alpha/background mixtures and independent text commands.
   Test resize/DPI/phase, target-loss, upload failure, frame completion and Sokol
   host-owned runtime, delayed fences and shutdown/context loss. Native automation ONLY private-Xvfb wrapper, browser headless;
   hardware unavailable in virtual display is explicitly deferred, never marked PASS.
5. Versioned bridge + all five language authoring paths and installed consumers;
   old library/old header/unknown version refusal, resource serialization, WASM
   memory growth/strict unknown handling. Component-owned parts/theme gradients,
   remaining source patterns/focus/text backgrounds and all78 coverage stay explicit
   remaining work. No generic setter alone completes the component roadmap.

Run component-usage guide synchronization tests whenever usage metadata changes,
plus exact generated SDK/amalgamation/skill freshness. This checkpoint changes
only this design document; no public ABI, renderer or implementation is enabled.


## Correction: pinned Sokol GLCORE retirement and honest support limits

This section supersedes the earlier `submit_and_wait` hook and any claim that
Sokol itself supplies a GPU completion wait. Scope is CUI's EXISTING GLCORE
adapter, not Metal/D3D/Vulkan/WebGPU Sokol backends.

Pinned evidence: `third_party/dependencies.json` pins Sokol commit
`401f21f8b7039258c35fef75c11d9a8a0e616771`; checked header SHA256
`e23802d543747b1d24b4ab76e52634fb8b0a4a30afdb9f1437f63814f6bc77d7`.
Primary [pinned header](https://github.com/floooh/sokol/blob/401f21f8b7039258c35fef75c11d9a8a0e616771/sokol_gfx.h):

- Public declarations near5898–5940 expose resource operations/commit, no GPU wait.
- `sg_commit` near28267 calls backend commit, stats/listeners and increments frame
  index. `_sg_gl_commit` near13230 clears cached bindings; neither waits for GPU.
- GL draw near13214 issues `glDrawArrays` immediately; end-pass/commit is not an
  external deferred command-list submission requirement for this GLCORE path.
- `_sg_gl_discard_image` near11904 invalidates cache then deletes GL textures;
  `_sg_gl_discard_view` near12382 deletes view textures. Deletion is not a measured
  completion signal or a reason to stop counting uncertain in-flight allocations.
- Public `sg_alloc_image/view`, `sg_init_*`, `sg_uninit_*`, `sg_dealloc_*` support
  actual slot reservation. ALLOC slots are not initialized GPU images. Immutable
  image `num_slots` is1; transient writes use SG_NUM_INFLIGHT_FRAMES (near9005).

CUI `backends/sokol/cui_sokol.c` currently requires GL>=3.3, calls `sg_end_pass`,
commits only if owns_runtime, and uses a commit listener only to reset rendered.
The owned runtime configures image/view pools512 each. White1+glyph256+bitmap64
can consume321 pairs:191 is merely an arithmetic upper bound, NOT available
capacity. Borrowed runtime defaults can be128 images/256 views, or host-defined;
host allocations and other adapters share those pools. The prior4096 blanket
background count cannot be advertised by this implementation.

### Concrete completion state machine; no generic host wait promise

Resolve real `glFenceSync`, `glClientWaitSync`, `glDeleteSync` privately for this
GLCORE adapter before advertising V2. GL3.3 includes the GL3.2 sync contract;
pinned Sokol does not export those calls or include them in its Win32 loader.
Use an adapter-private loader/platform GL linkage (including external-runtime
builds), fail capability negotiation if unavailable; do not patch pinned Sokol or
expose GL types through portable public records. No CPU-upload/commit shortcut.
Khronos primary contracts: [fence](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFenceSync.xhtml),
[bounded wait](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glClientWaitSync.xhtml).

After all CUI GL commands and end_pass, owned runtime performs its ordinary
sg_commit; borrowed runtime DOES NOT. Insert a real fence with
GL_SYNC_GPU_COMMANDS_COMPLETE and flags0. Poll with glClientWaitSync and the flush
flag, timeout0 by default; an optional wait budget must be finite and<=1ms per
call, never an unbounded glFinish or busy-spin. Sync calls do not change GL texture,
program/framebuffer bindings, so they do not invalidate Sokol's binding cache.
If future upload code binds GL resources directly, restore state AND invalidate
Sokol cache explicitly; current plan uses sg_init_* for upload instead.

| State | Ownership and allowed next operations |
| --- | --- |
| IDLE | Reserved ALLOC slots, no background GPU payload. Query may advertise prepared capacity. |
| IN_FLIGHT, valid fence | Render may return true (submission accepted); retain initialized views/images and any unconsumed CPU/transfer data. TIMEOUT_EXPIRED stays here; next query/render refuses a new frame until nonblocking polling signals completion. No fresh allocations/batches on timeout. |
| RETIRED | ALREADY_SIGNALED/CONDITION_SATISFIED: delete fence, uninit views then images to ALLOC, release staging and return IDLE. No inference from sg_commit. |
| QUARANTINED | Fence creation failure/WAIT_FAILED/unknown completion: render false, retain the ONE batch and refuse further renders/configuration. Explicit recovery may establish ONE replacement fence after the last use in the SAME still-valid GL context (at most two sync objects total); only observed signal retires the payload. No further replacement while one is pending/failed. Do not repeatedly allocate fences/batches on query. |
| CONTEXT_LOST | Host notifies loss; stop GL calls and refuse reuse. Handles are not valid in a replacement context even if integer names match. Runtime/context owner coordinates teardown; never destroy uncertain resources merely to meet budget. |

A borrowed host still calls its normal sg_commit to advance Sokol frame accounting;
our fence neither calls that commit nor resets the listener/host frame index.
Require BOTH GPU retirement and host commit before another render; a commit alone
is insufficient. A signaled private fence permits resource release without an
extra host commit. Keep host context/current-thread/lifetime requirements explicit;
glGetString succeeding in another context does not prove the original survived.

New opt-in V2 lifecycle needs a bool close/release operation preserving the caller's
adapter pointer on BUSY/QUARANTINED. Poll boundedly; if uncertain, keep ownership
and retry later. V1-only void destroy stays unchanged. For a V2-enabled pending
adapter, old void destroy must fail closed/diagnose without freeing uncertain
resources; applications must use the checked V2 release contract. Once GPU is
retired, release reserved slots and normal adapter resources under the original
valid context. If the host destroys that context, the driver owns final GPU
reclamation; do not call sg_destroy_* in a replacement context. A dedicated
lost-context teardown contract must discard CUI CPU metadata only AFTER the old
Sokol runtime/pools are also torn down by their owner. In borrowed mode this is
a host responsibility, not adapter sg_shutdown. Pinned Sokol has no established
public abandon-without-GL operation: permanent loss of the original context has
NO promised in-process cleanup/recovery here. A mere host bool acknowledgement
cannot make sg_destroy_* safe. Fail closed with the one batch quarantined until
original-context recovery permits verified cleanup, or owner/process teardown
reclaims it; do not initialize a replacement runtime and accidentally delete its
reused numeric objects. Any new context-abandon facility is a separate reviewed
architecture decision. This limitation must be explicit before production.

### Reserve counts, exact byte formulas and failures

Add explicit adapter-local V2 prepare/configure outside Core composition, with
requested limits and a completion budget. For Sokol it atomically reserves N
image AND N view slots via sg_alloc_*; rollback only uninitialized ALLOC slots
on failure. Reconfiguration reserves only a positive delta, never a duplicate
full reservation; rollback that delta without losing prior limits. Shrink only
while IDLE, deallocating unused ALLOC slots. Keep them reserved across frames so the borrowed host cannot steal
advertised slots. Query reads successful reservation, not total-minus-cache or
predicted eviction. A reasonable owned-runtime REQUEST is128 pairs (321+128<512),
but even that must actually reserve or fail; do not silently enlarge V1 pools.
Borrowed runtime must also actually reserve requested slots or refuse setup.
Initialization/payload allocation occurs during frame preparation, within byte
bounds; reservation itself is not proof that every later GPU allocation succeeds.
After fence retirement, uninit returns those same objects to ALLOC for bounded
reuse; checked close deallocates them. A quarantined batch still occupies these
slots and all retained payload bytes: never advertise them as free.

Support reports actual reserved N plus complete pixel/CPU/texture/transfer/metadata
limits added to the record above. Values are minima of requested limits, shared
hard ceilings and adapter capacities. Query fails BUSY for active pass, pending
frame/host commit, lost target/context or quarantine; it cannot advertise support
while secretly relying on the next render to evict an uncertain batch. Input
max_commands is a bound on input records, not a guarantee that arbitrary text
fits max_quads: full-list preparation separately counts expanded glyph/mesh draws.

Core admission remains precise: C=min(config.max_commands,16384,support.max_commands,
floor(command_bytes/sizeof(command_v2))); B=min(config.max_elements,4096,
support.max_backgrounds). Background storage requires checked
M=align_up(config.max_elements*sizeof(uint32_t),alignof(owner_record)), then
M+B*sizeof(owner_record)<=background_bytes for one optional block; every multiply,
alignment and sum is checked. Command storage C*sizeof(command_v2)<=command_bytes.
Reject C0, and B0 when backgrounds requested, before input. A growth begin may
invalidate/free old frame-only storage before allocating replacement (old borrowed
lists already expire at begin); failed allocation leaves input queued/context
usable. Do not allocate a duplicate high-water block and quietly exceed the32MiB
Core payload cap during growth.
Zero budgets select defaults; insufficient storage fails begin BEFORE input,
never quietly shrinks B below the advertised admission bound. Node-map bytes
are Core-owned; raster/handle reservations are adapter-owned. Copying support
copies no reservation ownership and does not guarantee availability on a different
consumer: compatible consumers revalidate their own capacity before painting.

For owned N128/default4096 nodes, common64-bit estimates are16KiB node map +11KiB
owner records, not4096 records. Use actual sizeof/alignment on every ABI. The
97MiB staging ceiling remains a conservative additional upper bound, including
ONE quarantined batch, not97MiB new memory allowed on top of that batch. Immutable
Sokol images avoid transient multi-slot texture multiplication. Pool reservation
metadata and up to two sync objects count in metadata_bytes; existing Sokol pools and
legacy caches/target/driver storage remain separately documented overhead.

Preparation failure BEFORE any GL upload can release CPU data and ALLOC slots
safely. After any upload/draw has entered GL, even pre-clear failure must finish any
CUI-owned pass if the context remains valid, then fence/retire or quarantine its
issued resources; never close an unrelated host pass; preserve old target when failure was
pre-clear. Timeout/target resize/unknown submission does not transfer ownership
to Core or caller and never replays actions. Ordinary resize may re-query only
AFTER retirement; context loss follows the explicit owner teardown above.
Tests must mock/simulate delayed/failed fences, ALLOC versus initialized pools,
host stealing unreserved slots, no extra borrowed commit, unchanged V1 pools,
strict quota arithmetic/alignment, repeated failure without growth, release refusal,
recoverable context loss versus permanently quarantined teardown, and next-frame
recovery where completion was actually established. Real GLCORE fence/readback
proof uses private-Xvfb where available; unsupported GPU checks remain deferred.
