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
bidirectional visual navigation, language services, or font shaping; those remain separate host/backend responsibilities.

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

Plain Up/Down collapse of an existing selection now uses the frozen directional
effective-point behavior described below. Explicit legacy `WORD_PREVIOUS`/`NEXT`
motions retain their heuristic contracts; modified keyboard word behavior is
described below.

Unicode word movement and deletion
----------------------------------

Ctrl/Alt+Left targets the start of the last non-whitespace Unicode word segment
in the text before the cursor. Ctrl/Alt+Right targets the end of the first
non-whitespace segment after the cursor. Shift extends the selection with the
same targets. This follows frozen Input `previous_start_of_word_at` and
`next_end_of_word_at`: it uses default Unicode word segments, including
punctuation, rather than moving between identifier starts. For `one,  two`,
Right visits offsets 3, 4, and 9; Left visits 6, 3, and 0. Default boundaries
keep apostrophes in `can't`, numeric separators in `3.14`, and underscores in
`foo_bar`; ideographs remain separate segments without dictionary segmentation.
Unicode White_Space segments are skipped. Targets are clipped outward to
extended-grapheme boundaries for the existing whole-cluster keyboard contract.

Ctrl/Alt+Backspace and Ctrl/Alt+Delete erase to those targets as one history step.
A nonempty selection takes precedence and uses whole-grapheme selection erasure.
Undo restores the original byte selection. Readonly views permit navigation
while rejecting erasure; disabled and composition-owned events are unchanged.
These are portable modifier bindings; hosts may translate platform conventions
before routing events. OS-specific macOS Command line-erasure bindings are not
introduced by this portable model.

Hosts can call appended `CUI_EDIT_UNICODE_WORD_PREVIOUS`/`NEXT` motions and
`cui_edit_backspace_word`/`delete_word`. Explicit legacy `CUI_EDIT_WORD_PREVIOUS`/
`NEXT` continue to use their original identifier heuristic and next-start
behavior. `cui_word_next`/`previous`/`boundary` expose the Unicode 17.0.0 default
boundaries with the same complete UTF-8 validation and byte-offset contract as
the grapheme queries. They scan caller-supplied text without allocation; the edit
model continues to borrow its NUL-terminated buffer. This does not add code
Editor inline-token integration, multi-cursor editing, or dictionary segmentation;
ordinary atomic inline content is described below.

Pinned `WordBreakProperty.txt`, `WordBreakTest.txt`, and `PropList.txt` supply the
word properties, 1,944 official conformance cases, and Unicode White_Space.
`python3 scripts/generate_word.py --check` verifies their hashes and generated
inline properties/test fixture. Headless tests additionally cover both modifiers,
Shift selection, punctuation/ideographs, Hebrew quotes, numeric separators,
Katakana, identifiers, emoji/Indic/combining clusters, NBSP, partial-cluster
selection, atomic erasure/undo, scalar compatibility, readonly view behavior,
and Editor completion dismissal after modified navigation/deletion.

Plain vertical selection collapse
---------------------------------

Plain Up/Down from a nonempty selection derives a directional effective point
before moving one visible row, rather than moving from whichever endpoint is
the active cursor. It recomputes preferred x at that point and collapses the
selection at the target. Shift movement continues from the active cursor with
the original anchor; PageUp/PageDown also start from the active cursor, matching
the frozen reference's non-collapsing page calculation.

The calculation follows frozen Input `move_vertical` literally: Up calls
`previous_boundary(start.saturating_sub(1))`, whose helper subtracts another
byte before left clipping. CUI therefore floors `max(start - 2, 0)` to an
extended-grapheme boundary. Down calls `next_boundary(end - 1)`, which right-clips
the selection end. A point inside a folded gap clamps backward to the preceding
visible row end, or forward to the next visible row start. Down retains
upstream soft-wrap affinity only if the effective point equals the active
cursor; otherwise it starts on the downstream row at that shared byte offset.
These details matter at logical line starts, wrap boundaries, and folded gaps.

Tests cover both selection orientations, fresh/preserved preferred x, exact
line-start math, wrapped geometry, active-endpoint affinity, folded gap clamping,
readonly/disabled views, page behavior, whole-grapheme targets, and unchanged
document revision/undo history. This implements the retained single-selection
behavior; frozen multi-cursor merging remains outside this adapter's scope.
Atomic inline content belongs to ordinary Input/Textarea, as described below.

