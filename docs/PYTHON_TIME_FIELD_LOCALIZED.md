# Unreleased Python localized TimeField

Focused worker validation and sixteen configured root CTest entries pass,
including fifteen localized methods against actual current, old and partial
providers, the headless authored example and existing Python regressions. Canonical resource,
usage and installed-skill gates remain pending; no package or native-interactive
validation is claimed.
The C bridge has separate focused proof; see [bridge contract and limits](BRIDGE_TIME_FIELD_LOCALIZED.md).
Existing scalar `time_field(...).time_value(...).on_change(...)` remains unchanged
and usable with older libraries. JS/TS, Go and Rust localized authoring remain
unimplemented; direct foreign C state/style and pure parser/formatter records are
not exposed by this Python subset.

The [complete authored example](../bindings/bridge/examples/time_field_localized.py)
creates independent English/H23 and Arabic/H12 fields, current-value summary,
read-only, disabled, inclusive-bounds and clear-validation controls. Default mode
composes and edits the actual UI headlessly. `--font` shares that same tree and
handlers with the existing native SDL runner, without scripted input. Neither
mode has runtime proof for the Python layer yet.

Intended commands, using a matching existing built bridge (no installation):

```sh
PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY=/absolute/path/libcui_bridge.so python bindings/bridge/examples/time_field_localized.py
PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY=/absolute/path/libcui_bridge.so python bindings/bridge/examples/time_field_localized.py --font /absolute/path/font.ttf
```

The second is an intentional interactive launch, not an automated desktop test.
No native launch has been executed here. Automated checks must run through
`python scripts/test_local.py --build-dir build` in a private display, with the
integrator registering `bindings/python/test_time_field.py` and the windowless
example. Existing Python3 and ctypes suffice; no new dependency is introduced.

## Owned configuration and actual C editing

`localized_time_field(key, config=TimeFieldConfig(), value=TimeValue(0,0), text='')`
returns `LocalizedTimeField`, a kind55 widget opt-in; it does not replace the
legacy factory. The App owns native time/editor state. Python owns immutable
copies, never a native state view. All locale grammar, editing, validation and
focus routing runs through the landed C widget, not a Python reducer.

`TimeValue(hour,minute,second=0)` uses integer24/60/60 ranges (bool rejected).
`TimeCycle.H23/H12`, `TimePrecision.MINUTE/SECOND` and `TimeTextStatus` mirror the
published numeric vocabulary explicitly. `TimeLocale` contains ten digit tokens,
separator, period separator, AM and PM; tokens are exact, pairwise prefix-free,
nonempty UTF-8 of at most16 bytes without NUL. No locale registry, normalization,
case folding or Python date parser is consulted. `TimeFieldConfig` adds optional
`minimum/maximum` TimeValues, nonzero uint64 `edit_generation`, four segment names
(at most64 UTF-8 bytes, empty selects C default) and strict bool
`disabled/readonly/owner_invalid`. Bounds are same-day/inclusive, not overnight;
minute precision requires zero active-bound seconds.

Temporary config/locale construction is safe: native configure copies the entire
616-byte owned wire value before return. Both mounted and unmounted configure
validate the same bounded record constraints. A native failure leaves the Python
stored config unchanged. Change `edit_generation` for locale/name/domain resets;
keep it stable for identical owner echoes. Cycle/precision and owner-time changes
follow the actual C synchronization rules on the next ordinary writable frame.
Do not manually clear/reset the native state every step.

Methods:

- `configure(config)` and `get_config()` update/copy owned config.
- `time_value(h,m,s=0)` and `current_time()` retain the old setter/tuple semantics;
  `get_value()` returns a `TimeValue`. Domain-outside owner time is not clamped and
  owner replacement does not manufacture an edit observation. Minute hidden
  nonzero seconds remain invalid under the C widget rather than silently dropped.
- `get_result()` returns immutable `TimeFieldResult`, or propagates the actual C
  unavailable/failed-frame status; it never substitutes a stale result.
- `clear_validation()` changes transient edit validation only, not time, draft,
  owner-invalid or bounds.
- `on_time_change(callback_or_None)` registers the typed applied-change observer.
  Localized `.on_change` rejects rather than silently promise a generic ASCII
  event; legacy `time_field` retains its original `.on_change` behavior.
- `remove()` removes the native node, clears its handler and disposes this widget.

## Dispatch, failure and lifetime

