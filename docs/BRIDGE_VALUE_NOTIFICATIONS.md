# Bridge scalar notification views (V1)

The C-only `cui_bridge_poll_value_notification_v1` opts into scalar views on the **existing shared notification queue**. It preserves the shipped V1 layout, ABI version, size/version queries, old polling functions and dispatch timing. [Initial V1 notifications](BRIDGE_NOTIFICATIONS.md) still define ownership and queue behavior. This API reports existing applied bridge mutations; it is not a direct-C facade, controlled request, callback registration or synchronous foreign veto.

```c
#include <cui/cui_bridge.h>

static cui_bridge_status next_scalar(cui_bridge_app *app,
                                     cui_bridge_notification_v1 *out,
                                     char *text, uint32_t capacity,
                                     uint32_t *required) {
    if (!(cui_bridge_notification_versions() & 1u) ||
        cui_bridge_notification_v1_size() != sizeof(*out) ||
        !(cui_bridge_notification_value_payloads_v1() & CUI_BRIDGE_VALUE_SCALAR_V1))
        return CUI_BRIDGE_INVALID_STATE;
    return cui_bridge_poll_value_notification_v1(app, sizeof(*out),
        CUI_BRIDGE_VALUE_SCALAR_V1, out, text, capacity, required);
}
```

Dynamic consumers must resolve the two new optional symbols before calling them. Older libraries lack these symbols; retain old APIs or report unavailable without draining through another poll. The category query currently returns only `CUI_BRIDGE_VALUE_SCALAR_V1` (`1u << CUI_BRIDGE_PAYLOAD_SCALAR`). Text and selection categories remain unavailable. No Python, JS/TypeScript, Go or Rust value-poll wrapper is included yet; their existing V1 manual methods retain their initial typed allowlist. Their bundled SDK/export updates are integrator-owned.

## Complete scalar producer allowlist

| Existing bridge producer | New opt-in payload | Precision and event boundary |
| --- | --- | --- |
| Slider | CHANGE / SCALAR | Existing local float is promoted to stored double; no precision beyond the actual float interaction is invented. |
| NumberInput | CHANGE / SCALAR | Stored double copied directly after existing aggregate `result.changed`; numeric draft text is not a text event. |
| Rating | CHANGE / SCALAR | Existing float value promoted to double. |
| Pagination | CHANGE / SCALAR | Existing page stored as model double; not an arbitrary integer/ID transport. |

Read `payload.scalar_value` only for SCALAR. The full `legacy` snapshot is retained, including the same double in `legacy.value`, checked/raw fields, historical selected handle, frame and text length. Typed values are captured beside those fields at enqueue, before later model mutation. Poll does not parse JSON or query current node state. Existing component aggregate counts, input ordering and render traversal stay unchanged; this does not add per-edit/per-input multiplicity or COMMIT/SUBMIT notifications. Programmatic setters and render-only Progress do not acquire notification producers.

All other producers retain their existing classification: initial CLICK/BOOLEAN/TABLE/SURFACE tags are unchanged, and remaining CHANGE producers stay LEGACY. In particular Input/Textarea/Editor, Combobox, selection composites, date/time/color and Menu/Dropdown commands are not newly classified. Plain and secure text policies are outside this scalar slice; the bridge does not inherit direct C Input facade redaction merely by using V1. No sensitive-text or family parity claim is made.

## Backward compatibility and failures

Existing Go/Rust V1 decoders reject unknown/reserved tags after consuming a record. Therefore `cui_bridge_poll_notification_v1` continues returning scalar producers as LEGACY, with complete unchanged legacy fields/text and an inactive zeroed union. The new poll with mask0 does the same. The new poll with the scalar bit returns SCALAR for only the four allowlisted producers. Legacy `poll_event_ex` retains its original scalar snapshot; filtered `poll_event` still skips/discards CHANGE while looking for CLICK. Every path drains the **same FIFO**; it never broadcasts duplicate events.

Unsupported mask bits (including reserved text/selection bits), wrong struct size, null fixed-output or null required-capacity pointer fail INVALID_ARGUMENT before stale cleanup or queue consumption, with outputs intact. This check does not modify the legacy poll contract. Existing stale origin/generation cleanup still happens during a valid poll. CAPACITY changes only `required_capacity`, preserving fixed/text output and the live record; preceding stale cleanup is not rolled back. Empty text requires capacity1 for NUL. Exact-capacity retry retrieves the same captured scalar. EMPTY and other failures preserve outputs. Caller storage extents must be valid and disjoint as required by V1.

The poll allocates nothing, adds no text cap, truncates nothing and introduces no queue or per-event typed allocation. Existing queue-full behavior is unchanged: a control may already have mutated before enqueue fails, and prior records remain queued. No rollback is implied. Copied scalar/metadata records survive later frames, origin removal and app destruction while caller storage survives; handles do not own live nodes.

`node` remains uint32 slot/generation identity; frame sequence remains exact uint64 and only HAS_FRAME is available. No input index, source, pointer/key/accessibility snapshot or Core `cui_id` is inferred. Do not convert uint64 frame identities to a lossy JS Number in future wrappers; the scalar's double representation is unrelated to frame/handle precision.

## Proof and validation

[test_bridge_value_notifications.c](../tests/unit/test_bridge_value_notifications.c) drives real headless bridge input for all four producers, including NumberInput key mutation above float precision, pointer Slider/Rating/Pagination changes, delayed capture after a later setter, disabled controls and closed Collapsible child suppression. There is no new public bridge inert/layout-only switch; the fixture exercises existing closed-content suppression rather than injecting Core routing. It freezes V1 size/offsets, verifies category negotiation, invalid mask/size/output atomicity, exact capacity retry, mask0/old-V1 downgrade, old extended/filtered poll interleaving, unchanged boolean/text classifications and queue exhaustion without false rollback. Existing initial-V1 and bridge core/component tests remain required.

```sh
python scripts/test_local.py --build-dir build -- -R '^(bridge_value_notifications|bridge_notifications_v1|bridge_core|bridge_components)$'
```

Native automated tests use the invisible wrapper; no desktop display is required. Build registration, exports, generated SDKs, wrapper additions and human/LLM index regeneration remain separate integrator work. The [design checkpoint](design/BRIDGE_VALUE_NOTIFICATIONS_SLICE.md) describes deferred categories and compatibility decisions; it is not evidence those categories ship.
