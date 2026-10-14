# Regex feasibility proof, next32

Read-only CUI worktree based `245ea80`; gpui-component0.7.1 contract pinned to `87d10ae5e1299d1be18670a37570c5402f952e34`. Production, build registrations and distributions are unchanged. The initial authorized crate fetch was regex1.13.1 into `/home/matteo/.cache/cui-validation/regex-feasibility`; its archive SHA256 matched source Cargo.lock **before extraction**. All five dependencies were independently verified from archives against that lock before task-local extraction; the four already cached dependencies were not downloaded. Native probes run `cargo --offline --locked`; private patch is separate from pristine receipts.

## Measured native result

Exact Regex1.13.1 is the semantic oracle; runtime is regex-automata0.4.18 Thompson NFA/PikeVM with `WhichCaptures::None`. Probe runs385 successful pattern/config compilations,63 equivalent parse/compile rejections and58,520 boolean comparisons. Patterns include search/anchors, named/capturing/empty groups, greedy/lazy/bounded repetition, epsilon-heavy nested alternatives, Unicode properties/Greek/Cyrillic/Han/emoji/Perl classes, combining text, case folding, word boundaries, CRLF and custom line terminators. Seven config sets include explicit case, multiline+CRLF, dot-newline+swap-greed, whitespace, Unicode-off+octal and nondefault line terminator. Haystacks include long adversarial nonmatches/matches, NUL, Unicode multiword sequences and128 deterministic randomized48-scalar strings.

All58,520 prepared runtime matches agree with the oracle and make **zero allocator calls**, including first use of each freshly prepared cache—there is no warm-up assumption. GlobalAlloc instrumentation counts alloc, alloc_zeroed and realloc only around provider matching; oracle calls occur outside that window. The original upstream cache allocates once on its first adversarial match. All seven cache preparation allocation positions were forced to return null; each returns a recoverable error and restores tracked live heap bytes. The adversarial fixture requires384 heap bytes: budget383 rejects before allocation; budget384 succeeds. This is evidence for this corpus plus the structural bound below, not an exhaustive formal theorem or a complete CUI regex feature.

A temporary C11 consumer links the Rust static archive with compiler-reported dependencies and runs1000 create/match/invalid-UTF8/destroy cycles, plus malformed pattern and zero-budget output-preservation checks. It passes strict `-Wall -Wextra -Wpedantic -Werror`, and its ASan/UBSan/leak-detection run passes. Rust itself was not compiled with ASan; this is C-side integration/leak evidence, not full Rust sanitizer instrumentation. FFI prototype does not implement production owner/generation leases, complete alias guards or Core/Input dispatch. Those remain mandatory follow-up work.

## Two concrete semantic pitfalls found and reproduced

1. Setting `syntax::Config::line_terminator` alone does not configure the NFA look matcher. For multiline `^abc$`, custom terminator `@`, haystack `abc\n`: exact Regex returns false, syntax-only PikeVM returns true, explicitly configured NFA `LookMatcher` returns false. The independent `line_terminator` probe reproduces all three outcomes. Production must forward that builder option into both syntax and Thompson look matcher.
2. Removing capture states can reduce compile memory enough to accept an expression the source facade rejects at its `size_limit`. A captured `(a)` run at size_limit1472 reproduces source rejection while capture-free NFA succeeds. Production must preserve source compilation admission: first compile/validate with Regex1.13.1 and identical builder options/limits, then produce the capture-free runtime, or prove an equivalent source-size accounting transformation. Pure boolean language equivalence does **not** establish equal compile resource policy.

The capture-free language argument is narrower and sound: source regex rejects backreferences/lookaround, so captures record offsets without conditioning accepted paths. Removing Capture states does not remove alternation/group syntax checks; named groups still parse. PikeVM `is_match` explicitly asks for no captures and earliest boolean search. UTF-8 input and empty-match boundary settings must stay aligned with `RegexBuilder::build_one_string` (`regex/src/builders.rs:70–88`). No byte-regex substitution. Verify all pinned builder knobs: Unicode, case_insensitive, multi_line, dot_matches_new_line, crlf, line_terminator, swap_greed, ignore_whitespace, octal, size_limit, dfa_size_limit and nest_limit. Defaults inspected: Unicode+UTF8 on; other syntax bools off; LF terminator; nesting250; facade NFA10MiB and hybrid cache2MiB. DFA cache size is a source compile/provider configuration knob even when this runtime has no DFA.

## Bounded fallible cache proof