`App.supports_time_field()` checks the **complete ten-symbol optional group**,
versions bit0 and exact616/80/64 native layouts. Missing/partial/unsupported groups
leave scalar authoring available; localized mount fails before creating the tree.
`enable_time_changes(capacity=4096)` can explicitly choose0–4096 (0 means4096);
otherwise localized mount enables the default queue. Capacity is immutable after
enable; same-size reenable is idempotent. Each configured live node reserves one
possible observation before step mutates time/native input/Core state. Queue-full
preflight leaves queued input and values/drafts intact; drain and retry.

Python `step()` applies native edits without running callbacks. `poll_time_changes()`
yields immutable `TimeFieldChange` copies; `dispatch_time_changes()` invokes typed
handlers. General `dispatch()` retains existing legacy callbacks first, controlled
Slider dispatch next, and localized TimeField callbacks last, each in its own FIFO.
No new global cross-queue ordering is implied. Manual polling consumes the same
records as dispatch; reading `get_result()` does not consume or replay them.

Records retain full integer `frame_sequence` and `(slot,generation)` without float
conversion. They retain no App/Widget/C pointer and confer no receipt authority.
A change is one **already applied net aggregate** previous/final pair. Do not
reapply it via `time_value` or invent an acceptance API. Net-zero editing emits no
change. Result `changed` is net difference; `queued_change_count` is0/1 native
queue production, not Python callback count; `rejected_count` counts original
rejected events, not digits. Operation `status`, edit `validation` and final bridge
`frame_status` are separate. Malformed output sizes/enums/reserved/UTF-8/identity
or canonical inactive fields are rejected before any callback sees a record.

A later failed frame retains applied changes with non-OK `frame_status`; explicitly
call dispatch/poll after catching step error to observe them. This proves model
mutation, not painting/native accessibility success. Latest-result getter stays
unavailable after failed/omitted/unready submission until ordinary healthy
recovery; diagnostic clearing alone is insufficient.

Callbacks run only after C returns and may update external owner state, sibling
text/config or handlers, remove a widget or close the App. Nested step/run/typed
dispatch/poll is rejected. The current record is consumed before calling Python;
on exception it remains a durable Python value, later C records remain queued
unless teardown discards them, and the dispatch guard resets in `finally`.
Close stops polling before another C call/callback and releases typed handler
references. Retained records survive teardown; stale or removed handles never
redirect to another widget. Do not mix live handles/apps/libraries; record copies
are readonly observations, not cross-image authorization.

Successful native enable uses the existing C owner-thread policy; Python records
the enabling thread for safe close because C destroy is unchecked. Owner thread
must stay alive through destroy; serialize calls, never race destruction. No new
thread migration/locking architecture is provided. Default/shared WASM remain
unsupported; this subset claims no IME/clipboard, one-root Tab parity or physical
AT/native interactive proof.

Prepared validation: real nested UI/paste/locale/bounds and aggregate copying,
preflight retry, readonly/disabled, callback exception/close/removal, owner and
app-local queue, failed-frame delivery through an actual later Slider capacity
failure, malformed copied records and each missing optional symbol. Actual old
module proof uses optional `CUI_TIME_FIELD_OLD_LIBRARY` in a separate process;
without that path it skips honestly. The local run supplied the preserved old
module and passed all 14 localized checks without skips; existing regression
modules retained two unrelated optional skips.

For optional real partial-group coverage, compile only the bridge translation
unit against a matching already validated engine archive (no engine rebuild):

```sh
python bindings/python/build_time_field_partial_provider.py \
  --engine-archive /absolute/immutable/libcui.a --output /absolute/libpartial.so
```

Set `CUI_TIME_FIELD_PARTIAL_LIBRARY=/absolute/libpartial.so` when running the Python
test, or configure `CUI_PYTHON_TIME_FIELD_PARTIAL_LIBRARY` for its registered CTest.
The provider renames just the change-size export; remaining exports are real.
Without this optional artifact the separate-process test skips. Keep it separate
from the real older-library proof (`CUI_PYTHON_TIME_FIELD_OLD_LIBRARY`).

The new, unreleased typed `TimeFieldConfig()` defaults to H23, minute precision
and edit generation 1, matching the C initializer and the other typed bindings.
Request `precision=TimePrecision.SECOND` explicitly for `HH:MM:SS` input (as the
complete example does). This alignment does not change the legacy scalar
`time_field()` factory or its seconds values. The focused default-wire and legacy
factory regression added for this change awaits the next serial validation batch.
