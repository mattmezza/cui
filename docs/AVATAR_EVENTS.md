# Avatar activation observation

`cui_avatar_events.h` adds an opt-in direct C observer to `cui_avatar_ex` behavior.
It does not change legacy Avatar options/layouts or the aggregate bool API.
`interactive` still enables BUTTON semantics and routing: supplying a handler to
a passive IMAGE does not turn it into a button.

```c
static void open_profile(void *user, const cui_component_event_metadata *event) {
    /* Copy *event if needed. Update application state; defer active teardown. */
    profile_owner *owner = user;
    owner->last_activation = *event;
    owner->open_requested = true;
}
/* owner remains alive through this call and the frame's render boundary. */
cui_component_action_result result = cui_avatar_events(
    ui, id, &theme, cui_string("Taylor Reed"),
    (cui_avatar_options){.interactive = true, .design_size = CUI_DESIGN_MD,
        .identity_colors = true},
    (cui_component_activation_handler){open_profile, owner});
/* Inspect counts for diagnostics; do not open the profile again from result. */
```

The complete [portable owner example](../examples/avatar_events/README.md)
contains a real owner definition and runnable model. It demonstrates a copied
payload and next-submission omission. Its synthetic accessibility input is not
native assistive-technology evidence.

The facade runs one original Avatar submission. It observes each matching event
at the existing explicit event loop after photo/icon/initial paint submission
and semantic/focus/cursor updates. Evidence is copied immediately before original
consumption; callback runs after consumption, then the loop checks the next event
live. Multiple inputs produce multiple notifications in event-index order,
including when the legacy bool would only report true once. Consuming later
input in a callback can suppress later callbacks; events are never prequeued.

`valid` reports argument preflight, not successful rendering. `submitted` reports
ordinary active paint-enabled, non-inert submission, including disabled and
passive avatars. `activation_count` counts observed legacy activation-loop
matches when submitted; a missing handler still counts and consumes as before.
`request_count` is always zero. Counts saturate at `SIZE_MAX`, setting
`count_overflow`; callback delivery continues. Copying/reading results does not
redispatch. Omission, disabled/passive/inert/layout-only work does not invent a
notification. The legacy API's own return/consume behavior remains unchanged.

Metadata kind is ACTIVATE and target is the Avatar ID. Genuine accepted snapshots
preserve frame, event index, input source and contact/key evidence. When the old
activation gate matches but Core cannot establish accepted evidence, notification
has proven frame only with UNKNOWN source and no index/activation flags. A
regression reproduces this honestly using a genuine pointer event plus a
sticky duplicate-semantic Core error. It does not fabricate a snapshot or claim
that preflight validity clears a Core error.

Avatar preserves default Core keyboard behavior: Enter nonrepeat key-down and
Space release after capture. It does not adopt source Button's different release
policy. Accessibility ACTIVATE needs prior supported semantics; failed admission
before registration is not an activation. Core evidence is separate from tested
native screen-reader integration.

Handler/user/payload pointers are borrowed synchronously; metadata is pointer-free
and copyable beyond frame/context lifetime. Existing pixels, caches, labels,
icons and users retain their rendering lifetimes. Updating unrelated application
flags and consuming later input are supported. Do not rebind/free active
resources, recursively submit the same component/frame or destroy context in a
callback. Request omission/teardown in application state, then apply it after
outer return and renderer consumption. No runtime destruction guard, owner veto,
CHANGE/COMMIT/SUBMIT, image loading lifecycle or foreign-language callback facade
is introduced.

## Stack activations

`cui_avatar_stack_events` adds the separately reviewed stack observer while
`cui_avatar_stack` retains its old aggregate index API:

```c
static void choose_person(void *user, const cui_avatar_stack_activation *event) {
    team_owner *owner = user;
    owner->chosen_index = event->index;
    owner->chosen_key = event->key;
    owner->last_activation = event->event;
}
cui_avatar_stack_event_result result = cui_avatar_stack_events(
    ui, id, &theme, people, people_count,
    (cui_avatar_stack_options){.size = 48, .interactive = true},
    (cui_avatar_stack_handler){choose_person, owner});
```

Callbacks follow visible child submission order, then that child's event-index
order. They are not sorted across the compound by input chronology. For events
on children 2, 0 and 1 in that time order, callbacks report children 0, 1 and 2;
`result.control` is 2, preserving the highest activated visible index returned
by the old API. Several events on one child generate several callbacks.
`result.action` uses the same validity/submission/count/overflow fields as the
single Avatar; request_count stays zero. A missing handler still counts and
preserves the legacy aggregate index and consumption.

The payload's metadata target is the stack ID, while genuine activation evidence
retains its child target. Index and application key are copied from the actual
visible iteration, including an original zero key. Nonzero keys retain stable
child identity across reorder; zero retains the existing index-based identity
rule. The payload does not replace zero with a generated key. A frame-only
fallback has no claimed child activation snapshot; index/key still identify the
actual legacy iteration. The regression exercises this with real routed input
and a duplicate-semantic Core error.

Width-limited hidden items and passive overflow never notify. Existing overlap
hit order, default visible count, images, semantics and appearance remain
unchanged. Active item arrays are borrowed and must not be mutated/rebound/freed
inside callbacks. Consuming later input can suppress later events/children; no
queue or replay is introduced. Resource lifetimes and deferred teardown rules
are identical to the single-Avatar observer.

The [checkpoint](design/AVATAR_EVENTS_SLICE.md) remains the reviewed design
history. Complete Avatar parity is not claimed. Integrator work includes test/
module registration, generated SDK/API/skill exports and honest binding
availability; a C declaration copied into an SDK does not expose a working
foreign callback facade.
