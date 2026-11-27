# Measured performance

Development host: Intel Core Ultra 5 235U (14 logical CPUs), Arch Linux x86_64 kernel7.2.8, GCC16.2.1, default unoptimized CMake build. `./build/cui_bench`:500 frames composing100 boxes+text at800×600, Clay culls outside viewport;28000 visible draw commands total. CPU `clock()` measurement:98.079ms total /0.196ms per composed frame in the initial run. This is CPU layout/draw-list generation, not end-to-end GPU frame time, and excludes renderer/text shaping and OS scheduling. Reproduce with `make bench`; numbers are not a comparison to other frameworks.

SDL_Renderer has bounded persistent glyph textures (256 LRU entries,4MiB coverage budget), text tint reuse tested. SDL_GPU has a1024×1024 grayscale atlas with4096 cached masks and32768 per-frame quad entries; uploads only after newly cached glyph masks. Atlas exhaustion returns failure instead of claiming successful output. GPU batching/regression thresholds and workload-specific measurement remain pending.

## Native DataTable inbox

The SDL software fixture keeps the viewport and columns constant and moves
the pointer between rows halfway through a 1,000- or 10,000-message inbox.
Each measured frame processes two motion events and checks that the last one
determines the highlighted row in the submitted draw list. Both model sizes
call the renderer for 30 virtual cells and emit 166 draw commands per frame.
One native wheel tick then moves the inbox by 48 logical pixels.

After those timed frames, the test also prepends a new message while the
viewport starts halfway through a row. It checks that the selected key and
its cell's vertical position and keyboard focus stay fixed, that the latest pointer sample
highlights the correct key on the rebind frame, and that Down moves selection
to the following message. The cell callback still runs only within the
viewport and its overscan. The timings below exclude the rebind.

Private X11 with SDL 3.4.18, SDL software rendering, vsync off, Inter, GCC
16.2.1 Release, and one CPU on the Core Ultra 5 235U. Measured 90 warm frames
per size. Values are p50 / p95 wall time in milliseconds.

| Rows | SDL poll | CUI composition | SDL software render and present | Total |
| ---: | ---: | ---: | ---: | ---: |
| 1,000 | .009 / .016 | .166 / .205 | 4.737 / 4.883 | 4.909 / 5.087 |
| 10,000 | .008 / .015 | .161 / .201 | 4.744 / 4.911 | 4.918 / 5.102 |

Timing starts just before SDL polling and ends when software submission and
present return. Physical input, scanout, display latency, model binding and
sorting fall outside that interval. This workload submits a virtual window
and reuses the bound model throughout. The test asserts work counts and
same-frame hover. Timings are diagnostic and vary across hosts.

```sh
cmake --build build --target test_sdl_inbox_hover
python scripts/test_local.py --build-dir build -- -R '^sdl_inbox_hover$' -V
```

### Retained C DataTable snapshots

The optional bridge setter owns a copy of every submitted cell and builds
key indexes for the actual row count. Replace the snapshot when data changes
and reuse it across frames. A requested capacity of 10,000 with zero rows
allocates only the empty matrix and a minimal key index. The C bridge
allows 10,000 rows per opted-in table and 32 MiB of retained allocation
requests per app, including copied strings, metadata and Base indexes.
Allocator overhead and caller-owned inputs are excluded.

This fixture uses 128 ASCII bytes per cell, 2 or 16 columns, a 340×160
logical pixel table, GCC 16.2.1 Release and one CPU on the Core Ultra 5
235U. It measures mean CPU time for allocation, validation, copying,
indexing and replacement, and separately measures a steady headless
bridge step with no renderer. The allocation probe counts requested bytes
and observes both matrices while replacing one. Semantic cell counts
include overscan. The same source string is reused in the input fixture.

| Setter | Rows × columns | Replace | Steady step | Retained | Peak | Semantic cells |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Ordinary keyed | 32 × 2 | 13.2 µs | 73.0 µs | 13,670 B | 27,340 B | 10 |
| Capacity | 32 × 2 | 12.8 µs | 72.7 µs | 13,670 B | 27,340 B | 10 |
| Capacity | 1,000 × 2 | 394.2 µs | 72.8 µs | 388,854 B | 777,708 B | 10 |
| Capacity | 10,000 × 2 | 4.23 ms | 72.6 µs | 4,269,894 B | 8,539,788 B | 10 |
| Ordinary keyed | 32 × 16 | 99.7 µs | 122.1 µs | 78,728 B | 157,456 B | 20 |
| Capacity | 32 × 16 | 100.2 µs | 122.6 µs | 78,728 B | 157,456 B | 20 |
| Capacity | 1,000 × 16 | 3.26 ms | 122.6 µs | 2,418,952 B | 4,837,904 B | 20 |
| Capacity | 10,000 × 16 | 34.76 ms | 124.5 µs | 24,569,992 B | 49,139,984 B | 20 |

