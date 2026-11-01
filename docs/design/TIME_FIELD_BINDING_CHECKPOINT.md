# Localized TimeField binding checkpoint

Status: source-only proposal; no new bridge or binding support implemented by this checkpoint. Audited integration source `ef893a34c6f1ff6adf48cd7a3b6d032ab1eb8014` (including the localized widget integrated at `49267cc4` and browser proof at `53472bb9`). No compiler, runtime, browser or generator was run for this document.

## Current delivery versus required delivery

The localized C widget is implemented, but the bridge still renders the legacy widget. Existing foreign TimeField authoring is real UI, not a launcher; it simply does not expose the new localized editor.

| Surface | Actual current entrypoints / ownership | Localized V1 gap |
|---|---|---|
| C widget | `include/cui/cui_time_field_v1.h`: caller-owned options/state/value, Base and Styled submission, copied aggregate `cui_time_handler` | Implemented C route; global segment Tab remains a documented source-parity exception; IME composition/clipboard shortcuts remain omitted |
| C bridge | `include/cui/cui_bridge.h`: kind55, `cui_bridge_set_time/get_time`, `cui_bridge_time {uint32_t hour,minute,second;}` | No locale/domain/options/state/result/clear-validation API |
| Python | `bindings/python/cui_bridge/__init__.py`: `time_field`, `Widget.time_value/current_time`, `disabled`, generic `on_change` | Cached tuple plus mounted scalar calls only; no localized records or typed time observer |
| JavaScript | `bindings/node/index.cjs`: `time_field`, `time_value/current_time`, `disabled`, generic `on_change`; addon `set_temporal/get_temporal` | Same scalar route; addon invokes old C functions directly |
| TypeScript | `bindings/typescript/index.d.ts`: `TimeField` tuple methods and `ChangeEvent` callback | Declaration of JS legacy route only |
| Go | `bindings/go/cui/native.go`: `TimeField`, `Node.Time/SetTime`, `App.Time`, `OnChange` | Owned `Time` value; old mandatory set/get symbols only |
| Rust | `bindings/rust/src/lib.rs`: `Kind::TimeField`, `App.time/current_time`, builder `time`, generic change callback | `repr(C)` three-u32 `Time`; no localized editor API |

Authoritative implementation: `src/bridge/cui_bridge_internal.h` owns `cui_time_state` and `cui_time`; `src/bridge/cui_bridge.c` legacy kind55 paint calls `cui_time_field` with `.seconds=true` and `.disabled=node->disabled`. It exposes neither read-only nor owner-invalid state, H12, minute precision, custom digits/periods, inclusive bounds, segment names, draft reset generation or validation clearing. Legacy `bridge_change` formats ASCII `HH:MM:SS` into the generic text payload. Preserve that route and payload exactly when localized mode is off.

The landed pure contract is `include/cui/cui_time_text.h`: H23/H12 × minute/second; fourteen prefix-free UTF-8 tokens, each1–16 bytes without NUL; exact case/spacing; full formatted text at most160 bytes; same-day inclusive bounds; minute precision rejects nonzero seconds. `include/cui/cui_time_field_v1.h` adds four segment names (at most64 bytes each), disabled/read-only/owner-invalid flags, nonzero edit generation, explicit validation clearing and caller-owned state. It does not retain borrowed strings. See [localized widget guide](../TIME_FIELD_LOCALIZED.md) and [completion contract](TIME_FIELD_COMPLETION.md).

## Recommended first slice: bridge-owned localized mode, then Python

Add an explicit opt-in on the existing kind55 node, rather than another kind or a replacement factory. A mounted Row/Column containing a localized TimeField, adjacent label, clear-validation button and copied change handler is the acceptance example. C plus Python is the first independently reviewable end-to-end slice; JS/TS, Go and Rust follow independently and remain marked pending until actual UI/runtime proof.

Keep all old ABI records, constructors, default rendering, scalar callbacks and dispatch timing unchanged. No controlled-value receipt/acceptance mechanism is needed: this widget applies edits to bridge-owned time and publishes a copied aggregate observation. Do not copy the controlled Slider proposal/receipt architecture or pretend time is a float.

Proposed additive exports (names and layouts require ABI review before production):

```c
uint32_t cui_bridge_time_field_versions(void); /* bit0: complete V1 group */
uint32_t cui_bridge_time_field_config_size_v1(void);
uint32_t cui_bridge_time_field_result_size_v1(void);
uint32_t cui_bridge_time_field_change_size_v1(void);
cui_bridge_status cui_bridge_time_field_enable_v1(cui_bridge_app *, uint32_t capacity);
cui_bridge_status cui_bridge_time_field_configure_v1(
    cui_bridge_app *, cui_bridge_node, const cui_bridge_time_field_config_v1 *);
cui_bridge_status cui_bridge_time_field_get_config_v1(
    cui_bridge_app *, cui_bridge_node, cui_bridge_time_field_config_v1 *);
cui_bridge_status cui_bridge_time_field_get_result_v1(
    cui_bridge_app *, cui_bridge_node, cui_bridge_time_field_result_v1 *);
cui_bridge_status cui_bridge_time_field_clear_validation_v1(cui_bridge_app *, cui_bridge_node);
cui_bridge_status cui_bridge_time_field_poll_change_v1(
    cui_bridge_app *, cui_bridge_time_field_change_v1 *);
```

