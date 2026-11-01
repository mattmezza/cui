# Unreleased SDL_Renderer V2 route

Use matching development headers and implementation. The C SDL_Renderer adapter
now has explicit `cui_sdl3_query_v2` and `cui_sdl3_render_v2` entry points in
`cui/cui_sdl3.h`. The V1 renderer table and default render path stay unchanged.
This is distinct from SDL_GPU; it does not add V2 foreign-language authoring,
Canvas, Sokol or SDL_GPU support, or establish complete component styling parity.

## Opt in

This fragment belongs inside an existing initialized CUI/SDL host frame loop;
`backend`, `ui`, dimensions, `dt` and `clear` come from that host. Keep the same
font provider for measurement and rendering. See [adapter setup](../backends/sdl3/README.md)
and [Core V2](CORE_DRAW_V2.md) for the surrounding contracts.

```c
cui_sdl3_v2_info info = {.size = sizeof(info)};
if (cui_sdl3_query_v2(backend, &info) != CUI_OK) return false;
cui_frame_v2_options options = {
    .size = sizeof(options), .support = info.draw,
    .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS
};
if (!cui_begin_frame_v2(ui, logical_width, logical_height, dt, &options))
    return false;
/* Compose ordinary CUI elements; apply documented V2 backgrounds as needed. */
cui_draw_list_v2 list;
if (!cui_end_frame_v2(ui, &list)) return false;
return cui_sdl3_render_v2(backend, &list, clear, &info.defaults) == CUI_OK;
```

Query before composition and after resizing. Query is a snapshot, not a reservation;
render checks current dimensions again. NULL render options choose fresh defaults.
Explicit options require matching physical output dimensions and valid positive
budgets no larger than queried limits. Defaults derive independent X/Y density
from current logical/output dimensions. Never cast V2 commands to V1 or silently
fall back after failed V2 admission.

## Painting and transfer

The shared bounded CPU painter prepares the entire frame, including backgrounds,
shadows, images, meshes and supported text. See [painting policy](SOFTWARE_V2_PAINTING.md)
for alpha, clipping, AREA shadows, gradients, text recording and precision.
Composition and layout still run in full; this is not incremental repainting.

Only the owned window backbuffer with supported sRGB output qualifies. HDR,
texture targets and unsupported native formats are rejected. Prefer advertised
RGBA32; BGRA32 uses an exact red/blue byte swap in the existing owned plane after
painting. Green and alpha are preserved. No second CPU image or implicit SDL
format conversion is used. Transfer uses a transient native texture, nearest
sampling, white modulation, no blending, physical 1:1 drawing and checked flush.

Query reports actual font/A8, contextual-text and tracking capability bits.
Unsupported text/resource requests fail during preparation. It does not promise
color fonts, every Unicode script, physical presentation or native codecs.

## Ownership, limits and failure

Main thread only. Borrowed draw resources and the opaque font/provider graph must
remain valid, stable and disjoint from the owner through synchronous return.
Do not recursively render the same font. Owner APIs refuse while V2 is busy;
void size/destroy calls are no-ops during dispatch, so retry destruction when idle.

Counted transaction ceilings are 32 MiB CPU, 1 MiB metadata and 32 MiB each for
texture/transfer, with the shared command, pixel and work limits. Dimensions are
at most 16384 per side. A positive native maximum further restricts them; zero or
an absent property means no reported native limit, not guaranteed allocation.
Checked texture creation can still fail. Opaque SDL/driver/font allocations are
outside these quotas. Inspect queried limits and the public header; ceilings are
not simultaneous reservations and do not guarantee every 4K frame fits.

Preparation failure frees transaction resources before native drawing. Once native
submission starts, backbuffer effects cannot be rolled back. Logical presentation,
scale, viewport, clip and color scale are restored after attempts; restoration
failure faults the owner until idle destruction/recreation. Success means checked
submission and a successful Present call, not physical display completion.

## Evidence and remaining acceptance

Worker strict C11 compilation and two private-Xvfb tests pass, including actual SDL
software BGRA upload/readback, independent channel/alpha checks, failure injection,
reentry, restoration and immutable V1 compatibility. The root configured gate is
tracked separately in validation receipts. Original failures are retained.
Physical GPU, macOS/Windows runtime and broad installed-consumer acceptance remain
separate. See [the reviewed contract](design/SDL_V2_PUBLIC_ENTRY_CHECKPOINT.md)
and [worker receipt](validation/sdl-v2-first-worker.json).