Atomic inline content for ordinary Textarea
-------------------------------------------

The frozen `inline_tokens.rs` exports `InlineToken`, `InputContent`, and token
replacement APIs only for ordinary Input and Textarea. Its `check_token_mode`
and `install_tokens` explicitly exclude code Editor mode. CUI therefore rejects
annotated models in Editor initialization and base/styled Editor preflight;
this feature belongs to `cui_edit` and the ordinary Textarea adapter.

`cui_edit_content` borrows the document and a sorted caller-owned collection of
`cui_edit_token_span`. Each span carries an ID, source text, and display-label
metadata. IDs may repeat; they must contain a non-whitespace scalar. Source text
and label are nonempty NUL-free UTF-8 without control characters or U+2028/U+2029.
Ranges must be nonempty, nonoverlapping, on grapheme boundaries, and exactly
match the token's source text. `cui_edit_content_valid` performs validation with
one forward grapheme traversal. No host callback runs during validation or
normalization, and no metadata allocation is hidden in these APIs.

`cui_edit_set_content` replaces the text as one atomic edit/history step, then
adopts the borrowed spans. Rejection preserves text, selection, history, and the
old attachment. When replacing annotated content, token strings must not alias
mutable edit storage; that alias is rejected before mutation. Use
`cui_edit_set_tokens` to attach annotations to unchanged text while preserving
selection/history. `cui_edit_get_content` returns a borrowed current snapshot:
the document view lasts until the next mutation/destruction, while arrays and
all token strings must remain immutable and alive until detachment. Call
`cui_edit_set_tokens(edit, NULL, 0)` before releasing their storage.

Pure `cui_edit_token_boundary` snaps strict interiors to the token start or end
according to the requested bias. `cui_edit_normalize_token_range` expands
nonempty ranges outward and moves collapsed insertion points to token end;
output is untouched on rejection. Attached spans protect insertion, deletion,
and range replacement using this normalization. Ctrl/Alt word deletion remains
one undo step even when its initial Unicode target is inside a token. Keyboard
motions skip token interiors, and Textarea hit testing/view selections use
atomic boundaries. Raw source text remains visible; wrapping keeps each atom
whole, with an oversized atom on one clipped overflowing row. Display labels
are metadata for the host, not automatically painted chips.

Explicit scalar `cui_edit_select`, PREVIOUS/NEXT and legacy WORD motions remain
available. `cui_edit_select_atomic` expands selections outward and snaps a
collapsed view cursor left. Attached atoms protect subsequent edits even when a
host selected a scalar interior. Whole-document indent is unsupported while
annotations are attached, so Textarea releases Tab ownership for those models.

Any text revision, including undo/redo, detaches annotations. Hosts must refresh
and reattach them; undo restores document text and the original byte selection,
but does not restore token metadata. This scoped borrowed-content model does
not implement owned content snapshots,
incremental span/history remapping, chip/label rendering, masked/secret Input
mode integration, grammar registries, or language services. Ordinary Input
adapter wiring outside this module must enforce its own supported-mode rules.

Headless tests cover source validation, repeated IDs, adjacency/overlap,
grapheme/text mismatch, controls/Unicode whitespace, biased boundaries,
normalization, scalar compatibility, atomic modifier deletion/undo, collapsed
insertion, invalid/capacity/alias rejection before mutation, invalid borrowed
range preflight, lifetime detachment, readonly/pointer/wrap geometry, Tab
semantics, and explicit code Editor rejection.

The ordinary edit model also provides `cui_edit_replace_with_token` and
`cui_edit_replace_range_with_token`. These validate the original grapheme endpoints,
expand existing atoms, remove intersected spans, and shift surviving spans. The
host supplies a separate output span array; success attaches that borrowed array.
All strings and output storage must remain alive and immutable until detachment.
Rejected operations preserve text, selection, history, output spans, and count.
Candidate insertion and surviving-span boundaries must remain grapheme boundaries.
Exact token/range replacements preserve selection and document revision. Undo still
restores document snapshots only and detaches annotations; token metadata has no
history payload. This is ordinary Input/Textarea functionality, not CodeEditor token
support. Masked/secret Input modes themselves are not currently implemented in CUI.

