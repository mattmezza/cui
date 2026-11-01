# SDL V2 baseline and API review checkpoint

Source-only followup to `SDL_RENDERER_V2.md`, based on signed `b386ec55`.
No SDL production/header change, compile, output capture or new support claim is
included. The owned CPU frame remains subject to root integration acceptance.

## Frozen original and actual dependencies

`tests/fixtures/sdl_v1_reference` freezes the exact original CUI SDL implementation,
mesh helper and public header (about 65 KiB), independently pinned by Python hash
constants and receipt. Original source blob is
`aeb9b7d43098605e12c8435f15d34b690ff1551d`. Pure bitmap/shape/mesh/shadow dependencies
and generic renderer header are hash-pinned without duplicate copies. The frozen
source will be compiled in its own prefixed TU; it must never be reconstructed
from the future candidate. Existing archive hash is recorded read-only.

Installed header/pkg-config/runtime and cached native CMake agree on SDL 3.4.18;
`SDL_GetVersion` returned 3004018 and revision `SDL-release-3.4.18-0-g829a65d76`.
Those version-only calls initialize no video or windows. Repository fallback is
commit `7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`; system SDL >=3.2 is also accepted.
Only two needed upstream renderer files were fetched into the existing builddir,
with URLs/hashes in the receipt; no checkout, installation or library rebuild.
Installed library identity does not prove absence of vendor patches.

