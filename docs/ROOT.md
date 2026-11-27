# Root

`cui_root` is a caller-owned, per-window content and overlay host. Zero-initialize it before `cui_root_init`, and call `cui_root_dispose` before reinitializing. Base owns the root and its ordered plugin registry. Applications register plugins during initialization, then create one root and a separate plugin-instance array for each window. Registration uses an application-defined numeric key: registering the key again replaces its factory in place for future roots, while existing roots keep their snapshot and state. Factories receive a per-window pointer and return caller-owned state that must outlive the root.

Each frame runs plugin `prepare` callbacks in registration order, then `style` callbacks, then applies an optional instance style refinement. Set the corresponding flag for each override field so unspecified plugin defaults remain intact. `decorate_begin` callbacks wrap the root in reverse registration order and matching `decorate_end` callbacks close the wrappers in registration order: the last plugin is outermost. Application content renders once; plugin overlays render once in registration order inside an absolute, full-root overlay host above it. Ordinary plugin children do not consume application layout space; they start at the root origin independently of root padding and follow outer decorators. Plugins may still open viewport portals for floating content. All callbacks run synchronously. Root requires a viewport starting at `(0, 0)` with positive finite dimensions. The plugin registry and root arrays have caller-specified capacities; invalid root capacity is rejected before any factory executes. If a later factory fails, Root calls each earlier plugin's optional `destroy` hook in reverse order and leaves the root uninitialized. `cui_root_dispose` calls those hooks for a completed root; factories that allocate resources must provide a matching hook.

For a styled window, initialize and bind caller-owned Dialog slots and an initialized Notification state once with `cui_root_bind_facilities`. Pass a `cui_surface_state` initialized as a Dialog or Alert to `cui_root_dialog_open`, or a Sheet to `cui_root_sheet_open`; both accept a content callback. `cui_root_render` mounts the active Sheet, Dialog stack and Notifications after application content on every frame. The app content callback never needs to mount those layers. Dialogs keep their own focus scopes and Escape handling; nested dialogs paint above their parents. At most eight dialogs can be bound so all Dialog layers remain below the Notification layer. Notifications are laid out in the remaining window area outside the active Sheet, using its current size and edge. Closed legacy Dialog slots are reclaimed on the next open. The root does not own the surfaces, notification entries, plugin state, strings or callbacks.


## Mount a composed Dialog or AlertDialog

Use the same stack to mount a six-slot Dialog or AlertDialog. Prepare
`cui_dialog_layout_v1` and `cui_dialog_composition_v1` using the
[Dialog composition guide](DIALOG_COMPOSITION.md). Keep this descriptor with
the layout and its state:

```c
/* root has been initialized and bound to caller-owned stack storage.
   layout and content were initialized for the dialog's lifetime. */
cui_root_composed_dialog_v1 profile_dialog = {
    .layout = &layout,
    .options = {.margin = 16},
    .composition = &content,
};

/* From the open button's activation handler: */
bool opened = cui_root_dialog_open_composed_v1(&root, ui, &profile_dialog, trigger);
```

Render the window with `cui_root_render` or `cui_root_render_with_options`.
Root submits the slots and gives them the current viewport and stack layer.
For custom sizing, `refine_sizing` receives fresh defaults and the viewport
once per submission. A dialog opened by application or Sheet content mounts
in that frame. One opened by another dialog's slot mounts on the next Root
frame. The shared capacity includes legacy surfaces.

Call `cui_root_dialog_close_top` to request that the top open dialog close.
The next submission runs the normal decision callback, so it can veto the
request. Accepted closure restores focus under the normal modal policy.
The owning component can still inspect its ordinary surface result.
The [headless Root test](../tests/unit/test_root_dialog_composition.c) demonstrates
a settings dialog with a scrollable Body, fixed Footer and nested AlertDialog.

Root borrows the descriptor, surface, layout, slot data and their contents. Use
one Root and one owner per mounted dialog. Keep storage alive until a Root
frame observes closure and ends, or dispose Root outside a frame after its last
render. Do not dispose Root, replace a mounted layout, or free any borrowed
storage inside a callback. A composed entry is reclaimed during the Root frame
that observes closure. Keep checking the end-frame diagnostic to detect delayed
layout failures. This authoring API is available in styled C.

## Responsive notifications

Use `cui_root_render_with_options` to wrap notification text on narrow windows
and make Close reachable by keyboard and touch before hover. Root mounts the
bound stack once, using the space outside the active Sheet. The regular
`cui_root_render` entry point keeps its original presentation.

```c
/* Inside an open frame, after binding the stack with cui_root_bind_facilities. */
cui_notification_advance advance = {0};
bool rendered = cui_root_render_with_options(
    ui, root_id, &root, &theme, viewport, NULL, render_app, app, now_seconds,
    (cui_root_render_options){
        .responsive_notifications = true,
        .notification_advance = &advance,
    });
/* After a successful frame, schedule a wake for a positive
   advance.next_wake_seconds using the same clock as now_seconds. */
```

The optional output reports transitions and the next wake for either
presentation. Zero means the stack has no pending timed work. Root only writes
the output when it renders successfully and the bound stack accepts submission. Initialize
it per frame, and check `rendered` and the result of ending the frame before
using it. These options are available in immediate C. See the
[Notification guide](STATUS_COMPONENTS.md#notification) for card sizing, action
callbacks and delivery.

## Base-only composition

For Base-only composition, call `cui_base_root_render` with application content and optional facility callback. It has no dependency on a styled plugin. A host can install `cui_root_set_copy_hook` to copy its current selection on an unconsumed Ctrl/Cmd-C event. The callback retrieves selected text and writes to the host clipboard; Root does not infer selection from widgets. Input and Textarea consume their own copy events before this fallback runs.

This covers the source Root's per-window state, registry snapshots, render stages, instance style precedence and automatic Sheet/Dialog/Notification mounting. The source's entity observation, deferred notifications, OS window helper, platform window chrome, global text-selection scope, tooltip/native-menu/touch-selection hosts, and copy selection layer remain unimplemented in this module. Applications currently drive frame scheduling and window lifecycle explicitly; completing those facilities remains part of the parity target. This root does not yet establish complete upstream parity.