No disable-to-legacy operation in V1: remove/recreate a node to change routes. Repeated compatible configure updates localized options atomically, not state ownership. `enable` establishes bounded queue storage before composition; missing group, unsupported target or allocation failure leaves legacy authoring available. Its exact thread/platform support must be reviewed against the existing private bridge owner-thread mechanism before implementation; versions must never advertise an unenforceable owner policy.

### Exact proposed wire records

All fields are fixed-width integers; no pointers, `bool`, enums, `size_t`, packed attributes or borrowed views. Use existing eight-byte `{slot,generation}` node handle and existing twelve-byte `cui_bridge_time`. These are ABI records, not serialized endian-neutral network packets.

* `token16`: `uint32_t length; uint8_t bytes[16];` (20 bytes).
* `name64`: `uint32_t length; uint8_t bytes[64];` (68 bytes).
* Config: eight u32 fields `struct_size,version,flags,cycle,precision,bounds_flags,reserved[2]`; `minimum,maximum` (two bridge times); `uint64_t edit_generation`; fourteen token16 entries ordered digits0–9, separator, period_separator, am, pm; four name64 entries ordered Hour/Minute/Second/Period. Proposed size616 bytes, generation offset56, tokens offset64, names offset344. Explicit flags disabled/read-only/owner-invalid use published bridge constants, not casts from future Core enums. Generic node disabled additionally gates editing; clearing config disabled cannot override generic disabled.
* Result: four u32 `struct_size,version,reserved[2]`; node; u64 frame_sequence; four u32 `status,validation,changed,rejected_count`; previous/value times; two u32 `queued_change_count,reserved_tail`. Proposed size80, frame offset24, previous offset48. `queued_change_count` is0/1; it is not foreign callback count or accepted-edit count.
* Change: four u32 `struct_size,version,reserved[2]`; node; u64 frame_sequence; previous/value times; u32 `validation,rejected_count`. Proposed size64, frame offset24, previous offset32.

Expected sizes/offsets match native LP64, native ILP32 and wasm32 because u64 fields begin at eight-byte offsets and total sizes are eight-byte multiples. They remain **unverified target expectations**, to be asserted by real strict C/C++/WASM ABI fixtures, not imposed as a portable unsigned-width assumption. `frame_sequence` is exact u64 (Python int, JS/TS bigint, Go/Rust u64); node fields remain two u32, never a lossy JS Number identity.

Exact size/version1, known flags/cycle/precision and all reserved zeros are mandatory. Token lengths1–16; every trailing byte zero; names length0–64, trailing bytes zero (empty selects the C default name). Reject invalid UTF-8, embedded NUL and non-prefix-free locale vocabulary using the landed validator after creating private borrowed views into the owned candidate. All fourteen tokens are explicit even when inactive in the current cycle. Inactive bounds are canonically zero on the wire; active bounds obey the pure validator including minute hidden seconds. Configuration failure changes neither node/config/value/draft nor output arguments. Value-domain rejection is distinct from malformed configuration. No OS locale lookup, normalization, casing or registry inference.

## State, observations and ordering

Bridge node owns localized state, copied config bytes and value; foreign wrappers never expose a mutable C state view. `edit_generation` belongs to the caller and must be nonzero. Configuration changes and owner value changes use the existing C widget synchronization rules on the next ordinary submission; do not rebuild state every frame or erase live edits accidentally. `clear_validation` clears the widget-owned validation only, does not change value, owner-invalid flag or draft ownership; document the exact landed helper behavior rather than promise an owner-invalid reset.

Old `set_time` remains legal on a localized node: retain its old24/60/60 argument validation and assignment behavior. A time outside the new domain is an observable invalid owner value, not silent clamping; localized `current_time` still returns that value. Wrappers may offer a separate validated convenience setter but cannot silently change old setter semantics. Externally authored change is not an edit notification. Configuration setter must allow a temporarily out-of-domain owner value so it matches the C widget contract.

During composition call the existing localized Styled facade with an internal copied aggregate handler. Reserve at most one typed change slot per localized submitted node **before** reducing any of its input. If reservation cannot be guaranteed, reject that node/frame before edits, callbacks or draft mutation; never apply a change then silently drop its observation. Capacity/duplicate-node rules and all-frame versus per-node preflight must be fixed in the C implementation review. No allocation per event or retained caller string is required. Existing `cui_bridge_queue_text` already validates/copies a NUL-terminated UTF-8 view bounded at4096 bytes through `bridge_utf8_limit`; the widget independently limits complete formatted text to160. Do not truncate to the old small inline text buffer or add an unnecessary input dispatcher. A separate queue avoids consuming or reformatting old generic events. Localized mode emits typed time changes only; it must not double-dispatch legacy ASCII CHANGE callbacks. Wrappers expose this as a distinct typed handler, not a reinterpretation of generic `ChangeEvent`.

