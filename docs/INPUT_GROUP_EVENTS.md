# InputGroup addon and text observers

Include `cui/cui_form_events.h`. `cui_base_input_group_events` and
`cui_input_group_events` mirror the legacy arguments plus a borrowed
`cui_input_group_handler`, returning `cui_input_group_event_result`.
`control` is the unchanged legacy result, including the last activated addon in
descriptor order and aggregate Input changed/submitted/error/rejected fields.
`addon_count`, `change_count`, `submit_count` count actual callbacks; counts saturate
with `count_overflow`. Reading results never replays callbacks or mutations.

```c
static void addon_activated(void *user, const cui_component_event_metadata *event) {
    bool *work_after_frame = user;
    *work_after_frame = true; /* Identify action using event->target. */
}
static void text_changed_or_submitted(void *user, const cui_component_text_event *event) {
    bool *work_after_frame = user;
    *work_after_frame = true;
    /* Inspect event->event.kind: CHANGE or SUBMIT. Text is borrowed/redactable. */
}
/* During submission with existing caller-owned model/state/addon descriptors: */
/* cui_input_group_event_result result = cui_input_group_events(
       ui, group_id, input_id, theme, edit, &state, addons, count, options,
       (cui_input_group_handler){.on_text = text_changed_or_submitted,
                                .on_addon = addon_activated, .user = &pending}); */
```

Base preserves addon source traversal. Each built-in Button addon first runs its
old interaction and last-addon write, adds its existing semantic details, then
notifies once per actual accepted activation in that addon's input order. Two
activations of B and one of A may dispatch A then B then B when A precedes B in
the descriptors. They are not sorted into global input order. Metadata kind is
ACTIVATE, target is the actual addon ID, with genuine accepted snapshot evidence.
If snapshot data is unavailable but accepted input identity is proven, only actual
frame/index/type facts are supplied; no activation is fabricated. A legacy
aggregate boolean alone does not invent a callback without accepted input evidence.

All addon notifications precede the nested Input reducer. `on_text` reuses
`cui_base_input_events` exactly once, preserving its per-event committed CHANGE and
eligible Enter SUBMIT notifications, revisions, rejection/errors, composition and
history behavior. Its metadata target is the nested **Input ID**, not the group.
Multiple inputs/submits can each notify; preedit and unchanged revisions do not
become CHANGE. A failure after earlier edits retains earlier notifications and the
old aggregate error. No extra whole-group final-text comparison or replay occurs.

Callbacks run before the group's final input-parent association and before Styled
paint. Missing handlers still permit all old work. Readonly input does not disable
otherwise enabled addon Buttons; disabled input, disabled/loading addons retain
their existing separate policies. Passive/layout-only/inert submissions preserve
old work but emit no new observation. Text/custom addons have no built-in action
notification; controls in custom renderers keep their own APIs. Later custom
renderer edits are not synthesized as nested Input events.

Text payloads inherit Input's secure redaction: masked or password-purpose inputs
have `has_text=false` and empty text; scalar snapshots remain available. Otherwise
UTF-8 is borrowed only through the callback. Use `cui_component_event_copy_utf8`
with explicit storage/byte limit and check `CUI_COMPONENT_EVENT_COPY_OK` to retain
it. Copy metadata/scalar snapshots by value. Handler/user/descriptors and all
active model/state/frame resources must remain live. Do not mutate/free/rebind
active model/state/descriptors/input/context, recursively submit UI or destroy
context in observers. Unrelated flags and copying are allowed; defer omission and
teardown until outer return plus frame/renderer consumption. Notifications cannot
reject already applied changes, consume by return or imply COMMIT/form submission.

**Compatibility slice limitation:** the existing Styled InputGroup paint path
currently does not forward masked/password-purpose paint configuration. This event
slice preserves legacy paint for equivalence and guarantees event redaction only;
it does not establish secure visual rendering. A separate reviewed fix is required
before using that Styled path to display secrets. Base event semantics remain valid.

`tests/unit/test_input_group_events.c` compares legacy/facade state, aggregate
results, focus/redraw, semantics and paint for Base/Styled, including addon ordering,
text/submit multiplicity, secure event redaction, IME/undo, validation rejection,
progress before capacity error, readonly addons, disabled/loading, custom children,
missing handlers, invalid admission, actual passive/inert scopes and deferred
omission. Existing `form`, `form_base_only`, `input`, `text_input_events` remain
regressions through `scripts/test_local.py`. No browser or full-family parity claim.
Bridge and language wrappers retain existing queued generic text notifications;
this does not expose synchronous typed addon/per-event handlers through bindings.
