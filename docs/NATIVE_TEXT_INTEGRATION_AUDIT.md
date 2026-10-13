# Native text integration audit

At integration/next18 `b291765`, native font shaping and explicit line geometry
worked, but ordinary wrapping and editor geometry did not preserve that result.
`tests/integration/test_native_text_geometry.c` reproduces four observable
mismatches using supported Hebrew glyphs and an explicitly registered fallback
face. The follow-up implements the unwrapped Input host geometry seam described in
[INPUT_VISUAL_GEOMETRY.md](INPUT_VISUAL_GEOMETRY.md). The default executable now
verifies fixed Input behavior and returns 0; `--audit` preserves the remaining
Textarea and automatic wrapped-paragraph observations and returns 1. It returns
77 when optional native bidi or actual supported font fixtures are unavailable.

## Runtime evidence

With Noto Sans as the primary and Noto Sans Hebrew as an explicit fallback:

| Operation on `אבג` at size 14 | Original editor result | Native shaped result |
| --- | --- | --- |
| Caret at original UTF-8 byte 0 | x=0 | x=22.094, the visual right edge |
| Hit at visual left edge | Input byte 0, Textarea byte 0 | byte 6, the logical end |
| Select first Hebrew character, bytes 0..2 | Highlight x=0 | Highlight x=13.250, over the actual rightmost character |

The current Input path matches all of these results when its geometry hook is
configured. Textarea still produces the prefix-based results in `--audit`.
These are shaped cluster edges, so no interior ligature/GDEF caret policy is
needed to reproduce the mismatch. The Textarea IME input rectangle follows its
incorrect prefix caret position as well. The test compares the actual painted
selection rectangle, not just the underlying owner's selection state.

The second fixture submits ordinary `cui_text("אבג 123 !")` in a narrow box.
Core emits a wrapped `CUI_DRAW_TEXT` command containing `123 !`. Drawing that
command through the renderer's actual native font entry point yields a raster
identical to independently analyzing `123 !` as a new LTR paragraph. It differs
from `cui_font_layout_line` on the original paragraph with scalar range 4..9:
that wrapped line retains RTL paragraph context and moves punctuation to the
other side of the number run. The command's independently analyzed paragraph
level is 0; the original paragraph's level is 1.

Before reporting gaps, the test independently shapes the Hebrew fixture using
HarfBuzz with explicit RTL/script/language/cluster flags. It compares nonzero
source glyph IDs, original byte clusters, glyph positions, and advance width.
Ordinary native font drawing matches explicit layout rasterization and emits
110 nonzero pixel samples for this fixture. Thus this audit does not mistake
missing-glyph tofu, an unshaped native measure callback, or a missing font for
an integration defect. Exact coordinate/pixel values depend on the selected
fixture; the comparisons derive expected values at runtime.

## Actual path through the code

- `src/core/cui.c` installs `cui_core_measure`, which forwards to the host's
  styled measurement callback. The native examples register
  `cui_font_measure[_styled]`. Those native callbacks already shape bidi text;
  this is not a claim that they use unshaped metrics.
- `cui_text` selects Clay word wrapping. The pinned Clay measurement cache
  measures word slices and spaces separately (`third_party/clay/clay.h:1717`).
  Core's text command conversion (`src/core/cui.c:1773`) copies the emitted
  line substring, size/style and bounds, but no original paragraph or line
  range. Merely switching the scalar font measurement implementation cannot
  restore context once that information is gone.
- SDL3 (`backends/sdl3/cui_sdl3.c:517`), SDL GPU
  (`backends/sdl_gpu/cui_sdl_gpu.c:433`), Sokol
  (`backends/sokol/cui_sokol.c:594`) and software rendering
  (`backends/software/cui_software.c:272`) call `cui_font_draw_styled` per text
  command. The audit invokes this common entry point headlessly; it does not
  claim to open a GPU window or test GPU pixels.
- The native font path correctly separates actual hard paragraphs and preserves
  U+2028 line context inside its own whole-string input (`src/text/cui_text.c`,
  `text_lines_create`/`text_paragraph_append`). It cannot reconstruct context
  already removed by Core or Textarea automatic wrapping.
- The original Input hit testing (`src/base/cui_base.c:399`) binary-searched logical prefix
  widths and immediately returned byte 0 at the left edge. Its legacy fallback
  still does; configured unwrapped Input now consumes host geometry. Original
  Input paint and IME rectangles also used prefix measurements (`src/components/cui_components.c`,
  `cui_input_paint`). Textarea's public row-x/row-offset and pointer hit functions
  use the same monotone-prefix assumption (`src/base/cui_textarea.c:408`).
