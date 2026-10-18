# Textarea aggregate CHANGE observers

Include `cui/cui_multiline_events.h` for `cui_base_textarea_events` and
`cui_textarea_events`. Each mirrors its existing Textarea entrypoint, appends a
`cui_component_text_handler`, and returns `cui_multiline_event_result`:

```c
cui_multiline_event_result result = cui_textarea_events(
    ui, id, &theme, cui_string("Notes"), edit, &state, options,
    (cui_component_text_handler){on_change, app});
/* result.control is the existing cui_textarea_result. */
```

This facade deliberately aggregates notification: **at most one CHANGE per
submission**, after Base Textarea finalizes its event loop, semantics and scroll
state, and before Styled Textarea performs its remaining layout and paint. Several
text edits, newline insertion, indentation, paste or Undo/Redo can coalesce into
one notification. It is not the per-event timing of
[Input observers](TEXT_INPUT_EVENTS.md). Existing Textarea entrypoints, options,
mutation timing, aggregate results and rendering behavior remain unchanged.

`change_count` counts actual callbacks, zero or one. A missing callback does not
make the control readonly: edits still apply, with zero callback count. A callback
receives the scalar committed `before` and `after` state, including primary
selection and revision. Revision change is the criterion: typing then Undo in one
submission can notify even if final bytes equal initial bytes. Selection, scrolling,
preedit or external edits completed before submission do not themselves notify.

Metadata has CHANGE kind, the target and actual frame sequence, UNKNOWN source,
and HAS_FRAME only. There is no event index or activation: a batch can have many
causes. Do not interpret default scalar index fields when HAS_INDEX is absent.
There is no SUBMIT event. Legacy `pressed_enter`, `enter_secondary` and
`enter_shift` remain in `result.control` with their existing aggregate semantics.

IME START/UPDATE/CANCEL normally changes only preedit presentation and does not
notify. A committed revision change on IME COMMIT notifies; a failed commit with
no revision change does not. Disabled/readonly behavior remains the legacy behavior.

Notifications describe committed edits, not successful layout or validation. If
an edit succeeds and a later row-capacity/layout failure occurs, the observer still
runs and `result.control` retains the legacy error. Initial validation/layout
failure before mutation produces no CHANGE. The Base facade observes every return
from the existing reducer, so an applied edit is not hidden by a subsequent error.
The Styled facade preserves this count even if its later layout/painting fails.

The event's committed text is borrowed only during the callback. Retain it with
`cui_component_event_copy_utf8` and caller-owned bounded storage; copying the event
struct alone does not retain text. Textarea has no password/masking option: its
text payload is not redacted. Hosts handling sensitive documents should avoid
retaining text and use scalar metadata as appropriate.

Observers may set unrelated application flags. They must not mutate, rebind or
destroy the context, edit, state, buffers, history/selection arrays, layout rows,
composition storage, theme or other borrowed descriptors; recursive UI submission
and painting are also unsupported. This is a caller precondition, not a runtime
guard. To close or remove the control, set an application flag and omit it on a
later frame; defer destruction until the frame and renderer have consumed borrowed
resources. An observer cannot reject an already committed edit.

This slice does not add Editor, multi-cursor selection, REQUEST, COMMIT, SUBMIT,
or language-binding observers. Editor's completion/multi-cursor/Textarea passes
need a separately owned outer notification seam; wrapping its nested Textarea
would miss mutations.

`tests/unit/test_textarea_events.c` checks old/new text, selection, revision/history,
aggregate results, semantic identity/value and draw-command equivalence; multiple
updates coalesce; IME preedit, success and capacity failure; absent observers;
readonly/disabled and Enter behavior; bounded copies; deferred omission; committed
edits followed by row-capacity failure; and the direct Base type-then-Undo case.
Legacy Textarea and Editor tests remain required. Native test execution uses the
private-display `scripts/test_local.py` wrapper. No browser/binding parity is claimed.
