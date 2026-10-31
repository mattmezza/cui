# Background transport: pre-composition negotiation and complete frames

Static addendum to [ELEMENT_BACKGROUNDS.md](ELEMENT_BACKGROUNDS.md), baseline
`f724017c`; no architecture/API is approved or implemented. This supersedes that
checkpoint's recommendation to return an incomplete base list or reject a legacy
`end_frame` after composition. Preserve the signed Button/background branches.

## Actual integration constraints

`cui_draw_command` is a public struct, not an extensible tagged union. It contains
bitmap, mesh, text-paint and shadow fields separately; its size/stride appears in
arrays, copies and adapters. Adding an inline brush normally grows stride.
Overlaying a new pointer/union onto shadow storage is NOT proven ABI-safe: size,
alignment, offsets and old source member access need every supported ABI checked.
An added enum tag alone carries no owned brush and is not backward compatibility;
enum representation/stride still need ABI checks, and exhaustive switches change.

`cui_renderer` currently has capabilities and `render(user,list,clear)`, but no
context/payload lookup. `cui_end_frame` is called inside bridge step
(`src/bridge/cui_bridge.c`), with SDL rendering in `cui_bridge_sdl.c`, and inside
`web/gallery.c`. Gallery exports each command field via `gallery_command` plus
separate text/bitmap accessors, enumerated by `scripts/build_web.py`. Canvas
`web/gallery.js` does not strictly reject every unknown tag: a new positive tag
can fall through to an ordinary fill. Native switches also do not uniformly
preflight an entire unknown-tag list before touching output. Neither a new enum
nor a capability bit added to an old struct repairs already compiled consumers.

## Alternatives

| Choice | ABI / ordinary frame API | Complete-frame and ownership cost |
| --- | --- | --- |
| A: existing command storage, NEW background tag + context-owned versioned descriptor lookup | Old structs stay byte-for-byte; existing begin/end/render calls can remain. New adapter binding is startup-only. | Background-aware renderer is explicitly bound to this context; tag is never a transparent placeholder. New context/consumer protocol and copy restrictions require approval. |
| B: inline command extension or `cui_draw_command_v2`/versioned list | Appending a field breaks old stride. Explicit V2 types preserve V1 but require new producer/consumer/export entry points. | Brush can travel with a command by value; simpler independent copying. Still must negotiate before composition; cannot cast V1/V2 arrays or silently drop the brush. |
| C: Core-owned raster lowered to existing IMAGE | V1 list is complete and render APIs unchanged. | Needs a NEW bounded pixel owner and authoritative target scale before composition. Existing bitmap helpers rasterize/crop at logical ceil sizes; merely supplying high-DPI pixels does not preserve physical fidelity. Fixing that globally changes old image pixels; hidden image tags/revision bits are unacceptable. |
| Earlier sidecar with transparent RECT base | Preserves physical struct size, introduces a separate per-frame view | Reject: an intermediate consumer can render an apparently valid but incomplete list; late end-frame rejection also occurs after actions ran. |

**Recommendation for review:** A minimizes per-frame API proliferation, ONLY if
one explicit context-bound consumer and current-frame command views are acceptable.
Choose B instead if independently copied/reordered/serialized semantic commands
must be a first-class public contract. C is not a shortcut to full five-adapter
DPI support; logical-only fallback must not be presented as equivalent semantics.
No implementation starts until this choice and the ownership/failure rules below
are approved. Storage ABI compatibility in A is not legacy transport compatibility.

## A: explicit opt-in before begin_frame, no new end-frame function

Sketch names only; the common value type remains unapproved:

```c
/* Versioned record, not an extension to cui_renderer or cui_config. */
typedef struct {
    uint32_t size, descriptor_abi, model_mask, space_mask;
    uint32_t shape_policy, output_policy;
    size_t max_brushes, max_raster_pixels, max_frame_bytes;
} cui_background_support;
/* Per-adapter bind/unbind outside frames, after preceding render completes. */
bool cui_sdl3_bind_backgrounds(cui_sdl3 *, cui_context *, const cui_background_support *limits);
/* Other four consumers need equivalent binding/handshake, not this native type. */
bool cui_background_command_read(cui_context *, const cui_draw_list *, size_t index,
                                 uint32_t expected_descriptor_abi, cui_background *out);
```

