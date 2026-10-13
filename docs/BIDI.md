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
cluster ordering (L3), wrap-position selection, caret navigation, hit testing, or
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

## Reusable paragraph and wrapped-line analysis

`cui_bidi_paragraph_create` analyzes the complete paragraph through I2 once,
copying decoded scalars into an owned immutable object. The caller can reuse both
input and decoding workspace immediately. `cui_bidi_paragraph_destroy` releases
the object and backend references; NULL destruction is accepted. No vendor type
appears in the CUI public API. Creation leaves the output pointer unchanged on
invalid UTF-8, insufficient workspace, multiple paragraphs, or allocation failure.

`cui_bidi_paragraph_levels` returns the paragraph levels before L1, scalar count,
and paragraph base. `cui_bidi_paragraph_line` takes a nonempty scalar range inside
that paragraph and applies L1/L2 to that line using the original paragraph
analysis. The caller chooses ranges after determining wrap positions. A Latin
continuation of a Hebrew paragraph consequently retains the Hebrew base;
resolving the continuation alone with auto direction would change that base.
Explicit embeddings and isolates crossing a wrap retain their paragraph context.
Line-local trailing whitespace resets to the paragraph base under L1.

Line output levels use indices relative to the requested range. Mapping entries
and run logical starts remain absolute paragraph scalar indices. Result scalar
count is the requested line length. Output capacity needs only that scalar
length, and every output and result remains unchanged on any failure. Paragraph
level output capacity needs the paragraph scalar count. Empty paragraphs are
valid objects but have no nonempty line range. Neither function mutates the
paragraph, so independent output buffers can be used concurrently, provided
no thread destroys the object or changes the backend global allocator.

Tests compare paragraph levels and every wrapped-line run, level, mirror and
visual scalar position with the pinned upstream paragraph/line API, across
Hebrew, Arabic, numbers, paired brackets, an isolate spanning wraps and explicit
embeddings. They also verify the L1 trailing-space reset, retained RTL base,
caller-buffer independence, bounds including SIZE_MAX, insufficient capacities,
empty objects and atomic preservation of every output buffer.

This adds wrapped-line bidi analysis, not a complete wrapping or shaping system.
Offsets are scalars, and callers must choose boundaries between whole grapheme
and shaping clusters. The API does not enforce those boundaries or apply L3.
Shaping, font fallback, painting, caret/selection and hit testing remain separate
integration work; no rendering parity is claimed.
