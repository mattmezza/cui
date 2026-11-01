# Localized TimeField bridge (focused C proof, unreleased)

The additive C bridge has passed focused strict C/C++ compilation, headless UI,
thread/queue/failure fixtures and matching Linux static example checks. The
independent frozen legacy trace matches exactly under private Xvfb. Pinned WASM
checks are compile-only. See the [first receipt](validation/time-field-bridge-first.json)
for initial failures and exact limits. Canonical generation, complete integration
and package gates remain pending; this is not released package functionality.
The existing tested scalar builders in all five languages remain unchanged;
none yet exposes this localized bridge mode. See the
[reviewed contract](design/TIME_FIELD_BINDING_CHECKPOINT.md) and
[direct C widget guide](TIME_FIELD_LOCALIZED.md).

The [complete C program](../bindings/bridge/examples/time_field_localized.c)
authors two real TimeFields inside Row/Column UI, English24-hour and Arabic12-hour,
with independent state, explicit same-day08:00–18:00 bounds, a summary label,
clear-validation, read-only, disabled and bounds-toggle buttons. It queues deterministic host pointer/text
input and drains copied changes after step, updating sibling text from application
state. Its default backend is windowless. Optional `--native` uses the same authored tree
and dispatch through the existing SDL bridge runner, with real host input and no
scripted edits. The headless path passed focused compile/runtime checks. Native interactive
launch remains untested.

When reproducing the focused gate, the intended reproducible invocation is:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_time_field_example --parallel 1
python scripts/test_local.py --build-dir build -R '^bridge_time_field_example$'
```

For an intentional interactive launch after validation, the prepared native path
uses the existing SDL/text adapters and an explicit font (no headless fallback):

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=ON -DCUI_REQUIRE_NATIVE=ON -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_time_field_example --parallel 1
CUI_TEST_FONT=/absolute/path/font.ttf ./build/bridge_time_field_example --native
```

This is a manual interactive invocation, **not** an automated desktop test. It has
not been executed. Automated native validation must remain under the private
Xvfb wrapper and needs its own bounded close/input fixture before registration.
The native callback drains owned observations and handles real button clicks
without nesting step/run. Headless success will not imply native proof.

Use existing build directories/caches where possible. CMake, Ninja, Python3 and a
C11 toolchain are required; see [dependencies](DEPENDENCIES.md). The worker used equivalent strict cached-engine compilation to avoid a139-step
engine rebuild; the exact CMake target commands remain integrator acceptance. Native automated checks always
use the private-display wrapper, never the maintainer desktop.

Include `cui/cui_bridge.h`. Check `cui_bridge_time_field_versions()` bit0 and all
three size functions before enabling mode. The complete optional group comprises
`cui_bridge_time_field_versions`, `cui_bridge_time_field_config_size_v1`,
`cui_bridge_time_field_result_size_v1`, `cui_bridge_time_field_change_size_v1`,
`cui_bridge_time_field_enable_v1`, `cui_bridge_time_field_configure_v1`,
`cui_bridge_time_field_get_config_v1`, `cui_bridge_time_field_get_result_v1`,
`cui_bridge_time_field_clear_validation_v1` and
`cui_bridge_time_field_poll_change_v1`. Future language loaders must resolve the
whole group optionally; existing old-library scalar authoring must remain usable.
This document does not provide those future wrappers.

Enable once with immutable capacity1–4096 (0 selects4096). Configure an existing
kind55 TimeField node to opt in permanently; unconfigured nodes use their legacy
rendering and ASCII generic CHANGE payloads. Localized nodes produce a separate
typed FIFO, never duplicate generic time CHANGE events. Scalar `set_time/get_time`
continue to use their existing records and validation. Owner values outside new
bounds are not clamped; precision/domain validation remains the landed widget's
contract, rather than a new setter coercion.

Configuration is an owned616-byte record, result80 bytes and change64 bytes on
the proposed supported target ABIs, pending target measurements. Set exact
`struct_size` and version1. All configuration reserved fields and unused bytes are
zero. Fourteen explicit tokens (digits0–9, separator, period separator, AM, PM) are
strict prefix-free UTF-8, nonempty and at most16 bytes each without NUL. Four
segment names are0–64 bytes; empty selects the C name. No locale registry,
normalization or automatic case/space guessing exists. Bounds are same-day and
inclusive; inactive bound values are canonically zero. Minute precision requires
seconds0 in active bounds and valid model values. Complete formatted TEXT is
limited to160 bytes independently of the bridge's4096-byte queue-text bound.

Configure copies every token/name byte; temporary caller buffers can be released
after return. Bridge owns state and time. Keep nonzero `edit_generation` stable
for identical owner echoes, increment it for locale/name/domain replacements
that must reset unfinished input on the next ordinary writable submission. Cycle,
precision or changed owner time also follows the existing C widget reset rules.
`clear_validation` changes only transient validation: it does not alter time,
pending/digit state or the independent owner-invalid flag. Direct borrowed C
widget state/options are not foreign records.

Drain changes between steps. Every configured live localized node reserves one
possible observation before time advancement, native polling, Core begin or input
mutation, including disabled/detached/omitted nodes. Insufficient free capacity
returns CAPACITY without consuming input or changing values/drafts. Configure
rejects a live-node count greater than capacity; increasing capacity requires a
new app, not an implicit queue resize. Removed records are pruned when polled;
slot reuse cannot redirect an old observation to a replacement node.