An UPDATED adapter factory/handshake supplies actual supported policies, format,
limits and descriptor ABI; callers cannot infer support from IMAGE, bridge ABI1,
or header presence. Bind checks complete model/space/shape/output compatibility
and establishes its target/resource policy before any frame. Core copies support
values; it does not own the adapter or retain arbitrary callbacks. The adapter
borrows the context until explicit unbind/destruction; unbind/change requires no
active frame and completed rendering. No global registry, resource graph, new
threads or multiple outstanding frames. Destruction order/lifetime is part of
this NEW contract, not something the existing renderer interface already ensures.

Without this negotiation, setters reject before changing paint/allocating brush
storage. An old app follows the exact legacy route: no tag, descriptor or new
end-frame failure. Once enabled, setters copy bounded descriptors into Core-owned
frame storage and end_frame emits the explicit tag at the actual fill position,
with existing bounds/corners and a validated descriptor lookup association.
No stop/color pointers are borrowed. No brush is hidden in bitmap/mesh/text fields.
The bound updated adapter uses its normal render entry point and verifies the
current context/list association, descriptor version and every tag before paint.
Manual fabricated background tags/prepaint emits are rejected, not guessed.
Lists containing only legacy tags retain their existing render route.

Copying a list header while retaining the original frame command view is permitted
through renderer completion; a new begin_frame already invalidates borrowed data.
Deep-copying/reordering command arrays does NOT automatically copy lookup identity.
An owned snapshot/export must copy typed brushes as well as commands and use an
explicit validated importer; no address matching, old bounds, raw C memory dumps
or magic revision bits. If that restriction is too costly, prefer B now. Accessor
checks cannot make an expired borrowed list safe after buffer reuse.

## Simplest coherent full-five-adapter path

Start with ONE shared checked CPU sampler/physical-raster preparer for both models
and spaces, including the agreed shape/alpha/clip policy. Use it in all five
adapters initially rather than implementing three shader/Canvas interpolation
variants at once. Existing typed brush values and previous mathematical evidence
are reused; extracting math must preserve old Plot/effects/text output exactly.

| Consumer | Minimum integration beyond common preparation |
| --- | --- |
| Software | New-tag preflight, descriptor access, prepared-pixel source-over in command order. |
| SDL_Renderer | Bind target scale; upload prepared physical RGBA; draw without logical-size rerasterization or duplicate shape/opacity. |
| SDL_GPU | New-tag branch, bounded prepared texture/upload ownership, current textured-quad path; no new gradient shader required initially. |
| Sokol | Equivalent bounded texture/view staging and command ordering; prepare before render-pass mutation. |
| Canvas/WASM | Version/feature handshake, explicit tag/descriptor field exports, physical raster helper bridge and strict JS dispatch; no native Canvas-gradient approximation. |

Thus minimum is 1 Core owned-descriptor/negotiation seam, 1 shared math/preparation
module, 4 native adapter integrations, 1 C+JS Canvas wire integration, and bridge
binding/error plumbing. This is a structural count, not a line/time estimate.
Each adapter needs a descriptor/version/full-tag preflight and actual output tests;
IMAGE support does not make any of those integrations free. Existing bitmap
logical-ceil cache paths cannot be reused blindly for physical prepared rasters.
Agree whether new raster coverage matches existing RECT per adapter or uses one
shared mask: old GPU/Canvas AA differs. A new consistent mask is an explicit
opt-in edge policy, not exact old-solid/source-GPU pixel parity. Untouched legacy
commands keep their existing paths. Shader optimizations/cache reuse come later.

Core descriptor count <=min(max_elements,4096), charged to frame_bytes, replacing
an owner record in place; no unbounded allocation per repeated setter. Approximate
common-ABI descriptor value is72 bytes, but budget by actual sizeof+index metadata,
not that estimate. Renderer owns transient CPU rasters and staged uploads, NOT
Core. Proposed ceilings remain4M pixels/brush,8M pixels and32MiB CPU/frame plus
32MiB upload/frame,16384 per physical side, checked arithmetic and target transform.
Include command-copy/staging overhead separately. No retained raster cache assumed.
This requires a NEW whole-frame preparation phase in each adapter, not an existing
atomic-render guarantee. Allocate/query/rasterize/stage before clearing; fresh
pixel revisions and renderer completion prevent overwritten live texture sources.
On validation/capacity failure retain previous style; conversion failure returns
an empty erroneous list, never an incomplete successful list. Render preparation/
upload failure before target mutation returns false and preserves the previous
surface; clean staging and reuse the context next frame. Device loss/failed submit
after mutation cannot promise GPU rollback. Existing action callbacks may
already have run: no event replay or implicit recomposition/rollback on failure.

