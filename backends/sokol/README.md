# Independent Sokol GLCORE renderer

This adapter consumes CUI draw lists directly through pinned `sokol_gfx.h`.
It does not call SDL_Renderer or SDL_GPU, does not include SDL, and does not use
`sokol_app`. The separate example uses SDL3 solely to own a desktop OpenGL host.

Create a desktop OpenGL 3.3 core context and make it current before calling
`cui_sokol_create`. Keep that same context current on the creating thread for
all adapter operations, including destruction. The host owns the window, GL
context, framebuffer and font. An RGBA8, single-sample framebuffer is required;
the pipeline does not use depth or stencil attachments.

```c
cui_sokol *renderer = cui_sokol_create((cui_sokol_config){
    .font = font,
    .target = {.framebuffer = 0, .width = pixel_width, .height = pixel_height}
});
cui_renderer backend = cui_sokol_interface(renderer);
backend.render(backend.user, &draw_list, clear);
/* The host presents its window after rendering. */
```

The target dimensions are physical pixels, while draw-list dimensions are
logical pixels. Update the target after resize or DPI changes. Nested clipping
intersects in logical coordinates and becomes a physical scissor. Command order
is preserved. The adapter clears and owns one complete pass per render; it is
not a function to call inside an already-active Sokol pass.

By default, the adapter initializes Sokol and commits each successful frame.
Only one owner may initialize the process-wide Sokol runtime. Other adapters
may set `borrow_runtime=true` to share an existing GLCORE Sokol runtime; in that
mode the host performs `sg_commit()` after all its rendering. Destroy borrowers
before the owner. Host-owned Sokol must stay alive and provide compatible
resource pool capacity. If the host already compiles Sokol's implementation,
compile this adapter with `CUI_SOKOL_EXTERNAL_RUNTIME` to avoid duplicate symbols.
The adapter changes GL state; host GL code must restore its state afterwards.
It resets Sokol's state cache before its pass to accommodate preceding host GL
calls. It does not own or destroy external framebuffer objects.
Each adapter renders at most once per Sokol frame; rendering it again before
the host commits returns `false`. Borrowing requires the exact pinned Sokol API.

Solid and rounded fills, rounded borders, rectangular clipping, and source-alpha
blending are implemented. Rounded coverage uses a signed-distance fragment
shader with derivative antialiasing. Ordinary sharp rectangles retain full
coverage even when only one pixel wide. RGB blending uses straight source colors
with SRC_ALPHA/ONE_MINUS_SRC_ALPHA, while alpha uses ONE/ONE_MINUS_SRC_ALPHA.
Transparent framebuffer readback follows that GPU blend convention; it is not
the software adapter's straight-alpha image export.

Optional `CUI_HAS_TEXT=1` enables the caller-owned FreeType/HarfBuzz font. Shaped
glyph coverage uses immutable GPU textures and a bounded LRU cache: at most 256
entries, 4 MiB CPU coverage, and 16 MiB RGBA GPU glyph storage, plus bounded
creation scratch. Glyphs already referenced by the current frame cannot be
evicted. A frame exceeding the cache's capacity fails explicitly rather than
destroying a texture still needed by that frame. Warm-cache glyphs allocate no
new textures and tint/alpha changes reuse coverage. Without a usable font, text
commands fail and the text capability is absent.

CPU frame staging and the GPU vertex buffer are bounded by `max_quads` (default
8192, maximum 100000). Malformed draw commands, unbalanced clipping, excessive
geometry or glyph capacity are rejected before a render pass starts. Shader and
resource creation states are checked. Underlying Sokol fatal allocation or GL
assertion failures are not a device-loss recovery path. Bitmap image commands
use the existing bounded RGBA cache. Gradient mesh paint, shadows, general
transforms and non-GL Sokol backends remain unsupported.

The unmodified upstream pin and license are in `third_party/sokol/PIN.md`.
Build this source with `include/`, a SYSTEM include for `third_party/sokol`, and
the platform desktop GL library plus math. Text additionally links `cui_text`.
The adapter library has no SDL dependency. The demo links core/components,
SDL3, GL and optionally shaped text:

```sh
cui_sokol_demo --smoke 3 --screenshot sokol.ppm
```

Verified on Linux Xorg with GL vendor Intel and renderer
`Mesa Intel(R) Graphics (ARL)`. FBO readback tests cover blend/order, nested clip,
logical/physical scaling, rounded fill/border, glyph coverage, persistent-cache
reuse/eviction, bounded frame failure and borrowed-runtime ownership. The same
geometric adapter compiles and runs without FreeType/HarfBuzz. Other operating
systems, Wayland, GLES, Metal, Vulkan, D3D11 and WebGPU are unverified or not
implemented by this adapter. Software OpenGL drivers may run the same GLCORE
API; the API name alone does not prove hardware execution.

Native mesh commands upload borrowed XY/ST triangles to the bounded vertex stream.
A separate premultiplied pipeline computes the pinned quadratic source coverage
from screen-space derivatives; meshes do not use the bitmap cache. The existing
RGBA8 target stores byte-domain sRGB-like values. Mesh linear premultiplied paint
is unpremultiplied, sRGB encoded, then premultiplied by alpha and coverage before
blending. This preserves primitive appearance but does not claim linear-light
framebuffer compositing. Mesh capacity consumes one quad slot per six vertices
(or one final three-vertex slot); overflow rejects the frame before its GPU pass.

The coverage equation follows `gpui-pre-wgpu 0.3.8`, pinned by the frozen GPUI
Kit lockfile: `src/shaders.wgsl::fs_path_rasterization`. The solid fast path tests
the norm of both S derivatives against0.001; this matters for vertical curves.
Real Intel/Mesa GL readback tests check numeric coverage in both orientations,
encoded half-alpha overlaps, mixed primitive/mesh ordering, translated DPI
clips, borrowed vertices, capacity failure and unknown-command rejection.
