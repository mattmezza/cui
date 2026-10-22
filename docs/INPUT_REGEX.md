# Optional Input regex acceptance

The portable matcher is an opt-in synchronous whole-value gate in `cui_edit_apply_checked` and Base/styled Input `.matcher`. A zero descriptor preserves existing behavior and requires no Rust. Public `cui_text_match.h` contains only C11 types; any host/provider can implement its by-value begin/match/end operations.

The source contract is gpui-component0.7.1 at `87d10ae5e1299d1be18670a37570c5402f952e34`: nonempty single-line candidate predicates run custom acceptance, token/number validity, then regex boolean matching, with short circuits. Empty values pass without predicates. A failing candidate rejects only if the previous visible value passes; an invalid old value can heal. Hard provider errors never become a false match or healing permission. Final formatting and secure presentation resource preparation happen before the lease or custom callback. IME UPDATE gates normalized unformatted composed text; COMMIT formats once and creates one Undo step. Replay, PRESENT, START/CANCEL and multiline edits do not invoke matching. Host COMMIT rejection ends the host session through the existing Base lifecycle; engine checked rejection preserves the prior prepared session.

A nonempty candidate acquires one lease before the custom predicate; candidate and old evaluation share it. End runs exactly once before adoption, including rejection and hard-error paths. End failure is a hard error while document/history/selection/coalescing and the prior defined IME view remain unchanged. `CUI_EDIT_MATCHER` denotes provider FAILED; STALE and BUSY have separate appended error tags. Invalid provider result codes become INVALID_ARGUMENT. Existing same-model mutation and cache-getter guards cover begin/match/end; unrelated side effects and direct caller-buffer writes are outside protection.

Declare the actual object and optional scratch extents. They must be valid, disjoint from each other and all known model, event payload, session, format and presentation storage. Ops live by value, avoiding a borrowed ops-table pointer. The descriptor is borrowed only during the checked call. A successful begin must publish a non-null lease; a failed begin leaves it untouched. No matcher descriptor or callback is retained in the model after return. Matcher-enabled single-line multi-selection/inline-token requests are explicitly unsupported; raw engine APIs and zero-matcher requests keep their existing behavior. This is partial Input parity, not a full-family claim.

Portable proof: `tests/unit/test_input_matcher.c` covers lease order/counts, custom/mask/regex short circuits, empty/replay bypass, invalid-old healing, all hard errors, unsupported result codes, aliases, callbacks attempting model destruction/insertion, and IME update/rejection/commit/Undo. It passes strict C11 warnings and ASan/UBSan. Unchanged registered CTest passes155/156; the sole failure is expected stale generated distribution after public header/source edits. Root owns regeneration/registration. No global build files or distribution were edited.

Registration needed: include the standalone public `cui_text_match.h` in header/package/amalgamation checks; register `test_input_matcher` linked to ordinary `cui`. Default Core/Base/Components must remain Rust-free. The optional provider is described in the follow-up section once its independent proof is frozen.

## Pinned optional provider

`cui_regex.h` exposes caller-owned zero-initialized compiled and session handles containing opaque state/generation only. Compile copies the pattern and performs the **actual source RegexBuilder admission before** reduced capture-free runtime compilation. Flags cover Unicode (default on), case, multiline, dot-newline, CRLF, greed, whitespace, octal and custom byte line terminator. Explicit limits cover source size/DFA-cache/nesting admission. Zero options preserve source defaults: nest250, NFA10MiB and facade DFA-cache2MiB. Bounded adapter pattern/text limits default1MiB each and can be configured; resource errors are separate from false matching. Pattern/text are UTF-8 spans, including empty/NUL where supported by the provider; the editing model keeps its independent input restrictions.

