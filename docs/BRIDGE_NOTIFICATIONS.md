# Bridge notification snapshots (V1)

The additive C API `cui_bridge_poll_notification_v1` returns a versioned typed view of the **existing** notification queue. It does not register callbacks, change timing, add input routing, or turn applied bridge mutations into controlled requests. The legacy ABI version, options, event structs and polling APIs remain unchanged. [The design](design/BRIDGE_TYPED_EVENTS_SLICE.md) includes future wrapper work; C, Python and JS/TypeScript manual polling are implemented here.

## Capability and polling

```c
#include <cui/cui_bridge.h>

static cui_bridge_status next_notification(cui_bridge_app *app,
                                           cui_bridge_notification_v1 *event,
                                           char text[4097], uint32_t *required) {
    if (!(cui_bridge_notification_versions() & 1u) ||
        cui_bridge_notification_v1_size() != sizeof(*event))
        return CUI_BRIDGE_INVALID_STATE;
    return cui_bridge_poll_notification_v1(app, sizeof(*event), event,
                                           text, 4097, required);
}
```

V1 capability is bit0. The size query includes native ABI padding; never pack the FFI struct manually. Exact size is required before consumption, and successful output includes `struct_size` and `schema_version=1`. Dynamically loaded consumers must look up the optional new symbols; older libraries lack them and continue supporting their old APIs. Future layouts require a new versioned symbol, not writes beyond V1 storage.

Step/render still produces snapshots before foreign-language dispatch. The V1 manual poll only consumes data; reading returned records never dispatches or changes component values. Poll V1, `poll_event_ex`, or filtered `poll_event` from one consumer: all drain the **same queue**, so mixed calls consume successive events rather than independent copies. The old filtered poll still skips/discards every non-CLICK event.

`kind` is ACTIVATE, CHANGE or LIFECYCLE. `payload_kind` determines the active union member. `node` and `node_kind` describe the origin captured at enqueue time; `legacy` retains the complete old checked/value/text-length/selected-handle snapshot. UTF-8 text is copied separately into caller storage. `frame_sequence` stays uint64; `HAS_FRAME` is present only for nonzero captured sequences. Identity is context-local, not a timestamp or global ordering. No input index, source, pointer/key/accessibility evidence, or Core `cui_id` is fabricated from bridge handles.

## Committed initial allowlist

The table is the complete typed producer allowlist for this slice. Do not enable additional tags solely because a generic numeric/text field looks plausible.

| Enqueue producer | `kind` / `payload_kind` | Typed payload |
| --- | --- | --- |
| Existing CLICK enqueue, including simple click adapters | ACTIVATE / NONE | No inferred input evidence. |
| Checkbox, Switch, Toggle CHANGE | CHANGE / BOOLEAN | `payload.boolean_value`, exactly0/1, already applied bridge state. |
| DataTable selection and sort | CHANGE / TABLE | `payload.table`: reason, row, column, sort_column, sort_direction, copied directly at the original enqueue point. |
| Existing CLOSE enqueue (surfaces and notification close) | LIFECYCLE / SURFACE | `payload.surface_result`, existing ACCEPTED/CANCELLED/CLOSED constants. |
| Every other CHANGE producer | CHANGE / LEGACY | Complete `legacy` fields and copied UTF-8 text; no guessed typed meaning. |

DataTable indices preserve the existing bridge convention: zero means no selected row/column/sort column; selected indices are one-based. The existing bridge supports at most16 columns and32 rows. Column2 and above remain integers in V1 instead of becoming booleans. Old `legacy.checked` still contains the raw column integer and old JSON remains unchanged. Typed table fields are captured beside JSON; no poll-time parsing or latest-state getter reconstructs them. Selection reason is `CUI_BRIDGE_TABLE_SELECTION`; sort reason is `CUI_BRIDGE_TABLE_SORT`.

SCALAR, TEXT and SELECTION tags/union members are reserved and **not emitted** by this slice. Input/Textarea/Editor/OTP, numeric controls, selection composites, date/time/color, questionnaire/speech/dock and other producers stay LEGACY. A selected-item handle in `legacy.selected` is a historical slot/generation snapshot. It may be stale at delivery; do not resolve it to a replacement node reusing that slot. Stale origins are dropped under the same rules as existing extended polling.

## Storage, failures and lifetime

Supply disjoint fixed-output, required-size and text storage, with the declared writable extents. Success copies complete text plus NUL, publishes the fixed result, and consumes exactly once. `text_length` excludes NUL. Empty text still needs capacity1. This poll allocates nothing, truncates nothing and adds no new text limit. Existing producers have their own bounds; common editable snapshots fit4096 bytes, so4097 is a useful starting buffer, not a universal schema limit.

On `CAPACITY`, only `required_capacity` changes. The live event remains queued, fixed output/text bytes remain unchanged, and retrying with that capacity retrieves the same snapshot. Preceding stale origins may already have been cleaned up; failure is not a transaction restoring stale entries. `EMPTY`, invalid size/arguments and other errors leave outputs unchanged. C callers choose allocation/retry policy. The Python and Node wrappers start with4097 bytes and retry once with the requested capacity, up to1MiB including NUL. An oversized, non-increasing or repeated capacity response raises an error without truncation; a capacity failure leaves the live event queued for another capable consumer. Memory allocation failures also propagate.

Queue storage keeps the existing max-event policy (default256, maximum4096). New typed fields add fixed per-slot bytes; no second queue or per-event typed allocation is introduced. Existing queued text allocations and failure handling remain. Queue exhaustion can happen after a component has mutated: a failed step does not roll back bridge state or previously queued events.

