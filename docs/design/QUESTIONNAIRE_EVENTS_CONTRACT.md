# Questionnaire typed observation contract and architecture decision

Implementation checkpoint: the architectural review selected persistent caller-owned
attachment, implemented in `cui_questionnaire_observer.h` and Base hooks on the
`9c6d0db2` integration base. One public-state pointer is the documented ABI change;
consumers must rebuild matching headers/library. Exact implemented declarations
and lifetimes are authoritative in that header and `docs/QUESTIONNAIRE_OBSERVER.md`.
The design rationale/source audit below records the alternatives considered; it
does not expose an additional wrapper or input metadata API. The implementation
also reports defensive PAYLOAD_INVALID for failed copy preflight rather than
mislabeling it overflow. Pure copy-out remains independent of event-time freezing.

Original audit base is `bba4eb75`, frozen source commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. Broader Questionnaire parity and
foreign bindings are not claimed by this observation slice.

## Implemented verification boundary

The worker implementation passed strict Release private-wrapper Questionnaire
observer/model/Styled/keyboard/snapshot tests (5/5), focused WASI equivalents
(5/5), focused ASan/UBSan observer execution, compiled C guide and C++ header,
and actual pinned final format.py --check. Tests compare attached/unattached
legacy callback mutations and handler/user replacement, freeze outer versus
nested Completed/Submit payloads, preserve failed-pair capacity across reset/
disabling, and characterize ordered key/pointer/accessibility/text/timer paths
with UNKNOWN metadata. They also exercise detach pinning and BUSY replacement.
Canonical registration/generated resources and foreign bindings remain separate.
The remaining proposed proof matrix below records broader future acceptance,
not a claim that every future scenario or binding is implemented here.

## Actual legacy emission and timing

Authority: `src/base/cui_questionnaire.c:28–32,217–245,303–334,353–535` and
`include/cui/cui_questionnaire.h`. The only notification mechanism is the retained
`state.event/state.event_user`. Each emit synchronously reads those live fields
and invokes the callback with `{kind,item,previous}` and a const state pointer.
No queue or copied name/answer/submission exists. The const pointer does not
prevent a host from holding another mutable model pointer.

| Trigger | State at emission | Later work and multiplicity |
| --- | --- | --- |
| Changed choice via activate | Effective selection updated; skipped cleared; pending and complete cleared; attempted validation refreshed. | One AnswerChanged only for effective change; multiple toggles change each time. Disabled activation is silent. |
| choose | Calls activate, including its synchronous callback. | After callback returns, live current/item-disabled/multiple determine whether to confirm an already-selected answer or schedule new selection at now+150ms (zero delay for reduced motion). Callback mutation can suppress that follow-up. |
| sync_input with emit=true | Effective retained answer synchronized; optional single-choice selections cleared; skipped cleared when appropriate. | One AnswerChanged on effective change. set_input and set_answer use silent paths. Styled ordered input can call sync repeatedly within one frame. |
| previous / successful next | Current index already changed, pending cleared. | One CurrentChanged for a different index. Previous does not validate; next validates before choosing live enabled successor. No synthetic CHANGE for an unchanged index. |
| failed submit | All currently enabled items validated; first invalid selected. | CurrentChanged only if current actually moves. No Completed or Submit. |
| successful submit | Pending cleared; if not already complete, complete becomes true. | Completed once, then Submit. Every later successful submit still emits Submit. Completed callback can change state or replace handler before Submit, whose live callback fields are read again. |
| skip | Effective choices/freeform cleared, retained editor draft preserved, skipped set and complete cleared. | AnswerChanged, then confirm against the live state left by that callback; may navigate or submit. No stale precomputed confirm target. |
| tick | Pending cleared before confirming, only when due and current still matches pending item. | Indirect CurrentChanged/Completed/Submit from normal confirm path; no invented timer event. |

