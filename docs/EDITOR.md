# Editor

`cui_editor` is a retained multiline editing model with an immediate styled
view. Create a multiline `cui_edit`, then initialize `cui_editor_state` with that
borrowed model and a caller-owned `cui_textarea_line` workspace. Both must outlive
the state. The editor adds no allocations to drawing or search; edit history
allocation remains controlled by `cui_edit_config`.

```c
cui_editor_state state;
cui_textarea_line rows[1024];
cui_editor_state_init(&state, edit, rows, 1024);
cui_editor_options options = cui_editor_default();
options.textarea.width = 600;
options.textarea.height = 400;
cui_editor(ui, id, theme, cui_string("Source"), &state, options);
```

Defaults enable line numbers, disable wrapping, and show twelve rows. Textarea
options control wrapping, readonly/disabled behavior, size, indentation, and
clipboard integration. The view handles caret blinking, selection, pointer hit
testing/dragging, keyboard navigation, scrolling, undo/redo, and Tab indentation
through the existing textarea reducer. Logical line numbers appear once for
wrapped lines. Gutter scrolling follows content vertically and stays fixed
horizontally. Accessibility exposes the underlying multiline textbox.
`cui_base_editor` provides the drawing-free reducer with explicit metrics and
caller-managed viewport geometry; the line-number and syntax options apply only
to styled rendering.

`cui_editor_find` performs case-sensitive literal UTF-8 search and selects the
next or previous match, optionally wrapping. No query is retained. The next draw
reveals the selection using textarea caret scrolling. Missing matches and invalid
queries preserve selection and text; errors use existing edit result codes.
Selection offsets and syntax token boundaries are UTF-8 byte offsets.

The optional `highlight` callback receives the borrowed language identifier,
whole document, and a visible token range beginning. It returns the next token's
exclusive end and color. It may inspect surrounding document text to support
multiline lexical context. Ends must advance, stay within the current visual
line, and fall on UTF-8 boundaries; invalid ends render the rest of that row in
the ordinary foreground color. The callback runs only for visible rows and must
not mutate the model. Language selection itself does not invoke a parser: the
host supplies actual language recognition and syntax classification. Token
segments use measured prefix positions; splitting a shaping run can alter
ligatures/contextual shaping. Disabled text may still receive host syntax colors.

Verified scope includes search direction/wrap, editing selected matches,
undo, readonly input, line-number output, colored syntax text, invalid-hook
fallback, invalid dimensions, and caret reveal. Frozen GPUI Editor has much
broader display maps, folding, diagnostics, completion, language services, and
search features; those are not implemented here. This is a usable adapter, not
full Editor parity. Navigation follows the existing UTF-8 scalar model rather
than extended grapheme clusters; visual bidirectional navigation is unsupported.
Fonts and shaping follow the configured core/text backend and textarea metrics;
this API does not independently select a code font family or weight. Capacity
errors require a larger visual-row workspace; document mutation is exclusively
through the attached edit model.
