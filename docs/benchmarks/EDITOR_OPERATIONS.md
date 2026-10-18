# Headless Editor operations benchmark

`tests/bench_editor_operations.c` exercises the actual styled `cui_editor`,
including its default line-number gutter and a one-token-per-visible-line
highlight callback. The older `tests/bench_editor_200k.c` remains a separate
Textarea microbenchmark. Neither establishes full editor parity.

Reproduce from the repository root (no native window or display is used):

```sh
cmake -S . -B /tmp/cui-editor-bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUI_BUILD_NATIVE=OFF
cmake --build /tmp/cui-editor-bench --target cui cui_software -j 2
cc -std=c11 -O2 -UNDEBUG -Wall -Wextra -Wpedantic -Iinclude \
  tests/bench_editor_operations.c /tmp/cui-editor-bench/libcui.a \
  /tmp/cui-editor-bench/libcui_software.a -lm -o /tmp/cui-editor-operations
/tmp/cui-editor-operations > /tmp/editor-operations.csv
```

The library uses the compiler's CMake Release flags (GCC `-O3 -DNDEBUG` in
this run); benchmark assertions stay enabled. Each size creates a fresh context
and 2K, 20K, or 200K LF-terminated 80-byte ASCII lines, without wrapping, with
an 800x400 viewport. The largest document contains 16,000,000 bytes. Caller row
storage is 11,200,056 bytes on this 64-bit build, plus 16,000,064 document buffer
bytes. These are exact caller capacities, **not** total memory or allocation
counts: model/context/history/frame allocations are not instrumented.

CSV records process CPU milliseconds from C11 `clock`, text-measurement and
highlight callback counts, and generated draw-command counts. The measurement
provider returns seven pixels per byte; it does not shape or rasterize text.
Timing covers begin-frame through end-frame, including event handling. Event
queuing and programmatic selection setup occur outside the timed interval.
Cold layout is sampled once per document; each hot workload has eleven
samples, except alternating middle edits (five) and tail edits (six). Each
insert is followed by Backspace to restore document length. Assertions verify
layout row counts, scroll movement, keyboard navigation, and edited bytes.
`pointer_move` measures ordinary editor pointer dispatch; no language-server
hover provider is installed. No elapsed-time pass/fail threshold is imposed.

Reference run: 2026-10-18, Linux x86_64, Intel Core Ultra 5 235U, GCC 16.2.1,
source base `81a8bd3a` plus this benchmark. Other engineering builds were active;
CPU timings and spread remain sensitive to frequency, cache and contention.
Raw samples are in [editor-operations-2026-10-18.csv](editor-operations-2026-10-18.csv).
Median CPU milliseconds (cold is a single sample):

| Operation | 2K lines | 20K lines | 200K lines |
| --- | ---: | ---: | ---: |
| Cold layout | 2.367 | 11.614 | 97.496 |
| Idle | 0.184 | 0.196 | 0.177 |
| Programmatic caret at tail frame | 0.180 | 0.188 | 0.157 |
| Actual Left key near tail | 5.249 | 50.912 | 454.884 |
| Wheel scroll | 0.183 | 0.196 | 0.212 |
| Pointer move | 0.172 | 0.176 | 0.170 |
| Tail insertion | 0.395 | 2.405 | 21.938 |
| Tail removal | 0.410 | 2.580 | 20.937 |
| Middle insertion | 0.637 | 4.656 | 45.987 |
| Middle removal | 0.641 | 4.703 | 45.035 |

At 200K lines the eleven Left-key samples span 326.947–509.495 ms. This
workload exposes document-size-dependent keyboard/edit cost even though steady
layout, scroll, and pointer frames remain nearly constant. It does not justify
an interactive 60 Hz claim. Investigate model grapheme navigation and document
validation/copying before attributing these costs to viewport painting. Cold
layout and middle edits still process substantial document content. Actual font
providers, Unicode, long lines, wrapping, folds, multiple selections, search,
IME, renderer execution and GPU presentation need separate measurements.

## Validated ASCII model-motion change

The follow-up change routes Edit model grapheme motion through the existing
validated-buffer helpers already used for grapheme erasure. Adjacent ASCII
scalars admit a local answer, with CR/LF handled as one cluster. Non-ASCII
context still uses the complete Unicode grapheme scan. The standalone public
`cui_grapheme_*` functions retain whole-input validation and their behavior for
malformed UTF-8 and arbitrary offsets. This does not introduce a Unicode
backward scan or a new segmentation algorithm.

The same executable source, flags, document sizes and machine produced these
Left-key medians after the change: 0.116 ms at 2K lines, 0.179 ms at 20K, and
0.182 ms at 200K (200K range 0.165–0.193 ms). Raw samples are in
[editor-operations-ascii-motion-2026-10-18.csv](editor-operations-ascii-motion-2026-10-18.csv).
These sequential runs are illustrative, without a controlled machine load;
the removed whole-document scan is also evident from the code path. Tail and
middle insertion medians at 200K remain 21.946 and 43.101 ms. Unicode navigation
and ordinary edits can still scale with document size.

`test_model_grapheme_motion` compares model previous/next motion against the
public segmenter at every scalar offset of every pinned Unicode grapheme test
case, surrounded by ASCII context. It includes offsets within clusters,
extended selections, and CR/LF selection collapse. Existing malformed-input
and byte-interior public API tests remain in force.
