# Bridge value notification views (V1)

The C-only `cui_bridge_poll_value_notification_v1` opts into scalar, plain-text and selected-handle views on the **existing shared notification queue**. It preserves the shipped V1 layout, ABI version, size/version queries, old polling functions and dispatch timing. [Initial V1 notifications](BRIDGE_NOTIFICATIONS.md) still define ownership and queue behavior. This API reports existing applied bridge mutations; it is not a direct-C facade, controlled request, callback registration or synchronous foreign veto.

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

Dynamic consumers must resolve the two new optional symbols before calling them. Older libraries lack these symbols; retain old APIs or report unavailable without draining through another poll. The category query returns `CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1 | CUI_BRIDGE_VALUE_SELECTION_V1`; each bit is `1u << payload_tag`. Request only categories understood by your decoder. [Python](BRIDGE_VALUE_NOTIFICATIONS_PYTHON.md) and [JavaScript/TypeScript](BRIDGE_VALUE_NOTIFICATIONS_JS.md) and [Go](BRIDGE_VALUE_NOTIFICATIONS_GO.md) and [Rust](BRIDGE_VALUE_NOTIFICATIONS_RUST.md) owning value polling are implemented separately. Rust static builds require the paired SDK; dynamic exports are negotiated. Existing V1 manual methods retain their initial typed allowlist. Bundled Go/Rust SDK source snapshots include the C API.

## Complete scalar producer allowlist

| Existing bridge producer | New opt-in payload | Precision and event boundary |
| --- | --- | --- |
| Slider | CHANGE / SCALAR | Existing local float is promoted to stored double; no precision beyond the actual float interaction is invented. |
| NumberInput | CHANGE / SCALAR | Stored double copied directly after existing aggregate `result.changed`; numeric draft text is not a text event. |
| Rating | CHANGE / SCALAR | Existing float value promoted to double. |
| Pagination | CHANGE / SCALAR | Existing page stored as model double; not an arbitrary integer/ID transport. |

Read `payload.scalar_value` only for SCALAR. The full `legacy` snapshot is retained, including the same double in `legacy.value`, checked/raw fields, historical selected handle, frame and text length. Typed values are captured beside those fields at enqueue, before later model mutation. Poll does not parse JSON or query current node state. Existing component aggregate counts, input ordering and render traversal stay unchanged; this does not add per-edit/per-input multiplicity or COMMIT/SUBMIT notifications. Programmatic setters and render-only Progress do not acquire notification producers.

All other producers retain their existing classification: initial CLICK/BOOLEAN/TABLE/SURFACE tags are unchanged, and remaining CHANGE producers stay LEGACY. Combobox, unaudited selection composites, date/time/color and Menu/Dropdown commands remain LEGACY. No family parity claim is made.

## Complete plain-text producer allowlist

Input, Textarea, Editor and InputGroup emit CHANGE / TEXT only when their existing aggregate changed result enqueues a record. Read the complete UTF-8 bytes from the separate caller buffer and `text_length`; there is no text pointer in the union. `legacy.text_length` describes the same owned queued copy. Existing input buffers hold4096 bytes plus NUL; lengths are UTF-8 bytes, not characters. Multiple edits in one frame remain a single final aggregate snapshot. No before/after selection, individual edit, submit, commit or reason is invented. InputGroup addon CLICK events retain their separate origin/order.

Text classification occurs only after the existing enqueue copy succeeds; allocation/queue failure behavior is unchanged. A later setter changes the model without rewriting queued bytes or producing an event of its own. Old V1 and unrequested TEXT masks still return the complete LEGACY text. Textarea/Editor do not gain secure masking. The bridge currently exposes no masked/password configuration and copies underlying plain model bytes; direct C Input facade redaction does not carry over. OTP, Speech and TextView remain LEGACY. Future secure support must address both typed and legacy queued copies before exposing a security option; this API makes no secure-text promise.

The public bridge key enum lacks Backspace/Delete/select-all keys, so the portable fixture cannot prove deletion-to-empty through that input surface. It tests initial empty model/setter silence instead of fabricating an empty edit event. The general V1 empty-text capacity contract remains one byte for NUL.

## Complete selection producer allowlist

RadioGroup, Tabs, Select, List and Stepper emit CHANGE / SELECTION at their existing aggregate enqueue sites. `payload.selected` is the full uint32 slot/generation handle captured at enqueue; `legacy.selected` retains the same historical identity and `legacy.value` retains its old numeric slot snapshot. This is a bridge Item handle, not a Core uint64 ID, numeric index, previous-selection record or direct-C selection callback. No reason/input index/activation evidence is invented.

Existing validation captures a nonzero selection only for a live direct child. A nonzero model value that fails that validation remains LEGACY rather than being mislabeled a clear. An explicit model0, if an existing producer enqueues it, represents an empty handle `{0,0}`. The public clear setter itself is silent; tests do not invent an empty selection input event to exercise that representation. Normal public selected setters reject stale/wrong-parent handles before mutation.

