# SDL_Renderer V2 public entry: next review checkpoint

Proposal only, following `SDL_V2_BASELINE_CHECKPOINT.md`, signed range checkpoint
4ca5bbe3 and accepted private owned-frame extraction. No SDL API, object change,
native measurement or renderer support is implemented here. Range maximum-boundary
runtime acceptance remains pending. Preserve the frozen SDL baseline and all V1
function signatures/table/cache/density behavior.

## Minimal additive header

```c
enum {
    CUI_SDL3_V2_TEXT_A8 = 1u << 0,
    CUI_SDL3_V2_TEXT_CONTEXT = 1u << 1,
    CUI_SDL3_V2_TEXT_TRACKING = 1u << 2
};
typedef struct {
    uint32_t size;
    cui_draw_v2_support draw;
    uint32_t text_features;
    cui_render_v2_options defaults;
} cui_sdl3_v2_info;
cui_error cui_sdl3_query_v2(const cui_sdl3 *, cui_sdl3_v2_info *out);
cui_error cui_sdl3_render_v2(cui_sdl3 *, const cui_draw_list_v2 *,
    cui_color clear, const cui_render_v2_options *options);
```

Include public `cui_draw_v2.h`; no software header, SDL type, generic registry or
V1 capability/table extension. Query requires exact `out->size`, known range
disjointness before reading/writing output and unchanged output on error. Output
and all inputs must remain disjoint from the opaque SDL/font graphs. Query can
invoke SDL allocation hooks; it is not advertised as allocation-free.

## Guard measurement before implementation

Use the existing frozen-source `layout_source()` fixture to measure original and
candidate `sizeof`, alignment, every old offset and appended `v2_busy/v2_faulted`
offsets. Compile native and wasm32 constants only; wasm32 is a layout diagnostic,
not SDL browser/WASI support. Report exact compiler/settings and constructor
allocation request delta before approval. No assumed padding, approved growth
bound or retained texture/frame/cache is inferred from the software guard proof.
V1 callback/cache/pixel and allocation-count comparisons use separately compiled
original SDL source; object-byte change must be explicit if approved.

## Query and render ordering

Refuse busy/faulted owner before native calls. Query validates output alias first.
Render uses the shared pure range walker with owner/control exclusions before any
busy mutation or SDL call, including NULL options. Then temporarily claim busy,
query fresh owned-window logical/output sizes, target, texture-format list and
colorspace/limits; restore guard before full common preparation. No dimension
query through mutating `cui_sdl3_size`, cached-DPI reservation or duplicate active
pointer parser. Borrowed metadata/resources remain stable throughout synchronous
return; callback side effects cannot be rolled back.

Only the owned window backbuffer, positive dimensions <=16384, native advertised
RGBA32 and supported sRGB output qualify. Reject texture targets/HDR/conversion
routes clearly. Defaults use independent output/logical ratios and origin zero;
explicit options use the common independent-axis domain and exact current physical
viewport. Ceilings/defaults match software/common planning, including physical
P=4wh, metadata and command descriptor admission; transfer/texture P has a separate
explicit ceiling. One counted CPU plane permits 4K P=33,177,600 within 32 MiB
before resources. This does not guarantee every 4K list fits. Opaque SDL/driver/font
allocations remain separately documented class 3.

The common frame prepares every command and resource before native mutation.
Claim owner busy at its first allocation boundary through native cleanup. Snapshot
SDL logical/scale/viewport/clip/color-scale state, establish physical 1:1 state,
create transient native RGBA32 texture, checked tight upload, NEAREST/white
modulation/BLEND_NONE, full-output copy and explicit Flush. No second CPU plane,
V1 draw replay or software-adapter linkage. Restore every changed property after
any failure; release texture and frame while busy. Restore failure sets faulted;
future query/render refuses until destruction/recreation. No native rollback or
synchronous driver-memory reclamation guarantee.

Present is an attempt after checked submission, not proof of physical presentation
or even backend-present error reporting (both audited SDL implementations hide
that result). Busy V2 query/render returns LIFECYCLE; V1 render returns false,
mutating size/native access routes must refuse as individually audited, and void
destroy is a no-op requiring retry. Faulted idle owner remains destroyable.

## Gates and metadata

First approve measured guard growth and exact native failure/error mapping. Then
implement all-command V2 query/render together with hidden-at-creation tests:
query-to-Core composition; original V1 pixel/cache/allocation differential;
Retina/fractional/anisotropic resize; all source aliases before native hooks;
reentry in allocation/free; native format refusal; upload/draw/flush/restoration
failure and retry/fault behavior. Capture before Present where needed; compare
CPU expected pixels independently, not native V1 equivalence assumptions.
Only passing actual gates permits public metadata/guide/software-exception updates.
SDL_GPU, Sokol GLCORE, Canvas and foreign bindings remain required followups;
this proposal changes no 78-component status.
