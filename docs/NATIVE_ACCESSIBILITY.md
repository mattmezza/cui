# Optional Linux native accessibility, phase 1

This backend publishes completed CUI semantic snapshots through pinned AccessKit
and executes targeted Focus/Click through Core's accessibility action queue.
The automated Linux X11 test uses a real SDL3 window and independent libAT-SPI
client over DBus. This is transport and action coverage, **not** full native
accessibility, editable-text support, or an Orca/physical screen-reader claim.
Windows/macOS adapters and Wayland screen extents are not implemented here.

## Build and use

Ordinary builds keep `CUI_BUILD_ACCESSIBILITY=OFF` and do not inspect the cache,
invoke Rust or fetch dependencies. Explicitly prebuild the pinned archive with
Rust 1.85+/Cargo, then enable the optional Linux adapter:

```sh
python3 scripts/build_accesskit.py
cmake -S . -B build -G Ninja -DCUI_BUILD_ACCESSIBILITY=ON
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure -R accessibility
cmake --install build --prefix /your/local/prefix
```

CMake imports the existing archive; it never invokes Cargo or downloads a missing
dependency. Missing or mismatched archive, lock, ownership extension or private
header fail configuration. The binary SHA is recorded in installed
`share/cui/licenses/accesskit/accesskit-build.json`; it identifies the local
artifact, rather than claiming a reproducible upstream binary. Run the pinned
builder before configuring to establish its provenance. This is a host-built
Linux package; cross compilation is rejected. Native runtime tests additionally
require SDL3/Freetype/HarfBuzz, libAT-SPI, DBus/gdbus and Xvfb. CMake reports when
those tests are omitted, while the portable snapshot/action tests remain available.

The standalone harness also remains available via
`cmake -S backends/accesskit -B build-accesskit -G Ninja`. It is a validation
harness rather than the installation/package entry point.
Use `--cache PATH` and `-DACCESSKIT_CACHE=PATH` together for another private cache.
Nothing installs globally. The script verifies the archive, every cached source
file and Cargo.lock, and uses `cargo build --release --locked`. The private
ownership module and exact entry-point extension have separate hashes in
`backends/accesskit/extension.json` and the cache's `cui-extension.json`.
See [dependency metadata](../third_party/accesskit/README.md) for licenses and
binary-distribution notice requirements.

Create one `cui_accessibility` per window. On the UI thread:

```c
cui_accessibility *a = cui_accessibility_create((cui_accessibility_config){0});
/* Each frame, before begin_frame: */
cui_accessibility_drain(a, ui);
/* poll host input, begin/build/end the ordinary CUI frame */
cui_accessibility_window window;
if (cui_sdl3_accessibility_window(host, &window))
    cui_accessibility_update(a, ui, window); /* after end_frame */
/* On close, before destroying host/callback user: */
cui_accessibility_destroy(a);
```

Publish **every completed frame**, including removal frames. `update` rejects
Core errors and invalid UTF-8/NUL/nonfinite metadata atomically. Core errors are
sticky: handle them and call `cui_clear_error` according to Core's existing
contract. Never call update during a frame or race public operations with destroy.
The SDL helper only returns portable metadata; it adds no Rust/native bridge
linkage to the SDL library. Its borrowed title is copied by update. Other hosts
supply equivalent title/focus/outer/inner metadata themselves. Window coordinates
and semantic bounds must use the same logical units. X11 1x is tested; mixed-DPI
and decoration behavior require additional platform validation.

## Ownership, generations and actions

Defaults allocate two banks of up to 2048 semantic records and 65536 string bytes
per bank, plus 128 requests. Callers can configure larger bounds before create.
Creation returns null on size overflow/C allocation failure. `stats.native_active`
reports observed native activation; successful creation alone does not prove a
working desktop accessibility bus/client. Snapshot overflow
preserves the previous tree/tokens and reports `CUI_A11Y_CAPACITY`; it never
truncates. Names/descriptions/title are deep copied, with no retained Core pointer.
Unchanged snapshots skip native tree allocations. Published changes currently
use full bounded tree updates; record/relationship lookup is linear, so large
trees should be profiled before increasing the default limit. AccessKit/Rust has
its own allocation behavior; native allocator exhaustion can abort and is not
covered by CUI's recoverable C allocation contract.

A native token is never reused within a bridge. Removing/reintroducing an ID or
changing its role generates a new token, so old queued requests cannot activate
its replacement. Applications must change the CUI ID for a same-frame logical
replacement with the same role; Core exposes no identity stronger than ID/role.
Skipping removal snapshots would also erase this distinction. Public token and
request helpers use the same bounded mailbox as native callbacks, useful for
portable host adapters and tests.

Foreign-thread AccessKit handlers only read the locked copied snapshot and enqueue
Focus/Click tokens. They never call Core or SDL. The optional wake callback must
be thread safe, must not reenter this backend/wait for the UI thread, and must not
call Core/SDL. Main-thread drain revalidates token and Core capability, discards
stale/disabled/modal-obscured requests, and preserves unqueued requests on Core
backpressure. Invalid/stale requests never enter Core. Core queue capacity errors
retain Core's normal observable/sticky error contract. Multiple Click requests
remain individual Core events; the existing boolean interaction API may coalesce
same-control activations within one frame.