The enqueue count and final-state aggregation remain unchanged. List can enqueue the same selected Item again on activation. Select's existing final comparison can suppress a complete frame that returns to its initial item; direct C's newer per-transition observer does not cause bridge records to appear. Delayed delivery preserves the captured generation after selected-item removal/reuse; it never resolves a historical slot to a replacement. Origin-staleness cleanup remains the existing poll policy.

Menu and DropdownButton commands remain LEGACY even when their old selected field contains an Item handle. Combobox query and selection enqueue sites also remain LEGACY, including their text, because they share a mixed snapshot model with different reasons. Tree/Carousel/Command/Sidebar/Dock/Settings are not classified merely because their value looks like a child slot. No additional manifest family or synchronous foreign callback is introduced.

## Backward compatibility and failures

Existing Go/Rust V1 decoders reject unknown/reserved tags after consuming a record. Therefore `cui_bridge_poll_notification_v1` continues returning newly captured value producers as LEGACY, with complete unchanged legacy fields/text and an inactive zeroed union. The new poll with mask0 does the same. The new poll returns SCALAR/TEXT/SELECTION only for the requested category and its explicit allowlist. Legacy `poll_event_ex` retains its original scalar snapshot; filtered `poll_event` still skips/discards CHANGE while looking for CLICK. Every path drains the **same FIFO**; it never broadcasts duplicate events.

Unsupported mask bits (including bits for NONE/BOOLEAN/TABLE/SURFACE/LEGACY), wrong struct size, null fixed-output or null required-capacity pointer fail INVALID_ARGUMENT before stale cleanup or queue consumption, with outputs intact. This check does not modify the legacy poll contract. Existing stale origin/generation cleanup still happens during a valid poll. CAPACITY changes only `required_capacity`, preserving fixed/text output and the live record; preceding stale cleanup is not rolled back. Empty text requires capacity1 for NUL. Exact-capacity retry retrieves the same captured scalar. EMPTY and other failures preserve outputs. Caller storage extents must be valid and disjoint as required by V1.

The poll allocates nothing, adds no text cap, truncates nothing and introduces no queue or per-event typed allocation. Existing queue-full behavior is unchanged: a control may already have mutated before enqueue fails, and prior records remain queued. No rollback is implied. Copied scalar/metadata records survive later frames, origin removal and app destruction while caller storage survives; handles do not own live nodes.

`node` remains uint32 slot/generation identity; frame sequence remains exact uint64 and only HAS_FRAME is available. No input index, source, pointer/key/accessibility snapshot or Core `cui_id` is inferred. Do not convert uint64 frame identities to a lossy JS Number in future wrappers; the scalar's double representation is unrelated to frame/handle precision.

## Proof and validation

[test_bridge_value_notifications.c](../tests/unit/test_bridge_value_notifications.c) drives real headless bridge input for all four producers, including NumberInput key mutation above float precision, pointer Slider/Rating/Pagination changes, delayed capture after a later setter, disabled controls and closed Collapsible child suppression. There is no new public bridge inert/layout-only switch; the fixture exercises existing closed-content suppression rather than injecting Core routing. It freezes V1 size/offsets, verifies category negotiation, invalid mask/size/output atomicity, exact capacity retry, mask0/old-V1 downgrade, old extended/filtered poll interleaving, unchanged boolean/text classifications and queue exhaustion without false rollback. Existing initial-V1 and bridge core/component tests remain required.

```sh
python scripts/test_local.py --build-dir build -- -R '^(bridge_selection_notifications|bridge_text_notifications|bridge_value_notifications|bridge_notifications_v1|bridge_core|bridge_components)$'
```

[test_bridge_text_notifications.c](../tests/unit/test_bridge_text_notifications.c) additionally drives all four text producers with real UTF-8 input, one aggregate for two queued edits,4096-byte boundary/exact retry, delayed copies, old/masked-out/extended-poll interleaving, unchanged disabled behavior and queue-full mutation without rollback. It does not inject a secure bridge configuration or allocation-failure hook.

[test_bridge_selection_notifications.c](../tests/unit/test_bridge_selection_notifications.c) additionally drives all five selection producers, old/masked-out/extended polling, delayed selected handles, stale selected generations and slot reuse, List repeat, Select return-to-initial suppression, clear-setter silence, disabled controls, exact capacity/error atomicity and overflow without rollback. Real Menu/Dropdown commands and Combobox query/selection remain LEGACY even when value masks are requested.

Native automated tests use the invisible wrapper; no desktop display is required. Build registration, generated SDK source snapshots and human/LLM indexes include this C API. Python, JS/TypeScript, Go and Rust have owning value wrappers. The [design checkpoint](design/BRIDGE_VALUE_NOTIFICATIONS_SLICE.md) describes deferred categories and compatibility decisions; it is not evidence those categories ship.
