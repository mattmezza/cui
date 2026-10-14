# Source label contract

The additive `cui_label.h` API complements the legacy `cui_label` plain text wrapper.
`cui_label_prepare` produces caller-owned display bytes and sorted nonoverlapping
color spans. `cui_label_source` copies those bytes into Core frame storage.
Initialize options to zero and provide `primary`. `has_secondary` adds exactly
one ASCII space even if either string is empty. Secondary text and its preceding
space use theme muted foreground. Matches override either foreground with theme
info (the semantic blue). `custom_color` changes primary foreground. A supplied
`color` is inert unless `custom_color` is true; zero initialization uses theme text.
Caller-selected error presentation uses `custom_color = true` and `color = theme.danger`.
Secondary text stays muted and matches override either foreground, including
matches that cross the primary/secondary separator. Masked bullets use the selected
base foreground uniformly and never emit secondary/highlight spans. Font size,
weight, italic, monospace and family are forwarded; line height defaults to
20 logical pixels, matching the source's 1.25rem at its 16px root size.

The frozen `crates/component/src/label.rs:53-59,61-91,192-215` has no disabled or
error state flags; it accepts inherited Styled text color. Its website error
example (`website/component/label.md:231-233`) selects `theme.destructive` through
`.text_color(...)`. C's caller color option covers that presentation rather than
adding an invented validation/disabled API. Required/optional markers likewise
remain ordinary secondary text. `test_label_source` checks prepared spans and real
Core draw commands for default and caller error foregrounds, muted secondary text,
a Unicode highlight crossing the separator, alpha preservation, default theme font
and 20px line metrics, and masked display/semantic privacy with a custom color.

Matching uses Unicode 17.0.0 default, locale-independent lowercase, including
one-to-many mappings and contextual Greek final sigma. Full matching finds all
overlapping occurrences; prefix matching considers only the start. Empty matches
are inert. Matching performs no normalization or full case folding: sharp-s and
SS are distinct. Lowercase positions map back to original UTF-8 scalar boundaries,
so expanding mappings and byte-width changes cannot split a scalar or shift later
matches. This deliberately repairs the source implementation's reuse of lowercase
byte offsets and original search length, which can miscolor or split Unicode text.
A match covering part of an expanded lowercase scalar colors that whole scalar.

Masking emits one U+2022 bullet per Unicode scalar of the combined text, including
the separator. It emits no highlight spans, and neither draw paragraph nor optional
semantic name contains original bytes. Inputs remain owned by the caller; this API
is not a secret-storage or memory-erasure facility. Optional `semantic_id` creates
a label node; `control` sets Core controls and focus_target relations. Masked nodes
are marked secret. Applications set the controlled widget's `labelled_by` relation
when they need that reverse accessibility relation.

All input strings require strict UTF-8. Prepare rejects invalid arguments, overflow,
allocation failure and insufficient capacity without changing text, spans or the
result structure. Output buffers must not overlap each other or input strings.
The text output is length-delimited with no NUL terminator. Span capacity is bounded
by Core's 4096 span maximum. Renderer failures are returned; an allocation failure
inside Core after beginning an optional semantic box can leave an empty box.

Rendering now submits a complete colored text paragraph through Core, wraps at
its current-frame parent width, and stacks hard paragraphs and empty lines.
CRLF is one break; LF, CR, NEL, paragraph separator and the Core information
separators also start new paragraphs. U+2028 starts a visual line while preserving
the full original bidi paragraph for shaping. Spans crossing either explicit or
automatic line boundaries are clipped and rebased to each draw command's original
UTF-8 paragraph. Blank and trailing lines contribute their configured line height.
`no_wrap` disables automatic wrapping while retaining explicit line breaks.
Masking occurs before line layout, so original paragraph separators become bullets
and reveal neither source text nor original newline structure.

