# Base Input change and submit notifications

Include `cui/cui_text_input_events.h` for two additive Base-only APIs:
`cui_base_input_events` submits the ordinary Input reducer and observes each event;
`cui_base_input_reduce_event_events` observes one event for composite controls that
already register their textbox and reduce events in queue order. Existing Input
signatures, options/results, validation, presentation and mutation policy remain
unchanged. A [Styled Input facade](TEXT_INPUT_STYLED_EVENTS.md) adds the same
observer at its existing reduction seam. [Textarea and Editor](MULTILINE_EVENTS.md)
have separately documented aggregate observers. Foreign bindings do not expose
these synchronous C observers.

```c
static void input_event(void *user, const cui_component_text_event *event) {
    unsigned *changes = user;
    if (event->event.kind == CUI_COMPONENT_EVENT_CHANGE)
        ++*changes; /* Update independent application state. */
    /* Inspect text only in this callback; copy with a bounded helper if needed. */
}
/* During submission, inside the textbox's same-ID layout box: */
cui_input_event_result result = cui_base_input_events(
    ui, input_id, cui_string("Name"), edit, &behavior,
    (cui_base_input_options){0},
    (cui_component_text_handler){input_event, &change_counter});
/* Reading result counts must not repeat the callback's side effects. */
```

This is drawing-free Base behavior. Painting, measured hit testing and styled
password mask resources must still use the appropriate existing composition.
Do not call an old reducer/Styled Input and this facade for the same events:
that would edit the model twice. The single-event API requires prior textbox
registration with `cui_interact`, increasing indices and the existing semantic/
paint completion responsibilities. It does not replay the rest of the queue.

## Meaning and ordering

CHANGE is a post-mutation notification when the existing single-event reducer
reports changed and the committed document revision actually differs. It is not
a rejectable proposed value and cannot veto or undo the edit. Selection-only
motion, preedit-only updates and rejected/no-op edits do not report CHANGE.
A missing observer does not make the control readonly: ordinary editing continues.
Use existing `validate`, normalization/format/matcher and readonly options for
their documented policies; no new REQUEST or COMMIT is invented here.

SUBMIT reports the existing single-line eligible nonrepeated Enter result.
It is an intent, not proof of accepted form data or successful validation. Existing
modifier, readonly, disabled, composition/preferred-character and consumption
behavior is preserved. In particular readonly Input can still report Enter under
its legacy policy; disabled Input does not. A rejected text edit followed by an
eligible Enter can produce SUBMIT without CHANGE. Multiline editing has no new
submit facade in this slice.

For each event: existing checked operation and presentation completion, final
revision comparison, CHANGE if changed, SUBMIT if eligible, then the next queue
event. TEXT/Enter/TEXT/Enter therefore exposes both intermediate committed values,
not just the final frame buffer. The whole-frame `control` field is the unchanged
legacy aggregate result; `change_count` and `submit_count` count actual observer
calls only, saturating at SIZE_MAX with `count_overflow`. No observer means zero
counts even when `control.changed` is true. Result reads/copies never dispatch.

IME START/UPDATE/CANCEL normally only affect provisional state. A successful
COMPOSITION_COMMIT that changes the committed revision reports CHANGE, not a
new COMMIT event. Rejected host COMMIT still ends its session using the existing
cleanup before observation; rejection does not become a document change.
Undo/Redo and synchronous clipboard edits report their actual committed revision
changes, retaining existing history/validation exceptions and clipboard errors.
No programmatic model edits outside these reducers are retrospectively replayed.

## Evidence, privacy and lifetime

`event` carries exact context-local frame/index identity. Known KEY_DOWN/UP
origins are KEYBOARD; actual pointer types are POINTER. TEXT and composition
origin remain UNKNOWN: a host-delivered text event does not prove a physical
keyboard. Presence flags establish evidence. No `HAS_ACTIVATION` or synthesized
activation snapshot is attached to text, movement, submit or IME operations.
No raw input/clipboard/preedit text is copied into metadata.

`before/after` are scalar `cui_edit_state` snapshots with UTF-8 byte offsets,
revision and history availability. They are not old text, a diff or a complete
multicursor selection set. `text` borrows the **current committed** model buffer
only through the callback. Copying the event struct alone does not retain text.
Use `cui_component_event_copy_utf8` with caller storage and an explicit byte limit;
check its status before retaining the returned owned-buffer span. No implicit
per-event document-sized allocation is introduced.

For masked Input or any password content purpose, `has_text=false` and `text` is
empty/NULL, including SUBMIT. Scalar length/selection/revision metadata remains
available; this API does not promise to hide those fields or make logging them
appropriate. A nonsecret empty value has `has_text=true` with an empty span.

Handler/user are borrowed synchronously. Observers may change unrelated app
state and record future edits, omission or teardown. They must not mutate,
rebind or destroy the active model/context/state/workspaces/caller buffers,
free userdata/resources or recursively submit. Apply such deferred changes only
after frame completion and renderer consumption. These are **preconditions**,
not runtime guards: the engine acceptance callback's mutation guard does not
extend automatically to a post-reducer observer. Existing presentation callbacks
retain their previous timing/side-effect contract. Never implement rejection by
calling Undo from an observer.

## Verification scope

`tests/unit/test_text_input_events.c` exercises ordered TEXT/Enter multiplicity,
intermediate values, exact indices/sources, passive counts, missing-observer edits,
selection/no-op behavior, Undo/Redo, accepted/rejected IME commits and preedit,
secure redaction, synchronous paste, presentation completion before observation,
bounded owned copies, invalid single-event indices and explicit composite
single-event reduction. Legacy Input/acceptance/password/Questionnaire tests
remain the behavioral regressions; native tests use
`python scripts/test_local.py --build-dir <build>`.

This establishes Base Input notification behavior, not browser/OS IME acceptance,
foreign request rejection, full event-family uniformity or general callback
reentrancy. Styled and multiline observer guarantees have their own guides. Integrators register the new test and
refresh source metadata/generated distributions/SDKs with normal tooling.