Create a session with a hard cache heap-byte budget; it owns the prepared runtime cache and retains compiled ownership. Destroying the compiled handle does not invalidate existing sessions. `cui_regex_matcher(&session)` borrows the public session extent/generation. Destroy/rebind while leased returns BUSY without mutation. Rebind prepares a replacement first; failed preparation leaves the session and generation intact. Success changes generation and makes old descriptors STALE; callers refresh their Input `.matcher`. Destroying a session leaves its caller-owned handle alive for stale-descriptor rejection. Generation allocation never wraps; exhaustion returns CAPACITY. The compiled/session handles must remain valid until operations finish; direct pointer tampering and unsynchronized concurrent access are outside the contract.

```c
cui_regex compiled = {0};
cui_regex_session session = {0};
if (cui_regex_compile(cui_string("^[0-9]*$"), (cui_regex_options){0}, &compiled) == CUI_REGEX_OK &&
    cui_regex_session_create(&compiled, 1024 * 1024, &session) == CUI_REGEX_OK) {
    cui_input_options options = {.matcher = cui_regex_matcher(&session)};
    /* Keep session alive while rendering/reducing Input with options. */
    (void)options;
}
/* After the last checked call: */
(void)cui_regex_session_destroy(&session);
(void)cui_regex_destroy(&compiled);
```

Build the optional archive with `scripts/build_regex.py` and the pinned private toolchain/cache as described in `backends/regex/README.md`. Its `pin.json` and lock preserve exact source archive/patch/toolchain provenance. No Rust dependency is introduced into default builds. Default-OFF native Linux CMake/package integration, generated declarations and dependency notices are registered separately from the optional compiled archive.

Measured provider proof uses its actual exported compile/session/begin/match/end API against source Regex1.13.1 across385 successful config/pattern combinations,63 equivalent compile rejections and58,520 matches. Every match makes zero allocator calls, including first use; all eight session allocation positions are forced to fail and restore tracked heap. Source captured-NFA admission limit rejection is explicitly retained even when reduced runtime could fit. Tests preserve the custom line-terminator LookMatcher configuration required for source semantics. Production code never logs text; test failure assertions use synthetic public fixtures.

Native strict C tests cover 1,000 lifecycle loops, leased BUSY, atomic failed rebind, stale/destroyed handles, compiled-owner destruction, malformed UTF-8/options/span/aliases and output preservation. Actual composed styled Input covers typing/paste/cut/delete, Undo/Redo, empty/invalid-old healing, Unicode, readonly, secure masking, IME update/rejection/commit and stale-provider hard failure before custom acceptance. The standalone prebuilt consumer CMake harness passes3/3 CTests. C-side Core/engine/provider/control ASan/UBSan+leak checks pass; Rust itself is not sanitizer-instrumented (allocator-spy/leak accounting covers its tested fallible allocation boundary).

`tests/web/build_regex_fixture.py` builds a real Core/Base/Components Input plus the optional Rust archive into a Zig0.16 WASI **reactor**; global gallery files are untouched. `test_regex_wasi.py` executes the composed C test entrypoint in Wasmtime49 after `_initialize`, with no `_start`. The same module runs in actual Chromium1208 (Playwright1.58). Its existing reusable secure proxy reads finalized Core owner/type/purpose/generation, and CDP `Input.imeSetComposition`/`Input.insertText` exercise real DOM composition through Core events to the CUI Input and provider. Browser assertions cover rejected candidate/provisional view preservation, commit exactly once, rejected host-end cancellation and following typing, Undo/Redo, masking, readonly proxy removal, and Greek Unicode matching. JavaScript performs no regex acceptance.

Reproduce browser proof with `python3 tests/web/build_regex_fixture.py --library WASI_ARCHIVE --output FIXTURE`, then `uv run --offline tests/web/test_input_regex.py --fixture FIXTURE` and `uv run --offline tests/integration/test_regex_wasi.py FIXTURE/regex.wasm`. Provider oracle: use task-local cargo `test --offline --locked --release --manifest-path CACHE/source/Cargo.toml -- --nocapture --test-threads=1` after the build script. Native harness: configure `backends/regex` with `CUI_REGEX_LIBRARY` and `CUI_REGEX_CORE_LIBRARY`, build, run CTest. Optional provider/native/package/browser tests and both public headers are registered; the isolated browser fixture is separate from the production gallery story.