The frozen Label source uses StyledText from its pinned gpui-pre 0.3.8 dependency.
That measured layout chooses the definite current parent width when normal white
space is enabled. The additive Core paragraph API supplies that available-width
behavior without relying on previous-frame label bounds. The prior single-line
and hard-separator rejection restrictions are removed.

Core now uses the pinned measured wrapping candidates, including punctuation,
Unicode word ranges and fallback inside long tokens. Trailing spaces remain in
the preceding visual row. Unicode 17 graphemes and provider shaping clusters
are kept intact during fallback; this deliberately strengthens the source's
individual-glyph fallback. See [the wrapping contract](UNICODE_WRAP.md) for the
exact policy, resource limits and measurement-only host limitations.
`alignment` supports physical left (default), center and right per visual row,
using current-frame available width. See [the alignment contract](TEXT_ALIGNMENT.md).
Arbitrary GPUI style refinement remains incomplete. Root-relative rem scaling
must be supplied through line_height when the application's root size differs
from 16. The legacy wrapper remains unchanged.

The general `cui_text_paragraph_colored` Core API validates all UTF-8 and full-text
color spans before publication. Invalid input and conservative storage/element
preflight failures publish no layout. Per-draw clipped spans require bounded frame
storage during final layout; exhaustion empties the draw list under Core's usual
capacity-error contract. The pre-existing unwrapped contextual APIs retain their
validation rules. `test_contextual_wrap_source` covers paragraph-relative spans,
Unicode separators, empty lines, current-frame reflow, measurement identity and
preflight failures. With `CUI_TEST_WRAP_RENDERER` it additionally renders colored
wrapped text through the real software font renderer and verifies a copied draw
snapshot after frame invalidation.

## Pinned Unicode provenance

Generated tables in `src/components/cui_label.c` derive from official Unicode 17:

- https://www.unicode.org/Public/17.0.0/ucd/UnicodeData.txt
  SHA256 `2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c`
- https://www.unicode.org/Public/17.0.0/ucd/SpecialCasing.txt
  SHA256 `efc25faf19de21b92c1194c111c932e03d2a5eaf18194e33f1156e96de4c9588`
- Vendored `third_party/unicode/17.0.0/DerivedCoreProperties.txt`, pinned by its
  adjacent manifest, supplies Cased and Case_Ignorable properties.

Generation reads field 13 (simple lowercase mapping) from UnicodeData, overrides
with unconditional SpecialCasing lowercase mappings, and extracts the two sorted
property range sets. Final_Sigma is implemented contextually; language-dependent
Turkish, Azeri and Lithuanian conditions are excluded by default lowercasing.

The following notice applies to these generated Unicode data tables:

UNICODE LICENSE V3

COPYRIGHT AND PERMISSION NOTICE

Copyright © 1991-2026 Unicode, Inc.

NOTICE TO USER: Carefully read the following legal agreement. BY
DOWNLOADING, INSTALLING, COPYING OR OTHERWISE USING DATA FILES, AND/OR
SOFTWARE, YOU UNEQUIVOCALLY ACCEPT, AND AGREE TO BE BOUND BY, ALL OF THE
TERMS AND CONDITIONS OF THIS AGREEMENT. IF YOU DO NOT AGREE, DO NOT
DOWNLOAD, INSTALL, COPY, DISTRIBUTE OR USE THE DATA FILES OR SOFTWARE.

Permission is hereby granted, free of charge, to any person obtaining a
copy of data files and any associated documentation (the "Data Files") or
software and any associated documentation (the "Software") to deal in the
Data Files or Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, and/or sell
copies of the Data Files or Software, and to permit persons to whom the
Data Files or Software are furnished to do so, provided that either (a)
this copyright and permission notice appear with all copies of the Data
Files or Software, or (b) this copyright and permission notice appear in
associated Documentation.

THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
THIRD PARTY RIGHTS.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE
BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES,
OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE DATA
FILES OR SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in these Data Files or Software without prior written
authorization of the copyright holder.
