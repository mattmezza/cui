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

This is phase one only. `cui_avatar_stack` still returns its legacy aggregate
index and has no new typed stack observer. Its distinct child/event ordering
requires the separately reviewed phase two in the
[design checkpoint](design/AVATAR_EVENTS_SLICE.md). Complete Avatar parity is not
claimed. Integrator work includes test/module registration, generated SDK/API/
skill exports and honest binding availability.
