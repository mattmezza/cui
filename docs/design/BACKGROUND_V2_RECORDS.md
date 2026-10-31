# Versioned background commands and five-adapter preparation

Proposed records/contracts only, following approved transport B. Values/math are
implemented by `2a435a5f` + `cf72b10e`. The integrator reviewed and approved
the contract through `638fc225`; checkpoint2 now implements Core mode/storage/emission/setters only.
Checkpoint3 supplies the private allocation-free CPU query/raster helper described in
[BACKGROUND_PREPARATION.md](../BACKGROUND_PREPARATION.md). Adapter queries/renderers,
upload lifetimes and bridge/wire records below remain proposals.
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
| Sokol `backends/sokol/cui_sokol.c` | Reserved-slot `sg_init_image`/view preparation, exact physical quads/UVs and ordinary same-context GL deletion after the last issued draw; no extra wait/commit. See borrowed-runtime boundary below. |
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
Conservative additional application-visible per-call payload budget is97MiB
when these copies coexist; it is NOT a physical GPU/process-memory peak bound. Existing text/image/shadow caches, target/offscreen
surfaces, allocator alignment and opaque driver allocation add separately. Query
must expose/reject needed capacity; incremental release may lower peak only with
a proven lifetime ledger. Do not claim the earlier64MiB CPU+texture shorthand as
a complete upload bound. No retention cache initially.

One application-owned prepared batch per adapter call is the initial allocation
policy. Render success means accepted/issued, NOT GPU completion. Before returning,
consume or copy every needed frame-owned CPU payload into accounted staging;
GPU execution cannot extend the Core array/text/stop lifetime. Private resource
release follows each backend's actual API guarantee, NOT a blanket requirement to
wait for GPU completion. GLCORE ordinary deletion after last issued use is proved
below; other native adapters still require their own concrete ownership proof.
Fresh private resources bypass legacy pointer/revision caches. Canvas's synchronous
byte/offscreen path may release C/JS staging after copying; asynchronous
createImageBitmap must retain it until copy completion and fit byte limits.
Driver-retained allocations after safe API release are NOT bounded by these
application-owned per-call budgets. No total VRAM/process-memory guarantee.

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
   policy, budgets and Sokol GLCORE deletion/pool-reservation contract; no renderer claimed yet.
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
   host-owned runtime, deletion before host commit, repeated reuse and context loss. Native automation ONLY private-Xvfb wrapper, browser headless;
   hardware unavailable in virtual display is explicitly deferred, never marked PASS.
5. Versioned bridge + all five language authoring paths and installed consumers;
   old library/old header/unknown version refusal, resource serialization, WASM
   memory growth/strict unknown handling. Component-owned parts/theme gradients,
   remaining source patterns/focus/text backgrounds and all78 coverage stay explicit
   remaining work. No generic setter alone completes the component roadmap.

Run component-usage guide synchronization tests whenever usage metadata changes,
plus exact generated SDK/amalgamation/skill freshness. This checkpoint changes
only this design document; no public ABI, renderer or implementation is enabled.


## Simpler correction: ordinary GLCORE deletion, not GPU waiting

This replaces the fence/quarantine/loader/new-close proposal in `6160b94e` as well
as the earlier generic submit_and_wait hook. Normal asynchronous execution does
NOT require CUI to retain API objects until GPU completion. Scope is the existing
single-current-context GLCORE adapter only; no claim for other Sokol backends,
exported views, cross-context consumers, persistent maps or deferred CPU replay.