Init, set_current, set_answer, set_input, reset, item/choice disable and external
error do not directly emit events. They may mutate pending/completion/current or
revalidate through existing paths. They must remain silent in typed observation
unless the legacy reducer itself emits a notification. Observer attachment must
not add per-frame, paint, validation, focus or lifecycle notifications.

Legacy callback-driven conditional disabling/editing/current changes are real
synchronous behavior. In particular choose checks live state after AnswerChanged;
skip confirms live state after AnswerChanged; submit emits its second notification
without revalidating after Completed. Nested public calls from a legacy callback
can emit their own events inline. Current tests prove basic kind order, repeated
submit and ordered input text traces, not arbitrary reentrant lifetime safety.
Destroying the model/editor/definitions or rebinding its active workspace during
an operation is unsafe today; preserving mutations is not a promise to make
use-after-free safe. Validators remain pure.

## Frozen source payload requirement

`crates/base/src/questionnaire/types.rs:535–632` defines copied item name,
answer/status, copied optional previous/current names, and submission records.
`state.rs:1037–1058` emits a copied answer after reduction; `1180–1193` captures
previous name before current changes and emits copied names. Most importantly,
`state.rs:746–767` constructs one submission after validation, emits Completed
with a clone, then Submit with the same data.

Therefore copying state after each legacy callback is insufficient. A Completed
handler can change an answer, disable an item, reset completion or initiate a
nested submit; the original outer Submit still requires its original frozen
payload. The pure snapshot functions can build those buffers, but they cannot
choose the event-time freeze boundary or hold a shared pair on their own.

## Alternatives and decision requested

1. **Callback replacement wrapper.** Save old callback/user, install a forwarding
   function around one public operation, then restore it. Rejected for full scope:
   a callback can replace event/user itself, nested calls can escape or double-wrap,
   timers/input/ordinary legacy calls are not all covered, and the forwarding
   callback sees Completed only after the reducer has chosen its event shape.
   State rereading at Submit cannot reproduce the pair. A limited wrapper could
   advertise only live-query observations, but must not claim copied source events.
2. **Explicit observed operation family.** Pass an observer argument through new
   private reducers and new public operations/Styled controls. Portable and no
   persistent state attachment, but every navigation/input/timer path needs an
   observed variant. A legacy callback calling an ordinary legacy operation loses
   observation of that nested event unless it explicitly forwards context. This
   can be a valid narrower API, but does not observe all legacy mutations/calls.
3. **Persistent caller-owned attachment (recommended for review).** Add one
   observer pointer to the retained model, initially NULL, and attach/detach a
   caller-owned observer with caller-supplied snapshot frames. Hook existing Base
   emission and the submit pair's freeze/release boundary directly. Ordinary
   Base, Styled, timer and nested legacy calls then see the same attachment.
   No global registry, thread-local forwarding, allocation, input dispatcher,
   capture/bubble rewrite or replacement of event/user is necessary.

Option 3 was selected in architecture review. It is an explicit ABI decision: public state grows by a pointer,
so matching headers/library and binding layout rebuilds are mandatory. Legacy init
must initialize that pointer to NULL; attaching occurs after successful init.
Reset retains the attachment. Copying/moving an attached model/observer by value
is forbidden. An external global registry avoids the layout edit but adds hidden
ownership/thread-safety/teardown problems and is not recommended. The implementation authority was the subsequent explicit architecture decision,
not the original design checkpoint alone.

## Proposed typed payload and origin contract

A new observer header would reuse existing `cui_questionnaire_event_kind`,
`cui_component_event_metadata`, and copy-out snapshot records. Proposed records:

