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
broader display maps, diagnostics, completion, language services, and
search features; those are not implemented here. This is a usable adapter, not
full Editor parity. Navigation follows the existing UTF-8 scalar model rather
than extended grapheme clusters; visual bidirectional navigation is unsupported.
Editor defaults select the portable monospace style. Set
`options.textarea.text_style` to choose the registered font family, weight,
italic style, and monospace flag; an all-zero style explicitly selects the
ordinary default font. Measurement, syntax tokens, gutter numbers, selection,
caret, and pointer geometry all use this complete style. Font size and line
height continue to follow textarea size/design metrics. Fonts and shaping
follow the configured core/text backend. Capacity
errors require a larger visual-row workspace; document mutation is exclusively
through the attached edit model.

The optional `cui_editor_find_bar` renders a single-line query input, match count,
Previous/Next actions, and Close. Initialize `cui_editor_search_state` with a
borrowed **single-line** edit model. Call the bar before the editor every frame,
including when closed, passing the editor's ID. Ctrl/Meta-F on that editor opens
and focuses the query, selecting its current contents. Enter navigates forward,
Shift-Enter backward, and Escape closes and returns focus to the editor. Buttons
expose ordinary button semantics; no-match navigation buttons are disabled.
A host may also set `search.open` directly. The query remains caller-owned and
no buffer or match array is allocated. Match counts use nonoverlapping literal
matches; query edits update counts immediately, while navigation explicitly
selects a match. Search bars use the existing Input/Button styled components,
so editor amalgamations require the basic components module as well as textarea.
This panel adds no replace mode, regex, case-folding, or all-match decorations.


Attach sorted caller-owned `cui_editor_fold` ranges with `cui_editor_set_folds`.
Line indices are zero-based, starts must be unique, and each range needs at least
one interior line within the document. Nested or overlapping collapsed ranges
hide the union of their interiors. This follows the frozen GPUI FoldMap boundary
rule: opening **and closing** logical lines stay visible. Every wrapped row of
an interior line is hidden. Ranges are validated on attachment and layout;
malformed caller mutations reject layout rather than reading beyond bounds.

`cui_editor_set_folded` changes presentation without modifying text/history.
Collapsing while either selection endpoint is inside moves both endpoints to
the opening line's end. A selection spanning a whole fold is preserved, and
copy/delete still operate on the original full document. Programmatic selection,
find results, or horizontal navigation entering hidden lines automatically
expand containing folds on layout. Vertical navigation and pointer hit testing
use visible rows and skip folded interiors. The line-number gutter exposes
Expand/Collapse buttons with expanded semantics. Activation requests redraw;
its updated projection is rendered on the next frame. Disable line numbers to
use folding through the model API without gutter controls.

Any document revision detaches all ranges, including after undo/redo; the host
must refresh and reattach candidates against current text. This deliberately
avoids applying stale line indices after arbitrary edits. It differs from
GPUI's incremental parser/candidate remapping. No fold discovery, automatic
indent/syntax folding, fold-all command, hidden-text placeholder, or parser-owned
persistent range tracking is supplied. Array memory stays caller-owned and must
remain alive and unmodified except for `collapsed` until detached. Selection
motion remains scalar-based, and styled syntax callbacks receive original byte
ranges of visible document rows.
