# Contextual native text

Core text commands retain the original paragraph and a half-open UTF-8 byte
range for the selected visual line. Native software, SDL3, SDL GPU and Sokol
renderers use that context for bidi analysis. A wrapped line beginning with
numbers or punctuation therefore keeps the direction of its original paragraph.
This change preserves Clay's existing wrap choices; it does not implement UAX14
line breaking or visual arrow navigation.

## Portable interfaces

`cui_text_line_source` contains `paragraph`, `byte_start` and `byte_end`.
Endpoints are paragraph-relative UTF-8 scalar boundaries. Hard paragraph
separators are excluded. U+2028 remains inside the paragraph: native drawing
splits its physical lines while retaining the shared bidi context. A geometry
factory request selects one physical line and cannot include U+2028 in its range.

`cui_text_context_line` copies a paragraph and submits a selected line.
`cui_text_context_line_colored` additionally copies sorted, nonoverlapping color
spans, also in paragraph-relative bytes. Colored submission selects one physical
line and rejects a range containing U+2028. There are at most
`CUI_TEXT_COLOR_SPAN_MAX` (4096) spans. A shaped cluster takes the color at its
first original byte; a color boundary inside a ligature or combining cluster
does not split shaping. Both submission functions return false on invalid input
or capacity failure. Capacity reports `CUI_ERROR_CAPACITY`; failure does not
consume frame bytes or submit an element.

Ordinary `cui_text` wrapping also retains provenance. Core records submissions
in an array bounded by `max_elements`, resolves Clay's exact `baseChars` pointers,
and copies text into `frame_bytes`. It does not search for matching substrings.
Contextual lines share paragraph storage but have unique slice storage, so
repeated text and equal-start/different-end ranges remain distinct. CRLF suffixes
retained by Clay are removed from the visible line without changing the original
paragraph boundaries. Draw descriptors and color spans have the same lifetime
as the draw list: consume them before the next frame or context destruction.
Any renderer retaining commands must deep-copy their paragraphs and spans.

## Native host setup

Add the range factory to a native font configuration:

```c
cui_config config = {
    .measure_user = font,
    .measure = cui_font_measure,
    .measure_styled = cui_font_measure_styled,
    .font_metrics = cui_font_get_metrics,
    .text_geometry = cui_font_text_geometry,
    .text_geometry_line = cui_font_text_geometry_line,
};
```

A measurement wrapper needs matching geometry wrappers sharing `measure_user`.
`cui_get_text_geometry_line` preserves output on failure, checks the session
metadata, and releases a partial failed session. Successful sessions are owned
and immutable; release them once with `cui_release_text_geometry`. `byte_length`
is the full paragraph length. Caret, hit and selection offsets are original
paragraph-relative bytes, including when the selected line starts later.
The native adapter translates validated byte endpoints to the existing scalar
line-layout API. It retains shaped arrays and font faces, not the source buffer.

The old factory contract is unchanged. When a range selects the entire
paragraph, the helper can use the old `text_geometry` factory. Other ranges
require `text_geometry_line`; unavailable geometry uses the legacy measured
editing path. BIDI-off native factories return UNAVAILABLE. Ordinary contextual
drawing retains legacy BIDI-off drawing; colored contextual drawing requires
the optional native bidi implementation. Editor selects its established
fragment path when visual geometry is unavailable. Browser renderers do not
consume these native contextual/color descriptors in this batch.

## Multiline consumers and ownership

Textarea row records include original paragraph boundaries and hard-line
indices. Textarea and Editor share visual geometry for hits, caret x, preferred
vertical columns, reveal, selection spans and composition underlines. Primary
pointer-selected bidi affinity survives idle frames. The final IME rectangle
comes from the actual painted caret child's bounds after layout, including when
its blink is invisible. Logical editing offsets and keyboard motion keep their
existing policies; secondary Editor cursors do not gain persistent dual bidi
affinity or visual navigation.

One context resource caches at most 64 requested line sessions and 1 MiB of
unique paragraph bytes per frame. Visible rows and requested navigation rows
are created lazily; identical paragraph/range/style requests share a session.
Queries outside a frame use a temporary owned session. Beginning a new frame
releases cached sessions/source copies, observing host font registrations and
metrics changes. Source changes within a frame create a new immutable session.
Capacity, allocation, unavailable and unsupported-boundary failures retain the
measured fallback. Styled `cui_textarea_result.visual_error` reports the first
visible-line factory failure separately from edit/layout errors.

For a configured visual host, an active composition also retains one exact
preedit snapshot per state in a context resource, bounded to 65,535 bytes.
Unchanged composition reuses row layout across idle frames. Its resource owns
and frees the snapshot; caller state receives no allocation handle or cleanup
duty. Omitted resources and context destruction clean up ownership. Model
identity is compared without dereferencing a previous model. Rebinding to a
different model invalidates cached rows. Reinitialize state before destroying
and replacing a model at the same address; Editor's borrowed model must outlive
its state. Caller row arrays must remain alive and must not be shared by widgets.

Editor shapes each highlighted contextual line once, then supplies at most 64
color spans. Excessive spans report `CUI_TEXTAREA_CAPACITY` and paint a plain
contextual line. Gutter line indices are cached during row layout; gutter paint
visits visible rows, and digit-width counting refreshes on model revision.
Long individual paragraphs still require paragraph analysis for each selected
native line; this is not an incremental native paragraph-analysis cache.

Host measurement, geometry and raster calls are UI-thread confined. Keep host
font/style registrations consistent through draw-list consumption. After an
external metrics change, call `cui_text_metrics_changed` before the next frame.
No GDEF interior-ligature caret, cross-run grapheme, OS font discovery, bidi visual
arrow, or complete international editing parity is claimed. Prefix fallback is
not claimed visually correct for unsupported interior-cluster boundaries.

## Validation and integration

`tests/unit/test_contextual_text.c` exercises malformed UTF-8/ranges, duplicates,
CRLF provenance, frame ownership, copied colors, atomic capacity failures,
immutable cached sessions, exact cleanup and the 64-session bound with a 200 KB
paragraph. `tests/integration/test_contextual_text.c` uses actual Noto Sans plus
explicit Hebrew/Arabic fallbacks. It checks RTL caret/hit/selection geometry,
rendered selection pixels, wrapped punctuation raster hashes, U+2028 context,
Editor cluster colors and software-rendered pixels, pointer affinity through
idle frames, IME update/cancel/commit/Undo, retained sessions after source/font
changes, and a document over 200 KB with bounded idle viewport/composition work.
Missing font/bidi fixtures return 77; missing-glyph raster never counts as success.

The portable unit target links `cui`. The native integration target links
`cui`, `cui_text`, and `cui_software` (with native text enabled), and should use
CTest's skip-return-code 77. Root CMake registration and distribution regeneration
are integrator-owned. The older audit's standalone `cui_font_draw_styled` call
intentionally bypasses descriptors; use the new contextual test for the actual
renderer path rather than treating that legacy call as contextual rendering.
