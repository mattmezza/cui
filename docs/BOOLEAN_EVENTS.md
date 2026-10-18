# Controlled boolean Base requests

`cui_boolean_events.h` adds `cui_base_switch_events` and
`cui_base_toggle_events`. These portable C11 Base reducers draw nothing; submit
one inside its same-ID layout box. Switch retains `CUI_ROLE_SWITCH`; Toggle
retains `CUI_ROLE_BUTTON`. Neither is implemented by remapping Check semantics.
The separate shared `cui_control_events.h` provides the result and metadata types.
Styled facades, language bindings and build/export registration are separate work.

```c
#include <cui/cui_boolean_events.h>

typedef struct { bool enabled, allow_change; } model;
static void requested(void *user, const cui_component_boolean_request *request) {
    model *state = user;
    if (state->allow_change)
        state->enabled = request->requested; /* Otherwise reject by leaving it alone. */
}
/* Caller submits the same-ID box during an ordinary frame. */
cui_component_action_result submit_flag(cui_context *ui, cui_id enabled_id, model *state) {
    return cui_base_switch_events(ui, enabled_id, cui_string("Enabled"), state->enabled,
        (cui_boolean_event_options){0},
        (cui_component_boolean_handler){requested, state});
}
```

The bool argument is an immutable entry snapshot. Each supported accepted event
invokes the optional callback synchronously with typed `current` and `requested`
values: `current` is the entry bool and `requested` its inverse. Two accepted
activations produce two identical value proposals, even when the owner accepts
the first. Semantics retain the entry snapshot during that submission; the next
submission observes owner assignments. There is no hidden write, getter, retained
value, acceptance return flag, or synthesized CHANGE notification.

Missing callback means read-only. `activation_count` still reports accepted
attempts; `request_count` counts callbacks actually delivered. Rejecting a request
does not undo Core input consumption or reroute it to the parent. Counts are
per invocation, saturate at `SIZE_MAX`, and set `count_overflow` if an increment
would overflow. Saturation does not stop dispatch. Reading or copying the result
has no event effects.

`valid` reports argument preflight: nonnull context, nonzero ID and readable-name
descriptor (nonempty names require nonnull data). It does not establish that the
frame will finish without layout/capacity/duplicate-ID errors. `submitted` means
ordinary active submission was reached, including disabled controls; it is false
outside an active frame, under inert/layout-only ancestry, and in protected
measurement/prepaint phases. Disabled/inert/omitted submissions dispatch nothing.
No loading state is added.

Options preserve role-default keyboard and pointer focus unless explicitly
modified. `no_pointer_focus` retains pointer activation and explicit/keyboard
focus; `skip_tab` and `tab_index` configure tab navigation. Base disabled Switch
and Toggle preserve their existing parent routing; they do not add Check's
required/focusable/disabled-hit-barrier policy. Existing mutable Base entry points
remain aggregate: several accepted events still produce one legacy bool flip.
Existing Styled aggregate callbacks are unchanged by this slice.

Request metadata comes from genuine accepted activation snapshots in event-index
order, frozen before the callback. It includes pointer/touch contact/device
identity, keyboard evidence or accessibility origin as present; never fabricate
pointer evidence for keyboard/accessibility. Request and metadata structs are
pointer-free and can be copied beyond callback/frame/context lifetime. The
callback pointer, user storage and name bytes remain borrowed for their existing
synchronous submission/frame boundaries.

The loop checks eligibility immediately before each delivery; it does not queue
all future callbacks. A callback consuming a later accessibility event can prevent
its later delivery. Core may already have accepted events during `cui_interact`:
`cui_cancel_event_activation` then returns false rather than undoing acceptance.
Cancellation before reducer submission remains effective. This API does not alter
Core's event-specific consumption rules.

Callbacks may update owner/unrelated state, omit later siblings through ordinary
caller control flow, or mark close/removal pending. **Destroying the active
context, recursively submitting the same component/frame, or freeing active
storage during a callback is unsupported.** Defer teardown until submission/frame
completion. No destruction guard, asynchronous callback registration, global
routing, or bridge-runner destruction guarantee is introduced.

The unit fixture exercises mouse/touch, Enter/Space/repeat filtering,
accessibility multiplicity and later-event consumption, both initial bool values,
owner acceptance/rejection, no-handler read-only behavior, invalid arguments,
protected phases, inert/layout-only/omitted/disabled submissions, exact role/focus
policy, disabled-parent routing, and legacy aggregate compatibility. Build
registration and generated SDK resources must be integrated by the module owner;
this guide does not claim those snapshots already contain the new API.
