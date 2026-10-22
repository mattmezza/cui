# SDL3 analytic mesh preparation checkpoint

Design only; no renderer implementation or runtime mesh/readback proof is included.
Audited base: `939c5c2faa7d24c4c8cd2b2fad84cc4284f5a7e0` (cached
`origin/main`). The DateTime gallery worker branch is preserved separately.

## Existing contract and gap

`include/cui/cui.h:473–497` and `src/core/cui.c:1991–2018` define borrowed
triangle vertices, finite XY/ST, positive scalar placement, linear premultiplied
color and alpha in [0,1]. Degenerate triangles are valid empty geometry. Core
validation imposes no renderer allocation budget. Vertices must remain alive
through render completion; a renderer must not retain their addresses as a cache
identity after that call.

`backends/software/cui_software.c:73–151` is the coverage authority. It transforms
XY in double precision, normalizes winding, applies pixel-center top-left edge
ownership, interpolates ST, and evaluates `S*S-T`. Coverage uses its screen-space
gradient; the solid-S threshold is 0.001 and zero gradient has explicit inside,
outside and half-coverage outcomes. Color conversion unpremultiplies linear RGB,
clamps SDR channels, applies sRGB encoding, and rounds to bytes. Coverage scales
alpha. Each triangle blends into straight RGBA8 in input order, including rounding
after each blend (`blend`, immediately above the mesh code).

SDL_GPU and Sokol have mesh paths; this does not establish SDL3 support.
`backends/sdl3/cui_sdl3.c:782–898` clears before command dispatch and has no mesh
case: a valid mesh currently fails through `default` after clearing, potentially
after earlier commands. SDL_RenderGeometry exposes vertex color/texture
interpolation, not the required programmable analytic ST coverage. Plain geometry
submission therefore cannot implement this contract.

The SDL shadow path is a useful preparation precedent, not a mesh cache ready
for reuse. `backends/software/cui_shadow.h` owns copied commands/pixels, bounds
cache capacity and pins current-frame entries. Its transform accepts only nearly
uniform positive scale (four float epsilons). SDL bitmap preparation currently
runs before clear only when shadows are present. Glyph/resource failures and
several command checks can still occur after clear; existing rendering is not a
transactional frame API.

## Proposed first supported slice

Extract a **private CPU coverage helper**, with no SDL dependency or public ABI,
from the software mesh path. Preserve software arithmetic, top-left and color
rules bit for bit before using it from SDL. The helper accepts device-space XY,
unchanged ST, integer clipped raster bounds and a destination supplied by the
caller. Keep software's ordered destination blending separate from texture
preparation.

SDL owns a frame preparation object. Prepare one transparent RGBA8 raster per
visible triangle, then draw those textures in original command/triangle order.
Do not flatten overlapping triangles into one command texture: an extra
intermediate RGBA8 blend can change ordered quantization. No retained vertex
pointer or cross-frame mesh cache is needed initially. Repeated mutable payloads
at the same address must produce fresh content.

Proposed admission limits, to be confirmed against representative component
meshes before production: 256 triangles total, at most 16384 pixels per raster
axis, and 8,388,608 total clipped raster texels (32 MiB RGBA payload). Count and
multiply with checked arithmetic before allocating. Empty/degenerate triangles
consume no texture. These are adapter capacity limits, not changes to
`cui_mesh_valid`. CPU scratch is reused one triangle at a time; uploaded frame
textures remain owned until submission/present completes. Release previous-frame
resources before preparing another frame. SDL/driver internal allocations are
outside this application-owned byte bound.

Surface wrappers borrow scratch only during synchronous texture creation; destroy
the wrapper before reusing scratch. On any preparation failure, destroy all new
textures and leave the target uncleared. Destruction handles partial preparation.
An execution/device failure after clear cannot be rolled back: return false,
avoid presenting, release frame resources and allow a later valid frame to retry.
No hard-real-time claim follows from bounded memory.

### Color and target restriction requiring review

Use straight sRGB texture RGB with coverage in alpha and SDL_BLENDMODE_BLEND.
The first slice should accept mesh frames only with **opaque clear alpha** and
an ordinary SDR render target. SDL's documented BLEND equation does not preserve
the software renderer's straight RGB storage on a transparent destination.
Premultiplied blending alone does not resolve that representation difference.
Reject unsupported transparent-clear mesh frames before clear rather than claim
RGBA equivalence. Existing commands can themselves reduce destination alpha only
if their blend mode permits it; validate the opaque-destination invariant for the
whole supported command set. RGB rounding can still differ between SDL renderers;
measure and bound it in differential tests rather than advertise byte identity.
Transparent-target support needs a separate representation/compositing decision.

