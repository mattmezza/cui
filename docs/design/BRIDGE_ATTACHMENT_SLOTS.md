# Retained Attachment slots checkpoint

Baseline `ba4b15b9` exposes fitted metadata, decoded previews and built-in
Open, Remove and Retry in retained apps. Slot composition is implemented in
immediate C through `cui_attachment_options`. The retained tree treats a card
as a leaf. The frozen reference describes independent media, overlay, metadata
and action content with ordinary child controls. [Attachment reference](https://gpui-kit.com/versions/v0.7.1/component/attachment/)

| Operation | Owner and lifetime | Input and notifications |
| --- | --- | --- |
| Immediate custom media, overlay and actions | Caller holds render functions, userdata and child models for submission and keeps borrowed paint resources live through rendering. | The containing card gives built-in actions their existing observation order. Child controls retain their own IDs and handlers. |
| Retained card | App owns file state. Bridge copies options, labels and pixels. Removal invalidates the node generation. | Built-in actions queue after reduction in their existing category order. Old and typed polls consume the same FIFO. |
| Proposed retained slot | Bridge creates and owns a normal generational container under the card. Its descendants use their existing retained lifetimes and theme. | Ordinary child controls emit ordinary events with their own handles during slot submission. Never reinterpret a child's action as a file action. |

## Contract

An optional C factory creates one container per role. Media hosts fallback
content in the clipped preview box, overlay hosts separately composed content
above the preview, metadata appends content after typed title and description,
and actions holds a row of ordinary controls. Only actions takes a preferred
lane width. The actual lane fits the card. Use existing node functions for
child creation, keys, layout gap, bounds and removal. Removing a card or slot
recursively removes its subtree. Stale handles follow existing generation
rules. A rejected creation preserves the current tree and caller output.

The factory alone can attach a slot to the card. Ordinary attachments continue
using their existing presentation, event queue and lifetime contracts. Slots
keep the standard retained tree validation and bounded depth and node capacity.
There is no cross-parent sharing, external userdata or callback retention.
A decoded image takes precedence over the fallback media child. Lifecycle
feedback can omit fallback media. Overlay is independent and can coexist with
an image. Typed metadata keeps its styling and retry routing. Card disablement
suppresses input to slots without overwriting child configuration.

Rendering reuses the existing immediate composition and relative paint order.
A retained overlay and vertical action row are attached to the card's current
layout, with ancestor clipping. The overlay covers the preview. Built-in
lifecycle feedback keeps its foreground layer. Standalone Notification nodes
use the app-wide stack and cannot be created inside these slots.
A slot marked disabled or omitted cannot activate its descendants. The
contents of the action lane use the input and event semantics of the submitted
controls. Child events can be queued earlier than the outer card's built-in
actions in the same frame because child submission occurs before the parent
observer call. Reading the FIFO performs no operations or dispatch.

## Slices and alternatives

1. Add the optional C factory and headless visual, input, ownership, capacity
   and legacy checks. Reuse current children and queue APIs.
2. Add typed authoring and reproducible examples in the language builders.
   Keep portable C handles and existing ordinary child notification shapes.
3. Qualify real assistive technology and any additional slot styling needed by
   the full family acceptance. Media loading and host resource ownership remain
   separate from layout composition.

An attachment-wide generic children list would leave the slot of each child
implicit and weaken validation for old callers. Borrowed foreign renderer
callbacks would add reentrancy and lifetime rules. Fixed explicit containers
let the same retained widgets serve each role through their existing runtime.
