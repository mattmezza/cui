# API and ownership

Public headers live in `include/cui/`; generated embeddable headers in `dist/`.

```c
cui_context *ui = cui_create((cui_config){.width=800, .height=600});
cui_theme theme = cui_theme_light();
cui_begin_frame(ui, 800, 600, 1.0f/60);
cui_begin_box(ui, cui_id_from(ui, "root"), (cui_box){
    .width=-1, .height=-1, .padding=24, .gap=12,
    .background=theme.background
});
if (cui_button(ui, cui_id_from(ui, "save"), &theme,
               cui_string("Save changes"), false)) {
    /* Save application data here; explicitly invalidate if changed UI requires another frame. */
}
cui_end_box(ui);
cui_draw_list list = cui_end_frame(ui);
/* renderer.render(renderer.user, &list, theme.background); */
cui_destroy(ui);
```

Width/height zero fit contents, negative grow, positive fix logical-pixel size. Begin/end pairs must balance. Frame text and accessible names are copied into a bounded context arena. The draw list and semantic tree remain valid until the next begin; consume them synchronously or copy to host-owned storage. User strings can be transient because CUI copies them during submission. The error state is sticky until explicitly cleared; capacity errors do not authorize reuse of truncated output as complete UI.

Stable nonzero `cui_id` values are supplied by applications or derived from scoped keys. Identity follows keys when lists reorder. Duplicate public IDs and folded layout-ID collisions are errors. CUI retains focus/capture and previous regions; it does not own arbitrary domain objects or provide a general widget state registry yet. Persistent domain data and edit buffers remain app-owned.

Queue input before begin. Events are interpreted against the prior frame, then consumed once by matching controls. Multiple activations of one control in a single frame coalesce into its boolean return. The first frame has no hit regions; initialize layout before accepting interaction. Removal/disable cancels retained focus and capture. Core handles Tab/reverse Tab, Enter and release-of-Space activation. See CORE.md, EDITING.md and OVERLAYS.md for exact limits.

UI context and optional font objects require single-thread ownership. Independent contexts may be interleaved on that thread; Clay's internal current-context pointer prohibits concurrent UI calls. Host asynchronous work must hand results back on the UI thread. Optional renderers own GPU resources with their device's thread affinity; borrowed host device/window/font must outlive the renderer.

The default text measurer is approximate and unsuitable for international text.
The native text adapter supplies contextual geometry, explicit font fallback and
optional bidirectional analysis; register matching measurement and paint callbacks.
Editing offsets are UTF-8 byte indices. Explicit legacy scalar operations remain
available, while interactive editing uses grapheme/Unicode-word operations.
Consumer and platform coverage remains partial: see [editing](EDITING.md),
[contextual text](CONTEXTUAL_NATIVE_TEXT.md), [Unicode wrapping](UNICODE_WRAP.md)
and [the platform matrix](PLATFORM_MATRIX.md) for the tested contracts.

## Responsive layout and typography roles

Two header-only utilities compose existing Core behavior without global style
state or new host dependencies:

| Header | Shipped API | Guide |
| --- | --- | --- |
| `cui/cui_responsive.h` | `cui_responsive_query`, `cui_responsive_at_least`, `cui_responsive_value`, `cui_responsive_fits` | [Responsive containers](RESPONSIVE_LAYOUT.md) |
| `cui/cui_typography.h` | `cui_typography_token_valid`, `cui_typography_resolve`, `cui_typography_text`, `cui_typography_text_colored` | [Reusable typography tokens](TYPOGRAPHY_TOKENS.md) |

Responsive queries use the caller's current logical container width and named
BASE/SM/MD/LG/XL/2XL thresholds. They select ordinary C box properties or whether
to submit a subtree; they do not implement CSS or own application state.
Typography roles copy caller-owned body/heading/code tokens containing supported
family/style, size, line-height, alignment and wrapping options. They do not load
fonts, synthesize faces, or add tracking/variable axes/gradient text paint.
Use the same font mapping for measurement and rendering; see [Typography](TYPOGRAPHY.md).

The native dashboard launcher uses the SDL3 callback runner; consult the current
`cui_sdl3.h` and [native application guide](NATIVE_APP_STARTER.md) for host
lifecycle and live-resize constraints. Portable width policy and text roles do
not provide a native event pump themselves.

For SDL3 applications, prefer `cui_sdl3_run` for callback-driven composition during
OS-managed modal resize. Manual `cui_sdl3_poll` loops remain supported, but their
next frame can wait until the operating system releases its modal event pump.
Inside runner callbacks, `cui_sdl3_poll` synchronizes IME/cursor state without
recursively pumping events. See [Live resize](LIVE_RESIZE.md) for lifecycle,
threading, frame scheduling and scoped native validation details.

## Decoded video frames

`cui/cui_video.h` exposes a caller-owned timeline and a borrowed RGBA frame surface.
The host supplies elapsed time and already-decoded frames, handles decoder/audio
synchronization, and retains pixels until renderer consumption completes.
Generation checks reject frames retired by a seek, restart or loop.
See [the video guide](VIDEO_GUIDE.md) for ownership, commands and reproducible
headless tests; [the video plan](VIDEO_PLAN.md) identifies controls and adapters
that remain unimplemented.