- Textarea measures and paints each selected wrapped byte slice separately and
  draws one prefix-based selection span (`src/components/cui_textarea.c:150`).
  Caret painting, reveal, vertical preferred-x navigation and IME rectangles
  follow those prefix coordinates. It does not call the native cluster geometry
  APIs. The runtime test verifies row geometry, IME rectangle and actual
  selection paint on an unwrapped RTL line; the wrapped Textarea context loss
  follows the inspected slice submission, rather than a separate GPU test.

The edit model continues to own valid UTF-8/grapheme selection and logical
movement. This audit does not equate native shaped clusters with every editor
grapheme or claim visual keyboard navigation already exists.

## Narrow next implementation ownership

The reproductions justified two independent changes. The first is now
implemented for unwrapped Input, with the explicit cluster-interior fallback
limits documented in INPUT_VISUAL_GEOMETRY.md. The second remains pending.

1. Implemented: **unwrapped Input visual geometry** through a portable host
   callback seam rather than making Core depend on native text/font types.
   The implementation covers the geometry callback public/config seam in
   `include/cui/cui.h` and `src/core/cui.c`, its native implementation in
   `include/cui/cui_text.h`/`src/text/cui_text.c`, Input geometry plumbing in
   `include/cui/cui_base.h`/`src/base/cui_base.c` and Input painting in
   `src/components/cui_components.c`, with focused headless/native integration
   tests. Caret, pointer hit, selection rectangles, horizontal reveal and IME
   rect now use a retained session while measurement-only hosts keep their
   existing path. Exact shaped-interior offsets deliberately use the documented
   legacy fallback without changing logical selection. Textarea remains a
   separate follow-up.
2. Separately preserve **original paragraph plus chosen line range** through
   wrapping and renderer submission. Initial ownership requires Core text
   command metadata/layout, native text, and the SDL/GPU/Sokol/software text
   adapters plus source/raster regressions. The public metadata must remain
   portable CUI types. The existing `cui_font_layout_line` already accepts full
   paragraph context and selected scalar ranges; adapters should receive enough
   original information to use it. Re-shaping each emitted substring cannot
   satisfy the reproduced UAX9 line-context behavior. Word-breaking policy and
   capacity/lifetime contracts need explicit tests; font fallback alone does
   not fix the handoff.

Full grapheme editing parity, interior ligature carets using optional GDEF,
visual arrow navigation, multi-line affinity policy, OS font discovery and
Unicode line-break parity remain beyond those narrow changes. The confirmed
fixtures need none of those claims. The relevant existing primary models are
[UAX9 line reordering](https://www.unicode.org/reports/tr9/tr9-51.html#Reordering_Resolved_Levels)
and [HarfBuzz cluster semantics](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html).

## Reproduction and validation

```sh
cmake -S . -B build -G Ninja
cmake --build build
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  $(pkg-config --cflags freetype2 harfbuzz) \
  tests/integration/test_native_text_geometry.c \
  build/libcui_text.a build/libcui_bidi.a build/libcui.a \
  $(pkg-config --libs freetype2 harfbuzz) -lm -o /tmp/cui-native-text-audit
/tmp/cui-native-text-audit
/tmp/cui-native-text-audit --audit # preserves remaining known gaps
ctest --test-dir build --output-on-failure
```

Use `CUI_TEST_FONT` and `CUI_TEST_HEBREW_FONT` to select explicit supported
fixtures; known Noto and DejaVu paths are tried when unset. The executable is
not registered by this worker because CMake belongs to the integrator. Its
default verifies fixed Input behavior; `--audit` still preserves four remaining
Textarea/wrapping observations rather than declaring full native integration.

Validation at this snapshot: portable warnings-as-errors native build and the
new manual integration compile pass; existing CTest passes 92/93. The sole
failure is the expected distribution regeneration check for changed Core/Base/
Components headers, which is integrator-owned. The enabled default
passes using actual Noto fallback fixtures; `--audit` retains the four remaining
Textarea/wrapping observations.
ASan/UBSan report no runtime errors. The BIDI-off audit compiles/links and
returns 77 with an explicit unavailable message. The follow-up changes only its
assigned Input/geometry production files; no CMake or distribution output is
changed.
