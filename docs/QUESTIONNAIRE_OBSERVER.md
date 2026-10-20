# Questionnaire typed observation

Include `cui_questionnaire_observer.h`. Rebuild every consumer/binding with matching
headers and library: `cui_questionnaire_state` gained one observer pointer. Legacy
initialization clears it; reset retains it. Legacy event/user fields, synchronous
callback mutations, reducer return values and routing stay in place. No new input
dispatch architecture or foreign-language callback bridge is provided.

The observer is persistent but entirely caller-owned. Attach only after model init
and while idle. A zero-initialized observer and immutable frame-array/arenas remain
alive until fully detached and the outermost model/Styled operation returns.
The handler/user pointers are retained across calls; their pointees stay host-owned.
All records/text/frame regions and observer/model/schema/editor storage must be
disjoint. Overlap/provenance and idle attachment to an unobserved model are caller
preconditions, not runtime-detected safety guarantees. Do not move/copy attached
models/observers, reinitialize them, or overwrite private bookkeeping.

## Basic lifetime

This bounded recorder copies a name into separate host storage for retention. It
never retains a callback pointer or a shallow payload view. Call `recorder_start`
after model init, keep the recorder at the same address, and detach while idle
before its storage goes away. Example capacities are application bounds; payload
failure is explicit, never silently truncated.

```c
#include "cui/cui_questionnaire_observer.h"
#include <string.h>
typedef struct {
    cui_questionnaire_observer observer;
    cui_questionnaire_observer_frame frames[3];
    cui_questionnaire_item_snapshot records[3][16];
    cui_str choices[3][64];
    char text[3][2048];
    char last_answer_item[128];
    bool name_too_large;
    cui_questionnaire_payload_status last_payload_status;
} questionnaire_recorder;

static void record_questionnaire(void *user, const cui_questionnaire_observation *event) {
    questionnaire_recorder *recorder = user;
    recorder->last_payload_status = event->payload_status;
    if (event->kind != CUI_QUESTIONNAIRE_ANSWER_CHANGED ||
        event->payload_status != CUI_QUESTIONNAIRE_PAYLOAD_READY)
        return;
    cui_str name = event->answer.name;
    recorder->name_too_large = name.length >= sizeof recorder->last_answer_item;
    if (recorder->name_too_large)
        return;
    memcpy(recorder->last_answer_item, name.data, name.length);
    recorder->last_answer_item[name.length] = 0;
}

cui_questionnaire_observer_result recorder_start(cui_questionnaire_state *state,
                                                questionnaire_recorder *recorder) {
    /* recorder must already be zero-initialized and neither attached nor in flight. */
    for (size_t i = 0; i < 3; ++i)
        recorder->frames[i].workspace = (cui_questionnaire_snapshot_workspace){
            .items = recorder->records[i], .item_capacity = 16,
            .choices = recorder->choices[i], .choice_capacity = 64,
            .text = recorder->text[i], .text_capacity = sizeof recorder->text[i]
        };
    return cui_questionnaire_observer_attach(state, &recorder->observer, 900,
        recorder->frames, 3,
        (cui_questionnaire_observation_handler){record_questionnaire, recorder});
}
```

## Delivery and copied payloads

Each active legacy emission produces one synchronous typed attempt **before**
the original legacy callback. The typed callback is observational: copy bytes and
update unrelated host bookkeeping only. Do not mutate/submit model/UI, invoke
validation or observer configuration, or destroy/rebind resources from it. Legacy callbacks retain
existing model mutations, nested calls and replacement of event/user. Subsequent
legacy follow-up eligibility is checked as before. Silent setters/reset remain
silent; attaching does not synthesize changes or lifecycle events.

CurrentChanged copies optional previous/current names; AnswerChanged copies item
name, effective choices/freeform and status. Presence booleans distinguish absent
from empty names. Completed and Submit share one submission frozen after successful
validation, before either legacy callback. If Completed edits, disables, resets,
replaces the legacy handler or performs a guarded nested submit, the outer Submit
still carries the same original data. Nested submissions receive different frames.
Repeated successful submit still emits Submit; failed validation emits no success
pair. No copied editor draft, custom validation-message or schema-edit event exists.

Snapshots follow the existing copy-out rules: schema order, selected enabled choice
values in definition order, skipped/unanswered entries included, disabled items
omitted from submission, copied strings with length-excluded NUL terminators.
Current model UTF-8 validation rejects embedded NUL. Frame arenas and observation
views are borrowed through their callback only. A shallow structure copy is not a
deep retained event; copy all required strings/arrays into separate host storage.
The outer pair's backing frame is pinned through the entire outer submit, including
intervening legacy callbacks, but its stack-local observation pointer is not retained.

Metadata maps current/answer to CHANGE, completed to COMMIT, submit to SUBMIT;
family `kind` preserves exact meaning. The nonzero application target does not
register an element. Origin is UNKNOWN and evidence flags are zero even for actual
pointer/key/accessibility/input/timer paths. A Base hook has no proven causal
frame/index/activation; do not infer one from nearby queued events. Genuine Styled
metadata forwarding would require a separately audited API.

## Bounds, detach and errors

One frame is reserved per active emission; a submit pair holds one across both
notifications. Nested legacy emitting calls use extra frames. NO_FRAME gives
honest empty copied fields; CAPACITY gives exact requirements and unchanged arena
bytes; OVERFLOW and defensive INVALID give empty data/zero requirements. Failure
is shared by an outer pair, with no retry against later mutated state. There is no
allocation, event replay, legacy event loss, rollback or altered model return.
A missing typed handler counts attempts but performs no copy/delivery. Statistics
count active attempts, deliveries and failed payloads, saturating with an overflow
flag. Removed/inactive observers do not count later emissions.

Idle detach returns OK. Detach from a legacy callback returns BUSY to report pending
removal: later typed attempts stop immediately, but existing frames remain pinned
until final in-flight return. BUSY does not reject removal. Reattach/reconfigure
while known in-flight returns BUSY, including a different observer attempting to
replace a pending detached one. Already attached idle state returns ATTACHED.
Once drained, the model association clears; the caller can attach again while idle.
No destruction notification is emitted. Keep all users/frame/model/schema/editor
storage alive through outer return and renderer consumption before freeing it.
There is no arbitrary destruction guard or concurrent-use contract.

The focused tests compare attached/unattached legacy mutation traces, immutable
outer versus nested submit data, event/user replacement, live choose/skip follow-up,
capacity/depth errors, detach pinning, copied names including empty identity,
ordered input plus actual key/pointer/accessibility routes and delayed timer.
Existing Questionnaire, Styled, keyboard and snapshot suites remain regressions.
The worker fixture compiles this example and a C++ header consumer; WASI uses a
worker extension of the pinned runner. General event/binding uniformity, physical AT
acceptance and the family's presentation/localization/Form/style gaps remain open.
