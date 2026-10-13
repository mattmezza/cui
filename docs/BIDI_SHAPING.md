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
not an ink bounding box. Height/ascent come from the primary and used fallback face metrics. View
arrays remain valid until layout destruction.

Creation owns all glyph/run storage and releases transient paragraph/shaper
buffers before return. Input text may be reused immediately. Allocation and
validation failures preserve the caller's layout output pointer. The layout retains its selected and fallback faces independently of caller
references: closing fonts or replacing registrations does not invalidate it.
Borrowed fallback/style registrations still require their source handles to stay
alive for future layout creation until the registration is cleared. Fonts are UI-thread confined, as in the existing text adapter. Draw
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
style registration selects the primary face; an explicit caller chain can
supply complete script/directional subruns. There is no OS font discovery or
per-grapheme mixed-face fallback inside a subrun. There is no language detection,
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

## Explicit caller-registered font fallback

`cui_font_set_fallbacks` installs an ordered borrowed chain of actual CUI font
handles; `cui_font_register_fallbacks` opens and owns an ordered list of file
paths. Both replace the complete chain transactionally, accept zero entries to
clear, and reject counts above `CUI_FONT_FALLBACK_MAX` (32), NULL entries,
duplicates/self handles for the borrowed setter, and failing paths. A failed
call leaves the old chain working and releases partially opened owned faces.
Current owned entries cannot be reborrowed into the replacing chain.

The chain belongs to the root handle passed to layout/measurement/drawing, not
to a style variant. Existing style-selection rules are applied to the primary
and each direct fallback entry. Chains are not recursively traversed. Borrowed
fallback and style handles must remain alive while registered for future calls.
Owned entries are released on replacement/owner close. Font close releases a
reference, and layouts retain each actual candidate face independently: a layout
can still be painted after chain/style replacement or caller font close. Its
view exposes borrowed retained CUI handles in `faces`; never close those view
handles. `run.face_index` identifies the face whose glyph IDs the run contains.
All font/registration operations remain confined to the UI thread.

Coverage is chosen for each entire script/directional subrun. The first face
that shapes the complete subrun without glyph zero is selected. This keeps
joining Arabic letters and their marks together and never switches faces inside
a HarfBuzz output cluster. HarfBuzz's REMOVE_DEFAULT_IGNORABLES flag lets joining controls, CGJ,
variation selectors and tags affect shaping without requiring standalone cmap
entries. Coverage uses actual shaped output, not nominal cmap checks: valid
normalization/composition can cover combining sequences even when a standalone
mark has no cmap entry. Every visible shaped glyph must be nonzero.
No complete covering face, or any shaped glyph zero, yields explicit
`CUI_TEXT_LAYOUT_MISSING_GLYPH` and preserves the output pointer. Ordinary draw
returns false with no callbacks if layout/coverage fails; ordinary measure returns
{0,0}. Missing-glyph tofu is not reported as a successful fallback layout.

Runs use their selected faces for both HarfBuzz shaping and FreeType rasterization.
All glyph origins share one baseline. If fallback is actually used, ascent and
descent are maxima across the primary and used faces, and line height is the
maximum of all used native line heights and combined ascent/descent. No-fallback
lines preserve the existing primary face height exactly, including rounded metric
conventions. Unused registered fallback faces do not enlarge the line. Hard-line
baseline progression uses each computed line height. `cui_font_get_metrics` remains
the selected primary face's generic metrics, since it has no text argument;
text-specific fallback extents are available from layout/measurement.

Tests verify a Noto Latin primary actually lacks Arabic/Hebrew, then compare
successful fallback glyph IDs, marks, advances and pixels with direct HarfBuzz/
FreeType shaping on the registered script faces. They verify original source
clusters, script run order, mixed baseline maxima, Arabic joiners/marks/VS/CGJ,
missing coverage failures, first-fit selection, invalid/capacity registration,
partial owned-open cleanup, style-face selection, and retained raster stability
after owned/borrowed chain replacement and root/variant font close. ASCII metrics
and raster regressions remain unchanged. Font lookup also accepts portable
DejaVu faces and explicit `CUI_TEST_FONT`, `CUI_TEST_ARABIC_FONT` and
`CUI_TEST_HEBREW_FONT`; cases already covered by the primary are explicitly logged
as untested missing-primary cases rather than inventing missing-glyph evidence.

