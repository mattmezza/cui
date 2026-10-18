# Styled Input observers

`cui_input_events` accepts the same arguments as `cui_input_ex`, followed by a
`cui_component_text_handler`, and returns `cui_input_event_result`. For example:

```c
cui_input_event_result result = cui_input_events(
    ui, id, &theme, cui_string("Name"), edit, &state, options,
    (cui_component_text_handler){on_text_event, app});
```

The handler receives committed CHANGE and eligible Enter SUBMIT events in input
order. A batch can deliver several of each; `result.control` preserves the legacy
aggregate result and the counts describe actual callback invocations. A null
handler still edits and produces zero callback counts. SUBMIT does not certify
form validity. Validation remains in the existing options.

Payloads and exact frame/index evidence follow [Base Input](TEXT_INPUT_EVENTS.md).
Text is borrowed only during the callback; use `cui_component_event_copy_utf8` to
retain a bounded copy. Masked and password-purpose inputs redact committed text
on both event kinds. IME preedit is not a committed CHANGE.

The observer runs after each event's presentation completion, before Styled Input
finalizes its mask cache and paints. Existing formatting, secure staging and
allocation failure behavior remain in the same submission. Failed mask preparation
does not notify or apply queued edits.

Keep context, edit, state, theme, buffers, workspace and resource descriptors alive
and unchanged throughout submission. Observers may update unrelated application
state, including a flag to omit the control next frame. They must not edit, rebind,
free or destroy these objects, recursively submit UI, or paint. This is a caller
precondition, not a runtime guard. Defer removal/destruction until after the frame
and renderer have consumed its borrowed resources.

The additive facade does not change `cui_input`, `cui_input_ex`, their options or
results. [Editor and Textarea observers](MULTILINE_EVENTS.md) have separate
aggregation and timing contracts. Language bindings do not expose these C observers.

`tests/unit/test_text_input_styled_events.c` compares legacy and facade results,
selection/revision and draw commands, ordered intermediate copies, masked IME,
password redaction, missing observers, deferred omission and mask allocation
failure. Existing Input acceptance and password tests remain required.
