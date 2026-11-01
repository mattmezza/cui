# Rust controlled Single/Range Slider

Unreleased bridge-owned V1 subset, separate from `Kind::Slider` and its existing
scalar float/autocommit/CHANGE behavior. This binds owned controlled values and
receipts, not arbitrary direct C Styled style/motion records. Native numeric AT,
typed SET/touch injection, V2 gradient adapters and vertical registered-scroll
admission remain separate open work. Linux dynamic tests passed on Rust1.99.1: 14 actual UI tests, three private
wire tests, 43 actual-old/provider processes, four authored example modes and
four compile-fail thread/context checks. The current legacy/native suite also
passed under private Xvfb with bundled Inter; separately gated historical fixture
branches are not implied. Matching default bundled-static source closure also passed, with the static cfg
actually compiled and no dynamic bridge dependency. MSRV1.85.1 remains a required
follow-up. This is not a package publication claim.
Read the [C authority](BRIDGE_SLIDER_MODEL.md).

## Complete authored UI

[Copy the complete Rust program](../bindings/rust/examples/slider_model.rs).
It authors a nested Row/Column control, heading/status labels, an acceptance
button and an external Single/Range owner. Its restricted callback explicitly
accepts whole pairs and then updates the owner and real status label. It focuses
actual thumb bounds, queues a key and checks acceptance or rejection. Default
execution is windowless; this is not a launcher-only example.

