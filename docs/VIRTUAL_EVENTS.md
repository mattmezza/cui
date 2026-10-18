# VirtualList final-offset observers

Include `cui/cui_virtual_events.h`. `cui_base_virtual_list_events` and
`cui_virtual_list_draw_events` take the legacy arguments plus `cui_scroll_handler`.
They execute the old operation exactly once and return its unchanged result in
`control_result`, alongside `valid`, independent `changed`, delivered `change_count`
and `count_overflow` (always false for zero/one aggregate delivery).

The callback receives the existing pointer-free `cui_scroll_change`: float x/y
previous and final offsets, `has_previous`, and CHANGE metadata. Nonfinite entry
coordinates produce `has_previous=false` and a zero previous pair. Final offsets
must be finite. Source is UNKNOWN, with genuine frame only; no source-presence,
input index or activation evidence is fabricated. Copy payload by assignment into
caller storage; copies remain usable after frame and context lifetime.

Base observes after owner keys and semantics. Styled observes after all row and
measurement callbacks, deferred wheel finish, translation, scrollbar paint and
box closure. It does not run a Base observer internally. Deferred main-axis movement
can leave `control_result.view.scroll` describing the already-rendered window while
the payload describes final offset. `window_pending` and next-frame invalidation
retain their legacy meaning; there is no completion callback or repeated rendering.

Net-zero offset operations are silent. Disabled normalization can notify in an
ordinary frame. Missing handlers still reduce and set `changed`; counts remain
zero. Inert/layout-only frame-zero work preserves maintenance/results but suppresses
callbacks. Non-submission entry rejects before work. Invalid legacy arguments or
measurement rejection preserve legacy invalid result and produce no notification.
Omission retains no handler; result reads never replay callbacks.

Handler/user are borrowed until return. New observer callbacks may copy payloads
and update unrelated application data, but must not mutate/free active model,
widget state/options/input, recursively submit or destroy context. Defer teardown
until frame completion. These restrictions apply to the new observer; existing
legacy row and measurement callback behavior, timing and restrictions are unchanged.
Model keys/heights must remain unchanged during existing row callbacks as required
by `cui_virtual_scroll.h`. Measurements remain allowed to reject the frame.

This observes float widget offsets, not double model mutation, selection, rendered
visibility, data delivery or GPU completion. No retained registration, capture/
bubble, veto, context-destruction support or foreign-language facade is added.
MessageScroller has a separate [double-offset and tail observer](MESSAGE_EVENTS.md); its explicit mutators remain result-only.

## Observe scrolling in an existing frame

Retain the model, widget state and application data across frames. This helper
assumes the caller already began a frame and supplies its existing row renderer.
The callback records the applied offset; reading the result does not replay it.

```c
#include "cui/cui_virtual_events.h"

static void remember_scroll(void *user, const cui_scroll_change *change) {
    cui_scroll_offset *saved = user;
    *saved = change->value;
}

static cui_virtual_event_result draw_list(
    cui_context *ui, const cui_theme *theme, const cui_virtual_list *model,
    cui_virtual_widget_state *state, cui_virtual_render_item_fn render_row,
    void *row_data, cui_scroll_offset *saved_offset)
{
    cui_virtual_widget_options options = {0};
    options.scroll.width = 320;
    options.scroll.height = 240;
    return cui_virtual_list_draw_events(
        ui, cui_id_from(ui, "records"), theme, model, state, options,
        render_row, row_data, (cui_scroll_handler){remember_scroll, saved_offset});
}
```

Check `result.valid` before using the result. Use either the callback or
`result.changed` for each application side effect. `result.change_count` counts
delivered callbacks, so it stays zero with a missing handler even when offsets
change. Keep the saved-offset storage alive through the call.

`tests/unit/test_virtual_events.c` compares legacy and observed Base/Styled results,
state, consumed input, semantics and composition counts, exercises real deferred child routing, exact wheel,
keys/net-zero, invalid entry, normalization/disabled, missing handlers, passive
work, omission and copied payload lifetime. Intrinsic measurement tests preserve
legacy callback state mutation on both success and rejection, with one service call
and no notification on rejection. It also runs the existing Styled
virtual regression assertions. Native executions use
`python scripts/test_local.py --build-dir build -- -R virtual_events`.
