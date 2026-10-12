# Frames, state and callbacks

CUI uses immediate frame composition with persistent application and context state. Call components inside `cui_begin_frame` / `cui_end_frame`; they submit this frame's layout, draw commands and semantics. Keep values and long-lived models across frames. These are two parts of the same API, rather than separate immediate and retained rendering engines.

## What persists for each kind of component

| Components | Frame composition | Application state retained across frames |
| --- | --- | --- |
| Button, Label, Badge, Avatar, Separator | Call the component each frame | Usually no separate control model; keep application data such as click counts |
| Checkbox, Radio, Switch, Toggle, Slider, Rating, Pagination | Submit with the current value | Boolean, enum, number or selected page; animated controls may also use an explicit state struct |
| Input, Textarea, Editor model | Submit an editable control | `cui_edit` and its application-owned buffer; styled input/textarea behavior and layout state |
| Dialog, AlertDialog, Sheet, Popover | Submit while its surface is active | `cui_surface_state`, open state, decision result and any form data |
| Select, Combobox, Menu | Submit trigger and current popup contents | Choice/menu state, selected values and query editor where applicable |
| DataTable, Tree, Chart, TextView, MessageScroller | Compose visible content from the model | Data, keys, selection and the explicit model/state required by the public header |
| VirtualList, Dock, Plot infrastructure | Use the model to compute geometry/ranges for a frame | Virtual-list/dock handles or application-owned plot scales/data; their current gallery stories demonstrate partial base models |

A retained data model is not a retained application widget tree. CUI's context also retains focus, prior hit regions, resources and transient behavior. Destroy the context after the host stops consuming its draw lists; destroy separately owned models according to their headers.

The table describes ownership and submission. It does not claim that every entry has reached complete reference behavior. The gallery's Implementation scope lists the remaining features.

## Handle return-value events

Many controls report a change or activation by returning `true`. Invoke your application function in that branch:

```c
/* Inside an active frame; ui, theme, save_id and disabled are supplied. */
if (cui_button(ui, save_id, &theme, cui_string("Save"), disabled))
    save_document(application);

if (cui_checkbox(ui, notify_id, &theme, cui_string("Notifications"),
                 &application->notifications, disabled))
    persist_preferences(application);
```

The button reports activation for the frame's routed input. The checkbox writes the caller's value and reports a change. Rendering another frame does not call these application functions again unless another activation/change occurs. Keep application actions out of paint-only child callbacks, which may run during layout measurement.

## Use a controlled callback

Some option structs expose `on_change`. For Switch and Toggle, a supplied callback receives the requested value; the component leaves the authoritative application value to your code. Without the callback, the control updates its value pointer directly.

```c
#include "cui/cui_components.h"

typedef struct {
    bool sync_enabled;
    cui_switch_state sync_motion;
} preferences;

static void request_sync(void *user, bool requested) {
    preferences *app = user;
    app->sync_enabled = requested;
    /* Persist or validate here, using the application's own services. */
}

void draw_sync(cui_context *ui, const cui_theme *theme,
               preferences *app, double now_seconds) {
    cui_switch_ex(ui, cui_id_from(ui, "sync"), theme,
        cui_string("Automatic sync"), &app->sync_enabled,
        &app->sync_motion,
        (cui_switch_options){.on_change = request_sync,
                             .change_user = app,
                             .now_seconds = now_seconds});
}
```

Zero-initialize `preferences` before use and retain it between frames. The callback is synchronous; keep its user pointer valid during submission. A controlled callback may reject a request by keeping the old value. The next frame submits the accepted value.

## Connect host events

The host queues `cui_event` values before a frame. CUI routes them using previous-frame regions, focus, capture and overlay scopes. Your app submits controls for the new frame, obtains its draw list and semantic tree, then paints and bridges semantics through the chosen adapters.

Use [core lifecycle](CORE.md) for event ordering and [host interfaces](INTERFACES.md) for renderer/platform callbacks. A platform callback such as clipboard access is different from a component callback such as `on_change`; the host supplies the former, and application behavior supplies the latter.

## Schedule redraws

After application state changes outside a component, call `cui_invalidate`. Hosts redraw when `cui_needs_redraw` is true and honor component-specific animation deadlines. Pass real elapsed time and system reduced-motion preferences through the host. Avoid rerunning application side effects solely because an animation paints another frame.