Use existing [C11/CMake/Ninja prerequisites](DEPENDENCIES.md), Rust 1.85.1 or
newer and the dependency-free crate. From the repository root:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target cui_bridge
CUI_BRIDGE_LIB_DIR="$PWD/build" CARGO_BUILD_JOBS=1 cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example slider_model
CUI_BRIDGE_LIB_DIR="$PWD/build" CARGO_BUILD_JOBS=1 cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example slider_model -- --reject --vertical
CUI_BRIDGE_LIB_DIR="$PWD/build" CARGO_BUILD_JOBS=1 cargo run --offline --locked --manifest-path bindings/rust/Cargo.toml --example slider_model -- --single
```

The shared-library directory must be absolute. Without CUI_BRIDGE_LIB_DIR, the
existing Linux source build compiles the matching bundled SDK. That path passed the same 14 UI/three wire/four example checks locally; it is
not compatibility with arbitrary older static archives. Dynamic old libraries are supported through optional-symbol fallback.
No command installs dependencies. Automated native tests must use the private
`scripts/test_local.py` wrapper; physical display/GPU/AT proof is separate.

## Owned records and explicit authority

`SliderValue::Single { end }` and `Range { start,end }` use f64 throughout. Config
and domain are ordinary owned Copy records; `SliderConfig::default()` supplies
one complete configuration. Public application records may be edited locally;
submission validates and copies them atomically. Domains require finite bounds,
finite nonnegative step and finite maximum-minus-minimum. Logarithmic domains
require minimum > 0 and maximum > minimum. Parent layout ignores authored length
and copies 0; malformed native parent length rejects rather than normalizing.

`App::slider_model(parent,key,config,value)` requires an existing Row/Column
parent. It validates before the C factory, which validates actual ancestry
atomically. Rust has no retained unmounted typed node or implicit root. Invalid
creation leaves previously authored UI intact. `configure_slider` replaces the
whole config and pair; `set_slider_value` replaces only the current-kind pair.
The corresponding owned getters are `slider_config` and `slider_value`. Use the
existing `component_disabled` for this control; the legacy `disabled` method
retains its Button-only contract.

`supports_slider_model()` resolves the complete 16-symbol optional group, V1 bit
and all five exact size queries. An incomplete/unsupported group returns false;
new operations return Error(9), with no scalar emulation. Old operations do not
enable or allocate the new queue. Call `enable_slider_requests(capacity)` before
authoring controls, polling or registering callbacks; capacity 0 means 4096, maximum 4096. Reenabling does
not silently resize or discard a backlog.

`poll_slider_request()` returns one owned `Option<SliderRequest>` and invokes no
callback. Every C uint64 stays u64, including frame/index absence sentinel,
epoch/source/revision/sequence and pointer/device/accessibility IDs. Values and
domains are never narrowed through f32. Only actual geometry uses f32. Unknown
wire enums/masks and inconsistent unavailable evidence reject before publication.

`SliderReceipt::clone()` preserves issued authority; `info()` returns only inert
metadata. Private Rc namespace identity contains no App/native pointer and keeps
receipts local, not Send/Sync. Copies remain readable after removal/App Drop;
acceptance still expires in C. Keep receipts/handles within the matching App and
loaded bridge image. This is not a security or process-global namespace promise.

`accept_slider_request(&receipt,whole_pair)` is explicit; it may choose another
valid pair. Update external owner storage only after success. Reading, polling,
callback absence and rejection never autoaccept or emit CHANGE. Request current
is the immutable submission entry, while requested intent evolves in original
input order. Later errors never roll back effects already accepted by the owner.

## Restricted callbacks and teardown

Register `on_slider_request(node,handler)` with:

```text
for<'a> FnMut(&mut SliderRequestContext<'a>, SliderRequest) -> Result<()> + 'static
```

The context temporarily borrows the App and exposes only accept, configure,
set/get value, get config, text, disabled, remove, queued pointer/key/text input,
request_close and typed handler replacement/removal. There is no Deref/as-App,
step/run/dispatch, owned destruction or App replacement escape. Mutating external
owner state/userdata and enqueueing future work are allowed. Native buffers and
callback-map borrows never span user code; each request is copied before calling
the handler. Replacing/removing the current handler is supported.

`step` only steps. `dispatch` drains the unchanged legacy FIFO first, then the
independent typed FIFO if enabled. No cross-queue global order is invented.
`dispatch_slider_requests` drains only the latter. Counts follow Rust's existing
convention of callbacks actually invoked. Error or panic consumes the current
entry and stops that drain; later entries remain queued. Its local guard resets
on both paths. Manual panic propagates normally; native `run_with` retains its
existing catch/resume boundary outside C.

`request_close` in the context calls the existing request-only App::close. It
records typed-local closing evidence, clears new handlers and stops before
another typed poll. It does not destroy App or assume Python teardown semantics.
Drop remains the owner of native destruction after the borrow ends. Successful
remove releases new registrations for the removed node/subtree. Legacy callback
registration policy stays unchanged.

App remains !Send/!Sync through its existing Rc marker. Keep the creating OS
thread alive through Drop and serialize calls. Part bounds return actual completed
geometry or None; disabled track/fill remain, thumbs omit. Presentation returns
copied status and absolute accumulated Core-time wake values, not physical render
success, V2 support or an automatic Rust scheduler.

## Reproducible optional protocol proof

Normal `cargo test` includes current-library UI and private wire-copy tests.
Historical old-library/provider proof requires a separately supplied actual old
module; no old binary is vendored or inferred from a successful current run.
On Linux, with existing Cargo/C11 tools and reused build/target directories:

```sh
python bindings/rust/tests/test_slider_protocol.py --build-dir build --target-dir /absolute/existing/cargo-target --old-library /absolute/pre-typed/libcui_bridge.so
```

The runner compiles only the Rust protocol test against that old library and 17
tiny providers, then runs each process through private Xvfb. The actual-old case
has no preload; the other cases explicitly preload one optional-group provider
while legacy entrypoints still come from the old module. All16 missing symbols,
version/size rejection, malformed copied records and u64 extremes are separate
cases. This synthetic evidence does not replace actual authored UI tests, prove
another OS, or establish physical input/accessibility delivery.

The same optional Linux protocol proof can be registered in the repository CTest
suite with `-DCUI_RUST_SLIDER_OLD_LIBRARY=/absolute/path/libcui_bridge.so`.
`CUI_RUST_SLIDER_TARGET_DIR` selects a reused Cargo target directory (default:
`build/rust-slider-target`). It requires existing Cargo/C11 tools and cached
prerequisites; the runner uses offline/locked builds, downloads no historical
binary and runs through the private-display wrapper. This optional provider gate
does not replace current authored UI, default-static or minimum-compiler checks.
