# Retained Attachment action checkpoint

The baseline at this design checkpoint was `06551a0c`. The first two slices
are implemented. See the [current API guide](../ATTACHMENT_EVENTS.md#retained-actions)
for the C and language APIs. The tables below describe the original baseline.
The [component event contract](COMPONENT_EVENTS_CONTRACT.md) and
[notification checkpoint](BRIDGE_VALUE_NOTIFICATIONS_SLICE.md) establish the
existing ownership and compatibility rules.

| Surface | State and results | Dispatch and lifetime |
| --- | --- | --- |
| Immediate C | Application owns file status and data. `cui_attachment_result` summarizes Open, Remove and Retry. The optional observer copies typed action evidence. | Legacy and observer callbacks run synchronously before focus-scope end in Open, Remove, Retry order. At most one notification per category. Observer payload is pointer-free and borrowed for the call. |
| Retained C | Bridge copies display options, labels and preview pixels. Application owns file selection, lifecycle and file operations. A retained Attachment currently sends no action events. | The app polls the bridge's bounded event queue after stepping. Each record carries its retained handle and frame. Removed handles are discarded during polling. |
| Language builders | Python, JavaScript, TypeScript, Go and Rust expose the retained presentation, sizing and previews. | Their existing dispatch and notification iterators have no typed Attachment action. The first implementation adds the C producer and wire contract. Binding methods and examples require a separate slice. |

## Proposed contract

An additive action mask chooses Open, Remove and Retry independently. A zero
mask keeps the existing noninteractive behavior. The component never performs
the file operation. Retry is available only when the status is Failed. Open
and Remove remain available while a file is uploading or processing when enabled.

Retained Open makes the existing group focusable and gives it a visible focus
indicator. Retained Remove remains visible with no pointer present, allowing
tab navigation and direct touch. Existing immediate C callers keep their
interaction and visibility policies. Disabling or omitting the retained node
suppresses new activations. An accepted action already copied into the queue
retains its place until polled or its originating handle is removed.

Use the existing immediate observer to snapshot the action at its current
interaction point. The bridge copies at most one activation per action category
per card and frame into the ordinary FIFO. No foreign callback runs inside the
C observer. Polling never reruns the action. Missing or ambiguous origin
evidence remains unknown. Source and event index use the actual observed
metadata. A physical input target becomes a semantic part only when its
identity is established.

An opt-in Attachment payload uses the existing V1 record and its bounded union.
The value-capability mask gains one bit. Callers request only supported tags
they know how to decode. Old notification polls and decoders receive a legacy
CLICK snapshot with the same handle, frame and numeric action code. Existing
typed shapes, function signatures and previously shipped payload tags retain
their meaning. A full queue follows the existing capacity error behavior.

## Alternatives and slices

A second FIFO would complicate ordering and shutdown. Extending the old
notification poll's unconditional tags would break its existing Go and Rust
decoders. Calling foreign handlers during composition would add a reentrant
execution path. The masked view of the existing queue avoids these changes.

1. Implement retained Open, Remove and Retry in C and validate real pointer,
   keyboard and accessibility input, per-frame aggregation, copied evidence,
   legacy and typed poll interleaving, capacity, disablement and removal.
2. Bind the action mask and typed queue projection without expanding existing
   exhaustive notification unions. Add runnable actions and fallback tests.
3. Exercise native accessibility and arbitrary retained child composition
   separately. Neither is established by the built-in action slice.