For a validated capture-free NFA let S=states, P=patterns and:

`K = 1 + Σ max(Union.alternates.length - 1, 0) + count(BinaryUnion)`.

Every epsilon exploration inserts its state into `next.set` before branching; each state branches at most once while that active set is being built. A Union pushes at most its remaining alternates, BinaryUnion one, and no Capture restoration pushes exist. Each root traversal begins with one Explore push and drains its stack before returning. Therefore maximum simultaneous stack entries are at most K, including duplicate queued targets (which are bounded by source edges before visited-set rejection). Sparse sets have fixed capacity S and never grow during search. `is_match` uses zero capture slots; the inherited two active slot tables are prepared with2P scratch entries each and zero state stride. No growth/reset-to-different-NFA is permitted for a bound session.

Upfront requested heap bound, checked for every multiply/add:

`4*S*sizeof(StateID) + 4*P*sizeof(Option<NonMaxUsize>) + K*sizeof(FollowEpsilon)`.

This covers dense+sparse vectors in each active set, both slot tables and epsilon stack. It excludes inline Cache/handle/NFA storage and allocator overhead, which require separate resource accounting; `Vec::try_reserve_exact` reserves requested vector capacity, not a promise about allocator slab overhead. Cache `memory_usage()` reports stack length, not reserved capacity, so must not be used as the sole reserved-resource receipt.

The task-local patch adds only `Cache::cui_try_new` and `SparseSet::cui_try_new`; existing upstream constructors/searches are unchanged. It rejects capture-bearing NFAs, checks budget/overflow before allocation, calls seven fallible reserves, and initializes only after reserve succeeds. Partial construction drops prior vectors automatically. Patch SHA256: `9230e33b37e44fcfb1e6ae1fa0a425b7a7b76146f6c3537efed38364d4a172cb`; exact diff follows for review, **not production approval**:

```diff
--- a/src/nfa/thompson/pikevm.rs
+++ b/src/nfa/thompson/pikevm.rs
@@ -1889,6 +1889,36 @@
 }
 
 impl Cache {
+    /// Task-local proof: fallible bounded capture-free Input cache.
+    pub fn cui_try_new(re: &PikeVM, budget: usize) -> Result<Cache, &'static str> {
+        if re.get_nfa().group_info().slot_len() != 0 { return Err("captures unsupported"); }
+        let states = re.get_nfa().states().len();
+        let mut stack_count = 1usize;
+        for state in re.get_nfa().states() {
+            let add = match state {
+                State::Union { alternates } => alternates.len().saturating_sub(1),
+                State::BinaryUnion { .. } => 1,
+                State::Capture { .. } => return Err("captures unsupported"),
+                _ => 0,
+            };
+            stack_count = stack_count.checked_add(add).ok_or("overflow")?;
+        }
+        let slots = re.get_nfa().pattern_len().checked_mul(2).ok_or("overflow")?;
+        let bytes = states.checked_mul(4 * core::mem::size_of::<StateID>())
+            .and_then(|x| slots.checked_mul(2 * core::mem::size_of::<Option<NonMaxUsize>>()).and_then(|y| x.checked_add(y)))
+            .and_then(|x| stack_count.checked_mul(core::mem::size_of::<FollowEpsilon>()).and_then(|y| x.checked_add(y)))
+            .ok_or("overflow")?;
+        if bytes > budget { return Err("budget"); }
+        let mut stack = vec![];stack.try_reserve_exact(stack_count).map_err(|_| "allocation")?;
+        let active = || -> Result<ActiveStates, &'static str> {
+            let set = SparseSet::cui_try_new(states).map_err(|_| "allocation")?;
+            let mut table = vec![];table.try_reserve_exact(slots).map_err(|_| "allocation")?;
+            table.resize(slots, None);
+            Ok(ActiveStates { set, slot_table: SlotTable { table, slots_per_state: 0, slots_for_captures: slots } })
+        };
+        Ok(Cache { stack, curr: active()?, next: active()? })
+    }
+
     /// Create a new [`PikeVM`] cache.
     ///
     /// A potentially more convenient routine to create a cache is
--- a/src/util/sparse_set.rs
+++ b/src/util/sparse_set.rs
@@ -106,6 +106,15 @@
 }
 
 impl SparseSet {
+    pub(crate) fn cui_try_new(capacity: usize) -> Result<SparseSet, ()> {
+        if capacity > StateID::LIMIT { return Err(()); }
+        let mut dense = vec![];let mut sparse = vec![];
+        dense.try_reserve_exact(capacity).map_err(|_| ())?;
+        sparse.try_reserve_exact(capacity).map_err(|_| ())?;
+        dense.resize(capacity, StateID::ZERO);sparse.resize(capacity, StateID::ZERO);
+        Ok(SparseSet { len: 0, dense, sparse })
+    }
+
     /// Create a new sparse set with the given capacity.
     ///
     /// Sparse sets have a fixed size and they cannot grow. Attempting to
```

