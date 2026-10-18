# Tree applied selection events

Include `cui/cui_tree_events.h`. `cui_tree_reduce_events`, `cui_base_tree_events`
and `cui_tree_events` add synchronous selection observers to the corresponding
legacy calls. They edit the same caller-owned selected key. A missing observer
still permits editing; its user pointer is ignored. No request/rejection API is
implied. Required pointers and nonzero compound ID are checked before admission;
invalid arguments return `result.error = CUI_TREE_INVALID` without legacy work.

```c
static void selection_changed(void *user, const cui_tree_selection_change *change) {
    bool *needs_save = user; /* unrelated application state */
    *needs_save = true;
    /* Copy *change here to retain it after this callback. */
}
/* During UI submission, with existing caller-owned model/state/selected: */
/* cui_tree_events(ui, id, theme, model, &state, &selected, options,
       (cui_tree_handler){.on_selection_change = selection_changed, .user = &needs_save}); */
```

Each different selected-key write emits one CHANGE immediately after the write.
A→B→A emits two callbacks even when legacy `result.selection_changed` is false.
`selection_count` counts actual observer calls, saturating at SIZE_MAX with
`count_overflow`; `result` preserves every legacy aggregate field. Already-selected
row invocation emits no selection CHANGE. Manual selection suppresses navigation
writes but retains existing row-invocation selection. Cursor repair, scrolling,
search timeout and programmatic model edits do not emit selection events.

Payload `previous`/`value` are stable keys; `node_index` is the current affected
model slot, not stable across reorder and not an input index. `event.target` is the
compound ID. Genuine accepted activation snapshots are copied when available and
retain their original row/toggle `activation.target`. Normal Tree reduction runs
before current-frame semantic submission, so snapshots need not be available:
fallback carries only proven active frame, actual event index and known event-type
source. Typeahead TEXT has UNKNOWN source; no keyboard device is inferred. Presence
flags are authoritative. No observer runs for disabled/passive/layout-only work.

Notifications precede existing later cursor/focus/scroll/semantic/paint work:
row selection notifies before cursor assignment; navigation notifies after cursor
assignment but before focus. Do not assume operation-final state or semantics in
an observer. Reducers, consume order, visible-row layout and paint run once.

Callbacks, user and payload pointer are borrowed during the call. Payload values
are pointer-free and copyable. Never mutate/rebuild/free active model/nodes/labels,
expanded array, selected key, state, input or context, or recursively submit UI
from an observer. Unrelated app flags may change. Defer structural changes,
omission and teardown until outer return plus renderer consumption. Nodes/expanded
arrays remain borrowed until next successful model rebuild; model must be explicitly
destroyed. Omission does not reset it. There is no new bridge/language binding API.
