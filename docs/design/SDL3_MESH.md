# SDL3 analytic mesh: current adapter contract

Design for review, based on signed extraction `d4ca8c15`; no SDL mesh implementation
or SDL mesh readback proof exists yet. This document is the current proposal,
replacing earlier opaque-only/uniform-only restrictions.

## Existing behavior and source authority

Core (`include/cui/cui.h`, mesh declarations; `src/core/cui.c`, `cui_mesh_valid`)
provides borrowed triangle vertices, finite XY/ST, positive scalar placement,
linear premultiplied color and alpha in [0,1]. Degenerate triangles are empty.
Borrowed vertices live through rendering; no renderer may retain a pointer as
cross-frame content identity. Core validity is separate from adapter capacity.

`backends/software/cui_mesh.h` now owns private CPU traversal/color conversion;
software retains its original ordered RGBA8 destination blending. Coverage uses
double XY, normalized winding, pixel-center top-left edges, interpolated ST and
`S*S-T` with screen-space derivatives, the existing 0.001 solid-S threshold and
explicit zero-gradient outcomes. Color unpremultiplies linear RGB, clamps SDR,
encodes sRGB and rounds; coverage scales alpha. SDL_RenderGeometry alone cannot
evaluate that analytic fragment coverage.

SDL currently has no mesh switch case (`backends/sdl3/cui_sdl3.c`,
`cui_sdl3_render_capture`); rejection occurs after clear. Software/SDL_GPU/Sokol
support does not establish SDL3 support. Existing shadow preparation owns copied
commands/pixels and pins resources before clear, but its uniform-scale condition
is a shadow limitation, not a mesh requirement.

## Actual host surface and target contract

`include/cui/cui_sdl3.h` exposes an opaque backend, create/destroy, size, render,
render_capture and BMP capture. All operations run on the main thread. There is
no borrowed renderer constructor, raw SDL renderer getter or public target,
viewport, clip, scale or presentation setter. `cui_sdl3_create` creates an owned
resizable high-pixel-density window and default SDL renderer; it selects standard
SDL_BLENDMODE_BLEND. `cui_sdl3_size` queries logical window size and selects
SDL_LOGICAL_PRESENTATION_STRETCH at that size. No production path sets a texture
render target. Thus the supported public path is the owned window target, whose
physical resolution/presentation can change on resize/display migration; it is
not a user-configurable offscreen-renderer API. Do not add such an API here.

Mesh supports the same host target/compositing behavior as existing images and
shapes, including transparent clear values. Use straight sRGB texture RGB and
coverage alpha with SDL_BLENDMODE_BLEND. Do not require clear alpha 255. SDL's
encoded target storage/compositing can differ from software's straight-RGBA8
storage on transparent destinations: compare visible composites or an independent
SDL blend reference, not an invalid raw-storage equality assertion. Preserve
alpha and command/triangle order; do not normalize away a genuine blend error.
Linear mesh color conversion remains the existing SDR conversion, not new HDR
or linear-light target blending support.

For robustness and tests, a private fixture can configure viewport/presentation/
scale on the owned renderer. Do not reject positive nonuniform mappings: stretch
and scale are axis-specific. Respect existing render_density admission (finite,
positive effective axis scales <=16), target/resource size bounds and real query
failures. Texture-target injection may test the preparation mechanism privately;
it does not create a public offscreen support promise. No new target-kind
rejection should be introduced without a concrete unsupported operation.

## Coordinate mapping and restoration

The cached archive `/tmp/cui-deps-SDL.tar.gz` names pinned SDL revision
`7f3ae3d57459e59943a4ecfefc8f6277ec6bf540`. Its `src/render/SDL_render.c` functions
`UpdatePixelViewport`, `UpdatePixelClipRect` and `UpdateLogicalPresentation`
establish the mapping, rather than the shadow approximation:

- Effective axis scales are SDL render scale times presentation rectangle size
  divided by logical size (or render scale alone when presentation is disabled).
- Physical viewport origin is **floor(viewport origin * effective scale + logical
  presentation offset)** independently on X/Y; explicit viewport extents use
  ceil(extent * effective scale). Default viewport uses the active pixel extent.
- SDL clip is relative to that viewport; its physical origin uses floor(clip
  origin * scale), and its extent uses ceil(clip extent * scale), not a substituted
  ceil(right)-floor(left) rule. Preserve the current adapter's logical clip-stack
  rounding before applying this mapping.

Snapshot presentation dimensions/mode/rectangle, axis scales, viewport and its
explicit-set flag, active target and physical target extent. Query the window's
true output size for the public window target; SDL_GetCurrentRenderOutputSize
is presentation-adjusted and alone is not the full physical target extent.
Bake device clipping against target/viewport and the current CUI clip stack.
The adapter currently clears incoming SDL clip state at frame start; preserve
that existing contract, not a newly invented external-clip intersection.

