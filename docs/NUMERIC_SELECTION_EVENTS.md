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

Rating adds a distinct `cui_rating_change_event` containing native `float before`
and `after`, `bool before_finite`, and effective defaulted/capped `unsigned maximum`.
The legacy reducer normalizes NaN and infinities to zero; an eligible observation
preserves that nonfinite `before` and sets `before_finite=false`. Do not compare
NaN using ordinary equality or assume canonical NaN payload bits. The committed
`after` is finite. Finite values compare exactly, with no epsilon; signed zeros
compare equal. Whole/half-step rounding and range clamping can cause CHANGE even
without input. Readonly suppresses callbacks while preserving normalization.
Pointer half-star selection and keyboard/full-star activation retain their
existing differences; Home still clears regardless of `clearable`, while
Delete/Backspace clearing requires it. Rating is a float API, unlike NumberInput's
double API. Failed later icon preparation does not undo a committed change.

Legacy entrypoints remain unchanged. These facades do not expose per-event
REQUEST, COMMIT or SUBMIT, and add no bridge notification tags or binding wrappers.
The native `size_t` payload is not a guarantee about legacy bridge float transport.

Verification uses actual reducers, unchanged legacy tests, and paired Base/Styled
submissions comparing scalar owners, bools, semantics, focus, geometry, draw text,
colors and Rating bitmap pixels. Tests include queue-rejected disabled accessibility
actions, input coalescing/cancellation, nil handlers, passive normalization, and
callback payload copies with teardown deferred until the outer call returns.
Native test execution uses `scripts/test_local.py` with a private display; hardware
GPU behavior is outside this portable slice.