Without bidi enabled, chain registration/lifetime APIs still work and are tested,
but the existing legacy ordinary shaping path does not consult the chain; explicit
layout creation remains UNAVAILABLE. There is no automatic system font discovery,
color-emoji rendering, per-grapheme mixed-face segmentation, automatic word
wrapping, or caret/selection/hit-test implementation. Whole-run coverage may fail
when separate fonts collectively cover a run but no one font covers it completely.

## Renderer fixture validation

The native software renderer test no longer treats nonempty `.notdef` pixels as
multilingual evidence. Its Arabic and Hebrew samples first create layouts on
explicit optional fixture faces and assert every source glyph ID is nonzero.
Those validated faces are registered in the primary font's fallback chain, and
ordinary drawing is checked against the explicit layout's raster checksum,
coverage and measurement. Existing ASCII/ligature and renderer geometry checks
remain independent. Tests search known portable Noto/DejaVu/Liberation fixture
paths, or honor `CUI_TEST_ARABIC_FONT` and `CUI_TEST_HEBREW_FONT`; missing or
unsupported fixtures explicitly report the script case skipped. With bidi OFF,
the test reports source-glyph proof unavailable and keeps its ordinary ASCII
regressions. Legacy nonempty tofu pixels are never labeled successful RTL proof.
This fixture search is test setup, not production OS font discovery.

The native line layout also exposes a cluster-boundary geometry foundation.
`view.clusters` partitions each shaped directional/script subrun into complete
HarfBuzz monotone grapheme clusters, sorted by visual advance position. Byte
ranges remain offsets into the original paragraph, including caller-selected
wrapped lines. Geometry uses glyph advance pens, not ink extents or glyph offsets;
selection rectangles use the layout's common baseline line height across actual
fallback faces. Layout ownership retains both these arrays and their faces.

`cui_text_layout_carets` reports the preceding logical cluster's end (upstream)
and following logical cluster's start (downstream). At directional transitions
these affinities can have distinct visual positions. Results sort by x, then
upstream before downstream. Empty lines provide both affinities at x=0 and the
original line offset. `cui_text_layout_hit_test` projects finite coordinates onto
this single line and chooses the nearest cluster edge; equal distances prefer
the requested affinity, then the first cluster in visual order. The y coordinate
is validated but does not choose another line. Default-ignorable bytes omitted
by shaping attach to the adjacent logical cluster; a wholly omitted subrun has
one zero-width cluster. Zero-width edges follow the same deterministic tie rule.

`cui_text_layout_selection` normalizes reversed logical endpoints and emits one
rectangle per complete selected cluster, in visual order, including zero-width
rectangles. Callers can merge adjacent rectangles for painting. Caret and
selection endpoints inside a cluster (including interior ligature or combining
mark offsets) are rejected. NULL output with zero capacity queries the count;
INVALID and CAPACITY leave both output and count unchanged. Hit-test invalid
inputs also preserve output. All three APIs return UNAVAILABLE without changing
outputs in BIDI-off builds.

This is not full editing caret parity: there are no GDEF ligature interior
carets, independent stops inside omitted controls, cross-run grapheme analysis,
visual keyboard navigation, multi-line hit-testing, or Core/Edit/Textarea wiring.
Fonts can shape several characters into one cluster, so a valid UTF-8 character
boundary need not be an accepted geometry boundary. Actual automatic word
wrapping and editor selection/caret policies remain caller responsibilities.
The cluster grouping follows [HarfBuzz's cluster model](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html);
interior ligature carets would additionally require the optional
[OpenType GDEF ligature caret list](https://learn.microsoft.com/en-us/typography/opentype/spec/gdef#ligature-caret-list-table).
