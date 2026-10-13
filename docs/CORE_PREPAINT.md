# Paint after layout

Use `cui_set_prepaint` when drawing depends on a box's geometry in the frame being
rendered. Examples include a chart crosshair, a clipped selection overlay or a
fixed paint-only tooltip. Normal component construction still determines layout
and accessibility before this callback runs.

Register on an open box with a nonzero ID. CUI copies the payload into bounded
frame storage and invokes the callback after its single layout pass, before the
renderer consumes the finished draw list.

```c
#include <cui/cui.h>

typedef struct {
    cui_color normal, hovered;
} highlight_colors;

static bool paint_highlight(const cui_prepaint_info *info, const void *data,
                            cui_prepaint_writer *writer) {
    const highlight_colors *colors = data;
    cui_draw_command command = {
        .type = CUI_DRAW_RECT,
        .bounds = info->bounds,
        .color = info->pointer_visible ? colors->hovered : colors->normal,
    };
    return cui_prepaint_emit(writer, &command, false);
}

bool highlight_panel(cui_context *ui, cui_id id) {
    highlight_colors colors = {{24, 30, 40, 255}, {34, 55, 78, 255}};
    cui_begin_box(ui, id, (cui_box){.width = 320, .height = 120});
    bool registered = cui_set_prepaint(ui, (cui_prepaint_options){
        .callback = paint_highlight,
        .payload = &colors,
        .payload_bytes = sizeof colors,
        .command_budget = 3,
        .hover_hitbox = true,
    });
    cui_end_box(ui);
    return registered;
}
```

The three-command budget covers the rectangle and its two clipping commands.
The local paint stream uses global coordinates and clips to the box's current
visible bounds. It appears above the box's underlay and below its children.
Use `info->bounds` to position paint and `info->visible_bounds` to inspect clipping.
`pointer_visible` includes current geometry, passive overlay occlusion and modal
permission. A pointer position is unknown until a host supplies one.

For a floating paint-only tooltip, pass `true` to `cui_prepaint_emit`. That deferred
stream clips to the viewport and uses the requested layer; zero inherits the
owner's layer. Its clip commands also count toward the budget. A layer cannot
precede its owner, and modal rules can deny deferred output. Use
`cui_prepaint_measure` for text measurement and emit a text draw command; its string
is copied into frame storage. This does not construct an arbitrary component tree
inside a tooltip.

The callback receives an immutable geometry/input snapshot. Use writer operations
for paint, measurement and `cui_prepaint_request_frame` when another animation
frame is needed. Do not create boxes, modify focus, dispatch input, invalidate a
context or register additional callbacks inside it. Read-only queries are allowed,
but the snapshot provides the relevant current-frame geometry. Existing input
routing still uses the published dispatch graph; this hook does not replace it.

Payload bytes are copied with alignment. Pointers inside them remain borrowed.
Image and mesh storage must remain valid until rendering completes; the writer
itself exists only during the callback. CUI applies inherited opacity once, so
callbacks should not multiply it again.

A false callback or invalid emission discards that callback's output. Draw-capacity
failure empties the final draw list and reports `CUI_ERROR_CAPACITY`. This guarantee
does not roll back side effects in application code. Configure bounded element,
command and frame-storage capacities for the workload and handle registration
failure. Prepaint adds per-context storage; the detailed capacity and scaling
measurements are recorded in the development validation report.

Headless tests cover copying, clipping, ordering, opacity, capacity and mutation
guards. An actual WASM browser fixture covers stationary-pointer scroll/resize,
passive/modal occlusion and deferred pixels. These are Core-hook tests; a component
must explicitly integrate the hook to gain those current-frame behaviors.
