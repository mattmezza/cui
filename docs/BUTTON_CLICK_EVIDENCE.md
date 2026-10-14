# Button activation evidence

`cui_button_click_event` now appends `has_evidence` and `evidence`, a copied
`cui_activation_snapshot`. The existing release-derived fields retain their
behavior. In particular, legacy `click_count` normalizes a pointer count of zero
to one, while `evidence.press.click_count` and `evidence.release.click_count`
preserve the raw host counts, including zero.

```c
static cui_button_click_event last_click;
static void remember_click(void *user, const cui_button_click_event *event) {
    (void)user;
    last_click = *event; /* No contained pointers; usable after the callback/frame. */
}
static bool compose_button(cui_context *ui, cui_id id, const cui_theme *theme) {
    return cui_button_ex(ui, id, theme, cui_string("Save"),
        (cui_button_options){.on_click_event = remember_click});
}
```

For each accepted event, standalone Button copies the evidence before invoking
legacy `on_click`, then invokes `on_click_event` once with the frozen payload.
Legacy callback consumption of that accessibility event cannot erase the
already accepted notification. Multiple accepted events retain input order and
separate evidence; repeated keys and cancelled activations remain rejected.
Callback-driven changes to later events still follow existing cooperative
consumption and current-semantic validation rules. This is not capture/bubble
propagation and does not restore consumed events.

Styled ButtonGroup prepares each child's payload before its legacy callback,
then forwards it through the existing post-legacy event-index hook. Its bounded
transient forwarding records require no heap allocation or public workspace
extension. Existing child callback counts remain unchanged. Group `on_change`
continues to override both child notifications and their preparation hook,
including the existing source-pointer-only versus explicit all-activations
policy. This does not change borrowed artwork descriptor lifetimes.

Base Group appends an optional `prepare_click_event` hook. It receives a
borrowed, const copied Core snapshot before legacy/post-legacy callbacks. Copy
it during the hook if it must be retained. Preparation is excluded from
`cui_button_group_result.callbacks`, which counts action notifications. The old
`on_click_event(user, ui, event_index)` remains compatible and still runs after
legacy notification even if legacy consumption makes a later Core query fail.

The evidence is forwarded unchanged from Core. A matching retained capture can
provide press and release samples across frames, including mouse, pen or touch.
An accepted unmatched release has release evidence without a fabricated press.
Keyboard bounds describe the previous rendered clipped routing rectangle,
which can differ from this frame's relaid-out Button. Accessibility evidence
contains no fabricated pointer samples or key bounds. `has_evidence` reports
successful copying; callers should inspect the individual `has_*` flags.

The payload can survive later frames or context destruction. Target/frame/index
identity remains local to the originating context and cannot identify recreated
contexts or a complete frame-sequence wrap. The callback pointer itself is
borrowed only until that callback returns. Refer to `ACTIVATION_SNAPSHOTS.md`
for Core rejection and capture lifetime details.

Pinned v0.7.1 uses `Button` click listeners (`button/button.rs`) and the
`gpui-pre=0.3.8` MouseClickEvent/KeyboardClickEvent contracts audited in
`validation/button-event-contract-next33.md`. First-focusing-click and
long-press metadata, source key-up focus-generation policy, signed tab paths
and event-local capture/bubble propagation remain separate gaps. This slice
forwards supported evidence and does not certify whole Button-family parity.

The focused headless fixture also reruns the existing callback contract suite.
It covers repeated clicks, cross-frame contacts, leave/re-entry, mismatched
release, cancellation, raw versus normalized counts, modifiers, old keyboard
bounds after relayout, accessibility consumption by legacy callbacks, retained
copies, Group multiplicity/override and the old Base index hook. Noto and actual
WASM/Chromium fixtures exercise font layout and genuine mouse/keyboard/CDP touch
input. Injected CDP contacts are not physical-device or OS adapter validation.
