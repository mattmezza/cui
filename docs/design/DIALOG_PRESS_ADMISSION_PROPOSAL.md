# Dialog per-open press admission proposal

Status: design, not implementation approval. Supersedes the touch-only264-byte
candidate; keeps all strict old-release failure assertions. No generated/public
production API has been published for the component.

## Evidence and required behavior

Private-Xvfb63930 executed three independent plain caller-child cases. Held touch,
mouse and Space each produced actions1/accepted1/snapshot1 after close/reopen,
two empty frames and old release. The copied public accepted-activation snapshot
was read inside the callback, independently of the child counter. Existing
Body/bar generation is therefore insufficient. Actions/request_close timing and
new pointer/wheel behavior passed separately in5482 --actions.

Keep Dialog owner, all caller IDs, focus relations and ordinary widget semantics
stable. Do not synthesize events, alter Core contacts/capture, replay decisions or
require a post-end finalizer. Accessibility/programmatic activation and ordinary
Enter remain their existing independent paths; no global Enter mapping.

## Bounded state and lifetime

Proposed private record (actual sizeof/offset probe still required):

```c
typedef struct {
    uint64_t id, device;
    uint32_t type, button;
} cui_dialog_press; /* candidate24 */
typedef struct {
    uint32_t count, flags; /* only SPACE_ADMITTED flag */
    cui_dialog_press pointer[17];
} cui_dialog_press_state; /* candidate416, alignment8 */
```

One lazy public Core resource keyed by stable Dialog owner and a fixed private
namespace; no new context pointer, public layout growth or global registry.
Seventeen entries cover existing sixteen Core touch contacts plus one ordinary
primary mouse/pen capture. Only primary ordinary pointer capture is quarantined;
other buttons keep existing noncapturing semantics. No extra Core contact slots.
One416-byte local trial is used during preflight. Actual payload/allocation/slot
cost must be tested, including unavailable slot/bytes and ordinary omission GC.

Allocate only when a relevant fresh DOWN/Space-down needs storage. Missing or
collected storage never grants admission to a continuing press. Unready
composition clears admission (new open, skipped generation, failed snapshot,
clear-alone, previous Body omission where applicable). Inert/layout-only/measure
must not mutate admission, allocate or consume. Ordinary frame-gap detection
invalidates admission on resumption. Nested-modal ineligibility invalidates
local admission without consuming another scope's events.

## Ordered guard before application callbacks

Run after the actual owner scope is begun, before any selected slot callback or
widget reducer, only on ordinary admitted/current-eligible composition. Require
public event_within_scope(owner), still-unconsumed event, and the existing same-
open prior-ready gate. Unready composition keeps its reviewed owner-only guard.
Never consume an event merely because its screen coordinate lies inside Dialog.

Prescan in event order against a local trial. Fresh touch DOWN admits its exact
(type,id,device,button). Fresh primary mouse/pen DOWN replaces the previous
ordinary-pointer admission, matching Core's single ordinary capture. Duplicate
Core-suppressed/consumed DOWN cannot grant admission. A matching admitted press
allows continuation and release; an unknown old continuation/release is consumed
before children. Remove entries on genuine terminal evidence, including converted
SCROLL/GESTURE ENDED/CANCELLED; observe terminal bookkeeping outside scope without
consuming that outside event. Global CANCEL clears local state but is not stolen.

Fresh nonrepeat, non-IME/non-preferred-character Space KEY_DOWN admits Space;
Space KEY_UP requires that admission and clears it. Other keyboard, text,
accessibility and programmatic paths stay unchanged. Repeated Space-down cannot
turn an old held key into a new press. Scope consumption is independent of eventual
child acceptance/disabled state. Current children still apply their own eligibility.

Finish all capacity/size/resource checks before committing trial or consuming any
event/calling application callbacks. Then replay only the deterministic guard
scan against the same immutable routed event list, consume rejected scoped events
through public consume_event, and commit state. This is not input dispatch or
application callback replay. consume_event's existing Core implementation clears
pending/accepted activation slots; prove this through the unchanged strict public
receipt oracles rather than manually changing Core activation/capture.

Core retains quarantined physical contacts until genuine UP/CANCEL, preserving
existing multitouch blocking. No promise to free a physical contact early; after
UP/CANCEL there must be no orphan state and a genuine fresh press must succeed.

## Remaining precise public seam

Touch continuation and key release are identifiable from event metadata. Ordinary
mouse/pen MOVE carries no button-held bit. Suppressing all MOVE would steal normal
hover; allowing all would let a custom drag reducer mutate from an old capture.

Existing cui_pressed combines pointer and Space capture and says "submit behavior
before querying". Calling it before arbitrary children is not currently an
advertised contract. Even relaxing that timing leaves pointer-versus-Space
ambiguity. A conservative guard could suppress movement on any pressed target,
while ordinary unpressed hover and position-derived hover visuals remain intact;
that stronger suppression requires explicit review, not a full hover claim.

A precise alternative is a narrowly reviewed read-only primary-pointer-capture
query usable during ordinary submission before target behavior, returning only
current routed capture ID, with zero outside admission/inert/layout-only/error.
No new state, dispatch, observer, contact cancellation or event mutation. This is
an architectural/API decision for root, not authorized worker code. It would let
the guard leave all truly uncaptured hover MOVE untouched and quarantine only
old captured pointer movement. Stable caller IDs alone cannot reveal that fact.