PikeVM's prepared memory is bounded; execution is not deadline-bounded and may be slower than the source facade's adaptive engines for large/dense patterns. Pattern/NFA/cache/text limits control memory and accepted input size, not a guaranteed UI latency. The oracle/fault tests establish this boolean/resource slice; remaining Input contracts and other native platforms are separate work.

## Build and link the optional native provider

Ordinary CUI builds require no Rust toolchain. The portable caller-provided matcher is available in the default C11 library. Enable the regex backend only when you need its Unicode regular-expression implementation.

For the currently verified native Linux target, use an official Rust 1.85.1 prefix and build the separately pinned archive into a private cache:

```sh
python scripts/prepare_regex_toolchain.py --cache build/regex-toolchain
python scripts/build_regex.py --cache build/regex-cache --toolchain build/regex-toolchain/rust-1.85.1
cmake -S . -B build -G Ninja -DCUI_BUILD_REGEX=ON \
  -DCUI_REGEX_LIBRARY="$PWD/build/regex-cache/target/release/libcui_regex.a"
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/install"
```

The private preparation script verifies the pinned official manifest and both toolchain archives before extraction and installs into the specified cache with ldconfig disabled. The archive build script checks the compiler version and commit, archive checksums, lock and private cache patch before building offline. It does not install Rust or change global Cargo/rustup configuration. Obtain and verify the official toolchain against `backends/regex/pin.json`; the supplied installed prefix is caller-owned tooling. Compiler/target std must match. Native Windows/macOS package acceptance remains unfinished.

Within this source build, link a CMake target against `cui::regex`. Installed consumers use `find_package(CUIRegex CONFIG REQUIRED)` and `target_link_libraries(app PRIVATE CUI::cui_regex)`, with `CMAKE_PREFIX_PATH` pointing to the installed prefix. The package carries the Core dependency and the Linux archive's system link dependencies. For a direct compiler invocation:

```sh
PKG_CONFIG_PATH="$PWD/install/lib/pkgconfig" \
  cc -std=c11 -Wall -Wextra -Wpedantic app.c -o app \
  $(PKG_CONFIG_PATH="$PWD/install/lib/pkgconfig" pkg-config --static --cflags --libs cui-regex)
```

Include `cui/cui_regex.h`. Generated distribution headers also contain the matcher and regex declarations, but the optional compiled Rust archive remains a separate link dependency. Defining a distribution implementation macro does not generate the regex engine.

The optional-regex CI job shares Pages' exact OS/architecture/checksum-pin cache
key for only `channel.toml`, `host.tar.xz` and `wasi.tar.xz` under
`build/regex-toolchain`. Every run still verifies all three inputs, extracts and
installs the private toolchain, builds the provider offline, and runs all source
and CUI checks. Installed tooling, extracted sources and compiled outputs are
not cached. `tests/test_native_ci_cache.py` checks this policy offline.

A completed baseline, [CUI run 39773195384](https://github.com/mattmezza/cui/actions/runs/39773195384)
(22 October 2026), spent 44 seconds preparing this toolchain, 215 seconds building
CUI in optional-regex and 154 seconds testing it. Native Linux GCC build/test
steps took 293/218 seconds; Clang took 122/176 seconds. macOS native dependency
builds took 110 seconds, followed by 196 seconds building CUI and 148 seconds
testing. [Native binding run 39774911345](https://github.com/mattmezza/cui/actions/runs/39774911345)
spent 104 seconds on native dependency download/configure/build/install and
178 seconds validating callbacks. These combined steps do not establish their
download-only cost. No compiled dependency cache or skipped checks were added.
Warm-cache savings have not been measured: checksum verification and installation
still cost time, misses download normally, and cache transfer overhead can offset
savings. Compilation and test costs remain.