```c
typedef enum {
    CUI_QUESTIONNAIRE_PAYLOAD_READY,
    CUI_QUESTIONNAIRE_PAYLOAD_CAPACITY,
    CUI_QUESTIONNAIRE_PAYLOAD_OVERFLOW,
    CUI_QUESTIONNAIRE_PAYLOAD_NO_FRAME
} cui_questionnaire_payload_status;

typedef struct {
    cui_component_event_metadata event;
    cui_questionnaire_event_kind kind;
    size_t item, previous; /* exact legacy indices, including SIZE_MAX */
    cui_questionnaire_payload_status payload_status;
    cui_questionnaire_snapshot_requirements required;
    bool has_previous_name, has_current_name;
    cui_str previous_name, current_name;
    cui_questionnaire_item_snapshot answer;
    cui_questionnaire_submission_snapshot submission;
} cui_questionnaire_observation;
typedef void (*cui_questionnaire_observation_fn)(
    void *user, const cui_questionnaire_observation *);
```

CurrentChanged uses optional copied names, AnswerChanged uses copied item/answer/
status, and Completed/Submit use the same frozen submission. Irrelevant payload
fields are zero. Presence booleans distinguish absent names from legitimate empty
names. Name copying uses checked text requirements and the same NUL/length rules
as copy-out. Current model UTF-8 validation rejects embedded NUL. The source
snapshot has no input draft, label, validation-message or schema-mutation payload.

Metadata maps CurrentChanged/AnswerChanged to CHANGE, Completed to COMMIT, and
Submit to SUBMIT, while `kind` preserves exact family meaning. The observer has a
required nonzero application target ID; this does not register a UI element.
Default Base origin is UNKNOWN with no evidence flags, no invented frame/index/
activation and zero unused fields. A Base emission cannot infer which pointer,
key, accessibility request or host call caused it. PROGRAMMATIC may only be
marked through a future explicit operation context; frame metadata or genuine
activation needs an audited Styled forwarding seam. Handler installation alone
provides neither. First production slice should make no input-source claim.

## Ordering, freezing and legacy mutation compatibility

Recommended hook order for each existing emission:

1. Capture typed payload at the existing emission boundary, before any user
   callback can change the model. For submit, freeze once after successful
   validation and before setting/emitting the outer completion pair.
2. Invoke the observational typed callback synchronously, if still attached.
3. Invoke the original legacy callback through the existing live event/user fields.
4. Continue the original reducer, checking existing live eligibility unchanged.

Typed callbacks are pure with respect to CUI/model/observer configuration: they may
copy payload bytes and update unrelated host bookkeeping, but must not call model
operations, validators, UI submission, attach/detach, or destroy/rebind resources.
This ordering preserves chronological typed event order when a legacy callback
emits nested events. Delivering typed callbacks after legacy callbacks would
instead expose nested typed events before their parent; that is a possible
alternative but must not be silently mixed with this proposal.

Legacy callbacks retain allowed model mutations and nested calls. If Completed
clears complete or disables an item, the original outer Submit still occurs with
its frozen payload. No revalidation, veto or additional event is inserted. If the
legacy callback replaces event/user, Submit calls the replacement as before. A
missing typed handler never suppresses a legacy callback or changes a return.
One accepted legacy emission yields one typed attempt; repeated successful submit
and batched input edits are not coalesced. No queued replay or per-frame aggregate
may substitute for this synchronous stream.

## Bounded frames, errors and retention

Observer attachment retains only caller-owned object, handler/user and frame-array
pointers. Each frame owns independent item/choice/text workspace; regions must be
disjoint from every other frame, observer state, model and schema, as in copy-out.
Frame count is an explicit caller bound. One ordinary emission reserves a frame
until both callbacks return. A submit pair reserves one frame across both outer
emissions and all intervening nested legacy calls. Nested emitting operations use
additional frames, so they cannot overwrite the outer pair.

All snapshot capacity/overflow checks precede arena writes. READY payloads are
complete, never partial. Insufficient capacity yields CAPACITY and exact needs;
overflow yields OVERFLOW and zero needs; no free frame yields NO_FRAME and zero
needs. A typed attempt is still delivered with family kind/legacy indices and
honest UNKNOWN metadata, but all copied payload/presence fields are zero. No
allocation, retry, dropped legacy event, model rollback or changed legacy return
is permitted. Both members of a submit pair share the same readiness/failure and
requirements; do not retry Submit against mutated state. A missing typed handler
need not perform a copy, but counts must distinguish attempts from deliveries.

