# Retained completed-close observers

Include `cui/cui_surface_events.h` for additive Base and Styled surface facades.
They observe completed closes; they do not control close acceptance or notify
opening, destruction, dismissal attempts or vetoes. Existing surface state/options
layouts and entrypoints are unchanged.

```c
static void closed(void *user, const cui_surface_lifecycle_event *value) {
    cui_surface_result *last = user;
    *last = value->result;
}
/* These objects must remain at stable addresses through every possible close. */
cui_surface_event_state surface;
cui_surface_result last = CUI_SURFACE_NONE;
cui_surface_events_init(&surface, surface_id, CUI_SURFACE_DIALOG);
cui_surface_events_set_handler(&surface,
    (cui_surface_event_handler){closed, &last});
/* On each submitted frame: */
cui_surface_event_result result = cui_surface_begin_events(
    ui, theme, &surface, cui_string("Title"), cui_string("Body"), options);
if (result.control_result) {
    /* Compose the body. */
    cui_surface_end(ui);
}
```

`cui_base_surface_update_events`, `cui_base_surface_begin_events`,
`cui_surface_begin_events` and `cui_alert_dialog_events` each refresh the retained
registration in a by-value copy of options before native update. All other options,
including `decide` and `decision_user`, pass through unchanged. Do not separately
update then begin on the same input. `control_result` preserves the native bool;
for the void Alert composition it is always false. `valid` describes adapter
preflight only, not geometry/render success. `close_count` counts actual observer
invocations during that call, including a close before subsequent native failure.
An absent observer still closes normally and yields zero count. No result replay
occurs when replacing a handler or submitting another frame.

Use existing `cui_surface_open`, `cui_surface_close`,
`cui_surface_request_close` and `cui_surface_take_result` on `&surface.control`.
Direct close bypasses decision policy, exactly as before; request-close waits for
submission and consults the existing decision callback. Pending requests are one
slot, not a queue. The observer runs after the result is committed, focus is
restored and the overlay closes. Taking the persistent result neither triggers
nor cancels notification. Direct close within a valid active frame invokes the retained observer but returns
no facade-local callback count. Only request-close is documented safe between
frames. The existing imperative close calls active-frame focus-scope dismissal:
calling it outside a frame can commit/notify while reporting CUI_ERROR_LIFECYCLE.
This facade preserves that diagnostic; it does not make that operation safe.

The payload's `event` is shared LIFECYCLE metadata with the surface ID and UNKNOWN
source. It carries no frame, event index or activation flags: an imperative call or
animated completion does not establish an input cause. The pointer-free
payload can be copied beyond callback/context lifetime. It does not keep the
surface or context alive. Result is the existing ACCEPTED/CANCELLED/CLOSED value;
imperative legacy close retains its existing handling of other supplied values.

Storage and handler user data must outlive all possible closes. Never copy or move
an attached `cui_surface_event_state`, edit adapter fields, or replace its retained
`control.on_close/close_user`. A Sheet observer runs only at completed exit, with
the latest handler installed between calls; it can outlive the request's frame.
`set_handler` replaces one listener, never appends, and rejects a user pointer
without a function. An empty handler keeps the adapter installed.

Incoming options with either legacy close callback field populated are rejected
before any state/UI mutation. `decide` is allowed. Reentrant wrapper calls,
handler replacement and detach during notification fail preflight. Surface/context/
registration destruction, raw reentrant surface operations, and foreign exception
unwinding during callbacks are unsupported. Mutate unrelated application owner
state and defer teardown until the outer component call returns.

`cui_surface_events_detach` succeeds only when closed, not exiting, outside an
active call, and still holding the matching registration. It clears that
registration and handler without closing, taking the result or freeing anything.
After detach ordinary legacy calls may use `control`. Reinitialize closed detached
storage to use events again; init is not teardown of a live surface.

Plain legacy update/begin with ordinary options on attached storage is unsupported:
it overwrites the retained callback. Use the event-aware wrappers. Custom legacy
composition can call `cui_surface_events_options` and pass its transformed options
to that exact surface call. The helper preserves all other options, leaves output
untouched on failure, and provides no wrapper-local count. Never pass those
transformed options to another surface. A later wrapper detects overwritten
registration and fails; it cannot recover notifications already missed by an
unsupported legacy call. Keep the owning storage alive if mixing invalid calls;
there is no implicit recovery or safe teardown promise.

The test suite covers all wrapper refresh paths, actual callbacks/counts,
decision veto, imperative close, no-handler reduction, conflicts without mutation,
helper-driven legacy composition, stale registration detection, Sheet exit and
handler replacement, detach and reuse. Existing surface tests remain unchanged.
This C facade does not add bridge notifications or binding subscriptions.
