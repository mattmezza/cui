# Numeric selection CHANGE observers

Include `cui/cui_numeric_selection_events.h`. Pagination's additive Base and
Styled `_events` entrypoints accept `{change, user}` and return the original
aggregate bool as `control`, plus actual synchronous `change_count` (zero or one).
A missing callback ignores user and preserves normal reduction.

```c
static void page_changed(void *user, const cui_pagination_change_event *event) {
    size_t *last_page = user; /* Separate application storage, not the owner. */
    *last_page = event->after;
}
/* During the ordinary active UI frame: */
/* cui_pagination_events(ui, id, theme, &page, total, false, false,
 *     (cui_pagination_event_handler){page_changed, &last_observed_page}); */
```

The event contains pointer-free shared `event` metadata and exact native
`size_t before`, `after`, `total`. Pages are one-based, or zero when empty.
Normalization is part of the aggregate change. Multiple operations coalesce;
returning to the entry value produces no notification. Numbered-button mapping
continues to use the legacy normalized entry window.

Callbacks run after Base reduction/semantics and before Styled paint. Metadata
has CHANGE, root target, UNKNOWN source and HAS_FRAME for the current active
completion frame. It does not claim input index, activation count or physical
origin. Disabled, inert and layout-only submissions retain legacy normalization
and bool behavior but never notify: zero callbacks does not mean zero mutation.
`control` is not a validation or paint-success result.

Handler/user are borrowed for the outer call; no registration or replay occurs.
Copy events for later use. Do not change the owner, recursively submit, mutate
borrowed descriptors, destroy storage/context, or unwind foreign exceptions from
the callback. Request teardown or owner updates in separate application state
and perform them after the outer call returns. Styled paint rereads the owner;
owner mutation during observation has no supported rendering contract.

Stepper adds the same Base/Styled `_events` pattern with a distinct
`cui_stepper_change_event`: exact `size_t before`, `after`, `count`. Indices are
zero-based; an empty list normalizes to zero. Descriptors, titles and descriptions
remain borrowed through the call. Selection does not rewrite explicit statuses;
Styled paint still shows Current only on the selected item, retains Complete and
Error, and shows an unselected ACTIVE descriptor as Pending. Counts above 100 or
missing nonempty descriptor arrays retain legacy rejection. Disabled items cannot
activate, while list-wide normalization still occurs when disabled or passive.

Legacy entrypoints remain unchanged. Rating is the next family slice. This native
facade does not add bridge notification tags or binding wrappers.
