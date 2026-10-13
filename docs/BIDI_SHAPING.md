# Optional bidirectional glyph layout

`cui_font_layout_line` adds an explicit bidirectional shaping path alongside the
existing `cui_font_measure*` and `cui_font_draw*` functions. Existing calls keep
their behavior. The new path returns an owned `cui_text_layout`, a borrowed view
of visual glyphs and runs, and `cui_text_layout_draw` to rasterize that positioned
layout. Public types contain only portable CUI and C types.

The full UTF-8 paragraph is analyzed by the pinned Unicode 17 bidi module. The
caller then selects a scalar line range (SIZE_MAX length means through the end).
L1/L2 use the full paragraph context, including embeddings and isolates crossing
wraps. Visual directional runs are split into logical script subruns. Script
properties come from the linked HarfBuzz Unicode functions. Common/Inherited/
Unknown characters adopt a preceding strong script in the directional run, or
the next strong script when none precedes them; a wholly weak run remains Common.
This is basic script itemization, not full UAX #24 Script_Extensions resolution.

RTL script subruns are placed in reverse subrun order. Each logical UTF-8 subrun
is shaped with an explicit HarfBuzz direction, script, language (BCP47 or default
`und`) and `MONOTONE_GRAPHEMES` cluster level. HarfBuzz's output glyph order is
preserved, including ligatures and combining glyphs sharing one cluster. There
is no scalar-level reversal of shaped glyphs. The shaping input uses the selected
line as pre/post context; BOT/EOT flags mark the line's actual boundaries. Bidi
paragraph context is retained, but Arabic joining does not cross a line boundary.
Default ignorables, including bidi formatting controls, affect shaping but are
removed from the final glyph stream. HarfBuzz handles direction-dependent glyph
mirroring; the scalar mirror output from the bidi analyzer is not reshaped.

Glyph `cluster_byte` values are absolute byte offsets into the original UTF-8
paragraph, including for a wrapped line starting after byte zero. They may repeat
or skip source bytes; they are not scalar indices or one-to-one caret positions.
Run byte ranges use the same source coordinates. Glyph `x/y` are final origins
relative to the line baseline, with y increasing downward; advances use that same
coordinate convention. Width is the sum of absolute horizontal subrun advances,
not an ink bounding box. Height/ascent come from the selected face metrics. View
arrays remain valid until layout destruction.

Creation owns all glyph/run storage and releases transient paragraph/shaper
buffers before return. Input text may be reused immediately. Allocation and
validation failures preserve the caller's layout output pointer. The selected
font/style face is borrowed: keep it alive and registered through layout use and
destruction. Fonts are UI-thread confined, as in the existing text adapter. Draw
coordinates refer to the line top-left; partial raster failure can already have
emitted glyph callbacks. The renderer can instead consume the portable glyph
view. Drawing another glyph size between layout creation and drawing is safe:
the saved layout size is reapplied to the face.

## Integration

When both optional native text and bidi targets exist, define `CUI_TEXT_BIDI` for
`cui_text` and link it privately to `cui_bidi`. Without that definition, the new
creation function returns `CUI_TEXT_LAYOUT_UNAVAILABLE` without modifying its
output; plain text needs no bidi link. No public compile definition is necessary.
The normal native text adapter still requires FreeType and HarfBuzz.

Register `tests/unit/test_text.c` as a new test linked to `cui_text`, `cui_bidi`
and the existing FreeType/HarfBuzz package target, with a private include path
`third_party/sheenbidi/Headers`. Define `CUI_TEXT_BIDI` for the test target when enabled, and keep assertions
active. Without that definition, the same test checks the atomic UNAVAILABLE
contract and needs no bidi/HarfBuzz reference include or link. The test uses
`CUI_TEST_FONT` or a portable Noto/DejaVu/Liberation font search, and
`CUI_TEST_ARABIC_FONT` or Noto Sans Arabic/DejaVu Sans. The Arabic case checks
character coverage and explicitly logs any untested joining/mark raster case.
It compares visual directional coverage/levels directly to pinned SheenBidi,
then glyph IDs, source clusters, offsets and advances directly to HarfBuzz. It
checks mixed Latin/Hebrew/Arabic/numbers/brackets, isolates, nonzero wrapped-line
source offsets, RTL script-subrun ordering, actual Arabic mark clusters and
joining with a supporting face, style face selection, invalid UTF-8 and atomic
failure. Existing ASCII metrics and raster checksums match the plain interface.
Both enabled and disabled builds compile with strict warnings; enabled tests
pass with address and undefined-behavior sanitizers.

The linked HarfBuzz and selected font determine shaping/version coverage. These
headless comparisons demonstrate the tested shaping and bidi behavior, not
complete international-text rendering parity. No system font installation is
performed.

## Limits

The caller chooses line breaks at complete grapheme/shaping-cluster boundaries;
this API accepts scalar ranges and does not enforce cluster-safe boundaries,
compute wrapping widths, or expose unsafe-to-break glyph flags. Script/directional
itemization is basic and is not a full cluster-aware script resolver. Existing
style registration chooses one face for the complete line; missing glyphs remain
missing, with no per-script or OS font fallback. There is no language detection,
vertical layout, justification, caret mapping, selection or hit testing. Core,
widgets and backend draw commands do not automatically use this new layout path.

The shaping contracts follow the primary
[HarfBuzz buffer documentation](https://harfbuzz.github.io/harfbuzz-hb-buffer.html)
and [cluster documentation](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html);
paragraph/run rules follow [Unicode 17 UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html).
