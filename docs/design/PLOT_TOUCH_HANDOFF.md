# Plot touch handoff decision

Status: design checkpoint. The implementation and runtime evidence follow in a
separate change.

## Existing behavior

| Surface | Ownership, value and lifetime | Consumption and timing |
| --- | --- | --- |
| Core viewport gesture | Core retains up to 16 physical contacts by device and contact identity. A converted gesture carries relative pan and scale. The event is borrowed through the frame. | Core resolves a target before component composition. A single contact generates pan after the tap threshold. A pair can produce a scale of one, including on its terminal event. |
| Styled Plot | The caller owns the viewport and supplies enabled axes and bounds each frame. Transform updates are synchronous and transactional across both domains. There are no Plot action callbacks. | The reducer consumes targeted gestures, including ones with no domain movement. Consumption is independent of the result code. |
| Gallery host | Browser capture survives a page claim. Host contact storage is bounded. | The host offers unconsumed dominant vertical movement to the document after composition. An enabled viewport currently reserves the whole contact. A claim queues one physical-contact cancellation for Core. |
| Foreign builders | The retained builders expose chart/plot data and presentation subsets. | The direct C viewport policy and dispatch metadata remain outside those subsets. |

## Decision

Add a read-only Core query for the number of physical contacts that formed a
particular dispatched gesture. Core records one for a synthesized pan and two
for a synthesized pinch. Zero means there is no verified contact count. Keep
the metadata alongside the frame events, including any synthetic-cancellation
reordering. Existing public struct layouts and events stay compatible.

Add an opt-in C Plot reducer with a vertical touch-yield policy. The existing
reducer keeps its capture policy. For a known single-contact gesture whose
vertical delta is larger than its horizontal delta, stage the transform. If
the staged vertical domain cannot move, decline the entire gesture, keep both
domains unchanged and leave the input unconsumed. A partial vertical movement
commits and consumes the entire sample. Model errors still return their status
and retain input. Horizontal gestures, multi-contact gestures, unknown contact
counts, terminal events, mouse drag and wheel use the capture policy.

Core keeps the original event target. The page host may act on an offered
gesture after composition. This change does not add Core propagation or send a
Plot gesture to an enclosing native scroll reducer. Host policy owns browser
default prevention, movement accounting and cancellation. Only the gallery's
explicitly registered Plot permits this exception to viewport protection.
Existing drag/modal/multitouch protections and frame-level consumption checks
still apply. Page ownership is retained until release or cancellation.

## Alternatives considered

- A different CSS `touch-action` can separate axes before dispatch in a static
  layout. It cannot express a Plot boundary and can preempt touch pinch.
- Inferring pinch from scale would misclassify two contacts translating without
  a size change. Use Core's contact count.
- Native ancestor chaining would need compatible gesture-to-scroll conversion,
  ordering and consumption semantics. It is a separate architecture decision.

## Implementation and verification

Implement the Core query and the opt-in reducer together with headless
dispatch and compatibility checks. Connect the gallery with a pan-axis
control and verify real Chromium contacts, including partial movement at a
boundary, refused vertical movement, retained horizontal pan, and pinch on the
same Plot. Keep the examples and metadata aligned with direct C support.
