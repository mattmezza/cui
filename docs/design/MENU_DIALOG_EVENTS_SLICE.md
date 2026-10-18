# Menu and dialog event facade proposal

Design only; audited at `6231cc2a` on `integration/next39-components`. No new API
below exists yet. Preserve legacy entrypoints, caller-owned state, command dispatch,
input routing and callback timing. Do not introduce bubbling, a general dispatcher,
controlled menu requests, or binding subscriptions in this slice.

## Current contracts and evidence

| Entry point | Actual ownership and delivery | Consequence |
| --- | --- | --- |
| [`cui_menu`](../../include/cui/cui_choice.h#L220) | Caller owns `cui_choice_popup` and action ID; returns true for command activation, including the same action again. Popup options expose a render slot, not an activation callback. | Do not detect commands by comparing old/new selected ID. This is command activation, not selection CHANGE. |
| [`cui_base_menu_tree`](../../src/base/cui_choice.c#L1272) / [`cui_menu_tree`](../../src/components/cui_choice.c#L1893) | Caller owns overlay/path/list state and borrowed recursive descriptors. Reducer writes `*action`, toggles `*checked` or writes `*radio_value`, then may close the overlay. Styled reduction precedes trigger/popup painting. | These are committed uncontrolled writes. A void observer cannot become a veto or controlled request. |
| [`cui_base_context_menu`](../../src/base/cui_choice.c#L1612) | Opens against an existing target and delegates tree reduction; it does not submit/paint that target. | Keep target, popup and command identities distinct; do not submit another control to obtain events. |
| [`cui_base_dropdown_button`](../../src/base/cui_choice.c#L1637) / [`cui_dropdown_button`](../../src/components/cui_choice.c#L2126) | Explicit action and trigger IDs are independently focusable; Styled container is paint-only. Result has `action_activated` and `menu_activated`. Action reduction runs before menu reduction. | `menu_activated` means a menu command, not merely opening its trigger. Both flags can be true; result order is not global input order. |
| [`cui_surface_state` / options](../../include/cui/cui_surface.h#L24) | Caller owns state. `decide(user,result)` may refuse a requested close. `on_close` runs after close/focus restoration; its function and user pointer are copied into state during open updates. | Decision and completed lifecycle notification are separate contracts. Close registration is retained, unlike a render-call-only observer. |
| [`cui_alert_dialog`](../../src/components/cui_surface.c#L217) | Composes surface plus cancel/confirm buttons using derived part IDs. Base surface reduction recognizes those activations on later submission. | Do not attach an additional button observer that closes again or bypasses decision policy. Ordinary Dialog instead uses composed begin/end. |

Tree details matter. At [`activation commit`](../../src/base/cui_choice.c#L1528),
checks/radios mutate their owners before `*action` and dismissal. With
`keep_open_checks`, several accepted events can run in one reduction, while the
public result remains one bool and the action output is the last ID. Submenu
opening, keyboard navigation, scrolling and dismissal are not commands. Disabled
entries, labels and separators retain existing eligibility rules; missing check/
radio storage is handled by `cui_menu_entry_is_disabled`. Render callbacks are
borrowed presentation slots, not action listeners. Generic List's
[`on_activate`, `on_confirm`, `on_cancel`](../../include/cui/cui_choice.h#L168)
are distinct contracts; their per-event callback capabilities must not be
attributed to Menu or DropdownButton.

Dropdown loading/disabled policy is asymmetric: `action_loading` disables the
action half; whole-control/menu-popup disabled also gates the menu. Hidden halves
are omitted via zero IDs; both zero is invalid. Omitting the trigger closes an
existing menu. Preserve these policies, tooltip registration, independent focus,
custom rendering and inherited variants/sizes without resubmitting either half.

For surfaces, [`finish_close`](../../src/base/cui_surface.c#L58) sets the result,
restores/dismisses the focus scope, closes the overlay, then calls `on_close`.
[`cui_surface_close`](../../src/base/cui_surface.c#L77) is an imperative close and
bypasses `decide`; [`request_close`](../../src/base/cui_surface.c#L91) stores one
pending reason, normalizes invalid reasons to CLOSED, and invalidates. The next
[`update`](../../src/base/cui_surface.c#L267) clears that pending reason before
consulting decision policy. A refusal is not a close, and pending requests are
not a queue. `take_result` consumes the persistent result, not a callback/event.
Opening resets prior result/pending state.

The update loop also offers close reasons for confirm/cancel/close activation,
Escape and eligible outside pointer-down. Alert outside dismissal and its close
button require opt-in; generic dialog defaults differ. A veto can leave the
surface open so subsequent events can offer further decisions in that frame.
Do not infer acceptance from event consumption. Animated Sheet retains its modal
while exiting and notifies only on completed exit; although this first proposal
covers Dialog/Alert, a shared lifecycle hook must not retime Sheet. Options are
refreshed before animation completion, so the currently registered callback can
receive completion of an earlier request.

Existing regression evidence: [`test_menu_tree`](../../tests/unit/test_choice.c#L513),
[`test_context_menu`](../../tests/unit/test_choice.c#L648),
[`test_menu_configuration`](../../tests/unit/test_choice.c#L1406),
[`test_dropdown_button`](../../tests/unit/test_choice.c#L1465), optional-half,
loading and popup-paint tests in the same file; surface
[`sheet_motion`](../../tests/unit/test_surface.c#L480) and
[`requested_close`](../../tests/unit/test_surface.c#L557) check completion timing,
focus restoration and decision refusal. These are existing tests, not execution
claims for this documentation-only work.

## Smallest first facade: aggregate command observation

Use a separate Base-only events header including `cui_component_events.h`.
Proposed `cui_menu_command_event` contains shared metadata, an explicit owner ID,
the activated command ID and a discriminator for plain menu command versus split
primary action. It contains no descriptor pointers, labels, callback/user values,
check storage pointers, or invented item generation. Callback payloads are
copyable; command IDs alone do not extend owner lifetimes.

Add `cui_menu_events`, `cui_base_menu_tree_events`, `cui_menu_tree_events`, and
context-menu equivalents by private common implementations; legacy functions
supply no observer. Callback occurs once after successful aggregate reduction,
before Styled painting, even when the command repeats the previous ID. Its
metadata is ACTIVATE, target the command (or split action) ID, source UNKNOWN,
HAS_FRAME only when this submission actually has a frame. No event index or
activation snapshot can be reconstructed from the final action/bool.

For keep-open tree reduction explicitly name delivery **aggregate last command**.
The event guarantees only that command's ID, not its old/new checked/radio value,
all intermediate commands, or a unique physical cause. Absence of an observer
still performs the same writes. Return the original aggregate control result plus
actual observer invocation count, not a fabricated native activation count.
A future per-event tree reducer is a different review: instrument acceptance
inside the loop and freeze evidence before mutable callbacks; never replay it.

For `cui_dropdown_button_events`, preserve the original two flags and action ID
output. Emit zero/one primary-action aggregate and zero/one last-menu-command
aggregate, in existing action-then-menu reduction order. State clearly that this
is not event-index order across halves. Reject invalid handler combinations before
mutation; do not reinterpret content/tooltip callbacks as conflicts. Keep popup
opening/closing, submenu navigation and check/radio requests out of this facade.
Implement simple Menu/tree first; split composition follows after callback-before-
paint behavior is tested, without using two legacy submissions.

## Surface lifecycle needs explicit retained registration

Proposed payload: `cui_surface_lifecycle_event { metadata; cui_surface_result result; }`.
First expose only completed CLOSED lifecycle, whose result is ACCEPTED, CANCELLED
or CLOSED. Do not promise OPEN, requested, vetoed, destroyed or per-input events.
Keep existing `decide` unchanged; typed veto/request callbacks are separate future
work. Notify at the existing finish-close seam after focus restoration and overlay
close, independent of whether the owner later takes the result.

A stack-local adapter in `surface_begin_events` is invalid: state retains
`on_close/close_user` and imperative close can call it after the render function
returns. Attaching a trampoline to an ordinary state once is also insufficient:
every open legacy update overwrites those fields from its options. Use a new
caller-owned state wrapper, not a promise that attach intercepts old functions.

### Proposed exact first-slice API

Names are proposals; keep these declarations in a separate header, with Base
functions implemented below Components and Styled functions implemented in
Components. No platform types or heap allocation are required.

```c
typedef struct {
    cui_component_event_metadata metadata;
    cui_surface_result result;
} cui_surface_lifecycle_event;
typedef void (*cui_surface_lifecycle_fn)(
    const cui_surface_lifecycle_event *event, void *user);
typedef struct {
    cui_surface_lifecycle_fn closed; /* NULL means no observer. */
    void *user;
} cui_surface_event_handler;
typedef struct {
    cui_surface_state control;
    cui_surface_event_handler handler;
    /* Adapter-owned: callers must not edit these fields. */
    bool attached, active, notifying;
    size_t call_close_count;
} cui_surface_event_state;
typedef struct {
    bool valid; /* Adapter preflight accepted; not a rendering success claim. */
    bool control_result; /* Original update/begin bool; false for void Alert. */
    size_t close_count; /* Observer invocations during this call only. */
} cui_surface_event_result;

void cui_surface_events_init(cui_surface_event_state *, cui_id,
                             cui_surface_kind);
bool cui_surface_events_set_handler(cui_surface_event_state *,
                                    cui_surface_event_handler);
bool cui_surface_events_detach(cui_surface_event_state *);
bool cui_surface_events_options(cui_surface_event_state *,
                                cui_surface_options input,
                                cui_surface_options *output);
cui_surface_event_result cui_base_surface_update_events(
    cui_context *, cui_surface_event_state *, cui_surface_options);
cui_surface_event_result cui_base_surface_begin_events(
    cui_context *, cui_surface_event_state *, cui_str title,
    cui_str description, cui_surface_options);
cui_surface_event_result cui_surface_begin_events(
    cui_context *, const cui_theme *, cui_surface_event_state *,
    cui_str title, cui_str description, cui_surface_options);
cui_surface_event_result cui_alert_dialog_events(
    cui_context *, const cui_theme *, cui_surface_event_state *,
    cui_str title, cui_str description, cui_str confirm, cui_str cancel,
    cui_surface_options);
```

The proposed struct is concrete portable storage, not a variable opaque allocation.
Init initializes `control` with the existing init function,
sets the empty handler and installs the internal trampoline with the enclosing
state as its user pointer. This enclosing object's address is stable until detach;
copying/moving an attached state is unsupported. Init is for fresh or detached,
closed storage only; it is not an implicit teardown/reinitialization operation.

`set_handler` replaces one retained registration; it does not append listeners.
Reject NULL/detached state, a non-NULL user with NULL function, unexpected legacy callback
fields, and calls during an active wrapper/trampoline. Failure changes nothing.
Changing the handler while open is allowed between component calls: the latest
handler receives a subsequent completed close, including an already-started Sheet
exit. A NULL handler still keeps adapter registration intact and native mutation
continues. No callback count or queued close is replayed on replacement.

Every event-aware update/begin wrapper preflights the **whole call** before any
state or UI mutation, then calls `cui_surface_events_options` on its by-value
options. The helper rejects NULL/detached state, NULL output, non-NULL incoming `on_close`
**or** `close_user`, an
unexpected retained callback/user pair, or an active invocation; it leaves output
and state untouched on failure. On success it copies every option unchanged,
including `decide`/`decision_user`, then sets only `on_close` to the trampoline and
`close_user` to this enclosing state. The internal common implementation handles
the guard once; wrappers must not recursively call another public guarded wrapper.
The existing open-update assignment therefore refreshes the *same* retained
trampoline each frame. Do not mutate the caller's options or cache a pointer to it.
The options helper itself never changes native state or starts an active call.

`valid=false` means adapter preflight failed; native reduction/submission must not
have run. After successful preflight, preserve the exact legacy return in
`control_result`, including false for closed/invalid native geometry; `valid=true`
does not upgrade native failure. For the legacy void Alert composition,
`control_result` is always false and callers inspect `control.overlay.open` when
needed. A completed close can precede a later submission failure; its callback
and count remain real. Base/Styled end and footer helpers stay unchanged and are
called only when the corresponding begin's `control_result` is true. Never run
update separately and then begin on the same input to manufacture notification.

Imperative lifecycle operations use the existing functions on `&state.control`:
`cui_surface_open`, `cui_surface_close`, `cui_surface_request_close` and
`cui_surface_take_result`. This deliberately adds no second close policy. The
retained trampoline receives direct close between frames; request-close still
waits for a submitted update and its unchanged decision policy. Imperative
operations have no per-call facade result/count. An invalid/no-op repeated close
emits nothing. Taking a result neither unregisters nor replays the callback.

Existing legacy update/begin calls on `&state.control` with ordinary options are
**not supported while attached**: they can clear the retained trampoline before
an imperative close, and the adapter cannot intercept them. Prefer the wrappers.
For custom composition that must use a legacy update/begin, call the options
helper first and pass its returned options to that exact call; this preserves
registration but reports no wrapper-local count and supplies no wrapper completion
frame evidence. Keep both options and their enclosing state alive for the call;
only the enclosing state/handler owner must remain alive afterward. The helper
must not be used to pass the callback to a different surface state. A subsequent
wrapper detects a lost/replaced registration and fails; it cannot recover a close
already missed through an unsupported legacy call. Document this restriction in
examples and migration guidance rather than imply attach handles arbitrary calls.

Detach is an explicit operation between calls. It succeeds only when closed,
not exiting, and neither a wrapper nor trampoline is active. It clears only the
matching trampoline/user pair, empties the handler and marks the adapter detached;
unexpected replacement means failure with no mutation. After detach ordinary
legacy entrypoints may use `control`, and the enclosing storage may be released.
To resume event-aware use, initialize a closed detached state again (discarding
its old control/result as explicitly requested by init). Detach does not close,
consume a result, invoke a callback or free resources. Rejecting open detach
prevents accidentally dropping an outstanding animated completion.

The trampoline copies the handler and pointer-free payload before calling the
observer, after native focus restoration/overlay close. Increment the active
call's count only when invoking a non-NULL observer; outside a wrapper there is
no returned count. Reentrant surface operations on this state, including the
legacy imperative operations, are unsupported during notification; the owner may
change unrelated application state. This bounds completion notification to the
existing close transition rather than enabling observer-driven close recursion.

Its event is LIFECYCLE with surface ID and committed result, UNKNOWN source and
no frame/index/activation flags in this first slice, consistently for imperative
close, wrapper update and the options-helper escape hatch. Do not read a possibly
previous `cui_frame_sequence` value between frames and advertise it as a completion
frame. A later extension may add frame evidence only where active-frame validity
is actually established; no retained request frame is completion evidence.
No source inference comes from result:
ACCEPTED is a result, not proof of keyboard/pointer/programmatic origin. Explicit
programmatic origin would require a separately reviewed new operation.

This design leaves `cui_surface_state` and all legacy signatures/layouts intact;
its cost is distinct event-aware storage plus explicit migration of *all* update/
begin calls for that storage. That is smaller and safer than changing the legacy
state's callback overwrite rules, adding a context registry, or retaining stack
adapters. Implement Dialog/Alert registration and imperative-close tests first;
shared Sheet paths remain compatibility gates, not a claim of new Sheet coverage.

Callbacks may copy metadata/result and mutate application owner state. Destruction
of context/state/registration or descriptor storage during any callback is
unsupported. Queue teardown until the outer component call returns; a close
callback is not an unmount/destruction guarantee. C++/foreign exceptions must not
unwind through C. A future binding adapter must catch/store exceptions and report
them after native stepping; partial native transitions cannot be rolled back.

## Binding boundary and delivery gates

The bridge currently uses simple [`cui_menu` and DropdownButton](../../src/bridge/cui_bridge.c#L3109)
and translates returned aggregates into queued changes; primary split action is
CLICK. It does not expose recursive check/radio menu descriptors, these proposed
synchronous C callbacks, or surface decision callbacks. V1 types surface CLOSE
with its result at [`bridge_surface_close`](../../src/bridge/cui_bridge.c#L2887),
but menu changes remain LEGACY. Surface results are queued after Styled rendering
via `take_result`, not delivered at the native `on_close` seam. Manual V1 polling
in all four bindings is committed queued observation, not these facades; it shares
the legacy FIFO and omits native per-input activation evidence. Do not promote
binding parity or expand reserved V1 selection tags as part of this work.

Before implementation, add headless acceptance tests that compare old/new state,
output IDs, semantics, focus, event consumption and paint geometry. Cover repeated
commands, multiple keep-open checks/radios and last-command coalescing; disabled/
inert/omitted/loading controls; pointer, keyboard and actual supported accessibility
activation; submenu/scroll/dismissal producing no command; both split flags and
explicit observer order. Callback owner mutation must be visible to subsequent
painting without reprocessing input. Test invalid options before any mutation.
For lifecycle, cover all three results, veto/retry, pending overwrite, direct close
versus request close, take-result before/after callback, focus restoration,
re-registration/detach, registration preserved across changing options in every
Base/Styled wrapper and helper-bridged legacy call, unsupported plain-legacy
update clearing registration detected on the next wrapper, no callback on repeated close, and animated completion
across frames. Retain existing legacy tests unchanged. Compile with strict C/C++
header checks and run CTest through the private headless wrapper. No dispatcher,
Core teardown, generated SDK or binding changes belong in this first slice.