The largest wide replacement took 34.76 ms on this host. For data that
changes on every frame, consider the C11 [borrowed model](TABLE.md#virtual-datatable)
and its row-anchor helpers. Use the bridge setter for bounded owned snapshots.
This workload also sweeps allocator failures at every replacement allocation.
The ordinary keyed and opt-in rows above use the same current bridge build.

~~~sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUI_BUILD_BRIDGE=ON
cmake --build build --target test_bridge_data_table_capacity_allocations
CUI_TEST_DATA_TABLE_BENCHMARK=1 python scripts/test_local.py --build-dir build \
  -V -R '^bridge_data_table_capacity_allocations$'
~~~

## Core hover with individually submitted rows

`cui_bench_hover` submits every row into a 500×480 clipped box. It moves the
pointer across the visible rows and verifies exactly one matching ID after
the first frame. It queries all submitted IDs against the prior snapshot
before composing each new frame. This measures CPU time without a renderer.

GCC 16.2.1 Release, one CPU on the Core Ultra 5 235U, 25 measured frames
after the initial frame. Times are p50 / p95 in milliseconds.

| Rows | Hover before | Hover after | Whole frame before | Whole frame after |
| ---: | ---: | ---: | ---: | ---: |
| 100 | .010 / .011 | .002 / .002 | .101 / .108 | .092 / .103 |
| 1,000 | .697 / .709 | .010 / .014 | 4.061 / 4.120 | 3.367 / 3.807 |
| 4,000 | 10.701 / 11.012 | .038 / .041 | 56.922 / 58.750 | 46.340 / 47.675 |

The first query scans the clipped semantic snapshot for the pointer location.
Subsequent queries reuse up to 32 overlapping semantic indices. More than
32 overlaps can still require a scan for the requested ID. The foremost
focusable paint root is found across all overlaps. The cache uses 296 bytes
per context on the measured 64-bit build and makes no heap allocations.

### Box submission and semantic ancestry

Using the same workload, the next change replaces repeated scans of submitted
box IDs with a per-frame set keyed by the exact layout identity. Named boxes
still reject duplicate and aliased IDs. While resolving the semantic snapshot,
Core records the first matching semantic for each painted box. Parent lookup
can then walk actual layout ancestors directly.

| Rows | Submission before → after | End frame before → after | Whole frame before → after |
| ---: | ---: | ---: | ---: |
| 100 | .079 / .091 → .044 / .051 | .050 / .057 → .031 / .040 | .133 / .155 → .079 / .101 |
| 1,000 | 2.585 / 2.738 → 1.119 / 1.143 | .815 / .865 → .310 / .329 | 3.425 / 3.605 → 1.440 / 1.486 |
| 4,000 | 36.711 / 38.376 → 12.516 / 12.916 | 9.811 / 10.258 → 1.398 / 1.585 | 46.652 / 48.380 → 13.992 / 14.520 |

These are separate before/after runs on the same host and settings. The box ID
set uses the same eight bytes per configured element as the previous ID array.
Core clears the set after a frame with boxes, so reset work scales with the
configured element limit. The semantic owner field fits in existing box
metadata padding on the measured 64-bit layout.

Core still composes all submitted rows. The remaining CPU time and actual
rendering time depend on widget count and content. Use a virtualized component
for long lists when the layout permits it. The native DataTable workload above
also includes SDL rendering and presentation.

```sh
cmake --build build --target cui_bench_hover
./build/cui_bench_hover
```

## Hosted check scheduling

Portable CI runs at most two CTest processes at once and sets
`CMAKE_BUILD_PARALLEL_LEVEL=2` for nested consumer builds. Each test uses its own
process and temporary consumer directories; reference fixtures remain read-only.
On Linux, use `python scripts/test_local.py --build-dir build` to isolate the
display for local tests.

The alpha.12 portable Ubuntu Release job took 1,026 seconds, of which 933 were
compilation and 83 were CTest. The next candidate uses four workers for this
job's primary compile on the four-core public Ubuntu runner. The other portable
builds use two workers. Use the hosted compile and CTest step durations for the
comparison. Candidate timing is pending. [Alpha.12 job](https://github.com/mattmezza/cui/actions/runs/45905181988/job/140200121683) · [Runner specifications](https://docs.github.com/en/actions/reference/runners/github-hosted-runners#standard-github-hosted-runners-for-public-repositories)

The baseline Windows Debug job in [run 41249695765](https://github.com/mattmezza/cui/actions/runs/41249695765)
spent 165.88 seconds on `agent_skill_contract` and 98.43 seconds on
`binding_package_source_manifest`. Bounded overlap targets this serial cost;
these timings are a baseline, not measured savings from the scheduling change.
Hosted duration, memory use and any contention regressions still need comparison.

The installed-bindings job validates the embedded and source-only Go artifacts
in independent clean consumers. The same Rust crate is built and exercised once
alongside the embedded Go artifact. The runner still checks that the installed
executables work after removing build sources and blocking compiler tools.
Hosted timing for this change is pending.

### Release tag scheduling

The two alpha.6 version tags triggered six additional workflow runs after the
same commit passed the main-branch release checks. GitHub reported 38 started
jobs and 13,015 seconds (about 217 minutes) summed across job start-to-finish
times. This sum includes successful and failed jobs and measures neither wall
time nor billed minutes. The build and evidence runs alone accounted for 12,213
seconds. [Library-tag build](https://github.com/mattmezza/cui/actions/runs/45734713874)
· [Go-tag build](https://github.com/mattmezza/cui/actions/runs/45734713599)
· [Library-tag bindings](https://github.com/mattmezza/cui/actions/runs/45734713629)
· [Go-tag bindings](https://github.com/mattmezza/cui/actions/runs/45734713625)
· [Library-tag Speech](https://github.com/mattmezza/cui/actions/runs/45734713957)
· [Go-tag Speech](https://github.com/mattmezza/cui/actions/runs/45734713556)

Branch pushes and pull requests continue to qualify source. The build,
native binding and optional Speech workflows now filter push events to branches.
Speech keeps its existing path filter, and bindings and Speech remain available
through manual dispatch. Pages already builds and deploys on main pushes.
Both alpha.7 tags resolved to the verified main revision. GitHub reported zero
tag-triggered push runs when checked 68 seconds after the push.
[Alpha.7 scheduling receipt](validation/alpha7-release.json)

## Browser probe transport

Eleven standalone headless browser probes use `route_wasm_probe` from
`tests/web/wasm_probe.py`. The helper serves the compiled reactor as
`application/wasm` through Playwright request interception.
Create the Chromium page with `offline=True` and pass the returned URL to
`WebAssembly.compileStreaming(fetch(url))`. The page receives the exact
local file. This avoids expanding each byte into a Playwright argument.

One Linux comparison used the same 5,954,126-byte Popover appearance reactor,
Zig 0.16.0, Playwright 1.58.0 and one CPU at nice 19. Each run opened a fresh
headless browser and passed the same eight Canvas cases. Timings exclude Zig
compilation. Peak RSS comes from Linux `getrusage`. The child column reports
the largest peak for a reaped child.

| Transport | Elapsed | Python peak RSS | Child peak RSS |
| --- | ---: | ---: | ---: |
| Playwright integer array | 70.893 s | 2,666,500 KiB | 1,413,528 KiB |
| Intercepted binary fetch | 2.133 s | 72,392 KiB | 257,576 KiB |

Run `uv run tests/web/test_popover_appearance.py --output build/popover-appearance-probe`
for the end-to-end fixture. The local comparison isolates the browser phase.
Hosted shard timings still need measurement.

## Generated output timestamps

The SDK, amalgamation and agent-skill generators compare generated bytes with
existing files before writing. Identical generation preserves file modification
times, avoiding timestamp-only invalidation of C/C++ consumers, Go wrapper
translation units and bundled SDK sources. This is not a build cache: generation
and freshness checks still compute their outputs, and changed/missing files are
written normally. SDK provenance is still recalculated; a changed revision or
source-diff receipt changes its bytes and therefore its timestamp.

Previously, all SDK wrappers/sources and all distribution headers were rewritten
even when identical. Running the canonical sequence after a documentation-only
change could therefore mark both native header consumers and foreign SDK
consumers dirty independently. Skill resources were also always rewritten,
although that alone does not establish a native compile dependency. No rebuild
count or elapsed-time saving is claimed without a measured build comparison.

Run the small offline entrypoint regression with
`python tests/test_generator_writes.py`. It uses temporary mini-repositories and
checks fixed nanosecond timestamps, output repair, changed input, read-only
`--check` failures and existing stale-file policy. It performs no compilation,
dependency installation or canonical generation. SDK obsolete files/wrappers
still require explicit review; amalgamation still leaves unrelated files alone.
Use the normal SDK → amalgamation → skill sequence and freshness checks when
updating actual generated resources.
