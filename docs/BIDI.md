# Bidirectional text analysis

`cui_bidi_resolve` resolves one UTF-8 paragraph as one unwrapped line using
vendored SheenBidi 3.0.0, commit
`cfe430e7375a7845b679adae9d51dac6deaa8858` (Apache-2.0).
The upstream [release](https://github.com/Tehreer/SheenBidi/releases/tag/v3.0.0)
updates its property tables to Unicode 17.0.0. Vendored files are unmodified;
`third_party/sheenbidi/manifest.json` records their SHA-256 checksums.

The engine applies paragraph detection/base levels, explicit embeddings and
isolates, weak types including numbers, paired brackets, neutrals, implicit
levels, line resets, and visual run ordering. The wrapper returns logical scalar
levels after L1, visual runs, a visual-to-logical scalar permutation, and
mirrored code points where odd-level characters have a Unicode mirror mapping.
It follows [Unicode 17 UAX #9 revision 51](https://www.unicode.org/reports/tr9/tr9-51.html).
Explicit direction overrides P2/P3; auto modes choose the requested fallback
when no outside-isolate strong character determines paragraph direction.

All offsets are Unicode scalar indices, not UTF-8 byte positions. Strict UTF-8
validation rejects overlong, truncated, surrogate, and out-of-range encodings.
Input containing more than one paragraph is rejected atomically. Empty input
succeeds. A final paragraph separator is accepted. Control characters, including
X9-removed characters, remain in the output permutation; consumers must suppress
formatting controls when painting. Odd-level runs are read from their logical
end toward their start. Mirrored output is analysis data; choosing the corresponding
mirrored glyph still belongs to the shaping engine.

Callers provide disjoint output buffers and a scalar decoding workspace. Each
capacity must be at least the UTF-8 byte length, a conservative bound sufficient
for every scalar/run. A capacity error, invalid input, or backend allocation
failure leaves all outputs and the result unchanged; workspace is scratch and
may change. Input and arrays must be valid for their declared sizes. The backend
uses transient heap objects, so the workspace contract does not promise an
allocation-free algorithm. The wrapper never changes SheenBidi's global allocator.
Do not change that global allocator concurrently with processing. Upstream
reference counts and scratch allocation are thread-safe by default; scratch
buffers may be retained in the backend's global pool after a call.

The headless test accepts paths to the pinned Unicode `BidiCharacterTest.txt`
and `BidiTest.txt`. It checks every expected level and visual position after
filtering X9-removed characters. The full suites pass 91,707 character cases
and 770,241 property/direction cases. Property tests use one representative
Unicode scalar per bidi class as permitted by the Unicode test instructions.
They cover isolates, embeddings, numbers, neutral/bracket resolution and deep
nesting. Additional checks cover malformed UTF-8, conservative capacities,
atomic failure, paragraph rejection, empty input and mirroring.

This API does not yet connect to rendering, font fallback, shaping, grapheme
cluster ordering (L3), wrapped line layout, caret navigation, hit testing, or
selection. In particular, reversing scalar indices in a run is not sufficient
to render a cluster correctly: shaping and cluster handling must follow. No
claim of complete international-text rendering parity is made.

CMake builds this as the optional `cui_bidi` static library by default. Disable it
with `-DCUI_BUILD_BIDI=OFF`; Core does not link it implicitly. Installed consumers
can use `pkg-config --cflags --libs cui-bidi`. Its archive contains the private
backend objects, so consumers do not need a separate SheenBidi library.

For manual integration compile `Source/SheenBidi.c` with `SB_CONFIG_UNITY` and private
include paths `third_party/sheenbidi/Headers` and `third_party/sheenbidi/Source`.
Only the CUI header is public. Preserve upstream notices and LICENSE in bundles.
