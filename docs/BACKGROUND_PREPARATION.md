# Unreleased deterministic background preparation

Maintainer building block only: `src/core/cui_background_prepare_internal.h`.
The public sized `cui_render_v2_options` record lives in `cui/cui_draw_v2.h`;
query/raster functions and their result record are private, not a stable consumer
API. **No software/native/Canvas adapter advertises or consumes V2 backgrounds.**
All five consumer integrations, full-list validation, target/upload lifetime,
wire exports, foreign authoring and all78 component-part coverage remain pending.
See [Core V2 transport](CORE_DRAW_V2.md) and the [five-adapter plan](design/BACKGROUND_V2_RECORDS.md).

The allocation-free helper takes owned draw-by-value, copied options, a logical
active clip and caller output storage. It has no context, cache, pointer retention,
resource graph, driver calls, target mutation or upload ownership. It uses the
existing [pure sampler](BACKGROUND_VALUES.md) for both models and both spaces.

```c
/* PRIVATE declarations, not new installed public CUI functions. */
cui_error cui_background_prepare_query(cui_background_draw_v2,
    const cui_render_v2_options *, cui_rect clip,
    cui_background_prepared_query *out);
cui_error cui_background_prepare_raster(cui_background_draw_v2,
    const cui_render_v2_options *, cui_rect clip, unsigned char *rgba,
    size_t stride, size_t capacity, cui_background_prepared_query *out);
```

Exact options size and SRGBA8_CENTER_V1 output tag are required. Positive finite
scales are [1/65536,64], origins [-1e6,1e6], physical viewport sides <=16384.
Draw origin follows the sampler's [-1e6,1e6] domain, width/height are [0,1e6].
A finite nonnegative logical clip may extend beyond that domain; it only culls
work. Radius values must be finite and independently clamp to [0,half min(w,h)].
Ancestor opacity is finite [0,1]. Invalid active brush values reject even offscreen.
Inactive value fields retain the pure sampler's ignore rules.

Map original bounds and clip to physical coordinates with double arithmetic,
intersect viewport, then floor left/top and ceil right/bottom. Return the integer
cover and its inverse logical rectangle. Never replace the gradient frame with
this covering/cropped rectangle. Inverse-transform target pixel centers; convert
sampler coordinates to float once. Validate the covering rectangle's first/last
center extrema against [-1e6,1e6] before any write. This conservative check also
rejects out-of-domain covering corners later excluded by the shape mask. Original
box edges may extend outside the domain when all covered centers remain valid.
No semantic fill is silently cropped to hide domain failure.

The CENTER_V1 mask tests centers against the original half-open box and independent
logical corner circles; unequal density gives physical ellipses. Outside cells
become transparent black. Active clip alpha is **not** baked into the raster;
future consumers apply their existing ancestor clip once. No analytic AA/source
GPU edge parity or linear-light destination compositing is claimed. Upload at the
integer physical cover, without generic IMAGE logical-ceil reraster, stretching,
second radius mask or second opacity. Adapter implementations still need review.

Shared output conversion takes the sampler's premultiplied linear float RGB/base
alpha, unpremultiplies by base alpha, clips straight RGB, encodes sRGB and rounds
half upward to bytes. Final alpha is baseAlpha*ancestorOpacity rounded once.
RGB remains independent of nonzero ancestor opacity, avoiding a multiply/divide
float roundtrip. Exact zero final alpha becomes transparent black; RGB may remain
nonzero when a positive alpha merely quantizes to zero, preserving sampler policy.
Existing sampler bytes are checked against independently frozen pre-refactor code.

Query reports packed minimum stride=width*4 and bytes=stride*height. Raster returns
**actual supplied stride** and validated CPU span=(height-1)*stride+width*4, with
identical geometry. It does not write any row padding, including last-row padding.
The query/result layout is not a wire format and padding has no defined bytes.
Empty clip/intersection, zero draw dimension or zero viewport dimension succeeds
with every result field canonical zero; raster permits NULL pixels/capacity0 and
writes no buffer bytes. Options/brush/alias validation still applies to empty work.

All options budgets are explicit nonzero ceilings: <=4M pixels/background,
<=8M pixels/frame, <=32MiB each CPU/texture/transfer and <=1MiB metadata. Query
checks packed bytes against all three storage envelopes and one nonempty query
record against metadata. Raster additionally checks actual padded CPU span against
CPU budget and capacity, including overflow. This is per-background accounting;
future adapters must separately preflight whole-list aggregate, draw expansion,
real transfer alignment and in-flight lifetime before clearing/submitting targets.
No total process/driver-memory bound or GPU recovery guarantee follows from it.

Options, query output and the whole supplied pixel capacity must be disjoint.
Detectable overlap/address-range overflow rejects; arbitrary pointer validity and
allocated object extent remain the caller's C contract. Owned draw and clip values
are copied. No query output or buffer byte changes on failure. Query/raster return
INVALID_ACTION for malformed policy/value/numeric/alias input, CAPACITY for work,
byte/stride/span/resource ceilings. Raster recomputes its complete plan and validates
all sample-domain and output preconditions before the first write; no data-dependent
failure follows it. Retry requires no context reset, rollback or replay.

Tests compare prepared pixels against frozen old sampler math plus an independent
long-double corner-distance/color reference, not candidate-generated expectations.
They cover source/CSS, sRGB/Oklab, transparent stops, one-step opacity, unequal and
fractional density/phase, corners, offscreen large extents, conservative domain
rejection, empty work, exact quota boundaries, padded buffers, alias/error atomicity
and retry. Existing sampler byte goldens and V1 rendering remain separate oracles.

The next software building block, `backends/software/cui_software_v2_internal.h`,
validates full V2 list structure and active borrowed resources, clip nesting and
aggregate known workspace reservations before any target access. Its counted
transaction arena is optional and has no V1 call site. A successful private
validation plan is NOT a prepared/renderable frame: explicit adapter text/shadow resource
preparation remains pending, as do transform painters and every public adapter
support query/render entry. See [software checkpoint](design/SOFTWARE_BACKGROUND_V2.md).

A private [counted text recorder](TEXT_PREPARATION.md) supplies source-grid glyph
coverage and explicit CUI ownership, but is not integrated into an adapter. Opaque
new font/dependency temporaries remain outside the transaction byte envelope.