Primary [OpenGL4.4 core specification](https://registry.khronos.org/OpenGL/specs/gl/glspec44.core.pdf):
§2.1 (printed p9) requires ordered command effects and interprets client data on
call; later changes cannot alter already supplied data. §5.1.2–.3 (pp48–49)
separates name deletion/current-context unbinding from underlying storage retained
by references/views. Combining those rules, later deletion cannot invalidate an
already-issued draw. Driver storage reclamation may be deferred; deletion is not
GPU completion or a synchronous memory-release guarantee. §8.4.1 distinguishes
client pointers from pixel-unpack-buffer offsets.

Pinned Sokol evidence: commit `401f21f8b7039258c35fef75c11d9a8a0e616771`, local
`third_party/sokol/sokol_gfx.h` checked SHA256
`e23802d543747b1d24b4ab76e52634fb8b0a4a30afdb9f1437f63814f6bc77d7`;
[exact pinned source](https://github.com/floooh/sokol/blob/401f21f8b7039258c35fef75c11d9a8a0e616771/sokol_gfx.h).

| Source seam | Actual behavior relevant to lifetime |
| --- | --- |
| `_sg_gl_draw`, near13180–13214 | Issues glDrawArrays/glDrawElements variants during sg_draw. There is no deferred CPU Sokol draw-list replay at commit. |
| `sg_commit`, near28267; `_sg_gl_commit`, near13230 | Commit updates accounting/listeners/frame index and clears GL binding caches. It is neither a GPU wait nor required to issue the preceding GLCORE draws. |
| `_sg_gl_create_image` / `_sg_gl_teximage`, near11810–11901 | Immutable client bytes go into GL texture calls; no retained caller-pixel pointer. Require PIXEL_UNPACK_BUFFER=0 for this path; validate/refuse a nonzero external binding rather than treating a pointer as a PBO offset. |
| `_sg_gl_discard_view`, near12382 | Invalidates cached texture/sampler bindings, deletes any GL texture views (and relevant auxiliary objects). Plain texture views may use the parent image without a separate GL view object. |
| `_sg_gl_discard_image`, near11904 | Invalidates cached texture/sampler bindings and glDeleteTextures for owned images. Background images are not injected/external. |
| `_sg_gl_cache_invalidate_texture_sampler`, near11389 | Unbinds matching texture/sampler slots and clears cached/stored identifiers, preventing numeric GL-name reuse from skipping a needed bind. |
| `_sg_uninit_view/image`, near26991–27023; reset helpers near8387/8423 | Calls backend discard, clears payload and resets to ALLOC, retaining reserved slot and incrementing uninit_count. |
| `_sg_sref_slot_eql`, near8712; resource-ref alive checks near8790 | Internal references compare ID AND uninit_count, so ordinary reserved-slot reinitialization invalidates old internal references. Public same-slot handles still require explicit private-reference clearing. |
| `_sg_dealloc_view/image`, near26834–26867 | Returns ALLOC slot to the pool and resets slot identity. |
| `_sg_slot_alloc`, near8490; `_sg_lookup_view/image`, near8566/8606 | Allocation increments generation; lookups compare full ID. Generation wrap is explicitly not permanently prevented by pinned code. Do not rely on immortal stale-handle rejection. |

### Proposed preparation/use/release sequence

1. Before composition, configure/reserve actual image AND view slots (below), query
   support; use the existing same-context/current-thread/no-active-host-pass contract.
2. Prepare RGBA bytes and initialize reserved owned immutable images, then views.
   Client source bytes may be freed after sg_init_image returns with the upload
   consumed, not after an arbitrary future fence. Check logger/GL errors as well
   as sg resource state BEFORE clear: GL create_image returns VALID after its GL
   calls, so resource state alone does not prove an allocation succeeded.
3. Emit every prepared draw through the normal immediate sg_apply_*/sg_draw path,
   preserving command order. End the CUI pass. Once there is no future CPU use of
   those private view/image handles, uninit views FIRST, then images to ALLOC.
   This ordered API deletion is safe even when GPU work remains outstanding.
4. Clear prepared draw references/counts before any next collection. Do not put
   these images in old bitmap caches, export them to the host, retain them in other
   contexts/containers, or replay a copied CPU plan after releasing them. Future
   initialization of the same reserved handle is a new private payload; no stale
   CPU command may survive. Sokol invalidates its GL cache on discard. No raw GL
   upload/bind bypass or additional cache-reset API is needed for this route.
5. Owned runtime performs its normal sg_commit; borrowed runtime DOES NOT gain an
   extra commit. Host still commits normally to advance Sokol frame accounting;
   current rendered/listener guard still prevents another adapter render before
   that host commit. Resource deletion may occur before commit because draws are
   already issued. No new loader, GPU-wait hook, fence polling, quarantine or
   checked-close lifecycle is required for ordinary asynchronous execution.

This is an API lifetime proof, not measured GPU completion. A test must delete
before host commit and read pixels AFTER commit without using the deleted handles.
Sokol has its own binding validation/state; next host draws must apply their own
valid resources as usual. No continued CPU draw may use the discarded CUI handles.
Immutable images use one GL storage slot (`num_slots` near9005); do not silently
switch to transient multi-slot images or persistent-mapped staging.

### Actual reservations and complete Core/adapter accounting

Keep explicit adapter-local V2 prepare/configure outside composition. Atomically
reserve N image and N view slots via sg_alloc_*; ALLOC is a slot, not an initialized
GPU object. Keep slots across frames, initializing/uninitializing only payloads.
On configure failure roll back newly reserved ALLOC slots; reconfiguration adds
only the positive delta and preserves prior limits on failure. Shrink only outside
rendering, deallocating unused ALLOC slots. A borrowed host cannot steal a slot
already reserved. Query reports successful reservation, not predicted evictions.

CUI currently uses owned Sokol image/view pools512 each. White1+glyph256+bitmap64
can consume321 pairs, leaving191 as arithmetic upper bound, not guaranteed free
capacity. A reasonable owned-runtime REQUEST is128 additional pairs, which must
actually reserve or fail; do not enlarge V1 pools silently. Borrowed hosts may use
Sokol defaults128 images/256 views or custom pools and concurrent allocations;
reserve actual requested slots or refuse setup. This corrects blanket4096 support.

Support reports actual N plus the complete pixel/CPU/texture/transfer/metadata
limits in the record above, constrained by requested/shared/adapter ceilings.
Input max_commands counts input records, not expanded glyph/mesh quads; full-list
preparation checks actual draw expansion separately. Query refuses active passes,
uncommitted previous adapter rendering, invalid/lost targets and unavailable
reservations, not merely a GPU that may still be executing safely deleted objects.

Core admission is C=min(config.max_commands,16384,support.max_commands,
floor(command_bytes/sizeof(command_v2))); B=min(config.max_elements,4096,
support.max_backgrounds). With checked alignment/multiplication/sums:
M=align_up(config.max_elements*sizeof(uint32_t),alignof(owner_record)), then
M+B*sizeof(owner_record)<=background_bytes for one optional block; command
storage C*sizeof(command_v2)<=command_bytes. Reject C0 and requested-backgrounds
B0 before input. Do not silently shrink B to hide insufficient map/record bytes.
A growth begin may free old frame-only storage before replacement (borrowed lists
already expire at begin); failed allocation leaves input queued/context usable.
Do not temporarily duplicate high-water blocks beyond32MiB Core payload bounds.
For N128/default4096 nodes, common64-bit estimates are16KiB node map+11KiB owner
records, not4096 records; use actual sizeof/alignment on each ABI.

Node maps/commands belong to Core, CPU rasters and live API textures to adapters;
copying support transfers no reservation ownership. Another compatible consumer
revalidates its own actual capacity. The97MiB CPU+live texture+transfer+metadata
figure is a conservative per-call ADDITIONAL application-visible envelope,
NOT a limit on process RSS, total VRAM or driver-deferred backing from past frames.
OpenGL may retain deleted storage/command copies across queued frames, so a strict
physical GPU-memory bound does not follow from N reserved slots or byte ledgers.
No bounded reclamation latency is claimed. Existing caches/targets/driver overhead
add separately. GLCORE client uploads need no CUI-owned transfer buffer; other
adapters' concrete upload copies must still be charged. If a hard driver-memory
bound is required, explicit throttling/completion becomes a separately reviewed
policy; it is not necessary for this image-lifetime correctness proof.

### Failure, resize and destruction

CPU/allocation/preparation failure before clear preserves the previous target.
With the original valid context, dispose partially initialized private views then
images through normal Sokol APIs, even after upload or already-issued draws; no
GPU fence is needed for deletion safety. Clear CPU references and preserve the
reserved ALLOC capacity for the next frame. Do not silently omit or flatten a
background. A late draw/device error can already have changed the target: report
failure, no rollback or event replay. A resize in the same valid context follows
the same API cleanup and fresh physical preparation, without waiting for old GPU
work merely because its deleted storage may remain driver-owned.

Normal destroy under the original valid context may deallocate reserved slots and
normal resources with the existing lifecycle; no new pending-GPU close protocol.
Actual context loss remains outside that valid-context deletion proof: stop GL
calls, do not delete old numeric names in a replacement context. The existing
host/runtime teardown requirements still apply; pinned Sokol provides no proven
abandon-without-GL API, so do not promise automatic in-process lost-context cleanup.
This limitation does not justify fence quarantine during NORMAL async execution.

Required tests: reserved-versus-initialized slot accounting/partial failure;
uninit view/image after last draw BEFORE borrowed host commit; copied CPU bytes
mutated/freed after upload; immediate repeated reinit with different brushes and
numeric-name reuse; no stale draw/cache references; correct ordinary host draw
bindings after disposal; unchanged V1 pools/routes; resize/error cleanup; byte and
alignment admission limits; original-context loss refusal. Native GLCORE proof
runs only via private-Xvfb where supported; no hardware PASS inferred from source
inspection. Other four adapter ownership proofs remain independently required.

## Approved text accounting clarification (V2 opt-in only)

Following `SOFTWARE_V2_FONT_SESSION.md` (224309bd), `cpu_bytes` bounds adapter-owned
transaction payload plus counted CUI text temporaries. Metadata is separately
bounded. Existing caller-owned font/cache state AND newly allocated opaque
FT/HB/Sheen/provider temporaries are outside that byte envelope. Their lifecycle
and failures are audited, not silently reported as zero or jointly <=32MiB. This
is not a process/RSS or arbitrary-provider allocation/time bound. No global
allocator swaps or host dependency replacements follow from this clarification.

Text admission additionally limits full paragraph input64KiB, aggregate repeated
input256KiB, selected/fallback faces64, shaping attempts8192, segments4096,
glyphs65536 and copied coverage8M pixels per frame; a bitmap has <=4096/side and
4M pixels. Enforce countable input limits before work; output-dependent limits
can fail after opaque allocation but before publication. Full shaping/fallback,
selection and approved glyph transforms remain required; these opt-in limits do
not alter V1. No adapter capability is advertised until preparation/painting and
independent tests are complete.
