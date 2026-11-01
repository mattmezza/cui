# Go controlled Single/Range Slider

Unreleased bridge-owned V1 subset, separate from legacy `Slider(key)` scalar
float/autocommit behavior and CHANGE timing. This does not bind arbitrary direct
C Styled style/motion records, native numeric AT, typed SET/touch injection, V2
gradient adapters or vertical registered-scroll admission. Rust remains pending;
full Slider closure remains open. Read the [C contract](BRIDGE_SLIDER_MODEL.md).

## Complete authored UI

[Copy the complete Go program](../bindings/go/examples/slider_model/main.go).
It defines external Single/Range owner storage, a real nested Row/Column control,
labels, an acceptance-policy button and callback. It focuses actual thumb bounds,
submits keyboard input and verifies explicit acceptance/rejection; it is not a
native launcher. Default execution is windowless.

With existing [C11/Go/CMake prerequisites](DEPENDENCIES.md), Go 1.23+ and no fetched
dependencies, first build the C bridge from the repository root:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target cui_bridge
export CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so"
cd bindings/go
GOMAXPROCS=1 GOPROXY=off GOTOOLCHAIN=local go run -tags cui_dynamic -p1 ./examples/slider_model
GOMAXPROCS=1 GOPROXY=off GOTOOLCHAIN=local go run -tags cui_dynamic -p1 ./examples/slider_model -reject -vertical
GOMAXPROCS=1 GOPROXY=off GOTOOLCHAIN=local go run -tags cui_dynamic -p1 ./examples/slider_model -single
```

Use an absolute shared-library path appropriate for the platform. The existing
Linux default build without `cui_dynamic` embeds matching source SDK objects;
that source build can require more compilation than the dynamic commands above.
Do not assume optional weak symbols imply successful static extraction: the
`TestSliderStaticClosure` test requires actual support and typed UI geometry.
Automated native tests run only through the private `scripts/test_local.py`
wrapper; SDL native tests additionally require an explicit pinned font. Local
proof is Linux, not a registry/package release or physical accessibility claim.

## Owned values and explicit decisions

`DefaultSliderConfig()` supplies one complete config. `NewSliderSingle(end)` and
`NewSliderRange(start,end)` return `(SliderValue,error)`, using float64. Value tags
and inactive fields are private; `IsRange/Start/End` expose owned scalar copies.
Config/domain fields are ordinary Go values, with uint64 SourceGeneration,
float64 domain/step, typed scale/orientation/layout and checked float32 geometry.
Configure copies them; modifying an application-local config does not mutate C.
Parent layout ignores authored Length and canonicalizes it to 0. Malformed native
output with nonzero inactive parent length rejects instead of normalizing.

`SliderModel(key,config,confirmed)` returns a retained typed node. Mount validates
records, the complete optional symbol group and Row/Column-only ancestry before
creating nodes. Bad builder records accumulate the existing node error; old trees
do not require or enable typed support. Missing/unsupported symbols produce an
explicit error, never scalar emulation.

Typed node methods are `OnSliderRequest(func(SliderRequest) error)`,
`ConfigureSlider`, `SetSliderValue`, `SliderValue`, `SliderConfig`,
`SliderPartBounds` and `SliderPresentation`. Configure replaces config plus whole
pair atomically and retains captured reducer cancellation. SetSliderValue cannot
change Single/Range kind. Existing Text/Disabled/Remove methods retain their
roles; typed nodes have no old OnChange path.

`App.SupportsSliderModel() (bool,error)` checks the full 16-symbol group, support
bit and all five record sizes. `EnableSliderRequests(capacity)` enables the
independent bounded FIFO (0 means 4096); typed trees enable lazily. Reenable does
not resize/discard a backlog. `PollSliderRequest() (request,bool,error)` pops one
owned copy; `DispatchSliderRequests() (count,error)` invokes registered handlers.
Returned request/input/domain/value/receipt data contains no borrowed C buffers,
pointers to C memory or retained App/native handles. Every C uint64 remains
uint64, including frames, sequence/config/source identities, event indices and
pointer/device/accessibility IDs. Unknown tags/masks and incoherent unavailable
input evidence reject rather than being guessed.

Receipts have private authority fields plus `Info()` returning inert metadata.
Copied input records require zero numeric evidence when HAS_NUMERIC_VALUE is
absent, including keyboard events and accessibility INCREMENT/DECREMENT. Explicit
numeric SET evidence remains supported. Malformed records are rejected before a
request or receipt authority is published; previously copied records stay valid.

Assignment preserves an issued receipt; constructing only metadata does not
invent authority. A nonzero-size private per-App namespace token retains no App
or C pointer. Keep handles/receipts within the matching App and loaded bridge
image; these tokens are not a security or cross-image uniqueness guarantee.
Copied records remain readable after removal/Close; acceptance can expire.

`AcceptSliderRequest(receipt,wholePair)` is explicit and may accept another valid
full pair. Update external owner storage only after acceptance succeeds. Polling,
callback absence or rejection never autoaccepts or emits CHANGE. Current stays
the immutable C submission entry; requested intent evolves across original-order
inputs. Already accepted effects are not rolled back by later failures.

## Timing, callbacks and OS-thread lifetime

`Step` only steps. `Dispatch` preserves old FIFO/callback behavior first, then
drains the independent typed FIFO when enabled; no global queue order is invented.
Typed count is popped records, including entries without handlers. Each callback
receives an owned Go copy after C returns. Application-local mutation cannot alter
the C snapshot. A callback may mutate external owner state, accept, configure,
replace/remove its handler, update labels, queue future input or request teardown.
Nested Step/Run/Dispatch/typed-dispatch reject only during new typed dispatch.

An error stops dispatch after consuming that entry; later entries remain queued.
A panic resets the guard and propagates normally in manual dispatch. Native Run
uses its existing panic/error trampoline, resuming only after leaving C. No C
view/borrow or foreign callback invocation spans user code.

Go keeps its existing affinity: New locks the creating goroutine to its OS thread;
all App calls check that thread. Keep it alive through Close and serialize calls.
Do not copy, overwrite or replace live App/Node storage during a callback; this
protects UI ownership, while external application owner/userdata mutation remains
allowed.
RequestClose requests shutdown; Close destroys and balances the lock (native Run
defers the unlock until it returns). Successful RequestClose records typed-local
closing evidence so a typed handler stops before another poll even though App is
not yet destroyed. Remove/Close release new typed handlers. Do not add finalizers,
move live Apps between goroutines, race destruction or treat receipt snapshots as
cross-thread UI authority. There is no temporary hide/reinsert contract.

Part bounds are actual completed geometry or `present=false`; disabled track/fill
remain and thumbs omit. Presentation copies render status and absolute Core-clock
wake values; neither getter certifies physical rendering/accessibility or V2.

## Proof and fixture limits

Actual headless tests cover Single/Range in both axes, full-pair decisions,
double precision, callback error/panic/removal/Close/RequestClose, expiry, readonly,
disabled parts, ancestry, owner thread and copied lifetime. Protocol providers
are synthetic evidence, not actual UI input. Optional-symbol tests use separate
processes loading a tiny provider that depends on an explicitly supplied real old
bridge; Go's explicit module lookup cannot be tested by preload interposition
alone. Default and dynamic/source modes are separate closure obligations.
Historical notification/shadow/visibility fixtures retain their own skip gates;
a passed current-library suite does not establish those omitted historical tests.

The durable optional protocol runner builds only the dynamic Go test package and
17 tiny providers using existing tools/caches. Supply an actual independent old
library; none is downloaded, copied into source, or substituted with fake old UI:

```sh
python bindings/go/tests/test_slider_protocol.py --build-dir build \
  --library /absolute/path/current/libcui_bridge.so \
  --old-library /absolute/path/pre-typed/libcui_bridge.so
```

This always tests current typed UI, actual-old fallback and synthetic optional/
malformed-record cases in separate processes under private Xvfb. Add `--legacy
--font /absolute/path/Inter-Regular.otf` only when the current library has SDL/text
adapters; this runs the complete existing legacy suite, including native fixtures.
CMake conditionally registers `bridge_go_slider_protocol` when configured with
`-DCUI_GO_SLIDER_OLD_LIBRARY=/absolute/path/pre-typed/libcui_bridge.so`. It is not
registered without that historical fixture. Matching default source closure is
separate: `go test -p1 -run '^TestSliderStaticClosure$' ./cui` from bindings/go,
with CUI_BRIDGE_LIBRARY unset; use the private wrapper for automated native tests.
