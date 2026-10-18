# Attachment action observers

Include `cui/cui_media_events.h` and call `cui_attachment_events` with the same
arguments as `cui_attachment`, plus a borrowed `cui_attachment_handler`.
It returns `control` (the legacy `cui_attachment_result`) and actual callback
counts `activation_count`, `remove_count`, `retry_count` (each zero or one).
The old options/state/result layouts and entrypoint remain unchanged.

```c
static void remove_requested(void *user, const cui_attachment_action_event *event) {
    bool *remove_after_frame = user; /* Unrelated host flag, not active card state. */
    *remove_after_frame = true;
    /* Copy *event to retain action/metadata. Do not destroy active resources here. */
}
/* During UI submission using existing caller-owned options/state/theme: */
/* cui_attachment_event_result result = cui_attachment_events(
       ui, id, theme, &state, options,
       (cui_attachment_handler){.on_remove = remove_requested, .user = &pending_remove}); */
```

Typed handlers are `on_activate` (OPEN), `on_remove` (REMOVE), `on_retry` (RETRY).
Each enables only its corresponding existing action path even if its legacy
callback is absent. Absent typed handlers retain original availability. With no
typed handlers, the facade uses the legacy operation directly. Original callbacks
always receive the untouched `options.action_user`; typed callbacks receive
`handler.user`. Other callbacks (images, children, actions, overlays) retain their
own userdata and behavior.

The complete old operation runs once. At its original callback point after paint
and tooltip work, callbacks run in fixed category order: old click, typed OPEN;
old remove, typed REMOVE; old retry, typed RETRY; then the original focus scope ends.
Absent callbacks are skipped. Repeated input coalesces into one callback per
category; media and description retry together still yield one RETRY. This is
category order, not global input order. Reading returned flags does not replay
callbacks. These are requested application actions, not proof that deletion,
opening or network retry has completed. They cannot veto an already observed
action, consume it retroactively, or become CHANGE/COMMIT/SUBMIT/lifecycle events.

Availability remains the existing planner's decision. OPEN needs an identified
card and matching handler. REMOVE also needs touch/hover and tracked card bounds.
RETRY needs identified FAILED status and matching handler. Media retry requires
media/tracked bounds; description retry is the first eligible non-title/non-custom
child. A description's individual status override does not independently gate the
retry link. Loading does **not** globally disable OPEN or REMOVE. Custom child
buttons and custom action clusters do not become these attachment actions.

ACTIVATE metadata targets the compound attachment. Genuine unique accepted
activation snapshots are copied at the actual root/button/link interaction seam,
before later custom rendering or old action callbacks can change live evidence.
`activation.target` retains the actual control ID: card, remove, media retry, or
its distinct description retry link. Multiple accepted activations/control origins
coalescing into a category produce proven-frame UNKNOWN source with no index or
activation; unavailable evidence uses the same conservative fallback. No arbitrary
first/last input is chosen. Flags establish which evidence exists.

Current Core supports LINK accessibility ACTIVATE and pointer-activatable GROUP
ACTIVATE, including the clickable card, once previous semantics register the
control. Admission before first registration fails with INVALID_ACTION. This
facade does not change routing, focus, supported actions or keyboard behavior.
Actual passive/layout-only/inert calls do not notify, while preserving legacy
work. Invalid options/pointers retain old validation behavior. No hidden heap
allocation, retained registration or input replay is added.

Payload is pointer-free and may be copied by value beyond the callback/frame.
Handler/user/payload pointer are borrowed synchronously. Legacy callbacks retain
all original side effects and must keep resources required by the remaining
operation and observers live. Typed observers must not mutate/free/rebind active
state/options/children/input/context, recursively submit UI or destroy the context.
Unrelated application flags and payload copying are allowed. Defer removal and
teardown until outer return plus renderer consumption. Resource/render callbacks
are not action observers and must retain their existing contracts.

`tests/unit/test_attachment_events.c` checks legacy/facade equivalence, availability,
userdata/order, real pointer/key/accessibility evidence, description/media retry,
ambiguity, late callback changes, custom child isolation, passive/inert scopes and
copied payload/deferred omission. The existing `media` suite covers image,
Attachment and AttachmentGroup regressions. Run native tests only through
`scripts/test_local.py`; browser/binding/full family parity is not implied.
Foreign languages retain their existing queued generic notifications; this direct
C facade does not add typed callback wrappers to them.
