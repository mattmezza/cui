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

The default text measurer is approximate and unsuitable for international text. FreeType/HarfBuzz handles individual shaped runs; paragraph bidi, font fallback, grapheme selection, full IME and line breaking are not complete. UTF-8 editor offsets are byte indices constrained to scalar boundaries, not grapheme boundaries.