The patch is limited to capture-free Input boolean matching and must not be advertised as fallible general-purpose capture cache creation. Semantic compile/NFA allocation still uses upstream infallible Rust allocation: OOM there can abort, and catch_unwind cannot convert that into CAPACITY. Bridge handle allocation can explicitly check null; session preparation now has a proven recoverable allocation boundary. After binding, provider match has no allocation under these invariants. User callbacks/host rendering may still allocate independently.

## Integration/resource contract retained

Retain the previous portable begin/match/end descriptor proposal, declared public extents/generation and same-model guards. Acquire one session lease after hard final presentation and before custom predicate; preserve custom→mask→regex order and candidate/old short circuits. Empty, replay and multiline bypass semantic matching; UPDATE uses normalized unformatted composed text, COMMIT formats once. Budget, stale, busy and runtime faults are hard errors and never false/invalid-old healing. Recoverable failures preserve model/history/selection/coalescing/prior IME view and owned resources; source-invalid candidate can still heal an invalid old value only after successful matching. Source compilation failures leave previous bound policy intact. The temporary cache probes do not implement/test these engine contracts yet.

Production ownership needed next: independent Core matcher header, edit checked stage/aliases/lease/error plumbing, Input option/reducer forwarding, new optional Rust adapter/public C handle header/pinned build script+metadata and focused engine/control/provider tests. CMake/CI/install/distribution remains integrator-owned. No requirement for Rust in default C11 Core/Base/Components; default optional provider OFF, prebuilt adapter artifact supported. Pin exact five archives and lock, keep immutable pristine copy plus separately hashed patch and license notices.

## Native and WASM feasibility boundary

Native x86_64 Linux static linkage proved; rustc1.99.0 reports `-lgcc_s -lutil -lrt -lpthread -lm -ldl -lc`. Other targets must query their own static deps, never copy Linux list. Pinned path-dependency builds emit upstream deprecation/lifetime warnings with rustc1.99; no blanket source warning suppression or crate edits were made. Production must select/review compiler lint policy separately. Probe C consumer and unchanged CUI build are strict-warning clean.

The original host rustc1.99 attempt failed E0463 missing target std. The subsequently authorized acquisition resolves that boundary without global installation: official Rust1.85.1 full x86_64 Linux toolchain and matching wasm32-wasip1 std were downloaded, checked against the official release manifest **before extraction**, inspected for absolute/traversing archive paths and installed with `--prefix=<task-cache>/rust-1.85.1 --disable-ldconfig`. Explicit task-local cargo/rustc paths, offline locked dependencies and a separate target directory were used. No rustup or global toolchain mutation occurred.

Official release date2025-03-18; manifest SHA256 `1e7dae690cd12e27405a97e64704917489a27c650201bf47780a936055d67909`; host archive `912c76f602f5f630138fca2388b2688c2c1aabcf823d71890b2b9155f743792e`; WASI std archive `98c0d9c141170c1cd26650fb853dba3fa08d454f1a440bc86073180d0fed4422`. URLs/receipts are in JSON. Checksums were fetched over official HTTPS; no signature-verification claim is made.

The Rust WASI staticlib reports only `-lc`. Pinned Zig0.16.0 links it successfully with C using `-target wasm32-wasi -mexec-model=reactor -std=c11 -O2 -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Wl,-z,stack-size=8388608`. `-UNDEBUG` is essential: Zig release defaults otherwise remove C assertions. The final module exports memory, `_initialize`, the C ABI and fixture entrypoints, **no `_start`**. There are no duplicate allocator/compiler-builtins link errors. Both consumers call `_initialize` exactly once before matching.

Actual imports are only WASI Preview1 `fd_close`, `fd_write`, `random_get`, `environ_get`, `environ_sizes_get`, `fd_seek`, `proc_exit`. No thread, clock, filesystem-open or Rust-specific host imports occur in this fixture. Wasmtime49 provides real WASI; Chromium1208 uses the existing gallery-compatible fd/environment/random/exit shim contract. Browser fd_write records corpus output for evidence; production must not log candidate text. The module is a reactor, not a WASI CLI success inferred to work in a browser.