Result `changed` is net final value difference. The internal aggregate observer emits at most one previous/final pair; multiple edits that return to entry value produce no change record. Rejections still update result status/validation/rejected_count. Keep operation status separate from edit validation. C `change_count` counts actual C observer invocations; the bridge result names queue production explicitly because foreign callback execution happens later.

Poll copies one complete owned record and consumes only on success; EMPTY or invalid output leaves output unchanged. Removed/stale node records are discarded; generation prevents reused slots from acquiring old events. Ordinary omission invalidates latest-result freshness and clears draft on reappearance through the existing widget contract; it does not invent an edit. Getter before first valid ordinary submission, after removal or failed bridge frame returns an explicit status with unchanged output. Clearing a sticky diagnostic or a geometry-only recovery frame does not republish a failed frame as an observed edit. Already delivered callbacks/effects are never rolled back; failed-frame queue publication needs an explicit implementation test and review, not an assumption based only on Core readiness.

No foreign callback runs inside Core submission. Drain copied records after step/native paint at the existing language dispatch boundary. Python and Go `Step`/`step` remain separate from explicit dispatch; JS step already dispatches, and Rust keeps its existing step/dispatch boundary; no silent retiming of legacy callbacks. Copy before invoking user code, release all C borrows first, and stop safely after removal/close. Propagate exceptions/panics only outside C frames; queue disposition on callback failure must be documented (current copied item consumed, later records retained unless app/node teardown discards them). Nested dispatch/step/run is rejected by a localized dispatch guard. Retained immutable records remain valid after teardown; they grant no authority tokens.

## Foreign ownership and compatibility

Resolve the complete ten-symbol group plus version and exact-size agreement before exposing support; never import the new symbols unconditionally into old dynamic libraries. Python/Node/Go already have optional Slider conventions to reuse, without reusing Slider receipts. Missing/partial groups return explicit unsupported without creating a partial localized node. Default legacy methods continue to work against old libraries. Node addon must copy UTF-8 buffers and reject malformed native outputs, not retain N-API/C views. JS/TS typed immutable objects use bigint for frame/generation; Python frozen records; Go copied values; Rust owned records and matching static SDK symbols.

Reuse Go's existing OS-thread lock and Rust's existing !Send/!Sync App, not another locking architecture. Rust callback access must preserve safe mutable borrowing: use an independently reviewed restricted context if mutation from the typed handler is exposed, rather than handing out an aliased App or changing generic callbacks. Python close destroys, Node close/dispose differs, Go Close balances its thread lock and Rust RequestClose is request-only: test each actual contract, not a shared fictional teardown rule. Same-loaded-image app/handle ownership remains mandatory. This checkpoint does not promise newly supported shared-memory WASM or physical accessibility delivery.

## Required proof and delivery boundaries

1. C ABI/headers: source/selective/umbrella, C++ include orders, native and pinned wasm32 sizes/offsets; invalid records and capacity failures atomic. Freeze actual old bridge runtime trace to prove default kind55 IDs, commands, scalar values, ASCII payload and after-step event order unchanged.
2. Real nested authored UI: two localized fields with independent state/locales, H23/H12 × both precisions, Arabic token vocabulary, bounds/owner-invalid/read-only/disabled, named segments and clear-validation. Split/coalesced text, >64-byte copied paste, unfinished UTF-8 prefix followed by complete valid/out-of-range paste, focus changes/Tab/omission/generation resets, net-zero edits and aggregate count. Keep original input evidence; no retargeting or fabricated one-root Tab guarantee.
3. Queue/lifetime: multiple fields, full FIFO/preflight, overflow, failed frame, removal/reuse/close in first callback, callback exception/panic, later records, nested dispatch, immutable copied record surviving teardown, and optional old/partial-library fixtures in separate processes. Output byte snapshots prove failure atomicity.
4. Each language: compile/run a complete UI-producing example, not a launcher; actual pointer/keyboard/text input and callback-updated sibling label; current/old/malformed provider tests. TypeScript must both compile and execute JS. Rust dynamic and matching default static need separate proof. Native automated tests use private Xvfb; browsers headless.
5. When implemented: exact public header authority, human usage guide, all78-preserving metadata/binding rows, SDK→amalgamation→skill canonical generation, usage/skill/freshness checks and honest per-language receipts.

Still open after the recommended C+Python slice: JS/TS/Go/Rust localized delivery until individually verified, direct foreign Base/Styled state/style APIs, source one-root segmented focus parity, IME/clipboard integration, and physical native accessibility acceptance. Existing tested legacy ASCII parse/edit is not “scaffolded”; new detached helpers alone do not close widget or binding delivery.
