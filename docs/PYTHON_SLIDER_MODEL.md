# Python controlled Single/Range Slider

Unreleased Python subset over the matching [C bridge V1](BRIDGE_SLIDER_MODEL.md).
Legacy `slider()` remains scalar/float/autocommit with its original CHANGE timing.
The separate [Node/TypeScript subset](NODE_SLIDER_MODEL.md) uses the same bridge.
Rust controlled builders remain pending, alongside
native numeric accessibility delivery, typed touch/SET injection and V2 gradient
adapters. This does not complete the Slider family or all language bindings.

## Complete authored UI

[Copy the complete Python program](../bindings/bridge/examples/slider_model.py).
It authors a label, nested Row/Column Slider, accept/reject button and owner/status
label. The handler prints actual phase/current/requested pairs and explicitly
accepts UPDATE using a receipt, or rejects by leaving the owner unchanged.
It contains all initialization, callbacks, composition and deterministic input.

With the [CMake/Ninja/C11 prerequisites](DEPENDENCIES.md), Python3.10+ and no pip
dependencies, from the checkout on Linux:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target cui_bridge
export PYTHONPATH="$PWD/bindings/python"
export CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so"
python bindings/bridge/examples/slider_model.py
python bindings/bridge/examples/slider_model.py --reject --vertical
python bindings/bridge/examples/slider_model.py --single
```

These commands produce real windowless Core UIs, pointer focus and keyboard
proposals. For an interactive native window, build the SDL/text adapters and pass
`--font /absolute/path/font.ttf`; the same authored UI uses `App.run()`.
Automated native checks always use private Xvfb through `scripts/test_local.py`.
Replace the explicit shared-library path for the platform; local runtime evidence
for this slice is Linux, not a claim of tested platform packages or registry release.

## Author and configure

Import from `cui_bridge`: `slider_model`, `SliderSingle`, `SliderRange`,
`SliderDomain`, `SliderConfig`, `SliderScale`, `SliderOrientation`, `SliderLayout`,
`SliderPhase`, `SliderReason` and `SliderPart`. Exact definitions are in
[`slider_model.py`](../bindings/python/cui_bridge/slider_model.py).

`slider_model(key, config, confirmed, text="Selection")` is a distinct builder.
Confirmed is `SliderSingle(end)` or `SliderRange(start,end)`, never a scalarized
range. Domain and values retain Python float/C double precision. Config owns
domain, horizontal/vertical orientation, fixed/parent layout, length/rem,
uint64 source generation, request emission, readonly, reverse fill, reduced motion
and focus-ring visibility. IDs/epochs/sequences/targets remain bounded Python ints;
no float conversion. Geometry is float32, exactly as the C contract specifies.

Records are frozen dataclasses. Numeric inputs reject bool/nonfinite values,
invalid ordering/domains and out-of-range integers before wire conversion. Parent
layout ignores/canonicalizes length to0. Configuration/owner validation is
authoritative again in C. `.configure(config, confirmed)` replaces the complete
pair/config atomically, preserving the C reducer for captured CANCEL; `.set_value`
replaces only the same Single/Range kind. `.get_config()` and `.get_value()` copy
actual C values once mounted. Use the existing `.disabled()` and `.label()` for
node policy/name. Call `.on_request(callback)` or `.on_request(None)` to register
or release the Python handler. Handler absence does not change explicit config
readonly/request flags; manual polling remains available.

Mount requires only Row/Column ancestors. Unsupported ancestry and missing API
are detected before any C node is created. Mount lazily enables the independent
FIFO at4096 records; call `app.enable_slider_requests(capacity)` first to select
another bounded capacity. Reenable with a different capacity rejects, without
resizing or discarding backlog. Normal old-only trees do not enable the feature.
Parent-fill follows actual host constraints; vertical registered-scroll admission
and all optional source presentation features are not established by this wrapper.

## Poll, dispatch and decide

`app.step()` only steps, exactly as before. `app.poll_slider_requests()` yields
owned `SliderRequest` copies without callbacks. Each copy includes frozen receipt,
Single/Range current/requested, current/gesture domains and original input/event
metadata. Input/event tags and flags retain the documented C vocabulary; absent
evidence remains absent, rather than guessing a key/pointer source.

`app.dispatch_slider_requests()` invokes typed handlers after C has returned.
`app.dispatch()` first runs the preexisting legacy event dispatch, then the typed
FIFO. There is no global ordering between queues. A legacy callback that advances
the frame can expire pending typed receipts; it is not replayed or retimed.
Native `run()` uses that same after-frame order and existing exception trampoline.

The handler may mutate external owner/userdata, enqueue later input/work, change
configuration, explicitly accept, remove the Slider or close the App. During a
typed callback, nested step/run/dispatch/typed-dispatch reject. No ctypes view,
mutable native borrow or live C call spans user code. Each entry is popped once
before its callback; an exception propagates outside C, resets the dispatch guard,
does not autoaccept/retry that entry, and leaves subsequent entries queued.

Call `app.accept_slider_request(request.receipt, whole_pair)` explicitly. It may
accept another valid full pair, not only `requested`. C validates actual issued
sequence, latest admitted painted frame, source/config revision and current policy.
Polling/rejection does not accept. Current stays the immutable submission-entry
snapshot; repeated original-order keys evolve requested intent even when earlier
proposals were rejected. Acceptance schedules later paint and never emits legacy
CHANGE. Already accepted work/user effects are not rolled back.

Receipts carry an opaque Python namespace marker with no App or C pointer. Keep
them within their matching app/library instance; serializing numeric fields does
not recreate acceptance authority. The copied data remains readable after future
frames, removal or close, while acceptance can expire. `.remove()` removes this
leaf, prunes later native queue entries and releases its handler. Closing releases
typed handlers; dispatch stops without another C call after close. The owner OS
thread must stay alive through destroy; typed-enabled close enforces that thread.

This retained builder has no temporary hide/reconcile API. Conditional omission
before mounting authors no node; dynamic removal is the supported omission route.
It does not claim retained hidden-node/reinsert semantics from C-only frame tests.

`.part_bounds(SliderPart...)` returns actual completed geometry or `None` for
absence; thumb boxes are outer rem-sized geometry, not inner surfaces/rings.
Disabled track/fill remain present, while thumb paint is omitted.
`.presentation()` returns a frozen render-status/Core-time/passive-wake copy or
`None` before observation. Failed completed frames raise without manufacturing
geometry. Neither getter certifies renderer/native AT success or enables V2.

## Compatibility and proof limits

`app.supports_slider_model()` requires the complete optional symbol group, V1
support bit and all exact native record sizes. Missing/partial/unsupported groups
leave old loads and old-only mount/dispatch unchanged; typed use fails explicitly.
No emulation using old scalar sliders or reduced precision is attempted.

Local checks use the real current bridge for authored UIs and a distinct process
loading the preserved actual pre-typed bridge for fallback. The old binary's
source revision is unknown. CI's actual-old-library case skips unless
`CUI_BRIDGE_OLD_LIBRARY` supplies such a fixture; offline partial-group rejection
still checks every missing symbol. Complete example modes and current-library
ordering/acceptance/errors/lifetime tests do not need that historical artifact.
