# Python owning bridge value notifications

`App.poll_value_notifications_v1()` is an explicit manual drain of the [C value notification queue](BRIDGE_VALUE_NOTIFICATIONS.md). It yields frozen owning values without invoking callbacks. Existing `poll_notifications_v1()`, `poll_events()`, `step()`, `run()` and `dispatch()` retain their behavior. All poll/dispatch paths consume successive records from the **same queue**; they do not broadcast copies.

```python
from cui_bridge import (
    App, column, input, TextChangeNotificationV1,
    ScalarChangeNotificationV1, SelectionChangeNotificationV1,
)

with App() as app:
    field = input("message", "")
    app.mount(column().children(field))
    app.step()  # submit; Python dispatch remains explicit
    # After later input/steps, own the queue drain:
    for event in app.poll_value_notifications_v1():
        if isinstance(event, TextChangeNotificationV1):
            print(event.text)
        elif isinstance(event, ScalarChangeNotificationV1):
            print(event.value)
        elif isinstance(event, SelectionChangeNotificationV1):
            print(event.selected_handle)
```

The additive `ValueNotificationV1` type includes the existing notification classes and three new frozen dataclasses. Shared fields remain `handle`, optional origin `node`, exact integer `frame_sequence`, `has_frame`, `node_kind` and frozen complete `legacy` snapshot.

| New class / `kind` | Owning fields | C allowlist |
| --- | --- | --- |
| `ScalarChangeNotificationV1` / `scalar-change` | `value: float`, copied binary64 | Slider, NumberInput, Rating, Pagination. Float-based controls retain their actual float precision; NumberInput retains stored double. |
| `TextChangeNotificationV1` / `text-change` | `text: str`, copied and decoded UTF-8 | Plain Input, Textarea, Editor, InputGroup only. |
| `SelectionChangeNotificationV1` / `selection-change` | `selected_handle: tuple[int,int]`, optional `selected_node` | RadioGroup, Tabs, Select, List, Stepper only. |

Selection handles retain full uint32 slot/generation identity, including a stale historical selection. `selected_node` is resolved only by that exact pair from the current Python registry; slot reuse cannot resolve to another generation. It is a live Widget reference, not independent node ownership, and can later become disposed. The handle, scalar, text and complete legacy snapshot remain usable after later frames and app destruction. Frozen records do not freeze referenced Widgets. Python currently has no public per-node remove API; the stale-generation test explicitly uses the C fixture boundary and updates the test registry.

Existing activation, boolean, table, surface-close and Legacy classes are also returned by the new method. Combobox query/selection, Menu/Dropdown commands and unaudited families retain Legacy records. Typed values reflect existing aggregate enqueue behavior, not direct C's per-event facade semantics or new submit/commit events. The C allowlist and its secure-text limitations remain authoritative: the bridge exposes plain edit text and does not inherit direct C Input facade password redaction. No new secure input, synchronous foreign requests, registration hooks or family parity is claimed.

## Negotiation, errors and lifetime

The generator resolves optional V1 size/version and value-category/poll symbols before reading any queue record. It requests only the intersection of compiled-known SCALAR/TEXT/SELECTION bits and the library's advertised bits; unknown advertised bits are never enabled. An older library lacking symbols, or a library advertising no known value category, raises an explicit `BridgeError` without draining through an old poll. Existing old APIs remain usable. A native V1 layout/version mismatch likewise fails before polling. Existing `poll_notifications_v1()` continues receiving newly captured values as complete Legacy snapshots from the old C symbol.

Text starts with4097 bytes including NUL and retries once for a strictly larger requested capacity, at most1MiB including NUL. Oversized, zero/non-increasing or repeated capacity responses raise `BridgeError`; no truncation occurs. C CAPACITY leaves the live record queued; allocation failures propagate. Invalid size/version/flags/frame presence, inconsistent legacy metadata, missing NUL/invalid UTF-8, unsupported tag/kind/category or malformed typed values raise protocol errors. A decode error follows a successful consuming C poll and does not restore that record. No fallback or replay is attempted.

Payloads are notifications of applied bridge behavior. Reading a record does not invoke its Widget callback; calling `dispatch()` separately drains later records through the existing legacy path. No callback fanout or ownership change is added. The borrowed C text is copied into Python strings before yielding; result reads do not reference a mutable C text buffer.

## Tests and compatibility

[The new Python fixture](../bindings/python/test_value_notifications_v1.py) covers real NumberInput key mutation beyond float precision, copied UTF-8, selection/live and historical generations, old/new shared draining, existing boolean/table views, immutable fields and snapshots surviving app close. Its synthetic Python protocol provider separately checks exact UINT64_MAX frames/uint32 handles, compiled-known capability masking, wrong layout, malformed typed output, large retry and capacity limits. These are conversion/protocol tests, not fabricated UI input evidence. Existing initial V1 and legacy Python suites remain required.

The real old-library case requires `CUI_BRIDGE_OLD_LIBRARY` pointing to an actual initial-V1 library without value symbols. It queues text there, proves the new method reports unavailable, and then retrieves the same bytes through old V1 polling. Without this explicit library the old-library case skips; it must not be counted as proved merely by the synthetic provider.

Adjacent NumberInput and Input composition is also exercised through real input.
A renderer identity collision exposed during this work is fixed; regression tests
cover both ordinary and focused fields without changing public viewport IDs.

Native validation uses `python scripts/test_local.py --build-dir build`; the Python bridge backend is `none` for these fixtures. CTest registers the fixture as `bridge_python_value_notifications`; set the explicit old-library environment variable to include its compatibility case. No other language value wrapper is implemented by this change.
