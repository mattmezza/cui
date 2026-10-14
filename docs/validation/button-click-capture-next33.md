# Button click capture proposal

The pinned component at commit `87d10ae5e1299d1be18670a37570c5402f952e34`
passes `&ClickEvent` to its Button handler (`button/button.rs:447–451, 840–847`).
Its pinned dependency `gpui-pre = 0.3.8` defines MouseClickEvent as both down and
up samples (`src/interactive.rs:220–226`), KeyboardClickEvent as button and hitbox
bounds (`264–271`), and TouchClickEvent as tap position/count/long-press (`275–285`).
The current portable Button payload deliberately describes CUI's accepted release,
key, or accessibility event. It does not fabricate those missing source fields.
The exact dependency source was inspected from the published pinned crate:
https://crates.io/api/v1/crates/gpui-pre/0.3.8/download .

The smallest next capture slice is a Core read-only accepted-pointer snapshot
query, populated by actual input routing. A private capture-start sample must
survive frame boundaries alongside the current primary capture. On an accepted
pointer release, Core copies both start and release into a bounded dispatch-index
record. The query returns false before current semantic acceptance, on cancellation,
disabled/loading or accessibility rejection, outside ordinary composition, and
for nonpointer activations. Button can append a `has_press` flag and copied press
sample to its public payload after this dependency is approved.

Each portable sample contains logical x/y, modifier bits, button, pointer type,
contact/device identity and host click count. No OS/Clay types or borrowed text
pointers are needed. The existing maximum dispatch count bounds the record array;
no heap allocation per event or caller contact registry is necessary. A validity
bit prevents an all-zero record from masquerading as a real down event. The query
must expose the original down target and accepted release target explicitly if
retargeting is ever supported; it must not infer down position from current or
previous layout. Existing target routing, modal/clip rules, raw activation queries,
and capture cleanup remain unchanged.

Cancellation, touch-pan takeover, a superseding down, disabled capture cleanup,
and context destruction clear the private start validity. Pointer LEAVE preserves
it because presence is independent of capture. CANCEL clears it; ENTER must not
manufacture it. A touch-primary tap uses the actual contact start/release, while
long-press and source `first_mouse` remain unsupported unless hosts/Core explicitly
provide them. Keyboard source current hitbox bounds are a separate routing-record
contract; no previous-frame bounds should be mislabeled as current-frame geometry.

Meaningful tests should cover same-frame and cross-frame down/up coordinates,
different down/up modifiers and counts, touch identities, cancellation, pointer
leave/reentry, disabled/loading cleanup, nested/modal and clipped hits, multiple
accepted releases in one frame, copied snapshot lifetime and capacity/reset.
Actual browser PointerEvents and CDP touch should verify those samples and ensure
page-pan/capture-loss produces no accepted click. Native SDL queued-event evidence
must remain distinct from physical device claims.

Event-local propagation is a separate remaining contract. GPUI handlers can stop
propagation during dispatch. CUI input routes against the previous hit snapshot
and components accept during ordered submission; consuming after acceptance does
not undo prior ancestor callbacks or recover a consumed event. A future proposal
must define event-local ordered activation participants and when callbacks execute,
including capture/bubble order, modal boundaries, cancellation and accessibility.
A callback return boolean alone cannot implement this. No Core propagation or
capture changes are included in the current Button payload implementation.