Literal replacement is available through `cui_editor_replace_current` (the
selection must exactly match the query) and `cui_editor_replace_all`. Replace-all
uses a caller-provided byte scratch buffer, scans nonoverlapping matches, and
commits one document replacement for one-step Undo. Its count is untouched on
failure; scratch contents are unspecified. Successful replace-all resets selection
and scroll to the start. These programmatic operations ignore view readonly and
disabled options; host UI must enforce its editing policy. These direct APIs remain case-sensitive; retained ASCII-insensitive matching and
current-index replacement are provided by the separate matcher APIs below.

`cui_editor_matcher` retains a borrowed immutable query and caller-owned match
ranges. Set-query activates it; close retains query/ranges but deactivates it.
Synchronization rebuilds after document revisions and resets current index to zero.
Next increments before selecting and wraps; Previous decrements and wraps. ASCII
case-insensitive matching folds only A-Z, preserving distinct non-ASCII case forms,
matching the frozen Aho-Corasick configuration. Queries must not borrow mutable
edit storage. Capacity rejection preserves session/range storage. Retained current
replacement preflights candidate match capacity, commits one edit, and keeps the
logical index (last replacement wraps to zero). No hidden allocation or callback
is involved. The existing literal APIs and find-bar retain their prior contracts.

The find-bar can opt into a built-in replacement row with
`cui_editor_search_enable_replace`, borrowing a retained matcher, distinct
single-line replacement edit model, and byte scratch buffer. Ctrl-H or
Meta-Shift-F opens replacement mode; Enter in the replacement field replaces the
current retained match. Semantic buttons provide case matching, replacement mode,
Replace, and Replace all; Escape from either field closes and returns editor focus.
`replace_disabled` lets the host enforce readonly policy while keeping find
navigation available. Replace-all respects ASCII-insensitive matching and remains
one Undo step. Existing find-only callers retain their original behavior. Search
matches are retained for navigation/status. Attach the matcher through the editor
options `search` field to paint visible active and non-current matches.

Retained search activation (`cui_editor_search_open`) seeds a nonempty document
selection into the query model, without changing document selection or history.
Only a changed query anchors its current index at the first visible projected row;
reopening an unchanged query preserves the current index. Every successful opening
increments `activation_revision`; the built-in bar reselects/refocuses its query.
Single-line query restrictions and capacity rejection are transactional. The
projected row anchor respects folding. Existing no-matcher find-only callers retain
their original query-retention behavior.

The editor options `search` pointer borrows the matcher. Active match rectangles
use measured prefixes with the complete code font style, including family. Hidden
fold interiors do not paint matches; viewport clipping applies to long lines.
Current matches use the selection color; other matches use a subdued selection
color. Hosts may supply exact colors with `custom_search_colors`. Document selection
paints after search backgrounds, and syntax callbacks remain read-only and paint
text afterward. Closing the matcher suppresses all search highlights. Revision
synchronization uses caller range capacity; capacity errors are surfaced rather
than drawing stale ranges. Match arrays and public matcher fields must remain
unchanged by callers while attached (except through matcher APIs).

The edit model has an opt-in bounded multi-selection foundation:
`cui_edit_attach_selections` borrows up to 64 live selections and one caller-owned
selection snapshot per existing document-history slot. It neither allocates new
storage nor changes document history sizing. `cui_edit_select_many` validates
extended-grapheme endpoints, merges overlapping/adjacent ranges, and preserves the
active selection's ID and orientation. `cui_edit_replace_selections` repeats one
text at every range in one transaction; before/after cursor collections round-trip
through Undo/Redo, including document-history eviction. Capacity/validation errors
preserve document, selections, and paired history. Candidate cursor endpoints
must remain grapheme boundaries. Inline-token annotations are excluded while this
storage is attached. Reattaching clears cursor metadata for older document entries,
which restore their legacy primary selection; detaching preserves text/history.
Legacy single-selection APIs collapse extra cursors. After attaching model storage,
call `cui_editor_multicursor_init` with a separate caller-owned array of
`cui_editor_cursor_anchor` entries covering the model's selection capacity. Keep
all model/history/anchor/visual-row arrays disjoint, alive, and unchanged by the
host while the editor is submitted. The adapter preflights this resource before
consuming events; no model storage means the existing single-cursor path.

