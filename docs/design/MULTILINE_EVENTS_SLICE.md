# Multiline CHANGE observer proposal

Design audit at `0426a540`; none of the APIs below is implemented. This is a
compatible notification slice, not a rejectable edit transaction or a parity claim.

## Existing ownership and ordering

| Path | Actual behavior | Consequence |
| --- | --- | --- |
| [`cui_base_textarea`](../../src/base/cui_textarea.c) | Owns one frame-event loop; snapshots committed state before reduction and compares final revision, primary selection, visual affinity and scroll afterward. IME has a separate early-continue branch. | A final committed-state CHANGE seam is available. Per-event hooks would need every mutation branch, not just the ordinary edit tail. |
| [`cui_base_editor`](../../src/base/cui_editor.c) | Validates/probes layout, snapshots state, handles completion, handles multiple selections, then calls Base Textarea. Synchronizes cursor anchors, completion and search afterward. | Observing only its Textarea call misses completion and multi-cursor edits. The composite owns notification. |
| [`cui_textarea`](../../src/components/cui_textarea.c) | Establishes viewport/composition, calls Base once, then lays out and paints Styled content. | An additive common function can pass an observer at the existing Base seam without replay. |
| [`cui_editor`](../../src/components/cui_editor.c) | Owns Styled geometry, gutter/folds/completion UI around Base Editor. | Keep the existing sequence and use a common implementation; do not compose a second Base submission. |

[`cui_textarea_result`](../../include/cui/cui_textarea.h) exposes aggregate
`changed`, `selection_changed`, `scrolled`, and **last** Enter flags, not a submit
count. Enter inserts a newline unless `submit_on_enter` is enabled without Shift;
readonly does not set `pressed_enter`. The ordinary Enter branch does not have
Input's nonrepeat qualification. These differences must remain intact.
Editor can consume Enter to accept completion before Textarea sees it. Its
multi-selection pass can consume edits before the Textarea pass. Neither the final
result nor consumed-event bits identify a unique cause for the whole submission.
There is no existing public single-event Textarea reducer equivalent to Input's.

## First independently reviewable slice: aggregate CHANGE

Propose a new `cui_multiline_events.h` with:

```c
typedef struct {
    cui_textarea_result control;
    size_t change_count; /* Zero or one actual observer invocation. */
} cui_multiline_event_result;
```

Reuse `cui_component_text_event` and `cui_component_text_handler` from
[`cui_text_input_events.h`](../../include/cui/cui_text_input_events.h), with an
explicitly different delivery granularity. Proposed entrypoints mirror each
existing signature, append the handler, and return the new result:
`cui_base_textarea_events`, `cui_textarea_events`, `cui_base_editor_events`,
`cui_editor_events`. Keep all existing options, results and entrypoints unchanged.
An absent observer still edits and yields zero callback count.

For Base Textarea, capture scalar `before` at its existing snapshot, compare the
final committed revision, then notify once after final semantics/invalidation and
before return. For Base Editor, capture at its existing pre-pass snapshot and
notify once after its final search/completion/cursor synchronization, before return.
Its internal Base Textarea call must remain unobserved. Styled wrappers delegate
at the current Base seam and paint afterward. Refactor a private common function
rather than call legacy UI twice.

Emit only if the committed revision differs, independently of final layout/search
error: edits already applied before a later geometry failure are still committed.
Include the unchanged control error in the return value; observer delivery does
not certify rendering success. Every early return after the snapshot must route
through the notification finalizer, including Editor's multi-pass failure. Early
validation failures before any mutation do not notify. Do not infer CHANGE from
selection, scroll, `pressed_enter`, event consumption or a callback count.

Metadata is CHANGE with target and actual frame sequence, source UNKNOWN,
HAS_FRAME only: no index, activation or claimed physical-input source. Several
edits, Undo/Redo, completion acceptance, newline/indent and multi-selection edits
can coalesce. Equal final bytes may still have a changed revision; report that
revision change. External programmatic edits completed before entry do not notify.
A scalar before/after snapshot describes primary selection only, not all cursors.
Document this and keep multi-selection snapshots a separate future payload.

## IME, lifetime and compatibility limits

Preedit START/UPDATE/CANCEL normally changes composition presentation without a
committed revision and must not notify. Accepted COMMIT notifies if revision
changes; failed commit does not. Focus/readonly cancellation keeps existing
behavior. Undo/Redo are ordinary committed revision changes, with no new undo
ownership or replay. See [`test_textarea.c`](../../tests/unit/test_textarea.c)
(composition navigation, indentation/history, readonly/disabled and pointer cases)
and [`test_editor.c`](../../tests/unit/test_editor.c) (completion, multi-cursor,
search, fold and history coverage).

Borrow committed text only for the callback; bounded retention uses
`cui_component_event_copy_utf8`. These multiline options have no Input password or
mask mode: do not invent secure support or silently promise redaction. Hosts with
sensitive documents should use scalar fields and avoid retaining text. A future
explicit text-redaction option needs its own reviewed contract.

Reuse Input's caller preconditions: no edit/state/context/buffer/workspace mutation,
rebinding, destruction or recursive UI submission during the observer. Theme,
selection/history arrays, layout rows, completion/search/fold descriptors and
borrowed document storage must remain alive. Unrelated application flags are safe;
defer omission/destruction until frame and renderer consumption. No runtime guard
is claimed. Notifications are post-mutation and cannot reject edits.

Do not add REQUEST, COMMIT, SUBMIT or selection notifications in this first slice.
In particular, aggregate Enter flags cannot faithfully produce per-event SUBMIT,
and a composition COMMIT is not a general editing-commit lifecycle. Do not change
Editor pass ordering to manufacture Input-style evidence.

## Review and verification slices

1. Base + Styled Textarea aggregate observer: new header, narrow common functions
   in `src/base/cui_textarea.c` and `src/components/cui_textarea.c`, declarations in
   `include/cui/cui_textarea.h`, dedicated tests and guide. Compare old/new buffers,
   revision/history/selection, aggregate results, semantics and draw output.
2. Base + Styled Editor aggregate observer: narrow common functions in
   `src/base/cui_editor.c` and `src/components/cui_editor.c`, declarations in
   `include/cui/cui_editor.h`, dedicated tests. Prove completion and multi-cursor
   changes produce one outer callback and no nested Textarea callback.
3. Only if requested, separately audit a genuine per-event Textarea seam covering
   IME, clipboard, indentation, normal editing and early failures. Editor per-event
   delivery requires a distinct decision about its multipass ordering; it is not
   automatically inherited from that work.

For both initial slices test multiple text edits in one frame, edit then Undo,
selection/scroll-only frames, idle/external changes, missing observer, IME preedit
and accepted/failed commit, readonly/disabled, Enter/Shift-Enter/submit_on_enter,
borrowed-copy retention and bounded-copy failure, deferred omission, and failures
before versus after mutation. Compare old/new timing and pixels headlessly; retain
legacy Textarea/Editor suites. Native checks use `scripts/test_local.py`, never the
maintainer display. Add actual event-loop fixtures rather than direct notification
helper tests alone.

These C proposals do not supply Python/JS/TS/Go/Rust observers. Existing queued
bridge events are a separate, post-frame contract with different payloads and
ownership; see [binding audit](COMPONENT_EVENTS_BINDINGS.md). Binding exposure,
exports, build registration and generated SDK copies need separately owned work.