A change is one copied **applied aggregate** previous/final pair, not an owner
proposal to accept again. Net-zero edits produce no change. Result `changed` is
net value difference, `rejected_count` counts rejected original input events and
`queued_change_count` is0/1 queue production, not foreign callback invocation
count. Result `status` describes the widget operation; `validation` describes edit
rejection. `frame_status` records final bridge step status after rendering. A later
frame failure retains already applied changes with non-OK frame status; it does
not prove paint succeeded or undo model/input effects. Explicitly poll even after
step errors if those observations matter. Latest-result getter returns CORE_ERROR
after failed submission and cannot be revived by clearing diagnostics or a
geometry-only recovery; it requires a healthy ordinary widget state-frame.
Ordinary omission makes a formerly current result unavailable.

Output getters/poll require initialized exact size/version; other previous output
bytes are ignored. Failure leaves the output unchanged. Poll copies and consumes
only on OK; retaining that copy after app destruction is safe. No borrowed C view
may cross application callbacks. Callbacks are application dispatch after step,
not inside Core. The example's local C dispatch function illustrates this boundary;
Python's later typed dispatch will require its own teardown/exception tests.

Successful enable opts the app into the existing private owner-thread check,
without enabling Slider. Owner OS thread must remain alive through destroy, all
calls serialized; destroy is unchecked/void, not concurrent-safe. Native NO_TLS
and default/shared-memory WASM are unsupported. Explicit unshared WASM reuses the
existing trusted assertion plus **final-module** verification recipe; no new
shared-memory guarantee is implied. Public support remains unvalidated here.

Still required: matching C target/runtime/failure/legacy proof, Python delivery
with complete actual UI and optional-symbol fallback, then JS/TS, Go and Rust;
direct foreign Base/Styled state/style bindings, IME/clipboard integration,
source one-root focus parity and physical native accessibility remain separate.

## Source packaging and portable closure (audit, not validation)

The new private `src/bridge/cui_bridge_time_internal.h` is included by the bridge
translation unit. It is a tracked `src/` file: `scripts/generate_go_sdk.py` includes
it automatically in both Go and Rust SDK snapshots and their SHA256 manifests.
No new portable engine module or standalone `.c` source-list entry is needed.
The Go bridge wrapper includes the bridge TU; matching Rust packaging compiles
that same TU against its staged SDK. This does not expose localized APIs through
those language builders. Regenerate SDKs only at the authorized canonical gate;
currently committed SDK copies intentionally remain stale relative to this draft.

Python/Node source package snapshots include tracked `src/`, `include/`, tests,
examples and CMake files, and their source manifest hashes every copied file.
The new header and fixture/example paths must be committed before generation or
packaging; untracked files are intentionally omitted. Go/Rust package staging
runs SDK `--check` and therefore must not claim closure before regeneration and
matching-source checks. Provenance remains generated, not edited by hand.

Bridge is separately compiled/linked and is not an amalgamation module in
`cmake/sources.json`. Do not add this private helper to the distribution umbrella
or claim a standalone header-only bridge. The landed localized widget's portable
sources are already in the canonical module list; source bridge tests link that
actual implementation, rather than introduce another state/layout copy.

Prepared durable checks: `bridge_time_field_source_closure` exercises actual SDK
snapshot and Python/Node source-copy producers on a bounded temporary tracked-file
fixture and checks the real private-header bytes/hash. CMake registers source
header C++11 consumers in both include orders when C++ is already enabled.
`bridge_time_field_example_static` separately links the actual bridge TU and C
example with the engine on Linux, without SDL; it is not Go/Rust installed-package
proof. Native ILP32 ABI remains an explicit target probe, not inferred from LP64.

The intended pinned WASM compile-only probe is:

```sh
python tests/fixtures/time_field_bridge/portable.py --zig /absolute/path/to/pinned/zig --build-dir build
```

It checks Zig0.16.0 and compiles ABI plus actual bridge TUs for default, asserted
unshared and atomics/shared-unsupported policies, sequentially with strict C11
warnings. It does not link/run, verify final memory or claim shared-memory support.
The worker ran the closure fixture, both C++ orders, Linux static example and all
four pinned WASM compile objects. Native ILP32, installed SDK rebuild and final
unshared module verification remain pending for this new route. The optional
`bridge_time_field_frozen_wrapped` CMake test requires
`CUI_TIME_FIELD_FROZEN_LEGACY_EXECUTABLE` and
`CUI_TIME_FIELD_FROZEN_LEGACY_LIBRARY_DIR`; it uses the committed comparison runner
inside the private wrapper. Worker equivalent scratch registration passed; the
integrator must validate the durable CMake registration.


## Geometry and deterministic headless input

A newly mounted localized field needs ordinary host frames to establish current
layout and an admitted input snapshot. Its latest-result getter can return
INVALID_STATE before that readiness; bounds alone do not prove input admission.
The complete examples warm ordinary frames and check the public latest result
before injecting deterministic pointer/text input. Keep such synchronization
bounded in tests; do not add sleeps, replay input or bypass the C reducer.

The node bounds describe the compound field, including separators. A center
click can land on a noneditable colon in minute precision. Target an actual
editable segment when writing synthetic interaction tests. The supplied examples
use the first text region and verify applied English/Arabic values plus copied
callback outcomes. Native applications use the ordinary host loop and real input;
this test setup does not authorize a new dispatch or focus policy. One-root Tab
and IME acceptance remain explicit separate gaps.