Proposed observer statistics are saturating attempted/delivered/payload-failure
counts plus an overflow flag, queryable without changing the model. Exhausted
snapshot capacity is an observation failure, not a failed model submit. The implemented attach/detach/statistics signatures and public observer control
record are in the new header. Its private-prefixed bookkeeping and explicit
workspace frames implement this no-allocation direction. Idle attach to an
unattached model remains a caller precondition; known in-flight attachments reject
reattachment with BUSY. There is no global registry that could detect every
unattached active operation.

The observation pointer and nested views are borrowed through that callback only.
READY bytes are independent of model edits, but frame reuse ends their storage
lifetime. A by-value copy of the observation is not a retained deep copy. Hosts
must copy strings/records into separate owned storage when retaining events.
The outer pair's backing frame is held until the outer submit returns; callbacks
must nevertheless not retain the stack-local observation pointer between calls.

## Removal, shutdown and reentrancy

Attach validates initialized state, nonzero target, idle/disjoint frame storage
and a not-already-attached observer; it does not replace legacy callbacks. One
observer per model is sufficient initially. Idle detach clears the association.
Detach requested from a legacy callback marks observation inactive immediately,
suppresses later typed notifications (including outer Submit), but pins its frame
storage until outermost in-flight release. Reattach/reconfigure while in flight
returns BUSY. No observer shutdown callback is synthesized.

Handler/user and all observer/frame storage remain alive through detach and the
last in-flight operation; detach is not permission to free them on that callback's
stack. Model, schema/editor/workspace and legacy user also remain alive until
outer operation and any renderer consumption finish. Host omission/destruction
must be deferred to that safe boundary. There is no arbitrary destruction guard.
Nested legacy operations retain their existing inline behavior, bounded only for
observation storage; frame exhaustion does not attempt to prohibit recursion.
Typed-handler reentrancy and concurrent use remain forbidden, explicitly.

## Regression proof and remaining scope

Before implementation approval, select persistent attachment versus the narrower
explicit-operation alternative and approve state-layout ownership. Then test:

- Zero/unattached paths: exact legacy event kinds/order/counts, return values,
  callback replacement, focus/timer behavior and silent setters unchanged.
- Answer callback changes current or disables the chosen item before choose's
  follow-up; skip callback changes current before confirm; live enabled policies
  and attempted validation behave identically with observation attached/unattached.
- Completed callback changes answers/disables/resets and performs a guarded nested
  submit. Outer Completed/Submit byte content remains equal to the pre-callback
  snapshot; nested pair has a distinct frame and live newer data. Legacy callbacks
  still observe live mutations and original per-emission handler replacement.
- Current names preserve previous/current identity, absent versus empty names;
  Unicode strings are copied. Keep event-time values even after legacy callbacks
  edit answers. No rereading state for the second pair member.
- Zero/one-short item, choice and text capacities, frame exhaustion, checked
  arithmetic probes, missing handler, removal during legacy callback, rejected
  in-flight reattachment and caller-deferred shutdown. No arena corruption,
  fabricated metadata, partial payload or model failure on observation errors.
- Real ordered pointer/key/accessibility/input/IME paths and due timer calls,
  including multiple accepted edits/events in one frame. Until genuine metadata
  forwarding exists, assert UNKNOWN rather than infer causality from queued input.

Use strict Release private headless wrapper, meaningful Base/Styled regressions,
focused WASI, compiled guide/C++ header and final pinned formatting. No desktop or
new dependencies. Browser marshalling requires a separate binding slice: existing
generic last-event fields do not own copied names/answers/submission arrays and
foreign callback/user lifetimes need an explicit design. Snapshot/observer support
does not complete compound presentation, localization, Form or general styling.
Canonical metadata, generated resources and bindings remain integrator-owned.
The focused observer implementation is the only new family event surface here;
no full-family promotion is justified.
