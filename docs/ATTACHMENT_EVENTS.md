# Attachment action observers

Include `cui/cui_media_events.h` and call `cui_attachment_events` with the same
arguments as `cui_attachment`, plus a borrowed `cui_attachment_handler`.
It returns `control` (the legacy `cui_attachment_result`) and actual callback
counts `activation_count`, `remove_count`, `retry_count` (each zero or one).
The old options/state/result layouts and entrypoint remain unchanged.
Use `cui_attachment_events_with_metadata` to opt into fitted titles and
descriptions with the same action timing. [Metadata example](MEDIA.md#fit-file-names-and-descriptions).

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
`scripts/test_local.py`. The tests cover immediate C.

## Retained actions

Use `cui_bridge_attachment_set_actions_v1` to enable built-in Open, Remove, and
Retry on a retained Attachment. These actions use the app's existing queue.
They work independently of fitted metadata and decoded previews.

```c
/* app owns a mounted Attachment called card. Its lifecycle belongs to your app. */
cui_bridge_status enable_file_actions(cui_bridge_app *app, cui_bridge_node card) {
    return cui_bridge_attachment_set_actions_v1(
        app, card, CUI_BRIDGE_ATTACHMENT_OPEN_V1 | CUI_BRIDGE_ATTACHMENT_REMOVE_V1 |
                       CUI_BRIDGE_ATTACHMENT_RETRY_V1);
}

cui_bridge_status read_file_action(cui_bridge_app *app, cui_bridge_notification_v1 *notice,
                                   char *text, uint32_t capacity, uint32_t *required) {
    return cui_bridge_poll_value_notification_v1(app, sizeof *notice,
        CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, notice, text, capacity, required);
}
```

Check `notice.payload_kind == CUI_BRIDGE_PAYLOAD_ATTACHMENT_ACTION` and switch on
`notice.payload.attachment_action.action`. Run navigation, removal, or retry once
through the polling path. The action does not update the Attachment's status
or complete a file operation. Read the queue after stepping. Each poll consumes
its next event, including ordinary events from other controls. Request any other
categories your decoder understands in the same mask. An Attachment action has
empty text. The general buffer and capacity rules apply.

Open can receive focus and keyboard activation. Remove remains available on
keyboard and touch without hover. Its control and the media Retry button use
the card's measured bounds and can appear on the paint pass after initial
layout. Retry is offered while Failed where the card has media or a description.
Open and Remove remain independent of upload progress. `cui_bridge_set_disabled`
suppresses new actions, and a zero action mask clears all three controls.
Already copied events remain readable after a disable or configuration change.
Removing the originating node discards its unread events.

The surrounding `ACTIVATE` record carries the retained node and its frame.
The Attachment record carries the action, the actual part when established,
and source and event index when their flags are set. Multiple Retry controls
activated during one frame give one Retry with unknown origin. Actions on the
same card queue in Open, Remove, Retry order. Polling never repeats an action.
The old polls still drain this queue. They expose `CLICK` as the event type.
Extended views carry the numeric action code in `legacy.value` or
`cui_bridge_event_ex.value`.

For dynamic loading, check the setter and
`CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1` separately. The existing V1 record
retains its layout. Custom retained slots and physical accessibility
verification remain open.

## Retained actions from language builders

Probe `supports_attachment_actions()` in Python, JS/TS or Rust, or
`SupportsAttachmentActions()` in Go. The probe checks the writer and the
shared queue's layout, version and Attachment category. On an older bridge,
leave the card passive and use the existing value poll or dispatch. The new
poll reports unavailable before touching the queue.

| Language | Enable actions on the card | Read the event after stepping |
| --- | --- | --- |
| Python | `card.attachment_actions(open=True, remove=True, retry=True)` | `app.poll_component_notifications_v1()` yields values and `AttachmentActionNotificationV1` records. |
| JS/TS | `card.attachment_actions({ open: true, remove: true, retry: true })` | `app.poll_component_notification_v1()` returns an ordinary value or a record with `kind === 'attachment-action'`, or `undefined` when empty. |
| Go | `card.AttachmentActions(cui.AttachmentAllActions)` | `app.PollComponentNotificationV1()` returns the event, presence and error. Inspect `AttachmentAction` or `Value`. |
| Rust | `builder.attachment_actions(AttachmentActions::ALL)?` | `app.poll_component_notification_v1()?` returns `Option<ComponentNotificationV1>` with `AttachmentAction` or `Value`. |

Each call replaces the three choices. Use `card.attachment_actions()` in
Python and JavaScript, `card.SetAttachmentActions(0)` in Go, or
`app.set_attachment_actions(card, AttachmentActions::default())?` in Rust to
clear them. Do this between frames. Polling returns copied action evidence and
a historical frame and handle. A resolved node is a reference to a live object.
An ambiguous input has no source or event index. JavaScript and TypeScript
carry the full frame as `bigint`.

Use `app.step()` in Python, `app.step_only()` in JavaScript and TypeScript,
`app.Step(dt)` in Go, or `app.step(dt)?` in Rust to advance the UI without
consuming its event queue. Then drain the component poll and handle ordinary
app events there too. Existing automatic dispatch consumes the same records.
Keep each side effect in one path. The built-in language runners continue to
use the existing dispatch contract. Typed reads belong to a manually stepped
loop. The examples below inject input headlessly and apply file actions after
polling. They show how the app can update status or remove a card.

Build the bridge and, for JavaScript and TypeScript, the Node addon. Set the
paths for your platform from the [language-builder instructions](BRIDGE_COMPONENTS.md#show-an-attachment-preview-in-a-language-builder). Then run the examples from the repository root on Linux.

```sh
PYTHONPATH=bindings/python python bindings/python/examples/attachment_actions.py
node bindings/node/examples/attachment-actions.cjs
node bindings/typescript/examples/attachment_actions.ts
(cd bindings/go && go run -tags cui_dynamic ./examples/attachment_actions)
cargo run --manifest-path bindings/rust/Cargo.toml --example attachment_actions
```

Use Node with built-in TypeScript stripping for the TypeScript example. The
[TypeScript type check](../bindings/node/test/attachment-actions.types.ts)
shows how to narrow the union. It uses the same runtime as the JavaScript example.
