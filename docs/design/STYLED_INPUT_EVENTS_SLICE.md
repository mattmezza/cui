# Styled Input event slice

Add `cui_input_events` with the same arguments as `cui_input_ex` plus a
`cui_component_text_handler`, returning `cui_input_event_result`.
This is a CHANGE/SUBMIT observer, not a rejectable request.

Move the existing Styled implementation into one private common function. The
legacy entry calls it without an observer. At the existing Base submission seam,
the new entry calls `cui_base_input_events` exactly once; all validation, formatting,
mask preparation, hit geometry, mask finalization, semantics and painting retain
their current order. Early failures return the same control error and zero counts.

Observers run after each committed event and its presentation completion, before
Styled finalization and painting. Payload text is callback-borrowed and secure text
is redacted. Copy with `cui_component_event_copy_utf8` when retaining it. An absent
observer still edits. Counts describe actual callbacks, not aggregate flags.

The caller must keep context, edit, state, buffers, workspaces and theme alive and
unchanged throughout submission. Observers may update unrelated application state;
model mutation/rebinding/freeing, recursive submission and context destruction are
unsupported. Defer omission/destruction until after frame and renderer consumption.
No additional runtime lifetime guard is promised.

Tests compare legacy and facade text, revision, selection, aggregate results and
paint across ordered text/Enter batches, IME updates/commit, secure masking and
observer absence. They verify intermediate copied payloads, exact event indices,
redaction and deferred omission. Existing password and acceptance regressions remain
required; this slice does not add Textarea, Editor or language binding APIs.
