# Base final-offset scroll observers

Include `cui/cui_scroll_events.h`. This slice adds only drawing-free
`cui_base_scrollable_events` and `cui_base_scrollable_finish_events`. Legacy
Scrollable APIs, public state/options layouts and reduction timing are unchanged.
Styled paired composition, scrollbar overlays, virtual widgets and MessageScroller
have no new observer facade in this slice. See the [checkpoint](design/SCROLL_EVENTS_SLICE.md).

```c
static void scrolled(void *user, const cui_scroll_change *change) {
    cui_scroll_offset *last = user;
    *last = change->value; /* Copyable values; not a request to accept/reject. */
}

/* During one frame; state/options/last belong to the application. */
cui_scroll_event_scope scope = {0};
bool begin_changed = cui_base_scrollable_events(ui, owner, &state, options, &scope);
(void)begin_changed; /* False can mean valid no-change; still finish. */
/* Compose the owner's same-ID layout and children here, as with the legacy Base
 * reducer. Children may defer wheel input back to this owner. */
cui_scroll_event_result result = cui_base_scrollable_finish_events(
    ui, owner, &state, options, &scope, (cui_scroll_handler){scrolled, &last});
/* Base does not update your viewport translation or paint. Use final state.x/y
 * for that work as in your existing custom layout. */
```

Base begin returns the exact old `cui_base_scrollable` bool, including offset
normalization, and does not mean successful submission. A nonnull context/state/
token, nonzero owner, zero-initialized inactive token and active ordinary or passive
submission admit the pair. Invalid arguments, outside-frame/protected phases or
an already-active token reject before reducer work. After an admitted begin,
finish once after children even if begin returned false. Calling finish on an
unadmitted token is harmless and returns `valid=false`; do not read private token
fields to decide application control flow. Reuse a consumed token or zero-initialize
new caller storage; never copy a live token or share it between owners.

Finish calls the existing wheel-only finish once, then freezes entry/final offsets,
consumes the token and optionally notifies. `valid` reports pairing and finite
final-state admission, not frame/layout success. `changed` compares the whole pair,
not only the finish delta. `change_count` counts actual callbacks (zero or one);
`count_overflow` is always false for this bounded aggregate. Reading a result never
replays dispatch. Missing handler still performs all old reduction and reports
changes with count zero. Net-zero offset sequences, drag-state-only changes and
unchanged offsets are silent, even if individual input operations occurred.

This is an applied final-offset notification, not a user gesture or controlled
value request. It includes clamping from range shrink, disabled axes, negative
entry values and nonfinite entry repair. The new payload has `has_previous=false`
and zero `previous` when either entry coordinate was nonfinite; otherwise both
previous coordinates are preserved exactly. Finite final values are copied into
`value`. An external assignment before begin becomes the baseline, not its own
event. Do not mutate active state between the pair to manufacture an event.

Disabled ordinary submission can normalize state and dispatch CHANGE. Inert or
layout-only scopes preserve the existing reducer's maintenance/layout behavior,
and can return `valid=true, changed=true`, but callbacks are suppressed because
the public frame sequence is zero. This does not introduce user requests under
inert ancestry. No callback means neither rollback nor absence of maintenance.

Metadata uses CHANGE and the public owner target. The only evidence flag is
HAS_FRAME with the genuine matching nonzero frame sequence. Source is UNKNOWN;
there is no HAS_SOURCE, event index or activation snapshot. Several inputs or
normalization can contribute, so guessing a final wheel/key cause would be false.
Payload is pointer-free and safe to copy beyond frame/context lifetime; it does
not keep the owner or context alive.

Same context, owner, state address, effective options, frame and ancestry must
surround the pair. Runtime checks reject identity mismatch, token reuse, outside
submission, differing nonzero frame identities, and zero/nonzero scope transitions
before finish effects. With both sequences zero the existing public API cannot
prove same-frame identity: crossing frames/scopes is unsupported caller misuse
without a guaranteed diagnostic. Same effective options/style lifetime is a caller
obligation, not a runtime descriptor-equality check. Keep each nested token separate.
A wrong pair is not automatically repaired or cancelled; an admitted layout must
still follow its legacy balancing obligations. Nonfinite final state from forbidden
caller mutation returns invalid after token consumption; no callback is invented.

Handler and user data are borrowed only through finish. They are never stored in
the scope or context. Callback may update unrelated application state, but must
not recursively submit this control/frame, mutate active input/scroll state/options,
destroy the context or free active resources. Defer teardown until existing frame
obligations complete. No lifetime guard or general event routing is added.

The focused tests compare old/new state, semantics, consumption and begin bool;
cover multiple wheel events, net-zero changes, omitted observer, normalization,
nonfinite repair, inert/layout-only maintenance, invalid/mismatched/repeated pairs,
nonzero wrong-frame rejection, overlapping owner tokens, real child-deferred wheel
routing, scrollbar keyboard/drag, and copied payload lifetime. Existing Scrollable,
virtual widget and MessageScroller tests remain separate regression evidence.
No completion callback, virtual-window rerender, precision conversion, bridge
producer or foreign-language handler is introduced. Bindings retain their existing
manual notification allowlists. This slice is not full-family or 78-family parity.
