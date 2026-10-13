# Editor

`cui_editor` is a retained multiline editing model with an immediate styled
view. Create a multiline `cui_edit`, then initialize `cui_editor_state` with that
borrowed model and a caller-owned `cui_textarea_line` workspace. Both must outlive
the state. The retained editor model and search add no allocations; edit history
allocation remains controlled by `cui_edit_config`. Fold chevrons use the
existing context-owned SVG icon cache, which prepares masks on demand.

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
full Editor parity. Ordinary keyboard navigation and erasure use Unicode extended grapheme
clusters; visual bidirectional navigation is unsupported.
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
remain alive and unmodified except for `collapsed` until detached. Styled syntax callbacks receive original byte ranges of visible document rows.


Host completions
----------------

`cui_editor_show_completions` borrows immutable UTF-8 label/detail/insertion
items and one validated document byte replacement range. It selects the first
item and opens the popup; zero items cancel it. The host owns fetching, language
analysis, ranking, and filtering. No provider or LSP connection runs implicitly.
The styled popup is shown while the editor owns focus. Up/Down cycle suggestions, primary Enter accepts,
and Escape cancels. Tab retains the existing indentation behavior; it does not
accept a suggestion. Pointer activation also accepts. Readonly/disabled views
suppress the popup and acceptance without changing document text.

The popup displays at most eight suggestions by default, keeping the selected
one in view; `completion_rows` allows up to 32 and `completion_width` defaults
to 320 pixels. It exposes a listbox with selected option semantics and retains
editor focus during pointer selection. Labels and details use the complete
configured code font. Positioning uses the last completed viewport layout;
opening before the first layout requests another frame to obtain an anchor.
The popup prefers below the caret and flips above when it fits inside the editor
viewport. Width, visible-row count, and position are bounded to that viewport;
a very small viewport without room for one item suppresses popup drawing while
keyboard acceptance remains available. If neither side fits, the popup may
cover the caret while staying inside the viewport. Global screen-edge placement
outside the editor is not supplied.

Acceptance uses `cui_edit_replace_range` as one atomic history step. Undo restores
the original selection as well as text. Capacity or validation failure preserves
text/selection/history and leaves the current menu open. Document revisions
cancel stale suggestions; normal editing, cursor movement, focus moving to another
control, or an outside pointer press also dismisses them. A host may refresh suggestions after those edits.
Earlier queued text/navigation events run before any later acceptance attempt,
so an old proposal cannot replace a newly typed prefix in that queue.
Borrowed items must remain alive and unchanged until cancellation or acceptance;
`cui_editor_cancel_completion` releases the references. A programmatic accept
operates on the attached edit model independently of styled readonly settings,
just like direct edit-model mutations.

Verified completion scope includes UTF-8 range/item rejection, undo selection,
capacity failure, stale revisions, bounded popup windows, code-style propagation,
Up/Down wrap, keyboard and actual pointer acceptance, Escape, readonly behavior,
invalid sizing, and queued typing followed by Enter. The frozen reference also
supports asynchronous provider resolution, richer completion documentation,
snippets, additional edits, and inline suggestions; these are not implemented.

Unicode grapheme editing
-----------------------

Ordinary Left/Right (including Shift selection), Backspace, and Delete now follow
Unicode 17.0.0 default extended grapheme clusters (UAX #29 revision 47). This
matches the Unicode version used by the frozen GPUI reference's pinned
`unicode-segmentation` 1.13.3. Combining marks, emoji modifiers/ZWJ sequences,
regional-indicator flags, Hangul, and Indic conjuncts remain whole. Textarea wrap
boundaries, pointer hit testing, and preferred-x visual row movement use the
same clusters. A cluster wider than the viewport occupies one overflowing row
and is clipped by the existing viewport; it is never split into scalar rows.

Offsets remain UTF-8 bytes. Explicit `CUI_EDIT_PREVIOUS`/`NEXT`, `cui_edit_select`,
`cui_edit_replace_range`, `cui_edit_backspace`/`delete`, and direct logical vertical
motions retain their scalar contracts. Hosts can opt into
`CUI_EDIT_GRAPHEME_PREVIOUS`/`NEXT` and `cui_edit_backspace_grapheme`/
`cui_edit_delete_grapheme`. Grapheme erasure expands partial cluster selections
outward; a collapsed scalar cursor inside a cluster removes that whole cluster.
Undo restores the original byte selection. Search/completion replacement ranges
and host syntax token ranges remain scalar byte boundaries, so host-controlled
operations can intentionally target part of a cluster.

`cui_grapheme_next`/`previous`/`boundary` validate complete length-bounded UTF-8,
including embedded NUL (which the edit buffer itself continues to reject).
Invalid UTF-8, continuation-byte offsets, and out-of-bounds offsets are rejected.
They allocate nothing and scan the supplied text. The forward iterator validates
once and traverses clusters in linear total time; its borrowed text must remain
unchanged. Textarea seeds this iterator at established row boundaries to avoid
rescanning the document for every wrap measurement.

Pinned property files, conformance data, SHA-256 checksums, and Unicode License
V3 are in `third_party/unicode/17.0.0`. Run
`python3 scripts/generate_grapheme.py --check` to verify generated properties and
embedded test fixtures. Headless tests cover all 766 official Unicode conformance
cases, iterator suffixes, keyboard selection/erasure/undo, scalar compatibility,
invalid inputs, and whole-cluster wrapping/hit geometry. This does not add
Unicode word segmentation, bidirectional visual navigation, language services,
or font shaping; those remain separate host/backend responsibilities.

Vertical selection at document edges
------------------------------------

Shift+Up on the first visible row extends the cursor to byte 0; Shift+Down on
the last visible row extends it to document end. The selection anchor stays
fixed. Plain Up/Down retain the preferred horizontal position at those edges.
This follows frozen Input `vertical_selection_target`, including soft-wrapped
rows and the visible row projection after folding. Readonly text remains
selectable; disabled or composition-owned key events remain ignored. Movement
changes selection only, without adding an undo step or changing the document
revision. Shift+PageUp/PageDown retain their existing CUI row-clamping behavior.

This slice does not change plain Up/Down collapse of an existing selection: CUI
still starts from the active cursor, while frozen `move_vertical` derives an
effective point outside the directional selection edge. Ctrl word movement
continues to use the documented existing heuristic rather than frozen Unicode
word-boundary segmentation.
