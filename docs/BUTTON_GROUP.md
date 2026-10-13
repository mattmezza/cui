# ButtonGroup

ButtonGroup composes direct Button children into a joined horizontal or vertical collection. It is distinct from ToggleGroup. The Base API depends only on Core; `cui_button_group_ex` supplies styled Button paint, icons, slots, hover/focus states and inherited sizing.

The caller owns a Boolean selection array, stable nonzero child IDs, and a zero-initialized workspace. Keep these alive through submission. Allocate the bounded workspace once, rather than on a small thread stack; it supports at most 256 direct children and performs no heap allocation. On the tested 64-bit build the Base workspace is 18,016 bytes and the styled workspace is 212,584 bytes; sizes vary with ABI. Labels, icons, content callbacks and optional Button states retain their existing borrowed lifetimes. Do not reuse a workspace, submit another frame, or mutate descriptors/theme/Button states from its callbacks. Selection callbacks may update the selection array. Descriptor and theme snapshots prevent descriptor changes from affecting later events in that submission.

```c
#include <cui/cui_components.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool selected[3];
    cui_button_group_style_workspace *workspace;
} app;

static void select_requested(void *user, const cui_button_group_request *request) {
    app *a = user;
    memset(a->selected, 0, sizeof a->selected);
    for (size_t i = 0; i < request->selected_count; ++i)
        a->selected[request->selected_indices[i]] = true;
}

static bool compose_group(cui_context *ui, const cui_theme *theme, app *a) {
    cui_button_group_styled_item items[] = {
        {.id = 101, .label = {"Day", 3}},
        {.id = 102, .label = {"Week", 4}},
        {.id = 103, .label = {"Month", 5}},
    };
    return cui_button_group_ex(ui, 100, theme, items, a->selected, 3, a->workspace,
        (cui_button_group_style_options){
            .behavior = {.on_change = select_requested, .change_user = a},
            .override_variant = true, .variant = CUI_BUTTON_OUTLINE,
        }, NULL);
}
/* Initialize: a.workspace = calloc(1, sizeof *a.workspace);
 * Compose inside a successful begin_frame/end_frame; free at shutdown. */
```

A callback is a controlled request. Ignoring it rejects the request. Single selection requests the clicked index, including a repeated click on the selected item. Multiple selection enumerates current selected indices in ascending order, then removes the clicked selected index or appends a newly selected index. Values are reread between accepted events, so two clicks in one frame produce two ordered callbacks and the second sees the first accepted change. Final checked/value semantics and styled selection paint reflect the final model immediately. The Boolean result of standalone Button remains “any accepted activation”; its callback now runs once per accepted event, rather than collapsing queued clicks.

The default group callback policy matches the pinned source's pointer-only callback path: mouse and touch taps request selection; keyboard and accessibility activation do not invoke the group handler. `CUI_BUTTON_GROUP_ALL_ACTIVATIONS` explicitly extends this to accepted keyboard/accessibility activations. Key repeat rejection remains Core behavior. If no group handler is installed, each child's own callback remains active, including keyboard activation. With a group handler, child callbacks are replaced. A genuine collection-background pointer activation reports `item_index == SIZE_MAX` and unchanged selection; it never reuses a previous child index.

Group disabled, child disabled and child loading states combine with OR. This intentionally avoids the pinned source's builder-order-dependent disabled propagation. Loading retains its busy description and blocks activation. Cancelled input and blocked releases do not dispatch callbacks. Pointer focus, custom cursors, slots, tooltips and Button state options remain per child. Group size/variant overrides take precedence; group compact/outline flags enable those child options. The group owns its direct child boxes, orientation, zero inter-child gap and segmented shape. First/last outer corners survive, internal corners are square, and later children omit the shared left/top border. A single child preserves its own shape. There is no invented roving-focus or mandatory-selection requirement; ordinary Core Tab behavior applies.

Under explicit or ancestor inert/layout-only scope, the styled API participates in layout and permitted paint without registering semantics, dispatching selection/hover callbacks, updating tooltip state, focusing or mutating the model or caller Button state. Custom content remains an application layout callback and must avoid side effects, as with ordinary Core layout-only composition. The staged Base API instead rejects suppressed scopes.

For Base composition, open the actual group box, call `cui_base_button_group_begin`, open each stable child box and call `cui_base_button_group_item` once, then call `cui_base_button_group_end` before closing the group. Missing/duplicate registration, stale frames, cross-context workspace reuse and reentrant dispatch reject without application callbacks. Partial Core registration is not rolled back; retain ordinary Core error handling. Reset abandoned workspaces before reuse, context recreation, or a complete 64-bit frame-sequence cycle.

Core's read-only `cui_event_accepted_activation` reports only activation accepted during current widget registration, in original frame-event order. Raw `cui_event_activation` remains compatible. Current disabled/loading role/modal metadata can invalidate a record; the query does not consume it. `cui_frame_sequence` is nonzero only during ordinary interactive submission and increments on successful begin even with zero delta time. `cui_frame_is_submitting` also admits passive inert/layout-only submission. Both reject measurement, layout resolution, prepaint and outside-frame queries; neither is a clock or persistent context identity.

Evidence: focused Core and Base tests cover multiple queued events, controlled accept/reject, source/all activation policies, cancellation, background identity, disabled/loading, snapshot and workspace lifecycle. Styled tests measure joins and text containment across four legacy sizes, fourteen design sizes, two density fonts, both axes and twelve variants, plus passive ancestry. Actual Chromium 1208 runs the WASM Core/components/software painter and genuine mouse/CDP touch events, checking joined pixels, shipped SVG pixels, custom cursor and callback behavior. The SDL Xvfb fixture pushes real SDL events through the adapter and renders native draw commands; it is queued-event evidence, not physical OS/device coverage. These tests do not establish complete Button-family parity, native font coverage at every size, or physical touch-device behavior.
