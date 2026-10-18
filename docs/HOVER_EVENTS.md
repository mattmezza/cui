# Hover model-open observers

Include `cui/cui_hover_events.h` for `cui_hover_update_events`. This additive pure
Base adapter preserves `cui_hover_update` and all its parameters, state and timing.
It observes actual model-open transitions for shared HoverCard/Tooltip policy.
It does not report painted visibility, rendered geometry, animation completion,
input acceptance or controlled requests. Styled `cui_tooltip_events` and
`cui_begin_hover_card_events` add final-model observation around existing
composition; see below and the [checkpoint](design/LIFECYCLE_RESULT_EVENTS_SLICE.md).

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
bool (entry open differs from final open), not successful submission. The explicit
`changed` flag independently records the same entry/final comparison even without
a handler; it is distinct from Styled composition admission. Pending,
target, elapsed or suppression-only changes are not open transitions.

One actual open transition invokes the optional handler once, after reduction,
with frozen `previous_open` and `open`. `change_count` counts actual observer calls,
zero or one; `count_overflow` is always false. Missing handler preserves ordinary
state mutation and `control_result`, with count zero and `changed` still observable. Reading a result does not
invoke another observer. Callback does not veto or confirm the state change.

Disabled pure policy closes an open state and may notify. Escape closes and
suppresses reopening until trigger/focus/content facts are absent. This function
has no UI ancestry or routing: disabled closing is a model transition, not accepted
user input; it cannot identify inert/layout-only frames or promise UI callback
suppression. The Styled adapters handle those boundaries separately, below.

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

No Focus Trap, virtual-window completion,
MessageScroller, bridge producer or language callback is added. Existing binding
notification allowlists remain unchanged. This is a bounded model observation
slice, not full-family or uniform coverage of all 78 manifest families.

The explicit `changed` field extends the unreleased event-result layout. Rebuild
consumers rather than mixing earlier objects; no legacy result/layout changed.

## Styled Tooltip and HoverCard

```c
cui_hover_event_result tooltip = cui_tooltip_events(
    ui, tooltip_id, theme, &tooltip_state, cui_string("Help"), options,
    (cui_hover_handler){open_changed, app});
(void)tooltip;

cui_hover_event_result card = cui_begin_hover_card_events(
    ui, card_id, theme, &card_state, cui_string("Details"), card_options,
    (cui_hover_handler){open_changed, app});
if (card.control_result) {
    /* Arbitrary child controls/content keep their own event contracts. */
    cui_end_hover_card(ui); /* Existing end, exactly once on true admission. */
}
```

Both functions call their legacy counterpart once, then compare the final model.
`control_result` is the exact old Tooltip bool or HoverCard begin admission;
`changed` independently compares entry/final open state, even without a handler.
`valid` requires nonnull context/theme/state and nonzero ID, not successful
geometry, resources, paint or full-frame completion. Invalid arguments reject
before model work. Missing observers preserve state/composition with count zero.
Callback count remains zero/one and overflow false.

Placement can force the model closed after delayed policy opens it. Closed→open→
forced-closed within one call is net-zero and silent. Conversely intrinsic card
measurement resource failure can return false with the model open: `changed=true`,
`control_result=false` and one OPEN model notification are valid. The callback does
not claim that a portal was composed or displayed. Never condition HoverCard end
on `changed` or callback count; pair only on `control_result`.

Notification follows the complete legacy Tooltip call or the legacy HoverCard
begin decision, **before arbitrary HoverCard children**. It is not a deferred end
callback. Do not call the new Base observer internally/on the same state first;
that would advance delay twice. Original content, placement, measurement redraw,
trigger description, child semantics and end balancing remain unchanged.

Styled metadata has only HAS_FRAME with the genuine nonzero local frame sequence;
UNKNOWN source, no source-presence/index/activation/reason. Delays, focus, geometry
fallback and several inputs may contribute. Frame-zero inert/layout-only/protected
work keeps legacy maintenance and `changed` results but suppresses observers.
Calling UI composition from protected measurement is still unsupported: the
adapter does not hide the old lifecycle diagnostic or grant permission to do it.

Legacy Escape policy scans globally without filtering consumed/repeated/modified
keys. This adapter preserves it rather than changing routing. Trigger focus opens;
HoverCard can hold open for panel hover/contained child focus, Tooltip cannot.
Omission does not close model state or retain handlers for a later close.
The original hover-state/options/text and arbitrary child storage obligations
remain; users/payloads are callback-borrowed, not retained during pending delay or
measurement redraw. Do not mutate active hover/trigger/input/options, recursively
compose, free active resources or destroy the context during callbacks.

Styled tests compare old/new paint command geometry/colors/text size, panel bounds,
state and semantics; prove pointer delay, contained child focus, global consumed
repeat Escape, disable close, missing handlers, omission, viewport/anchor fallback,
resource-capacity failure, inert/layout-only maintenance, protected-measurement
suppression with unchanged legacy diagnostic, and copied payload lifetime.
Strict Release and ASan/UBSan private-wrapper runs include Base and legacy content
regressions; sanitizer checks reuse instrumented Core/software archives. No actual
visibility/presentation, retained listener, bridge producer or foreign callback is
claimed.
