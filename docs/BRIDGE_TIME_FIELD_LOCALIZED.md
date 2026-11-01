# Localized TimeField bridge (source checkpoint, unvalidated)

The additive C bridge source is prepared; its compile, runtime, ABI, old-library
comparison and canonical generation gates have **not run**. Do not treat this
checkpoint as supported package functionality. The existing tested scalar
TimeField builders in Python, JavaScript, TypeScript, Go and Rust remain unchanged;
none yet exposes this localized bridge mode. See the
[reviewed contract](design/TIME_FIELD_BINDING_CHECKPOINT.md) and
[direct C widget guide](TIME_FIELD_LOCALIZED.md).

The [complete C program](../bindings/bridge/examples/time_field_localized.c)
authors two real TimeFields inside Row/Column UI, English24-hour and Arabic12-hour,
with independent state, explicit same-day08:00–18:00 bounds, a summary label,
clear-validation and read-only buttons. It queues deterministic host pointer/text
input and drains copied changes after step, updating sibling text from application
state. Its default backend is windowless; it is awaiting compile/runtime proof.

Once a validation lease is available, the intended reproducible invocation is:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF -DCUI_BUILD_BRIDGE=ON
cmake --build build --target bridge_time_field_example --parallel 1
python scripts/test_local.py --build-dir build -R '^bridge_time_field_example$'
```

Use existing build directories/caches where possible. CMake, Ninja, Python3 and a
C11 toolchain are required; see [dependencies](DEPENDENCIES.md). These commands
have not been executed for this source checkpoint. Native automated checks always
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
