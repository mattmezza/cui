# Controlled typed Slider through the C bridge

Unreleased C bridge, Python and Node/TypeScript subsets. Include `<cui/cui_bridge.h>` from the same revision as
the library. `cui_bridge_slider_model_versions() & 1` advertises this independent
schema; old libraries lack this symbol. Foreign loaders must resolve the entire
optional symbol group before use. The [Python controlled builder](PYTHON_SLIDER_MODEL.md) wraps this route; [Node/TypeScript](NODE_SLIDER_MODEL.md) uses the same copied C records.
Go and Rust typed builders remain pending. Existing `slider()` builders keep their scalar
float precision, autoacceptance and CHANGE timing.

Enable the owned request queue once, then create kind81 with
`cui_bridge_slider_model_create_v1(app, parent, key, ...)`. Parent and all ancestors
must be Row/Column; there is no implicit root. Generic `node_create` rejects81.
The normal bridge owns the UI tree, copied key/name, reducer and presentation
state. No model pointer or callback crosses this ABI.

## Values, authoring and lifetime

Config, queue options and receipts require exact `sizeof`, schema1 and reserved0;
all size arguments are bytes. Unknown enums/flags reject atomically. Domains and
values retain doubles: Single ignores `start` and copies it as0; Range requires
ordered start/end in the complete domain. Log bounds must be positive. Domain
step must be finite/nonnegative; zero means continuous. Source math and its
numerical limits are in [the value guide](SLIDER_MODEL_VALUES.md).

`rem` must be finite/positive; fixed `length` must be finite/positive. Parent-fill
ignores and canonicalizes length to0. Both orientations use ordinary source flow;
vertical parent-fill inside registered scroll remains unsupported. Reverse
changes fill, not numeric direction. Disabled uses `cui_bridge_set_disabled`;
readonly, request emission, reduced motion and focus-ring visibility are config
flags. Config/direct owner replacement/copied name/changed disabled policy revise
receipt authority. Configure retains the reducer so an active gesture can CANCEL
against its captured old kind/domain. Never automatically accept CANCEL values
under a changed domain.

The copied queue is separate from all legacy polling. Capacity is1..4096, default
4096. Before each node reduces input, reserve conservatively `3*frame_events+1`
free records; CAPACITY leaves that node's reducer, input and callbacks untouched.
Drain backlog before retrying. Already emitted records/application effects are
not rolled back when another node or a later Core operation fails.

## Poll, decide and accept

After a successful step (or from the existing native `after_frame` callback),
poll `cui_bridge_poll_slider_request_v1`. Each result owns every byte, including
64-bit identity, frame/index, raw input evidence and a full Single/Range pair.
No pointer is borrowed from the application. Records remain readable after the
next frame or destroy. Polling does not accept a value or invoke foreign code.

Request `current` is the immutable entry owner snapshot. Repeated original-order
keys evolve `requested` intent, even if an earlier proposal was rejected. The
owner explicitly accepts a complete valid pair using
`cui_bridge_accept_slider_request_v1`; it may instead supply another valid pair.
Only actual issued receipts in the latest admitted painted state-frame are
accepted, in increasing sequence order. This schedules a future repaint: it does
not repaint the already completed frame or enqueue CHANGE. Reject by doing
nothing. There is no rollback/replay of input or already accepted work.

Ordinary subsequent submission, omission, removal/reuse, configuration/source
replacement or invalid Core geometry expires authority. A live expired queued
record remains readable; polling prunes removed origins. Invalid size preserves
queue/output; EMPTY and failures preserve output. Clearing an error does not
restore an invalid semantic snapshot. Geometry-only recovery does not renew
model state-frame authority.

All app access is serialized on the owner OS thread, which stays alive through
destroy. Successful enable captures the TLS owner; later normal/typed ready
operations reject another thread. No migration/concurrent destruction is
supported. Do not nest step/run during native after-frame dispatch. Future
foreign callbacks must copy records before calling users, hold no mutable runtime
borrow, catch exceptions before returning through C, and preserve existing legacy
dispatch timing. This slice adds no foreign callback.

App handles and copied receipts stay within the matching loaded bridge image or
WASM module instance. Epoch uniqueness is scoped to that image/instance, including
all of its apps; independently loaded duplicate libraries/modules have separate
counters. Cross-image receipt mixing is unsupported. These checks validate
ordinary ownership and issued authority; they are not a security boundary against
malicious in-process callers.

`get_presentation` copies actual completed render status, absolute accumulated
Core time and passive deadline. Hosts coalesce
`host_now + max(0, next_wake_seconds - core_time)` with their own deadlines.
Part bounds report actual completed geometry, not ID-derived presence; absent
parts return EMPTY unchanged. Thumb bounds describe the outer rem-sized
interaction/layout box, not the inner `rem-2` surface or outward rings. Disabled
thumb paint is omitted, while disabled track/fill bounds remain available.
Neither observation certifies renderer/native AT
success. The current bridge uses V1 solid presentation; no V2 gradient adapter,
typed-part shadow authoring or native numeric AT delivery is added.

## Target policy

Native C11 TLS/atomic-flag targets are supported. Default WASM and threaded/shared
WASM report versions0 and reject new operations with BACKEND_UNAVAILABLE. An
explicit trusted **unshared final module** build may opt in, following the exact
[reviewed build and final-memory verification policy](design/BRIDGE_SLIDER_MODEL_V1.md).
Compile the actual bridge TU with `-DCUI_BRIDGE_SLIDER_WASM_UNSHARED=1`, without
atomics or reentrant/thread flags. Do not distribute an opted-in archive as if
unrelated final consumers were already verified.
Every final module must pass its normal engine validation and
`python scripts/verify_slider_unshared_wasm.py FINAL.wasm` after linking and any
transformation. The script inspects memory descriptors; it is not a complete WASM
validator or automatic CMake capability negotiation. No shared-Worker TLS claim.
The final host must independently check that its actual `Memory.buffer` is not a
`SharedArrayBuffer`; repeat verification after relinking, embedding or transforming.

Actual C tests compose nested Row/Column Single/Range UIs in both orientations,
deliver pointer/keyboard events, and explicitly accept copied requests. The
internal test additionally exercises the actual Core typed SET channel; that is
not a public bridge SET-injection API or physical AT proof.

## Complete portable example

Copy [the complete C program](../bindings/bridge/examples/slider_model.c). It
composes a label and real Range UI, delivers a pointer focus and keyboard input,
prints current/requested complete pairs, and either accepts UPDATE explicitly or
rejects all proposals. It needs no desktop window, renderer or font.

With the [C11, CMake and Ninja prerequisites](DEPENDENCIES.md), from the checkout:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_slider_model_example
./build/bridge_slider_model_example
./build/bridge_slider_model_example --reject
```

These direct executions are windowless. Other native automated checks use
`python scripts/test_local.py --build-dir build`; never substitute the desktop
display. The example demonstrates this C ABI, not an already implemented foreign
builder or native accessibility adapter.
