# Alert completed-dismiss observations

Include `cui/cui_alert_events.h` and call `cui_alert_events` to observe the
built-in Dismiss action. This additive native C facade calls the existing
`cui_alert` once. Its original options, Button style, content/action slots,
visibility assignment, drawing and bool result remain unchanged.

```c
static void dismissed(void *user, const cui_alert_dismissal *event) {
    app_state *state = user;
    state->last_dismissal = *event; /* Pointer-free copy. */
    state->dismissals++;
}

cui_alert_dismiss_result result = cui_alert_events(
    ui, alert_id, &theme,
    (cui_alert_options){.kind = CUI_ALERT_INFO,
                        .message = cui_string("Saved"),
                        .visible = &state.visible, .dismissible = true},
    (cui_alert_dismiss_handler){dismissed, &state});
```

The [standalone headless example](../examples/alert_events/main.c) shows the
complete setup. Link the usual `cui` library; no new dispatch service or retained
listener registration is required.

`legacy_dismissed` is exactly the old aggregate bool. `dismissal_count` is zero
or one for an eligible completed built-in dismissal, including when no callback
is installed. A callback runs at most once per invocation, even when multiple
accepted inputs activate Dismiss. Choose the callback or result for each side
effect; reading the result never invokes or replays a callback.

Observation occurs synchronously after the old function has ended its Alert box
and, if supplied, assigned `*visible=false`. `visibility_assigned` records that
assignment, even if a render slot already set the value false. It does not claim
a transition or closed lifecycle. Without visibility storage, accepted dismissal
still returns true and can notify; `visibility_assigned` is false. The owner
still decides whether to omit the Alert next frame.

Visibility is checked once at entry. Title/body/action slots retain their old
order and may change owner visibility or consume input before the Dismiss Button.
A slot changing visibility alone does not synthesize a completion. Consuming an
input uses existing Core policy: it cancels the tested accessibility action but
does not revoke an ordinary already-accepted Enter-down activation. The facade
adds no general input-cancellation promise. The observer
can set visibility true again; no second assignment or veto follows, and the
frozen completion result stays unchanged. Slot `options.user` and handler `user`
are independent. Invalid arguments produce `valid=false`; initially hidden
Alerts are valid omissions with `submitted=false`. Passive inert/layout-only
submission preserves legacy behavior but has no eligible observation/count.

The payload's common metadata has `kind=ACTIVATE`, public Alert ID, actual entry
frame, `UNKNOWN` source, and only `HAS_FRAME`. There is no per-input event index,
activation snapshot, pointer/touch coordinate, source, close reason or timestamp.
Even a single accepted input does not upgrade this aggregate metadata. Actual
input routing stays in the existing Button. Custom action-slot child events are
separate and are not Alert dismissals.

Options, handler and active eligibility/frame are copied at entry. Payload views
are borrowed synchronously; a copied payload outlives the frame/context. Keep
visibility, UI/theme, slot resources and users alive through their existing
submission/rendering lifetimes. Do not recurse into UI submission, destroy the
context, free/rebind active resources or unwind through C callbacks. Request
teardown in application state and perform it after the frame.

This native slice does not provide per-input callbacks, controlled dismissal,
veto/default cancellation, retained open/close policy, AlertDialog forwarding,
async listeners or a Python/Node/TypeScript/Go/Rust observer binding. The retained
bridge's existing Alert does not configure native dismissibility/visibility;
its foreign queued notifications do not implement this observer contract.
See the [design checkpoint](design/ALERT_EVENTS_SLICE.md) and
[binding coverage](design/COMPONENT_EVENTS_BINDINGS.md).

The focused headless regression covers pointer/touch, Enter/Space, genuine Core
accessibility activation, multiple accepted inputs, callback-independent counts,
no visibility storage, slot/observer owner writes, omission/passive submission,
custom actions, copied payload lifetime, legacy shape/SVG software pixels,
text command geometry/style/content and semantics. Font rasterization is not
exercised by the portable no-font fixture.
This is portable software/Core evidence, not native screen-reader, GPU or full
Alert event parity. Run CTest on Linux through
`python scripts/test_local.py --build-dir <build>`.