## Gate before production

Approve the movement seam first, then test-only admission proof: unchanged Body,
action and plain child old touch/mouse/Space failures become successes; fresh
DOWN/Space-down works afterward; unpressed hover, actual dragging, bar capture,
CANCEL, two-contact blocking, unknown ongoing contact, nested scopes/background/
trigger, error/omission/inert, allocation/slot failures and cleanup. Prove public
accepted receipts remain absent for quarantined releases and present for fresh
presses. No full Dialog lifecycle claim before these pass.

## Revised comparison: prefer test-only two-phase scope cancellation

The previous single final-capture query is NOT sufficient: MOVE+UP in one queued
batch clears final capture before callbacks, although earlier MOVE was captured.
A precise query alternative would need per-event evidence and a separate storage/
phase audit. Also, consuming events cannot make persistent cui_pressed false on
idle frames. The416-byte ledger therefore cannot establish arbitrary-child
capture closure by itself. Do not implement that ledger or query as recommended.

Proposed additive Core API, subject to root review and test-only proof first:

```c
/* Cancel owned persistent/pending press input; no focus/semantic withdrawal.
 * Ordinary submission, active named scope; no measurement/inert/layout-only,
 * prepaint/diagnostic/resolution or known-error mutation. */
bool cui_cancel_scope_input(cui_context *ui, cui_id scope);
```

No public record/version, context pointer or resource allocation. Proposed
implementation scans bounded existing current/prior scope/semantic arrays,
activation/event arrays and sixteen contacts. Resolve ownership through explicit
logical scope ancestry, never coordinate bounds/fold aliases. Require full IDs.
Unrelated scopes/background/trigger remain untouched. A valid scope with nothing
to cancel succeeds idempotently; invalid scope/phase fails without partial state
mutation. Exact diagnostic precedence follows existing mutation APIs.

Owned pointer capture and Space capture are cleared; matching capture-start
evidence is invalidated. Owned pending release and activation/accepted-receipt
slots are canceled explicitly, not inferred from event consumption. Preserve
event target/coordinates for diagnostics and existing genuine terminal routing.
No focus changes, semantics removal, model callback or application event replay.
The component's existing unready scoped-event guard separately consumes stale
owned events before children; no broad global consume is added to Core.

For an owned retained contact, clear owner and viewport and mark blocked until
real UP/CANCEL. Both clearing and blocked are required: pinch checks can otherwise
use retained viewport even when blocked. Do not mark inactive/free the physical
contact early; preserve existing multitouch blocking while physically held.
Identify touch capture contacts by copied capture-start type/id/device before
clearing capture, including owner0/viewport0 button/bar contacts. Scroll/gesture
contacts resolve by owner/viewport semantic membership in current OR prior scope.

The Dialog calls twice on an unready/new-lifetime ordinary eligible submission:
(1) after its actual scope begins and before any application callback, cancel
ownership resolved from prior scope/targets; then run the scoped stale-event
consume guard. This clears held mouse/Space before even idle child callbacks.
(2) after children have submitted, while scope remains active and before end,
resolve newly current owners and cancel remaining retained contacts. This catches
scroll-owner IDs that vanished across an omitted/failed prior frame and reappear
in arbitrary children, before the next frame can route against those new IDs.
If a callback synchronously closes owner, scope dismissal/withdrawal is preserved;
never recreate semantics or call the late helper against an inactive owner.

Why the late phase is plausible without retained ancestry growth: when a contact's
owner is absent from the prior snapshot, touch scroll_hit rejects it with no
fallback; a missing captured child also clears capture at end_frame. Such an old
contact must not mutate the first new callbacks; current submission then provides
ownership for the second cancellation. A completely unowned contact with no
owner/viewport/capture cannot acquire a new owner except through a genuine DOWN.
This is a required runtime proof, not an assumption that current publication can
roll back an earlier callback. Invalid partial ownership/current scopes and
ID reuse across unrelated scopes must be tested explicitly or rejected by the
component's ordinary identity contract.

Expected zero additional bytes is only a candidate: prove implementation uses
existing arrays/flags with actual before/after sizeof and allocator counts. If
missing/failed/nested ownership disproves this two-phase argument, stop and
propose bounded retained full-ID ancestry metadata; do not claim zero growth or
quietly add128 bytes to opaque context. No production Core edit approved here.

Proof gate extends all strict old-release cases with idle pressed-state, actual
child drag, MOVE+UP batch, omitted/failed prior scope, unknown ongoing contact,
new genuine DOWN after UP/CANCEL, nested modal/portal/background/trigger and
inert/layout-only/error no-mutation. Verify callback counts/order, current enabled
semantics, frozen legacy trace and no duplicate cancellation/dismiss callbacks.

## Integrator review resolution

The final two-phase scoped-cancellation alternative is approved for an unpublished
proof only. The earlier ledger and final-capture-query alternatives are withdrawn.
This is an explicit additive lifetime operation over existing dispatch, not a new
input-dispatch architecture; legacy routes must remain unchanged. Production API
publication waits for the complete proof gate above and source review. Pending
activation cancellation must use the explicit existing cancellation operation,
independently of event consumption. No new storage or zero-allocation claim is
established by this design approval.