Both Wasmtime49 and actual Chromium1208 (145.0.7632.6, Playwright1.58) execute the **entire same58,520 comparison corpus** inside the reactor, with zero allocator calls during every prepared Pike match and all seven injected cache allocation failures recovered without tracked leaks. Capture-free compile-admission discrepancy still reproduces; target-specific limits differ: WASM fixture cache332bytes and captured-NFA limit768, versus native384/1472. Never serialize host `sizeof`/budget assumptions into a WASM artifact.

Each runtime additionally executes3,000 C ABI create/match/malformed-UTF8/destroy cycles; bad pattern and zero-cache-budget preserve sentinel outputs. A70,000-byte C malloc remains live through Rust operations, then C realloc grows to140,000bytes and preserves the original contents before free. Browser also allocates input/output via C exports and directly calls Rust C ABI with Greek true/false/empty cases and invalidUTF8 output atomicity. Its final memory is13,500,416bytes, demonstrating ordinary shared linear-memory growth/allocator coexistence rather than isolated JavaScript matching. No host RegExp is used. Native Rust1.85.1 repeats the original58,520 zero-allocation/fault corpus successfully; upstream automata emits one retained dead-code warning on this compiler.

**Boundaries:** this proves optional pinned provider/reactor ABI feasibility, not a CUI widget/provider implementation. Engine lease/generation/aliases, Input dispatch, rebind/IME/history and supported build/package registration still require production work and tests. Compiler/parser OOM can abort; the seven fallible cache reserves are the recoverable resource boundary. WASI panic unwinding cannot be promised equivalent to native catch_unwind; no panic/abort recovery claim follows from these successful/fallible-cache tests. Invalid arbitrary handles or forged pointers are outside prototype safety contracts. The reactor uses an8MiB stack; no smaller-stack support claim is made. Other browsers, threaded WASM and unknown-unknown/Emscripten are untested.

## Reproduction and frozen artifacts

Task cache contains exact Cargo.toml/lock, source main allocator/oracle probe, private diff, temporary Rust FFI source and strict C consumer. JSON companion hashes each. Native oracle command:

`cargo run --offline --locked --release --bin cui-regex-feasibility --manifest-path /home/matteo/.cache/cui-validation/regex-feasibility/Cargo.toml`

Use `--bin line_terminator` for the isolated mismatch reproduction. `cargo rustc --offline --locked --release --lib … -- --print native-static-libs` produces the temporary archive/link receipt. C consumer uses the reported target link libraries; sanitizer command adds `-fsanitize=address,undefined -fno-omit-frame-pointer`, with `ASAN_OPTIONS=detect_leaks=1`. These cache prototypes are review inputs, not a supported public SDK or permanent test registration.

The prototype catches ordinary Rust unwind panics to avoid crossing C, but the default panic hook can print before catch_unwind. A production provider must review its panic/logging privacy policy without silently replacing a host-global hook; today's no-panic tests do not prove silent exceptional handling. Do not log candidate text, and do not represent allocator abort as a recoverable match error.

Unchanged CUI baseline: strict C11 warning build and CTest **154/154** pass. Logs `/home/matteo/.cache/cui-validation/input-regex-feasibility-{config,build,ctest}.log`. Native proof results and exact artifact hashes are frozen in the JSON companion. No CUI behavior tests or production registrations were added.

Task-local reactor reproduction: use explicit `RUSTC=<cache>/rust-1.85.1/bin/rustc CARGO_TARGET_DIR=<cache>/target-1.85.1`, then `<cache>/rust-1.85.1/bin/cargo rustc --offline --locked --release --lib --target wasm32-wasip1 --manifest-path <cache>/Cargo.toml -- --print native-static-libs`. Link `reactor.c` and `target-1.85.1/wasm32-wasip1/release/libcui_regex_feasibility.a` with the Zig flags above and `--export` each fixture/C ABI symbol. Run `uv run --offline <cache>/run_reactor.py` and `uv run --offline <cache>/run_browser.py`; both scripts declare exact already-cached runtime versions. Artifact SHA receipts include the module, source, scripts and terminal output. Only the two audit documents enter git.

Follow-up unchanged CUI incremental build and CTest154/154 pass again after reactor proof; receipts record these separate logs. No generated-distribution or registry edits were needed for this documentation-only task.
