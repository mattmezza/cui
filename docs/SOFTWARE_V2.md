# Unreleased software V2 renderer

Development source only; use matching headers/implementation. The C software
adapter has an explicit V2 query/render route in `cui/cui_software.h`; the V1
`cui_renderer` table and `cui_software_render` route remain unchanged. SDL_Renderer,
SDL_GPU, Sokol GLCORE, Canvas wire and foreign authoring bindings remain pending.
This does not establish all 78 component part/presentation coverage.

Query the initialized target **before composition**, then use its actual support:

```c
cui_software_v2_info info = {.size = sizeof(info)};
if (cui_software_query_v2(renderer, &info) != CUI_OK) return false;
cui_frame_v2_options frame = {.size = sizeof(frame), .support = info.draw,
    .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS};
if (!cui_begin_frame_v2(ui, logical_width, logical_height, dt, &frame)) return false;
/* Submit ordinary boxes; cui_set_background/cui_element_set_background opt in. */
cui_draw_list_v2 list;
if (!cui_end_frame_v2(ui, &list)) return false;
return cui_software_render_v2(renderer, &list, clear, &info.defaults) == CUI_OK;
```

Do not cast V2 arrays to V1, fabricate consumer support, or fall back to V1 on an
error. Query is an owned snapshot, not a reservation/token. Requery after resize;
render independently checks current dimensions and limits. NULL render options
select current defaults; explicit options require exact size/CENTER_V1 tag,
matching target dimensions and positive ceilings no larger than queried ceilings.
Independent scales range 1/65536..64, origins -1e6..1e6, viewport sides <= 16384.
Default scale 1/origin 0; rendering transforms do not change logical input/layout.

All ordinary V1 command kinds and owned solid/two-stop backgrounds are consumed in
array order, with exact logical ancestor clips and straight RGBA8 source-over.
Backgrounds use SRGBA8_CENTER_V1 once; shadows integrate unchanged quantized source
profiles over the full physical pixel footprint (AREA), including partial support.
This is CUI's documented policy, not a CSS exactness claim. Text uses owned GRAY256
coverage at q = max(1, scale_x, scale_y), q <= 64, and per-axis bilinear inverse-center
sampling; exact integer isotropic phase preserves the native glyph grid. Gradients
sample logical ink coordinates and alpha is truncated once. See
[private painter details](SOFTWARE_V2_PAINTING.md) for bounds/precision semantics.
No intrinsic RGBA/color-font support or universal Unicode coverage is promised.

`info.text_features` reports actual A8/font availability; CONTEXT additionally
requires the compiled Bidi recorder; TRACKING requires letter-spacing capability.
No-text/NULL-font rejects TEXT before reading its borrowed bytes. No-Bidi rejects
contextual sources/spans before reading inactive raw text. Missing faces and
unsupported bitmap modes fail before publication. Public scaled font APIs retain
their separate <= 16 limit; the owned source recorder supports 64.

Limits are ceilings, not simultaneous reservations: transaction CPU 32 MiB,
metadata 1 MiB, texture/transfer 32 MiB each, aggregate background/shadow/glyph target
rectangles 8M, background 4M each, shadow cells 1M/source taps 1M. Query commands are
`min(16384, (CPU_limit - target_RGBA_bytes) / sizeof(cui_draw_command_v2),
metadata_limit / sizeof(private_paint_descriptor))`; backgrounds = min(4096, commands). A target
with no room for one command returns CAPACITY. Smaller explicit CPU/metadata
recompute the count cap; final resource plans may still reject. The target's
persistent pixels/font state are separate class 1; counted temporary payload/text
blocks are class 2; opaque new FT/HB/Bidi/provider temporaries are class 3, outside
these byte limits. Input/work ceilings do not bound opaque allocator bytes. Core
composition storage is separate (default 16 MiB command/512 KiB owner/background).

Query allocates nothing and leaves output unchanged on error. All active list,
resource and transform validation, exact plans, background/shadow preparation and
owned text recording precede scratch clear. Any failure leaves published target
pixels unchanged, frees transaction blocks and allows retry; provider/font side
effects cannot be rolled back. Known list/options/commands/text/context/span/stop/
image/mesh ranges are rejected when overlapping target pixels or renderer object.
The whole opaque font graph/dependency storage must remain valid, stable and
disjoint; that graph cannot be dynamically range-validated. All borrowed resources
remain stable until synchronous return; copying commands does not deep-copy them.

The constructor borrows the font and destroy never closes it. No concurrent or
recursive rendering of the same font, including on another target. Busy V2
query/render returns LIFECYCLE; V1 render/resize returns false; void destroy is a
no-op and must be retried after the active call. Read-only pixels/interface access
remains valid. There are no retained V2 caches, uploads or asynchronous resources.
The private guard grows the opaque renderer allocation by 8 bytes on measured
native/WASM32 targets (one alignment unit); old field offsets, V1 function-table
ABI, V1 allocation call counts and ordinary V1 pixels remain unchanged.
