# JavaScript/TypeScript localized TimeField (source checkpoint)

Unreleased, source-only preparation over the integrated localized C bridge.
No Node compilation, runtime, TypeScript compiler or generated-skill acceptance
has run for this slice. The existing `time_field()` remains legacy kind 55.
`localized_time_field()` is a distinct subclass using that same kind and the
bridge-owned localized editor. Python/Go/Rust localized authoring remains pending;
this source checkpoint establishes no validated language support or all 78 parity.

Complete authored UI sources are
[JavaScript](../bindings/node/examples/time-field-localized.cjs) and
[TypeScript](../bindings/typescript/examples/time_field_localized.ts). They nest
English 24-hour and Arabic 12-hour widgets in Row/Column UI with real clear,
read-only, disabled and bounds controls. Locale tokens are explicit values, not
OS locale lookup. No foreign callback runs inside Core and no JS reducer exists.

Call `app.supports_localized_time_field()` before opting in. The addon resolves all
ten symbols as one optional group, checks version bit 0 and exact 616/80/64 native
record sizes. Missing/partial/old providers leave legacy scalar authoring usable.
`enable_localized_time_fields(capacity)` enables an immutable independent FIFO;
0 selects 4096. Mount preflight enables default capacity if needed and rejects
unsupported or non-Row/Column ancestry before making any nodes.

`localized_time_config()` returns a deeply frozen copy with explicit locale,
cycle h23/h12, precision minute/second, inclusive same-day bounds, four segment
names and nonzero uint64 `editGeneration` BigInt. Tokens are strict UTF8, no NUL,
prefix-free and 1–16 bytes. Names are 0–64 bytes; empty chooses the C default.
Minute bounds require seconds 0. `configure()` replaces the whole configuration;
it commits the JS owner copy only after synchronous C success. Increase generation
for locale/name/domain replacement that should reset unfinished editing. Identical
owner echoes keep generation stable. `.time_value()` retains the existing range
checks; values outside configured bounds are displayed invalid, never clamped.

The binding's internal Buffers are explicit little-endian copies, **not** the C
ABI's native-endian records. N-API converts every native uint32/uint64 field and
string length individually in both directions, with exhaustive layout assertions.
UTF8 byte arrays copy unchanged. No native struct pointer, borrowed Buffer view,
mutable model/draft or external callback crosses the C call. Input/output buffers
can be discarded or mutated after return without changing future C state.

`on_localized_change()` receives immutable **already applied** previous/value,
node slot/generation, BigInt frame, validation, rejection count and actual bridge
frameStatus. Returning a value does nothing; do not accept/reapply it. Typed
records use their own FIFO and do not duplicate generic time CHANGE events.
`step()` dispatches after successful submission; `step_only()` leaves dispatch to
the caller. After a failed step, explicitly poll/dispatch localized changes too:
a later failure may follow an applied edit. A failed frame's latest-result getter
stays unavailable until a healthy ordinary widget frame; clearing validation is
not recovery or undo. `clear_validation()` preserves time/draft and owner-invalid.

Polling consumes before callbacks. A callback exception never replays that record;
later records remain queued and the dispatch guard resets in finally. Nested
step/run/typed dispatch is rejected for that app. Setters/removal/close/dispose
inside poststep callbacks are supported; teardown clears callback references and
stops dispatch, and retained copied values remain safe after app destruction.
Removal prunes unread C records; slot reuse cannot redirect an old observation.
All calls remain serialized on the owning OS thread through destruction. No
shared-WASM, native AT, IME or native Tab traversal support is claimed.

Prepared gates cover protocol poisoning/reserved/UTF8/BigInt boundaries, real
English/Arabic queued input, disabled/read-only/bounds/clear, copied C ownership,
callback error/return/reentry/lifetimes, an actual later Slider capacity failure,
old/partial group fallback and compiled TS-authored runtime. These are prepared
fixtures, not test results. Compilation and native tests await the explicit lease;
native automation must use the private-display wrapper, never the desktop.