Both audited revisions keep Unlock void and can hide nested conversion-update
failure. Both Present implementations ignore implicit-flush/backend failure before
returning success. Explicit Flush reports its command-queue error. Therefore keep
native advertised RGBA32 admission and checked Update/Flush, and describe success
as submission checked and presentation attempted, never physical display success.
Returned public texture format alone cannot rule out SDL conversion wrappers;
advertised native support selects the direct texture-creation branch.
Primary source: [fallback renderer](https://github.com/libsdl-org/SDL/blob/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540/src/render/SDL_render.c)
UpdateNative2042, Unlock2484, Flush347, Present5040, native creation1422;
[installed-release renderer](https://github.com/libsdl-org/SDL/blob/release-3.4.18/src/render/SDL_render.c)
UpdateNative2291, Unlock2753, Flush375, Present5517. These exact downloaded-source
line numbers supersede the earlier checkpoint's inaccurate line references.

## Existing V1 behavior to preserve

The generic function table is unchanged: user/name/capabilities/render/destroy.
Public cache stats, hidden-window options, callback-app records and all25 function
signatures are frozen in the C11 compile-only `sdl_v1_api.c` fixture. Its assertions
use explicit old records/offsets and function-pointer types, never struct padding.
No platform/SDL types appear in those public records.

Constructor performs one CUI calloc for the opaque object, creates a resizable,
high-density window (hidden at creation when requested), then an SDL renderer.
V1 glyph cache: 256 entries, 4 MiB coverage, coverage bytes/hash/w/h key with exact
byte collision check, LRU hit/miss counters; color modulation does not rekey.
Gradient glyphs use their existing uncached surface/texture path. Bitmap cache:
64 slots/64 MiB RGBA; shadows additionally use the unchanged 32 MiB uploaded
subbudget plus the pure CPU shadow cache. Mesh tickets cap 256 triangles/8M texels.
Keep exact cache/request/eviction/revision behavior on the V1 route.
V1 is not generally transactional: some invalid non-mesh commands are discovered
after clear/earlier draws, and density/cache state can already change. Record and
compare those original failure effects; do not impose V2 atomicity on V1.

`size()` mutates logical presentation to STRETCH using current window size.
`render_density()` re-queries logical presentation rectangle and renderer scale
on every render, computes independent densities <=16 and max-axis text density.
Do not replace it or borrow it for V2's <=64 independent transform. V1 shapes use
device row spans, images ceil device dimensions and min-axis radius density,
shadows retain their old raster/rounding path, meshes retain their physical view.
Original clip rounding and all painter order stay exact. V2 must not populate,
evict or reset these caches, call size(), or overwrite these density fields.

## Baseline proof preparation, not yet executed

Compile original and candidate in distinct TUs with separate opaque types/backends
and independent identical real-font/fallback graphs. Capture readback before
Present, normalize actual surface format to owned RGBA rows, compare complete
pixels and active state/flags; never hash-only/padding/pointer-address equality.
Capture cold/warm/revised IMAGE and glyphs, contextual/gradient text, drop/inset,
mesh, asymmetric shape/clip and rejected late input, at logical STRETCH, disabled
presentation, unequal renderer scales, nonzero viewport/clip and simulated 2x/3x
output targets. Preserve actual target state; modern SDL refuses Present on a
texture target, so use original pre-Present capture instrumentation for that
Retina simulation rather than weakening a failed render assertion.

Count CUI-owned calloc/malloc/free requests separately from opaque SDL/font work;
trace native texture create/update/destroy and submission calls without treating
SDL allocator traffic as CUI bytes. Freeze constructor/object layout plus cache
requests, counts, lifecycle failures, cached entries/bytes/hits/misses and exact
V1 pixels. Real monitor migration/Retina hardware remains deferred. All native
execution uses hidden-at-creation private-Xvfb wrapper, never the desktop.

`layout_source()` prepares compact compile-only constants from the exact frozen
private struct prefix. Candidate only appends `bool v2_busy, v2_faulted`; no field
is inserted or reordered. Measure native size/alignment/all old offsets and guard
offsets first; wasm32 is a pointer-width diagnostic, not SDL/WASI support. No
opaque-layout growth or constructor-byte exception is yet approved: report actual
old/new before choosing. No new retained texture/frame/cache ownership is proposed.

## Exact public API proposal and ordering decision

Retain the previously proposed `cui_sdl3_v2_info` (size/draw/text_features/defaults),
SDL-specific A8/CONTEXT/TRACKING bits and typed `cui_error` query/render signatures.
Exact query out->size; errors preserve output. NULL render options use fresh query
defaults. Same default CPU 32 MiB/metadata 1 MiB and common work ceilings as software;
command ceiling uses physical P, command stride and descriptor size. Native
texture/transfer admission independently requires P within its ceiling. One CPU
image, not 2P: 3840x2160 P=33,177,600 fits 32 MiB before commands/resources. CPU
background planes are not extra native uploads; common producer limits still
apply. Opaque SDL/driver/font allocations remain separate class 3, not jointly bounded.

Query current owned window/output, reject non-NULL native render targets, unknown
or unsupported format/colorspace/size, and refuse busy/faulted backend. Temporary
guard protects SDL property/size queries that may invoke allocator hooks; restore
it before pure frame preflight. Fresh framebuffer ratios provide defaults, not
V1 cached density. Requery after resize; no stale-dimension reservation.

**Review needed before production:** NULL defaults require native queries before
full common geometry preflight, but active source aliases must reject before
changing owner busy. Checking only list/options/command arrays misses borrowed
image/mesh/text/span/stop ranges. Do not silently mutate aliased input bytes during
the query guard, duplicate the resource parser, or cache old dimensions to evade it.
Recommend a small pure common active-source-range preflight, reviewed separately:
`cui_error cui_cpu_v2_sources_exclude(list, ranges, count)` with bounded command/
span/stop/vertex counts and checked byte extents, no pointed-data/geometry/font/
SDL calls. It rejects known owner/control ranges before native callbacks. The
full validator must reuse that same field-selection authority, retaining actual
geometry/UTF8/resource validation and error/budget tests. Whole opaque SDL/font
storage remains explicitly caller-disjoint/stable, not falsely fully validated.
No common code change is included here; root owns that approval.

After preparation, snapshot logical/scale/viewport/clip/color-scale state; disable
logical presentation, use scale 1/full viewport/no clip/color-scale 1, upload tight
RGBA32, NEAREST, white texture modulation and BLEND_NONE. A single full-output
copy already carries clear pixels; native clear is unnecessary. Check draw/flush,
attempt Present, then restore every changed property even after failure. Failed
restoration poisons future render/query; destroy stays available when not busy.
No rollback after native drawing. Release native texture then frame while busy;
ordinary SDL destruction is not synchronous driver-memory reclamation. Lifecycle
and typed-error details need final API review before implementation. All other
three adapters after SDL, bindings and remaining 78-part coverage stay pending.