The Editor accepts the frozen platform add-cursor combinations: Ctrl+Alt or
Meta+Alt Up/Down, and Shift+Alt Up/Down. Each existing cursor contributes a target
on the adjacent visible row; duplicate targets and targets inside selected ranges
are skipped. Alt-click adds a measured cursor while preserving the active cursor;
Escape keeps only the active range. Each cursor has a measured visual column,
including short rows, soft-wrap affinity, and folded-row projection. All carets
and selections paint using the complete configured text style. A hidden selection
endpoint reveals its fold before layout. Style, font size, and document changes
reset preferred columns. Call `cui_text_metrics_changed` after changing a host
measurement provider to invalidate anchors and measured rows. Preferred columns
are view metadata and do not round-trip through Undo.

Typing and Enter repeat text at every range in one document/history transaction.
Backspace/Delete use extended grapheme boundaries; Ctrl/Meta/Alt deletion and
horizontal movement use Unicode word targets. Arrow, visual Home/End, Page, and
Shift selection motion apply to each cursor and merge overlapping/adjacent ranges
while preserving the active ID/orientation. Document Home/End and Ctrl Up/Down
retain the source behavior of collapsing extra cursors. Readonly permits cursor
selection and navigation while blocking edits. Pure `cui_edit_selection_motion` computes a
single target without mutation; `cui_edit_move_selections`,
`cui_edit_erase_selections`, and `cui_edit_add_cursors` provide bounded atomic model
operations. Add-cursor preserves an endpoint cursor next to a selected range;
replacement coalesces coincident resulting cursors. Errors preserve the document,
collection, and paired history. Read-only Textarea row helpers expose measured
positions without modifying selection or pointer state.

Tab and Shift-Tab now use source-backed multicursor indentation. A noncollapsed
range anywhere in the collection makes the operation a block action for every
cursor: all touched LF rows, including a range's endpoint row, are processed once.
Collapsed inline Tab inserts exactly the configured unit at each cursor (not a
visual tab-stop calculation). Outdent removes only an exact leading unit from each
touched row, once even when cursors share that row. Partial/mixed indentation is
left unchanged. Hard tabs use one tab byte; soft tabs use `tab_size` spaces.
`cui_edit_indent_selections` also exposes explicit block mode. Every endpoint maps
through every earlier edit, including edits belonging to other cursors; points
inside removed indentation clamp to the remaining line start. IDs, orientations,
and collection order survive. The operation records one Undo transaction.
Ctrl/Meta bracket block actions dispatch through `cui_key_character`; the host's
portable key mapping must supply `[` or `]`. Hosts may also invoke explicit block
mode directly. These actions use the attached multicursor adapter.

Multicursor clipboard commands use the existing synchronous
`textarea.clipboard` platform hooks. `cui_edit_paste_selections` treats empty text
as no operation. It normalizes overlapping ranges, then splits on LF, retaining
a trailing empty line. An equal number of lines and selections distributes one
line per range in document order; otherwise the whole text repeats at every
range. CR bytes are retained. One Undo restores the prior collection and text.
`cui_edit_copy_selections` joins nonempty ranges in document order with LF, leaving
collapsed cursors out. It writes a NUL-terminated caller buffer and returned slice
only after capacity/alias preflight; failure leaves both outputs unchanged.

Set `clipboard_scratch`/`clipboard_capacity` on Editor options for Copy/Cut. This
borrowed buffer must fit joined text plus NUL and stay disjoint from document,
model/history, visual rows, and cursor anchors. Paste uses model scratch and does
not require this buffer. Cut preflights the document operation before calling the
host write; a failed write preserves document and collection. Readonly Cut copies
without deletion; readonly Paste does not call the read callback. Callbacks are
synchronous, must not reenter UI/Editor or mutate the model/resources, and returned
paste bytes must stay immutable through the operation. The adapter checks the
revision and selection collection after a callback and rejects a stale paste/cut
if the host changed them; host mutations are not rolled back. The same-editor
base reducer rejects reentrant submission while a clipboard callback is active.
`cui_edit_can_replace_selections` provides a read-only model preflight; internal
scratch contents may change. No async clipboard permission/request API is claimed.