### Transform and clip restriction requiring review

Snapshot SDL logical presentation, its physical rectangle, render scale,
viewport, target dimensions and clip state on the rendering/main thread.
The existing shadow path computes presentation ratios and adds viewport offset
scaled by those ratios and render scale. Verify that mapping with actual SDL
readback; do not treat the formula as proof. First support positive uniform
effective scales (including DPI 1 and 2), finite translation, and a documented
ordinary window target. Reject nonuniform scales, unsupported targets or mappings
before clear; this is an adapter limitation, not a Core mesh limitation.

Transform XY before deriving ST gradients: coverage is sampled at physical pixel
centers. Bake CUI ancestor clips and effective viewport/presentation/target bounds
into each raster using an explicitly tested edge-rounding rule. Draw prepared
rasters at integer physical positions with logical presentation disabled, scale
one and viewport/clip reset. Restore every saved SDL property independently on
success and failure; restoration failure itself fails the render. Do not alter
legacy non-mesh transform behavior in this slice. Nonuniform mesh support can
follow using axis-specific derivatives after separate differential proof.

### Before-clear boundary

Walk the whole mesh-bearing list before clear: validate all mesh payloads,
command kinds/bounds, clip balance/depth, transform support, capacities and
required mesh texture creation/configuration. Preflight image/shadow resources
in such frames regardless of shadow presence; reject text without a font early.
Mesh payloads must not be concurrently mutated during preparation/rendering.
A malformed final mesh or failed final mesh upload must not clear/draw earlier
commands. This guarantee covers validation and prepared resources, not arbitrary
late SDL draw failures or unprepared glyph allocation. Extending it to every
glyph requires an explicit additional preparation phase, not a false atomicity
claim.

## Independently reviewable implementation and proof

1. Extract private raster math and prove software output unchanged with solid,
   analytic, winding, degenerate, shared-edge and overlapping-alpha vectors.
2. Implement the bounded SDL preparation path and private test access needed for
   SDL state setup/readback/fault injection. No new public renderer ABI or Core
   dependency is necessary. Keep component mesh producers unchanged.
3. Run actual SDL readback under `scripts/test_local.py` (private Xvfb), against
   the software helper/reference. Cover DPI 1/2, fractional placement, viewport
   offsets, logical letterbox/integer presentation, nested clips, offscreen
   bounds and supported uniform scale. Assert explicit pre-clear rejection for
   nonuniform and transparent-target cases. Test RGB threshold/out-of-gamut,
   alpha 0/0.5/1, ST gradient zero, shared edges and multiple overlapping layers.
   Compare RGBA after normalizing readback format; opaque output alpha must be
   255. Establish RGB rounding tolerance from specific blend vectors, with tight
   per-vector bounds; do not hide edge/coverage errors behind a broad tolerance.
4. Preserve a sentinel target for malformed-last-command, clip errors, triangle/
   byte overflow, allocation/texture/upload/configuration failure. Inject failure
   at each prepared resource, check cleanup and next-frame recovery. Exercise
   state restoration, resize, same-pointer payload mutation, repeated render and
   destroy-after-partial-preparation under strict C11 warnings and ASan/UBSan.
   Tests should inspect readback before Present where needed; a failed frame
   must not accidentally pass by reading a previous presented capture.

Existing `tests/unit/test_sdl3_rounded_geometry.c` demonstrates real SDL capture
and linker-wrapped failure/submission checks; shadow adapter fixtures provide
further transform patterns. A private fixture may expose renderer access without
adding public API. Software-renderer SDL proof does not establish hardware GPU,
macOS or Windows driver behavior. Those remain separate gates.

## Dependency and evidence limits

No dependency was added, fetched or installed. CMake accepts system SDL3 >=3.2
or falls back to pinned SDL revision
`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`. Locally installed SDL headers report
3.4.18; they expose geometry, presentation/viewport/scale, readback and blend
contracts, but are not runtime evidence for the fallback revision. Implementation
must compile against the supported pinned fallback as well as the selected CI
system version. This checkpoint is source inspection only: no native window,
readback fixture, build or renderer capability promotion was performed.
