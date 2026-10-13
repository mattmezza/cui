# Optional bidirectional glyph layout

`cui_font_layout_line` adds an explicit bidirectional shaping path alongside the
existing `cui_font_measure*` and `cui_font_draw*` functions. When enabled, those
ordinary calls also use this shaping path with automatic paragraph direction.
The explicit line API returns an owned `cui_text_layout`, a borrowed view
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
widgets and backend commands using the native text adapter benefit from the
enabled ordinary-call routing. No other Core/widget behavior is changed.

The shaping contracts follow the primary
[HarfBuzz buffer documentation](https://harfbuzz.github.io/harfbuzz-hb-buffer.html)
and [cluster documentation](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html);
paragraph/run rules follow [Unicode 17 UAX #9](https://www.unicode.org/reports/tr9/tr9-51.html).

## Ordinary text and hard lines

With `CUI_TEXT_BIDI` enabled, ordinary styled/unstyled measure and draw calls
analyze each P1 paragraph with auto-LTR fallback, use script/direction shaping,
and paint its visual glyph stream. These calls support every Unicode B paragraph
separator (LF, CR, U+001C/U+001D/U+001E, NEL and U+2029). CRLF is one break.
U+2028 creates a hard line inside the same paragraph, preserving the complete
paragraph direction/isolate/embedding context across that break. Separators
produce no glyphs. Empty and trailing lines still occupy one face line height.

Measurement returns maximum line advance width and the sum of line heights.
Drawing resets horizontal origin to the caller's x for each line and advances
the top/baseline by that face's line height. Empty text occupies one empty line,
as in the existing measurement interface. Every line is fully laid out before
any raster callback: malformed UTF-8 or allocation failure emits nothing and
returns false; measurement returns {0,0}. Raster failures can still be partial.
The explicit glyph-layout API permits an empty scalar range and retains that
paragraph's base and metrics, allowing empty U+2028 continuation lines.

Tests independently reproduce the original FreeType/HarfBuzz ASCII shaping and
raster loop to compare legacy single-line glyph pixels, metrics and baselines,
including ligatures/kerning. Ordinary mixed RTL/multiline rendering is compared
against explicitly selected paragraph-line layouts, whose run/glyph data is
checked against the upstream references. Tests cover all B separators, CRLF,
U+2028 context, empty/trailing lines, baseline progression and invalid input
following a valid line. Real Arabic joining/mark raster checks also call the
ordinary adapter APIs. Disabled builds retain the original adapter path.

This is hard-line processing, not automatic word wrapping or a caret system.
The simple script itemization/fallback limitations above still apply. Each
explicit U+2028 line currently reanalyzes its paragraph; large numbers of such
lines may warrant caching reusable analysis in a future text-layout object.
