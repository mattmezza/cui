# SDL V2 source checkpoint (unverified)

Source based on integrator decision04551753 and measured guard receipt24a19ff9.
Public support is not accepted or advertised from this checkpoint. No compiler,
native runtime, tests or canonical generation ran while root held the heavy lease.
Only source formatting and `git diff --check` have run.

The production source appends the measured two booleans, guards existing owner
entries/retained callbacks and implements native query plus common owned-frame
submission. V1 still makes one constructor calloc; no V1 cache or allocation path
was added. Query uses fresh owned-window/output dimensions, native advertised
RGBA32/sRGB and explicit limits. All source ranges exclude the owner before the
query busy guard. Common complete preparation precedes native texture creation,
upload/draw/Flush/Present attempt. State restoration attempts every setter and
poisons the owner on any failure; idle destruction remains possible.

Prepared CMake targets: `test_sdl3_v2` and `test_sdl3_v1_api`. The first compiles
current SDL source with native/allocator instrumentation and separately compiles
the frozen original SDL implementation with prefixed symbols. Frozen files/hashes
are unchanged; CMake changes only private include spelling in its binary copies.
The old offline whole-file equality check becomes old-field/constructor-count
compatibility because approved SDL production has started. Immutable authority
checks remain. No frozen expected pixels are generated from candidate source.

Pending gates include analytic rectangle RGBA, interleaved all-kind transport
against accepted CPU output (complete RGBA at upload; RGB at possibly opaque
window backbuffer), context/spans/gradient text, fractional/independent axes,
owner aliases before query callbacks, reentry through query/allocate/free,
preparation/native failures and retry, restoration faulting, and paired original
V1 pixels/cache/allocation counts. These fixture sources have not yet compiled;
first failures must be recorded and diagnosed without weakening expectations.
Native hidden-window execution must use scripts/test_local.py/private Xvfb.
