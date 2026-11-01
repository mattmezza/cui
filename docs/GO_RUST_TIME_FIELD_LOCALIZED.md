# Go and Rust localized TimeField (unvalidated source checkpoint)

This additive authoring slice is prepared against root `8a402c2b`. No Go/Cargo,
compiler, runtime, generation or installed-package gate has run for these wrappers.
The authoritative C contract is [BRIDGE_TIME_FIELD_LOCALIZED.md](BRIDGE_TIME_FIELD_LOCALIZED.md)
and `include/cui/cui_bridge.h`. Existing scalar TimeField APIs stay unchanged.
This does not bind direct Base/Styled options, drafts, clipboard/IME policy or
claim full source focus/accessibility parity.

The new modules are `bindings/go/cui/time_localized.go` and
`bindings/rust/src/time_localized.rs`. They copy an exact V1 configuration616,
result80 and change64 record. Fourteen explicit UTF-8 prefix-free tokens have
1–16 bytes; four optional segment names have0–64. English/Arabic constructors
are conveniences containing explicit tokens, not a locale registry. Same-day
inclusive bounds are optional. Minute precision requires seconds0. Tokens/names
and bounds are copied; rejected updates preserve the old C configuration and
Go builder value. No foreign reducer, normalization or owner-value clamping.

## Go authoring

```go
cfg := cui.ArabicTimeField() // H12, Arabic digits and explicit period tokens
cfg.Minimum, cfg.Maximum = &cui.Time{Hour: 8}, &cui.Time{Hour: 18}
cfg.EditGeneration = 1
field := cui.TimeField("appointment").Time(cui.Time{Hour: 14, Minute: 15}).LocalizedTime(cfg)
field.OnTimeFieldChange(func(change cui.TimeFieldChange) {
    // Applied copied observation. Do not replay it as another edit.
})
// Before Mount, optionally call app.EnableLocalizedTimeFields(capacity).
// Otherwise a tree with localized configuration enables the default4096 queue.
```

After mounting, `SetLocalizedTime` validates/copies and commits builder state only
after successful C configure. `LocalizedTimeConfig`, `LocalizedTimeResult`,
`ClearTimeValidation`, `PollTimeFieldChange` and `DispatchTimeFieldChanges` are
app operations. `Dispatch` also drains the typed queue after generic events and
controlled Slider dispatch. Callbacks are never invoked inside step. Existing
`SetTime`/`App.Time` remain scalar owner setters/getters.

An unmounted builder does not look up a global provider or call C. Mount uses its
owning app, preflights configuration, and uses existing rollback on any created
node failure; enabling queue capacity is an app-lifetime opt-in and is not undone
by failed mount. Go app creation locks its owner OS thread; all calls, close and
callbacks must remain there. Goroutines are not an alternate owner. Removal clears
callbacks, copied values remain safe after removal/close, and close during callback
stops further dispatch. Panic propagates after the current record was consumed;
a defer clears the dispatch guard. Step/Run/nested Dispatch are rejected during
typed callbacks. Ordinary owner setters and removal follow existing mutation rules.

## Rust authoring

```rust
let mut config = TimeFieldConfig::arabic();
config.minimum = Some(Time { hour: 8, minute: 0, second: 0 });
config.maximum = Some(Time { hour: 18, minute: 0, second: 0 });
config.edit_generation = 1;
let field = app.localized_time_field(
    root, "appointment", Time { hour: 14, minute: 15, second: 0 }, &config)?;
app.on_time_field_change(field, |change| {
    // Clone for an owned application observation, then update siblings after dispatch.
})?;
```

The app-owned constructor validates before node creation and removes its new node
if scalar time or configure fails. It enables default4096 capacity only if not yet
enabled. `configure_localized_time` changes an existing node atomically. An existing
app-borrowed `Builder::localized_time` is a setter; failure does not remove a
preexisting node. Use the constructor when atomic creation is needed. Rust has no
new unmounted declarative tree/provider: its existing builder already borrows App.

`localized_time_config`, `localized_time_result`, `clear_time_validation`,
`poll_time_field_change`, `dispatch_time_field_changes` map to the same C contract.
Normal `dispatch` drains the typed queue after the existing queues. App remains
!Send/!Sync through its existing Rc marker; it and all native UI calls stay on
the creating OS thread. Rust borrows prevent aliasing mutable App into callbacks;
callbacks can retain owned copies in local Rc/RefCell state, and `run_with`'s
post-dispatch hook applies UI updates. Panics unwind in Rust; the existing native
run trampoline catches them and resumes only after returning from C. The dispatch
guard resets on unwind. No copied record replays after a panic. Node removal prunes
stale typed callbacks; close requests stop typed dispatch. Outstanding owned copies
are independent of App lifetime and carry app-scoped node identity, not a live handle
guarantee after removal.

## Optional group and failed steps

All ten C symbols must be present, versions bit0 enabled and sizes616/80/64 match.
A missing/partial/mismatched dynamic provider makes the new feature unavailable;
no symbol is added to the existing mandatory loader. Old libraries can still load
and run scalar APIs. Go resolves through its selected app library and weak current
static symbols. Rust dynamic lookup follows its existing linked-library provider;
current source-built static packaging binds the current group. Linking a new static
wrapper to an older static archive is not promised as optional-symbol compatibility.
Default/shared WASM and native NO_TLS are unsupported by the C owner policy.
Cross-target Go/Rust package support and Rust MSRV for this new module are untested;
no new compiler minimum is claimed. Existing toolchain prerequisites still apply.

Capacity is immutable1–4096 (0 selects4096). Drain between frames. Changes are
already-applied aggregates, including records whose later rendering failed:
`FrameStatus`/`frame_status` is independent of value/validation. Explicit polling
after a failed step is supported; `frame()`-style convenience propagation does not
automatically drain after failure. A result getter rejects failed/omitted/unready
frames under C rules; clear-validation is not result recovery. Polling and dispatch
consume the same FIFO, so use one delivery strategy. Removing nodes prunes pending
records under the bridge's full generation identity. No receipt acceptance or event
replay occurs in either wrapper.

## Complete authored examples and pending gates

`bindings/go/examples/time_field_localized/main.go` and
`bindings/rust/examples/time_field_localized.rs` author real English24h and Arabic12h
fields, same-day bounds, read-only/disabled/bounds toggles, validation clearing and
summary updates from copied callbacks. Default mode is windowless scripted input;
`--native` uses existing host run and real input, requiring `CUI_TEST_FONT`.
Neither path has run. Automated native runs must use the private-Xvfb wrapper;
never launch these examples on the maintainer desktop during validation.

Prepared tests cover owned wire copies/prefix/precision validation, real typed
changes after step, invalid replacement, wrong kind, capacity/constructor rollback,
no result replay, copied failed-frame decoding, and Rust callback panic recovery.
Still required: actual matching C library runtime, Arabic/control behavior,
wrong-thread/close/removal callbacks, real failed-frame copied records, old and
partial-group provider subprocess tests, schema-poison checks, native run unwind,
MSRV/installed-package source closure, canonical SDK generation and required
skill tests. Prepared tests are not evidence of execution or complete bindings.