`destroy` closes the mailbox and disables wake synchronously. Pinned Unix adapter
removal is asynchronous: the private Rust Arc ownership wrapper retains C storage
until the last activation/action/deactivation handler drops, then releases it once
on that thread. Callers may destroy Core/window/wake userdata after public destroy
returns. No process-lifetime tombstone pool or dangling frame strings are used.

## Mapping and limits

All current CUI roles map explicitly to AccessKit roles. Generic containers can be
flattened by the platform. The tree includes parent/child order, names and copied
descriptions, checked/selected/expanded/disabled/readonly/required/busy/invalid
states, orientation, numeric range metadata, collection positions/table indices,
scroll metadata, live-region policy, focus and window-relative semantic bounds.
Static Label names use AccessKit's label-value convention. `controls` and
`labelled_by` map to native-node references; missing targets are omitted and parent
cycles are rejected. This is metadata mapping, not a claim that every property is
exposed identically by AT-SPI. Core does not expose an OS modal marker or clipping
metadata: the navigation tree retains behind-modal nodes and full semantic bounds;
Core suppresses their actions. OS modal tree hiding/precise clipping need a later
portable semantic extension.

Only Core-supported Focus/Activate are advertised. Focus changes Core focus;
OS window focus remains host metadata, and this bridge does not raise windows. Button, Checkbox, Switch,
Radio, Link and eligible menu items use the ordinary activation path. Tab/Option/
Tree/table compound reducers have conservative Focus-only support; merely showing
a role does not enable its domain action. There is no SetValue, editable text,
selection/caret, document text ranges, scroll action, or increment/decrement bridge.
Read-only scalar/static-label metadata does not imply native Value/Text editing.
See [Core action contract](ACCESSIBILITY_ACTIONS.md).

Pinned `accesskit_atspi_common` 0.20.0 exports `controls` as ControllerFor; its
`relation_set` does not export LabelledBy. The tree stores labelled-by references,
but an AT-SPI LabelledBy claim would be false. Installed libAT-SPI 2.62.1 also
warns about AccessKit's cache signal signatures; direct query/actions pass after
explicit cache refresh. Cache/screen-reader compatibility remains a limitation.

Primary provenance: [accesskit-c pin](https://github.com/AccessKit/accesskit-c/tree/0824c4a1e3a4d13ce5582df20e394fba49485a15),
[Unix adapter ownership](https://docs.rs/accesskit_unix/0.23.0/src/accesskit_unix/adapter.rs.html),
[AT-SPI relationship mapping](https://docs.rs/accesskit_atspi_common/0.20.0/src/accesskit_atspi_common/node.rs.html),
[SDL window bounds](https://wiki.libsdl.org/SDL3/SDL_GetWindowBordersSize).

## Validation

Headless tests cover copied-string lifetimes, unchanged frames, UTF-8 rejection,
atomic string/node capacity failure, configured larger capacity, generations,
missing/stale/disabled/modal targets, invalid actions, threaded mailbox overflow,
Core backpressure, exactly-once drain and boolean coalescing. Private counters
check release exactly once after repeated create/destroy and async native teardown.
The real native test queries roles, screen extents, ControllerFor target and checked
state; requests Component Focus/Action Click and observes actual Core effects;
checks disabled actions and removed/reintroduced nodes. It holds a real native
action pending during destruction and verifies wake runs off the UI thread. The frame label is
intentionally overwritten immediately after snapshot publication.

```sh
cmake -S backends/accesskit -B build-accesskit-asan -G Ninja -DCUI_SANITIZE=ON
cmake --build build-accesskit-asan
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-accesskit-asan --output-on-failure
ASAN_OPTIONS=detect_leaks=0 backends/accesskit/run_native.sh ./build-accesskit-asan/test_accessibility_native
```

C Core/backend/SDK code is ASan/UBSan instrumented; the pinned Rust release
library is not. Native leak detection is disabled for host libraries; C backend
release counters remain checked. The runner isolates DBus/Xvfb and uses in-memory
GSettings, never changing desktop accessibility settings. It starts the installed
registry explicitly because service activation may require a systemd user manager.
Installation includes the portable accessibility header, production adapter,
private static AccessKit archive, CUI Core, AccessKit C/Rust notices and the
aggregate inventory of all ninety pinned Linux dependency notices. Private
test hooks and the upstream AccessKit header are not installed.

A relocated installation supports either:

```cmake
find_package(CUIAccessibility CONFIG REQUIRED)
target_link_libraries(app PRIVATE CUI::cui_accessibility)
```

or `pkg-config --static --cflags --libs cui-accessibility`. CMake links Core,
the imported Rust archive, Threads, dl and m transitively; pkg-config includes
the static dependency flags. Set `CMAKE_PREFIX_PATH` or `PKG_CONFIG_LIBDIR` to
the installation. Neither interface references the original private cache.

`tests/packaging/test_accessibility_package.py --build build` installs into a
temporary prefix, relocates it, compiles/runs strict C11 consumers through both
interfaces, checks real Focus/Activate effects, all ninety notices, artifact SHA
and absence of private test symbols. It also builds ordinary Core with a
nonexistent cache while the option is OFF and checks missing/corrupt-cache
configuration failures. The optional root CTest suite registers this proof.
Full native runtime coverage still has the phase1 limits documented above.
The reserved distribution check may remain stale until integration regenerates
the global bundles.
