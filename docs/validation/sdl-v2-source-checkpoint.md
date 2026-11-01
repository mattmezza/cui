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

## Source-local text capability correction

Root review identified that CMake-only CUI_HAS_TEXT would leave direct-source and
native-bundle SDL builds advertising text while compiling a no-text common painter.
SDL's V1 source already unconditionally requires the text provider. The source now
locally enables CUI_HAS_TEXT before its common painter include; the SDL target's
new private CMake flag is removed. This preserves existing SDL text prerequisites
rather than requiring undocumented manual flags. The actual SDL library target
therefore exercises the default source path at its next strict build. Manual/native
bundle source closure remains an explicit acceptance gate; no such build has run.

## Additional source review before first compilation

The installed SDL3 public renderer header exposes texture RGB/alpha modulation
and renderer color scale, but no `SDL_SetTextureColorScale`. Removed that erroneous
call before compilation; white RGB/alpha modulation plus renderer color-scale one
already establish the proposed transfer semantics. This is a source-discovered
API error, not a claimed compiler diagnostic or completed runtime correction.

First configured microgate, only after the explicit lease, is:

```sh
taskset -c 1 nice -n 19 cmake --build build-shadow --target test_sdl3_v2 test_sdl3_v1_api -j1
python scripts/test_local.py --build-dir build-shadow -R '^sdl3_v(2|1_api)$'
```

Inspect dependency dry-run first: if the reused worker configuration would rebuild
Core/text broadly, use the already built immutable current archives for the two
fixture TUs and public ABI fixture, recording their source identities and hashes.
No full-engine build is authorized by this microgate. Capture initial strict
compiler and private-Xvfb logs; neither success nor fixture acceptance is claimed.

The paired same-TU fixtures explicitly select SDL_MAIN_HANDLED and call
SDL_SetMainReady in their own test host. This avoids two embedded implementations
emitting platform entry shims on targets where SDL_main.h supplies them; it does
not alter production SDL entry handling or frozen source bytes. A second fixture
source review corrected text gradient endpoint member names to actual x0/y0/x1/y1.
These are precompile source corrections, not runtime passes.