## CPU preparation lowered to IMAGE: simplest first rendering option

A shared bounded sampler can lower each semantic fill to an owned per-render RGBA
IMAGE at explicit physical scale; all five then reuse their image compositing and
texture paths, without five shaders or a retained cache. This is the preferred
first rendering implementation to investigate, independently of transport A/B.
It is not currently a drop-in Core IMAGE substitution: bitmap helpers regenerate
logical-ceil rasters and radius masks. Add an explicit prepared-image path/private
adapter staging so physical pixels survive, shape coverage/opacity apply exactly
once, and clip/geometry/order remain those of the original fill. Do not reinterpret
legacy IMAGE fields or alter old image output. Canvas needs equivalent owned bytes
and explicit scale/wire lifetime, not a native-gradient approximation.

Existing SDL_GPU bitmap upload copies raster bytes into a transfer buffer, records
an upload and releases that buffer; Sokol uploads through its image path. That does
not establish a universal borrowed-CPU-buffer completion contract. Each adapter
must prove when CPU bytes may be released and who retains textures/transfer storage
until queued GPU work completes. Public IMAGE currently borrows pixels through
render completion; asynchronous staging cannot borrow the next frame's scratch.
Use per-render owned staging and explicit completion/driver-safe release, or bound
in-flight work to one and wait; do not assume render-return means GPU completion.
With 8M pixels the proposed 32MiB CPU +32MiB texture budget can additionally need
32MiB transfer staging: budget96MiB for simultaneous whole-frame copies, before
command arrays, shadows, surfaces, driver alignment and previous in-flight frames.
Incremental release may lower peak only with a proven lifetime ledger. Charge costs
or lower the cap; blocking completion avoids unbounded frame multiplication but
has latency cost. Upload/allocation failure fails the frame, never flat-color
approximation. Performance evidence can justify shaders/cache later.

## Bridge, exports, failure recovery and review gate

Bridge must bind the actual SDL consumer after context creation and before step's
first begin_frame. Headless/no-consumer mode cannot advertise renderer support;
pure palette/value construction remains available independently. Check support
at attachment/update before retaining an unsupported requested brush. Future
Python/JS/TS/Go/Rust authoring must negotiate new symbols/capability versions,
not assume that their existing ABI1 library supports this feature.
Gallery/WASM must negotiate the host wire version BEFORE its first composed
frame, export full typed brush fields, and reject unknown tags/versions in JS.
Other end-frame forwarders, captures and serializers must join that negotiation
or remain legacy-only. A missing/old consumer never enables semantic backgrounds.
Updated adapters must reject unsupported whole lists before target mutation;
a caller deliberately bypassing the negotiated consumer cannot be made safe by
an enum alone. B provides stronger type separation if that must be prevented.
Recovery changes limits/target/binding between frames; it does not silently
flatten to a representative color or rerun accepted actions in the failed frame.

Slider coordination: signed resolver design `185ee042` uses private owned
`cui_slider_palette_color`/brush, straight linear float RGBA, fixed2stops, explicit
sRGB/Oklab and GPUI038 projection only. Keep it pure and renderer-independent;
source-Hsla conversion preserves hidden RGB without byte quantization, while
custom cui_theme bytes explicitly decode sRGBA8. Convert privately to an approved
shared type later; no duplicate universal brush ABI is being implemented.
Review must select A/B/C, version/lease/copy rules, shape/output policy and the
resource/failure protocol before production. Require all-five adapter output,
legacy ABI/pixel, old-consumer refusal, copy/version/resize/clip/budget recovery
and headless browser/private-Xvfb tests. All78 coverage, theme brush ownership,
source patterns, focus/text and foreign authoring remain separate pending slices.
