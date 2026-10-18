# Base hover model-open observers

Include `cui/cui_hover_events.h` for `cui_hover_update_events`. This additive pure
Base adapter preserves `cui_hover_update` and all its parameters, state and timing.
It observes actual model-open transitions for shared HoverCard/Tooltip policy.
It does not report painted visibility, rendered geometry, animation completion,
input acceptance or controlled requests. Styled panel observers are not implemented
in this slice; see the [checkpoint](design/LIFECYCLE_RESULT_EVENTS_SLICE.md).

```c
#include <cui/cui_hover_events.h>

typedef struct { bool details_open; } model;
static void open_changed(void *user, const cui_hover_lifecycle_event *event) {
    model *app = user;
    app->details_open = event->open; /* Copyable notification, no acceptance return. */
}

cui_hover_event_result update_details(cui_hover_state *hover, model *app,
                                    bool trigger_hovered, bool content_hovered,
                                    bool focused, bool escape, bool disabled, float dt) {
    return cui_hover_update_events(90, hover, trigger_hovered, content_hovered,
        focused, escape, disabled, dt, .3f, .2f,
        (cui_hover_handler){open_changed, app});
}
```

The application owns persistent `cui_hover_state`; the function owns no context,
clock or listener. Supply timing and boolean facts as with the old pure reducer.
Do not call this and the old reducer on the same state/timestep merely to inspect
both results: that advances policy twice. Omission does not advance timers or
invoke a late callback. Multiple calls advance according to their supplied dt;
there is no implicit once-per-frame guard.

`valid` requires a nonzero metadata target and nonnull state. Invalid arguments
return a zero result before any state/callback work. Other parameters keep legacy
policy: finite positive dt advances elapsed, invalid/nonpositive dt does not;
nonfinite or negative delay becomes zero. `control_result` is the exact legacy
bool (entry open differs from final open), not successful submission. Pending,
target, elapsed or suppression-only changes are not open transitions.

One actual open transition invokes the optional handler once, after reduction,
with frozen `previous_open` and `open`. `change_count` counts actual observer calls,
zero or one; `count_overflow` is always false. Missing handler preserves ordinary
state mutation and `control_result`, with count zero. Reading a result does not
invoke another observer. Callback does not veto or confirm the state change.

Disabled pure policy closes an open state and may notify. Escape closes and
suppresses reopening until trigger/focus/content facts are absent. This function
has no UI ancestry or routing: disabled closing is a model transition, not accepted
user input; it cannot identify inert/layout-only frames or promise UI callback
suppression. A later Styled adapter must handle those boundaries separately.

Payload metadata is LIFECYCLE with the supplied target and UNKNOWN source. All
presence flags are clear: no context/frame/index/activation/reason is inferred
from policy booleans. The metadata target is application-selected stable identity,
not a new ID allocator. The pointer-free payload can be copied beyond callback and
state lifetime; it does not retain state, context or application resources.

Handler, user and payload pointers are borrowed only through the call. No handler
is retained while a delay is pending. The callback can update unrelated owner
state, but must not mutate the active hover state, recursively run its reducer,
free active storage or unwind a foreign exception through C. Defer active-resource
teardown until the call returns. No runtime lifetime/reentrancy guard is added.

Tests prove explicit delay/reversal/content-hold/close/suppression/reentry/focus/
disabled sequences, missing handlers and repeated-state counts; compare all state
fields and legacy bool across 576 combinations/steps with finite, negative, NaN
and infinite timing/delays; verify invalid target/null-state atomicity, metadata
absence and copied payload after state deallocation. Strict C11 Release/C++ header
and ASan/UBSan checks run alongside existing content regressions through the private
test wrapper. Sanitizer validation uses prebuilt instrumented Core/software archives,
not a new full-project sanitizer build.

No Styled Tooltip/HoverCard composition, Focus Trap, virtual-window completion,
MessageScroller, bridge producer or language callback is added. Existing binding
notification allowlists remain unchanged. This is a bounded model observation
slice, not full-family or uniform coverage of all 78 manifest families.
