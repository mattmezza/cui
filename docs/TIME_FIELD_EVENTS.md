# TimeField applied change observers

Include `cui/cui_picker_events.h`. Use `cui_base_time_field_events` for semantics
without Styled paint, or `cui_time_field_events` for the complete Styled control.
Both preserve the old operation and return `cui_picker_event_result`:
`changed` is its exact legacy bool; `change_count` counts actual callbacks (0 or 1).
The existing entrypoints and options/state layouts are unchanged.

```c
static void time_changed(void *user, const cui_time_change *change) {
    bool *save_needed = user; /* Unrelated host state, not the active time owner. */
    *save_needed = true;
    /* Copy *change to retain previous/value and metadata beyond this callback. */
}
/* During UI submission, using existing caller-owned time/state/theme: */
/* cui_picker_event_result result = cui_time_field_events(
       ui, id, theme, cui_string("Appointment"), &state, &time,
       (cui_time_options){.seconds = true},
       (cui_time_handler){.on_change = time_changed, .user = &save_needed}); */
```

The callback reports `cui_time previous, value`, with hour 0–23 and minute/second
0–59. Twelve-hour display still reports the original 24-hour model; hidden seconds
are retained and participate in validity. An invalid time, even an invalid hidden
second, preserves legacy rejection without repair or notification. Required
pointers and a nonzero compound ID are checked before legacy work. Styled also
requires a theme. Invalid admission returns a zero result.

Each facade calls its complete old operation once, then compares all three time
fields. Base notifies after final semantics/invalidation. Styled notifies after
its existing paint. Missing callback still permits editing and ignores user.
Multiple inputs or digit writes yielding one final difference produce one callback;
net-zero changes produce none. Focus, digit draft, invalid-digit rejection and
segment repair alone are silent. Assigning a new owner value before the call sets
its baseline; it is not a programmatic notification.

Metadata kind is CHANGE, target is the TimeField compound ID. A proven active frame
is present; source is UNKNOWN, without input index or activation. An aggregate
can combine multiple routed segment inputs, so it does not promise a unique
pointer/key cause. Flags alone establish evidence. There is no request/rejection,
COMMIT, SUBMIT, draft lifecycle, IME composition, undo or new text-input support.

Disabled/readonly controls preserve old segment/draft repair, focus policy and
semantics, with no observation. Actual inert/layout-only scopes likewise suppress
new observations while retaining the legacy work; the facade does not override
Core routing. The old per-frame bool is preserved independently of notification.

Payload is pointer-free and copyable. Callback/user/payload pointer are borrowed
synchronously. Do not mutate/rebind/free active time/state/context, input, theme,
labels or other active resources, or recursively submit UI from the observer.
Unrelated flags and copying are allowed. Defer omission/destruction until outer
return and renderer consumption; keep caller resources live through that boundary.
These are notifications of already applied edits, not owner-rejectable requests.

`tests/unit/test_time_field_events.c` compares legacy/facade Base and Styled state,
results, focus/redraw, semantics/tab policy and paint. It covers routed pointer
selection plus editing, rollover, digit multiplicity/net-zero edits, 12-hour model,
hidden seconds, invalid input, missing observer, actual passive/inert scopes,
readonly/disabled maintenance, copied payload and deferred omission. Existing
`picker`, `picker_base_only` and `picker_events` remain regression gates. Tests run
through the private local wrapper; no browser/native-display parity is implied.

C bridge and Python/Node/TypeScript/Go/Rust retain their existing time values and
generic CHANGE events. This C facade adds no foreign typed previous/value payload
or synchronous callback timing contract. Full TimeField/binding parity is not
claimed.