Project XY into physical coordinates in double precision **before** calculating
S/T derivatives. Keep ST unchanged. Add a private axis-transform entry point or
internal projection seam to the shared helper; keep the existing software scalar
path/arithmetic unchanged. Do not round transformed mesh vertices through an
intermediate float payload. Anisotropic scale changes both derivatives and
coverage; scaling a uniform-resolution coverage bitmap is insufficient.

Draw prepared rasters in integer physical coordinates with presentation disabled,
scale one, viewport/clip reset after clipping is baked. Restore presentation,
scale, viewport-set state and the command-stream clip after each mesh draw so
following commands observe their original state. Restore every property even if
another restoration fails. Do not change target ownership or leave a different
active target. Query/set failures return false. Verify the pinned mapping against
actual SDL readback, including fractional presentation offsets, before claiming
it implemented; source arithmetic is evidence for a testable design, not proof
of every renderer driver's rasterization.

## Bounded preparation and failure boundary

Prepare one transparent raster per visible triangle and submit in original order;
flattening an overlapping command can change intermediate quantization. Initially
no cross-frame mesh cache: copied RGBA and owned textures remove borrowed-payload
lifetime hazards. Proposed fixed limits: 256 triangles/frame, 16384 pixels/axis,
8,388,608 total clipped raster texels (32 MiB uploaded RGBA payload). Confirm these
limits against actual component producers before implementation. Check counts,
coordinates and byte arithmetic before allocation/casts. Degenerate or fully
clipped triangles need no texture. CPU scratch is reused one triangle at a time;
SDL surface wrappers borrow it only until synchronous texture creation completes.
Free wrappers before scratch reuse. Retain textures through draw/present completion
and release previous frame resources before preparing another. Application-owned
budgets exclude SDL/driver internal staging; no hard-real-time claim.

Before clear, validate the entire mesh-bearing list's payloads, supported command
kinds/bounds and balanced clip stack, snapshot mapping, enforce budgets and create/
configure all mesh resources. Prepare images/shadows in such frames regardless
of shadow presence; frame-pin them against eviction during preparation. An invalid
last mesh or failed last mesh upload must not clear earlier contents. Failure
cleans all partial mesh resources; next valid frame retries. Do not claim whole
frame transactionality: late SDL draw failures or unprepared glyph allocation
can still fail after clear. Return false without presenting on such failure,
restore state, clean resources and permit recovery. Extending resource preflight
to all glyphs requires a separately reviewed phase.

## Implementation gates and meaningful proof

1. Completed extraction: frozen independent pre-change raster/color/blend oracle
   in `tests/unit/mesh_oracle.h`; 171 exact full-buffer comparisons plus known
   gradient-zero/shared-edge/overlap assertions. Strict Release and Debug
   ASan/UBSan each pass new `mesh_cpu` plus existing `render` (2/2) through the
   native wrapper. This is software proof only.
2. Next adapter slice, after review: bounded preparation plus private axis
   projection seam, no public ABI/Core/component changes. Add private test access
   and deterministic allocator/upload/configuration failure injection.
3. Actual SDL readback through `scripts/test_local.py` private Xvfb: DPI 1/2,
   positive nonuniform scale, stretch/letterbox/integer/overscan presentation,
   fractional offsets, explicit/default viewport, nested clips, resize and
   transparent/opaque clears. Compare analytic coverage/top-left ownership to
   software at device coordinates. Compare SDL compositing to independent SDL
   texture/point reference or visible composites when target storage differs;
   establish tight per-vector rounding bounds, not broad image tolerance.
   Include winding/degenerate/zero-gradient/overlap, alpha 0/0.5/1, sRGB threshold
   and out-of-gamut clamp, and mixed mesh/image/text/shape command ordering.
4. Sentinel readback before Present for malformed-last-payload, clip imbalance,
   capacities and each injected resource failure; target unchanged on preparation
   failure. Check partial cleanup, same-pointer mutation, destroy after failure,
   state restoration on every exit and next-frame recovery. Strict warnings and
   ASan/UBSan; compare owned resource counters with zero outstanding frame leases.
   Hardware GPU and non-Linux driver proof remain separate gates.

Reproduce extraction only with `cmake -S tests/fixtures/mesh_cpu -B build-mesh -G
Ninja`, build targets `test_mesh_cpu test_render`, then
`python scripts/test_local.py --build-dir build-mesh -R '^(mesh_cpu|render)$'
--output-on-failure`. Use a separate Debug build with `-DCUI_SANITIZE=ON`.
Existing rounded-geometry and shadow adapter fixtures demonstrate actual SDL
capture/fault-injection patterns, not mesh correctness.

No dependency was fetched or installed. CMake accepts system SDL >=3.2 or its
pinned fallback above; local system headers are SDL 3.4.18. Check the pinned
fallback and selected CI SDL version during production validation. No gallery,
Core, public capability or generated consumer guidance is changed by this design.
