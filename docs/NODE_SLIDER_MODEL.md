# Node and TypeScript controlled Single/Range Slider

Unreleased bridge-owned subset over [C V1](BRIDGE_SLIDER_MODEL.md). Legacy
`slider()` keeps scalar float autoacceptance and original CHANGE timing. This
route does not export direct C Base/Styled state, arbitrary owned styles or
motion structs. The [Rust subset](RUST_SLIDER_MODEL.md) has separate dynamic and matching
bundled-static Linux1.99.1 proof; MSRV1.85.1 remains untested. Native numeric AT, typed SET/touch injection,
V2 gradient adapters and vertical registered-scroll admission remain pending. Full Slider and all-language closure remain open.

## Complete authored applications

[JavaScript](../bindings/node/examples/slider-model.cjs) and
[TypeScript](../bindings/typescript/examples/slider_model.ts) define real labels,
nested Row/Column controls, external owner storage, accept/reject policy, callback
and composition. Their default programs construct windowless UIs, focus actual
thumb geometry, submit keys and verify explicit full-pair owner acceptance and
rejection. The JS example accepts `--single`, `--vertical`, `--reject`; `--native`
uses SDL with `CUI_FONT_PATH` supplied manually. Automated native tests always use
private Xvfb through `scripts/test_local.py`; browser tests remain headless.

With [C11/CMake/Ninja prerequisites](DEPENDENCIES.md), Python3, Node18+ and its N-API
headers, build without native windows from the repository root:

```sh
cmake -S bindings/node -B build-node -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build-node --target cui_node_bindings
export CUI_NODE_ADDON="$PWD/build-node/cui.node"
node bindings/node/examples/slider-model.cjs
node bindings/node/examples/slider-model.cjs --reject --vertical
node bindings/node/examples/slider-model.cjs --single
```

No npm dependencies are needed for JS. For TS use an existing TypeScript compiler
(verified locally with 5.8.3), without installing from these commands:

```sh
python bindings/node/test/run-slider-model-types.py \
  --tsc /absolute/path/to/typescript/bin/tsc --node /absolute/path/to/node \
  --output "$PWD/build-node/slider-model-types"
```

This compiles readonly/discriminant/BigInt negative cases and executes the complete
TS application. CMake registers it when `CUI_NODE_TSC_EXECUTABLE` finds an existing
`tsc`; override that cache path if needed. Local proof is Linux, not a claim of
verified packages/platform distribution or physical native AT.

## Owned authoring and receipts

`slider_single(end)` and `slider_range(start,end)` return frozen discriminated
values. `slider_config({...})` owns its domain, orientation (`horizontal` or
`vertical`), layout (`fixed` or `parent`), rem/length, sourceGeneration BigInt,
requests/readonly/reverseFill/reducedMotion/hideFocus flags. `slider_model(key,
config,confirmed,text?)` authors kind81. Mount requires only Row/Column ancestry;
the entire tree is preflighted before native node creation. Unsupported optional
symbols fail explicitly rather than producing an old scalar substitute.

Numbers retain double precision; geometry is checked float32. Every uint64 field
uses BigInt, including epoch/source/config/frame/sequence, event targets/indices
and pointer/device/accessibility IDs. Handles are checked uint32 number pairs.
Invalid/nonfinite active values, invalid domains and lossy identity conversions
reject. Parent layout ignores authored length and canonicalizes it to0; malformed
native output with a nonzero inactive length rejects instead of normalizing.

`.configure(config,pair)` replaces complete records atomically, retaining the C
reducer for captured cancellation. `.set_value(pair)` replaces the same kind;
`.get_value()`/`.get_config()` copy actual mounted values. `.disabled()` and
`.label()` use existing policy/name calls. `.part_bounds(0..4)` reads ROOT/TRACK/
FILL/START_THUMB/END_THUMB actual completed geometry, or undefined for absence;
thumb geometry is the outer rem box. Disabled track/fill remain, thumbs omit.
`.presentation()` copies render status and absolute Core-clock/wake values, or
undefined before observation. It certifies no renderer/AT success or V2 support.

`app.enable_slider_requests(capacity=4096)` enables the independent bounded FIFO;
typed trees enable lazily. Reenable is same-capacity only. `supports_slider_model()`
requires the complete 16-symbol group, V1 support and all five exact record sizes.
Old-only applications neither require nor enable this feature.

`poll_slider_request()` returns one immutable owned request or undefined. Nested
receipt/value/domain/input records are copied before leaving N-API and frozen;
no C pointer or view survives into a user callback.
Copied input records require zero numeric evidence when HAS_NUMERIC_VALUE is
absent, including keyboard events and accessibility INCREMENT/DECREMENT. Explicit
numeric SET evidence remains supported. Malformed records are rejected before a
request or receipt authority is published; previously copied records stay valid.
 Copies stay readable after
frames, removal, close or disposal. Keep the original returned receipt object:
reconstructed/cloned numeric fields do not confer acceptance authority. Authority
belongs to one matching App and loaded bridge image, not a process-global security
namespace. Returned records do not retain the App/native handle.

## Timing and decisions

`step_only()` steps without callbacks. Existing `step()` steps then dispatches;
`run()` uses the existing native post-frame trampoline. Legacy callbacks always
run first, then the typed independent FIFO. No global queue ordering is invented.
`dispatch_slider_requests()` explicitly drains that FIFO; its count is popped
records, including records with no registered handler. Existing legacy callback
counts/timing stay unchanged. Do not duplicate side effects through both manual
polling and callbacks.

`.on_request(callback)` or null sets/releases a handler. Call
`app.accept_slider_request(request.receipt, wholePair)` explicitly; another valid
full pair may be accepted. Doing nothing rejects. Immutable current stays the
submission-entry snapshot; requested intent evolves for repeated original-order
inputs. Acceptance never emits legacy CHANGE or retroactively repaints. Already
accepted effects are not rolled back if a later callback/frame fails.

Callbacks may mutate external owner state, enqueue future work/input, accept,
configure, remove or tear down. Nested step/step_only/run/dispatch reject during
typed dispatch. Exceptions propagate outside C, reset the guard and leave later
entries queued; the popped entry is not replayed or autoaccepted. Config changes
can expire later copied receipts without changing their original metadata.

Node preserves its existing teardown distinction: `close()` requests shutdown;
`dispose()` destroys resources. Both stop typed dispatch before another native
poll and release typed handlers. Removal prunes native queue entries and releases
the new handler. Owner OS thread must remain alive through destruction; do not
transfer handles/receipts between Workers, libraries or App instances. There is
no temporary hide/reinsert contract.

## Proof limits

Focused tests exercise actual Single/Range UIs in both axes, explicit acceptance
and rejection, copied lifetime/teardown and protocol errors. Synthetic conversion
fixtures are not UI evidence. Linux optional-group tests use a separate addon
linked to an explicitly supplied actual pre-typed bridge; all 16 missing-symbol
variants, support and size failures use tiny test providers over that old module.
These historical tests register only with `CUI_NODE_SLIDER_OLD_LIBRARY`; ordinary
current-library and synthetic copy tests do not need that artifact. Missing old
fixtures are an omission, not a claimed compatibility pass.