With attached selections, Alt + primary click adds a caret. Hold and move the
pointer to select a rectangular block; Shift+Alt + primary drag replaces the
collection with a block immediately. Ctrl+Alt works the same as Shift+Alt on
Linux hosts. The start row remains active, and each pointer update allocates
fresh IDs for the visible rows between the endpoints. Release or Cancel stops
the drag; moving near the viewport edge scrolls and extends it. Readonly still
allows block selection. The caller owns the model selection/history arrays,
Editor cursor anchors, and visual row array for the full document lifetime.
The row array needs enough entries for every displayed row, including wrapped
rows. An undersized row array reports `CUI_TEXTAREA_CAPACITY`; an undersized
selection array reports `CUI_EDIT_CAPACITY` without changing the collection or
ID counter.

Hosts can also call `cui_editor_select_rectangle` after a current-revision
Editor layout. Each `cui_editor_column_point` contains a byte offset on a
visible row and a count of virtual space-width columns beyond that row's end.
The method clips columns to the end of shorter rows; virtual columns do not
insert padding into the document. Soft-wrapped rows and collapsed folds follow
display order, and hidden fold interiors cannot be endpoints. Grapheme endpoints
clip left, an intentional CUI extension to the upstream byte-column behavior.
Selection changes create no document Undo step; a later edit restores the block
through Undo. Block geometry depends on the current text measurement provider;
call `cui_text_metrics_changed` after changing that provider.

Textarea and Editor accept documents beyond 64 KiB when the caller supplies
enough visual rows. A headless test exercises selection, scrolling, and editing
near the end of 200,000 LF rows. Layout stores the complete display-row set
and reuses measured rows while document revision, style, width, wrap mode,
and font metrics remain unchanged. The Edit model reports the earliest changed
byte for each revision. A single ordinary edit retains preceding hard lines
and rebuilds the affected logical line and suffix; wrapped lines rebuild from
their logical-line start. Caret and dirty-row lookup use binary search.
Undo/Redo, indentation, skipped revisions, attached folds, and inline tokens
conservatively rebuild the full layout. Edits at the document start rebuild
all rows; the caller still owns storage for all visual rows.

The reproducible headless benchmark is `tests/bench_editor_200k.c`. Build the
portable library with CMake at `-O2`, then compile the benchmark with
`cc -std=c11 -O2 -UNDEBUG -Iinclude tests/bench_editor_200k.c build/libcui.a
build/libcui_software.a -lm -o bench_editor_200k`. It prints process CPU time,
text measurement calls, and draw commands for cold, unchanged, tail-caret,
tail insert, tail removal, and middle insert frames. On an Intel Core Ultra 5
235U, Linux x86_64, GCC 16.2.1, headless software command generation, one
release run measured 18.6 ms cold/400,003 measurements, 0.024 ms unchanged/0,
0.022 ms tail caret/2, 0.022 ms tail insert/9, 0.463 ms tail removal/5, and
6.82 ms middle insert/200,007. Tail removal still copies document/history
bytes in the Edit model. The caller-owned row array occupied 6,400,032 bytes,
plus 400,000 document bytes. These are single-run CPU measurements; host font
providers, document content, and load change the results. No 60 Hz guarantee
follows from this headless workload.

Consecutive interactive text batches with unchanged cursor collection coalesce
into one Undo step, up to 1000 batches. Navigation, selection changes, folds,
clipboard actions and atomic edits break the group; paste and completion remain
individual Undo steps. This slice does not implement multiple IME composition
ranges. Optional caller-owned `cui_edit_column_anchor` storage and one
`cui_edit_column_snapshot` per document-history slot retain each cursor's
preferred pixel x, logical grapheme column, and soft-wrap affinity across
Undo/Redo. Attach it with `cui_edit_attach_columns` after selection storage;
the Editor uses it automatically for multi-cursor vertical navigation. It
recomputes x from the logical column when text style or font size changes.
Without this optional storage, preferred columns live only in the view.
Inline-token annotations are excluded from these attached
multicursor models and CodeEditor, so token-aware clipboard/indent remapping is not
claimed. Search mode buttons show selected presentation as well as their semantic
checked state.
