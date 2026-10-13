# Unwrapped Input visual geometry

An optional `cui_config.text_geometry` factory lets unwrapped styled Input use
host-supplied visual caret, pointer-hit and selection geometry. Core and Base
contain only portable CUI types; they do not depend on native fonts, HarfBuzz or
platform APIs. Existing measurement-only configurations keep their established
Input path. Textarea and automatically wrapped paragraph provenance are separate
integration work.

Native hosts using a `cui_font *` as `measure_user` can register:

```c
cui_config config = {
    .measure_user = font,
    .measure = cui_font_measure,
    .measure_styled = cui_font_measure_styled,
    .font_metrics = cui_font_get_metrics,
    .text_geometry = cui_font_text_geometry,
};
```

Register explicit font fallbacks on `font` before use. The callback shares
`measure_user`, so a host using a wrapper object for measurement must supply a
matching geometry adapter that calls `cui_font_text_geometry(wrapper->font, ...)`.
No OS font discovery occurs. Native BIDI-off builds retain the factory symbol
and return `CUI_TEXT_LAYOUT_UNAVAILABLE` without changing output.

## Factory and query contract

`cui_get_text_geometry` calls the optional factory with original UTF-8 bytes,
size and style. It preserves the caller's output on failure and rejects malformed
or nonfinite session metadata. A successful `cui_text_geometry` owns its session
and exposes an operation table that stays valid until release. Text is borrowed
only during factory creation: the factory must retain everything its later
queries need. Each successful session requires one
`cui_release_text_geometry`; the helper zeros the handle so a second release is
harmless. Core defensively releases a partial session returned with a failure
when it supplies a valid release operation.

Queries use original byte offsets. Carets report at most two affinities at an
exact supported boundary: upstream attaches to the preceding logical cluster,
downstream to the following cluster. Native dual caret positions come from the
actual bidi runs and shaped cluster advance pens. Hit tests choose the nearest
visual cluster edge. Selection queries normalize reversed endpoints in the
native implementation and return complete-cluster rectangles in visual order;
a mixed-direction logical range can have disconnected coverage. NULL output
with zero capacity queries the required count. INVALID and CAPACITY preserve
both output buffers and counts; buffers/counts must be disjoint. Hit-test
failure preserves its output. Native queries reject nonfinite points.

`cui_font_text_geometry` wraps the existing owned native line layout. It retains
the actual root/registered/fallback faces and shaped arrays, and does not retain
or reread the caller's text buffer. Its queries remain usable after source
mutation, owned fallback replacement, or closing the root font. A font handle
is still required for future factory, measurement and raster calls; this does
not authorize closing a host font before ordinary rendering finishes. Native
factory input is one unwrapped line: hard paragraph separators and U+2028 are
rejected. Empty text has two affinities at x=0.

## Input lifetime and behavior

Input owns a private immutable source copy and the session in a context
resource, keyed by widget ID. Caller `cui_input_state` receives no new handle or
cleanup duty. Reduction and paint share one session for matching bytes/size in
that frame. A same-frame text mutation releases it and builds a fresh session
before further hit testing or painting. Beginning the next frame releases the
previous session, so font registration changes are observed; omitted widgets
and context destruction also release it exactly once. Model rebinding clears
stored affinity, and text revision changes invalidate that affinity. Resource
collection never dereferences a previous model pointer.

The visual session supplies caret x, pointer hit, selection spans, horizontal
reveal and the IME candidate-window x. Pointer-selected upstream/downstream
positions persist through event-free repaint, including a boundary whose two
affinities have different visual x positions. Logical keyboard movement keeps
its existing edit model behavior; a new logical boundary defaults to downstream
(or the sole available edge at a line end). Geometry does not implement visual
arrow navigation.

Selection and composition underlines use individual visual spans. Their
vertical extent uses the existing Input line box, while the native text path
keeps its common fallback baseline. Text and placeholders use one unwrapped
line on the visual path. IME geometry resolves the actual painted caret child's
final bounds after layout through `cui_set_text_input_geometry_rect`, so changes
in size, alignment and viewport origin do not use the previous frame's position.
Query the final candidate rectangle after `cui_end_frame`.

The context resource requires one additional slot when geometry is configured.
Its private source copy is bounded to 65,535 bytes; selection/composition
workspace is bounded to 4,096 rectangles each. These nested allocations are
released with the resource. Failure to allocate or exceeding these bounds uses
the documented legacy geometry fallback. A context resource capacity error
remains observable through `cui_last_error`.

Host font/style registrations must remain consistent between measurement,
geometry creation and ordinary rasterization of a completed draw list. After
changing host font metrics outside a frame, call `cui_text_metrics_changed` to
refresh Core's measurement cache before the next frame.

## Explicit limits and fallback policy

Native geometry accepts complete shaped-cluster boundaries. A UTF-8/grapheme
boundary inside a ligature or merged mark cluster can return INVALID. Input
then uses its measurement-only geometry for that whole painted line, preserving
the editor's original logical anchor/cursor bytes. It does not silently snap
editing offsets to a nearby glyph or claim that this fallback is visually correct
for interior ligature carets. Factory/query UNAVAILABLE, INVALID, MEMORY,
MISSING_GLYPH and CAPACITY likewise use the legacy path. The public factory and
query APIs expose those statuses for callers requiring strict geometry.

`cui_base_input_hit_test` tries the optional visual host for reusable callers;
`cui_base_input_measured_hit_test` explicitly requests the legacy scalar prefix
path. `cui_base_input_visual_hit_test` consumes a borrowed session, maps viewport
and scroll coordinates, and returns the original byte offset; an optional
caller affinity output is unchanged on failed or nonfinite hits. Native
cluster-edge hits are still checked by the editor's UTF-8/grapheme selection
rules.

This is not full international editing parity: no GDEF interior ligature
carets, cross-run grapheme analysis, visual arrow navigation, multi-line
geometry, automatic Unicode line breaking or Textarea integration is claimed.
Automatically wrapped Core commands still lose original paragraph context;
the audit preserves that evidence separately.

## Validation

Portable headless tests exercise owned session cleanup, malformed/nonfinite and
capacity results, unchanged caller outputs, model rebinding, size changes,
private source copies, same-frame edits, omitted-widget cleanup and explicit
interior fallback without changing logical selection. Native tests compare the
factory with styled metrics and keep querying after owned fallback replacement
and root close. Enabled and BIDI-off API checks are included.

`tests/integration/test_native_text_geometry.c` defaults to a passing unwrapped
Input suite with actual Hebrew, Hebrew marks, mixed Latin/Hebrew/numbers,
dual-affinity idle repaint and supported Arabic joining/mark fixtures. It
compares real selection paint/IME/hits to native geometry, verifies HarfBuzz
source glyph/cluster/advance data and actual nonzero raster pixels, and reports
optional font cases explicitly. `--audit` retains the known Textarea and
wrapped-paragraph failures. No missing-glyph raster counts as success.