The returned struct and caller text copy remain valid after later frames, origin removal or app destruction, provided the caller storage survives. They do not own live nodes. C pointer copying is not ownership transfer, and copying arbitrary `cui_event` does not deep-copy borrowed text.

## Python and JS/TypeScript manual mode

Python already separates stepping from dispatch:

```python
app.step()
for event in app.poll_notifications_v1():
    if isinstance(event, BooleanChangeNotificationV1):
        print(event.value, event.frame_sequence)
```

Import `BooleanChangeNotificationV1` from `cui_bridge`. Python returns frozen dataclasses: `ActivationNotificationV1`, `BooleanChangeNotificationV1`, `TableChangeNotificationV1`, `SurfaceCloseNotificationV1`, or `LegacyNotificationV1`. Common fields include `handle` (slot/generation tuple), optional `node`, `frame_sequence` (exact Python integer), `has_frame`, `node_kind`, and frozen `legacy`. Table `reason` is `"selection"` or `"sort"`; indices remain one-based. `legacy.checked_raw` preserves the integer, `legacy.text` owns its string, and `legacy.selected_handle` preserves historical identity. Only `LegacyNotificationV1` adds optional `selected_node`.

```js
app.step_only(); // submit a frame; do not dispatch callbacks
let event;
while ((event = app.poll_notification_v1()) !== undefined) {
  if (event.kind === 'table-change') console.log(event.column, event.frame);
}
```

JS `step()` and `run()` retain automatic legacy callback dispatch. Choose `step_only()` and own the drain for manual mode; it has the same alive/running guards and default `dt=1/60` as `step`. Mixing default dispatch with manual polling consumes successive records from one queue; it never fans out copies. Python `dispatch()` likewise drains the same queue.

JS returns frozen records with discriminants `activation`, `boolean-change`, `table-change`, `surface-close`, or `legacy`. Shared fields are `schemaVersion:1`, frozen `handle`, optional `node`, exact uint64 `frame:bigint`, `hasFrame`, `nodeKind`, and frozen `legacy` (`type`, `checkedRaw`, `value`, `text`, frozen `selectedHandle`). Boolean records add `value`; table records add `reason`, `row`, `column`, `sortColumn`, `sortDirection`; surface records add numeric `result`; legacy records add optional `selectedNode`. TypeScript exports the discriminated `NotificationV1` union and requires BigInt-capable compilation/runtime (for example ES2020/Node18). Convert BigInt explicitly when serializing JSON; converting to Number can lose precision.

Text and raw handle copies survive later frames, removal and app destruction. Optional node references are resolved by exact slot **and generation** at delivery, never by slot alone; they are live wrapper objects, not independent node ownership, and may later become invalid. Returned record immutability does not freeze the referenced node. Python lazily checks optional C symbols, V1 capability and exact native struct size; older libraries remain usable through existing APIs and manual V1 polling raises an explicit unavailable/mismatch error. Node checks both addon availability and optional native symbols/layout. Neither wrapper registers listeners, auto-dispatches new views, or interprets reserved payload tags as scalar/text/selection values.

## Supported boundary and proof

Python and JS/TypeScript provide owning manual views; Go/Rust availability is independent of this slice. Existing callbacks retain their after-frame timing and old payload conversions, including legacy boolean column loss and JS Number frame precision limits. There are no new dispatch/fanout hooks, listener lifetimes, promises, synchronous rejection or request bindings. The [binding audit](design/COMPONENT_EVENTS_BINDINGS.md) remains authoritative for those existing limitations. No full family uniformity or parity is claimed.

`tests/unit/test_bridge_notifications.c` uses the real headless bridge to verify old ABI layouts, shared-queue old/new interleaving, FIFO boolean snapshots versus later mutation, queue exhaustion without false rollback, stale origin cleanup/generation reuse, stale selected-handle retention, size/capacity failures and exact retries, UTF-8/4096-byte copies, all16 DataTable columns plus sort fields, legacy raw fields/JSON and surface close outcomes. Legacy bridge C and Python regression suites remain required. Native CTest runs use the invisible wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R 'bridge_notifications|bridge_core|bridge_components|bridge_python'
```

`bindings/python/test_notifications_v1.py` and `bindings/node/test/notifications-v1.test.cjs` exercise real queued boolean/table/activation records, manual versus legacy consumption and owning copies. Python additionally injects optional-symbol/layout/capacity protocol failures. The Node test-only uint64 conversion fixture checks values beyond2^53 through UINT64_MAX using the polling conversion helper; these are conversion checks, not fabricated input/frame evidence. `bindings/node/test/notifications-v1-protocol.test.cjs` reuses the test-only C provider in `bindings/tests/notification_v1_fixture.c` in fresh Linux processes (preloaded via `LD_PRELOAD`, with `CUI_NOTIFICATION_FIXTURE=edges|capacity|limit|zero|repeat|size|unavailable`). It verifies full native-to-JS uint64 conversion, large copied payload retries, explicit protocol errors, and continued legacy dispatch. These injected records are separate from the real queued tests. `bindings/node/test/notifications-v1.types.ts` checks union narrowing/read-only fields when compiled with `tsc --noEmit --strict --target ES2020 --module Node16 --moduleResolution Node16`.

Build/test registration, exports/generated SDKs, and human/LLM guide indexing are integrator-owned. This guide documents implemented source behavior; wrapper availability must be checked separately.
