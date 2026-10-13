# Styled Dock

The Dock view composes the portable Dock model, Tabs, Buttons and Resizable into a themed workspace. Each leaf shows an overflow-scrolling tab strip and calls only its active panel's content renderer. Nested horizontal and vertical splits use pointer and keyboard resize handles; changed ratios are written back to the model and therefore included in its serialization.

This implementation was checked against the frozen GPUI Kit 0.7.1 reference, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, including `crates/component/src/dock/dock.rs`, `tab_panel.rs` and `website/component/dock.md`. It implements a specific subset, not full Dock family parity. Panel menus are implemented. Native floating windows are outside the frozen Dock API audited here. Edge regions have explicit limits described below. Programmatic `cui_dock_move` and `cui_dock_split` remain available. No platform or Clay types appear in the public API.

## Minimal composition

```c
#include "cui/cui_dock.h"

static void editor(cui_context *ui, uint64_t key, void *user) {
    (void)key;
    const cui_theme *theme = user;
    cui_text(ui, cui_string("Your editor controls go here"), 16, theme->text);
}

/* Create once, outside the frame loop. */
cui_dock *model = cui_dock_create((cui_dock_config){0});
cui_dock_view *view = cui_dock_view_create(0, 0);
/* Check both allocations before use. Panel keys are stable application IDs. */
cui_dock_insert(model, cui_dock_root(model), 42, 0);

/* Inside a begun CUI frame; theme and registry stay alive through rendering. */
cui_dock_panel registry[] = {
    {.key = 42, .title = cui_string("Editor"), .closable = true,
     .render = editor, .user = &theme}
};
cui_dock_view_options options = cui_dock_view_options_default();
options.width = available_width;
options.height = available_height;
cui_dock_view_result result = cui_dock_view_render(
    ui, 100, &theme, model, view, registry, 1, options);
/* Check result.error. Destroy view and model after the frame loop. */
```

`width` and `height` must be finite and positive. Content callbacks submit ordinary CUI controls inside a clipped active-panel region; the host provides frame/event processing and rendering. Registry keys must be unique. An absent registry entry produces an unavailable-panel placeholder rather than invoking a callback. Panel keys include zero and `UINT64_MAX`. Panel-key-derived tab and close IDs remain stable across reorder and persistence restore; node-derived group/handle IDs change when model generations change.

View storage is allocated once, with defaults of 128 nodes and 256 panels. Its capacities must cover the entire model; insufficient capacity returns `CUI_DOCK_CAPACITY` before submitting the view. Keep one retained view per model/workspace ID. Rebinding to another model or ID resets retained state; `cui_dock_view_reset` explicitly clears it. Registry strings, callback user data and retained view storage must remain valid through frame rendering.

## Interaction and state

- Clicking a tab selects its stable panel key. Tabs provide roving focus, arrow/Home/End navigation and overflow reveal; disabled tabs are skipped.
- A closable, enabled panel has a real close button. Ctrl/Meta+W closes the active panel when its tab or close button has focus, outside IME composition. Closing collapses empty nonroot groups through the model and restores focus to a surviving tab.
- Same-group pointer drag reorders tabs without changing their panel keys or active selection. `allow_reorder` disables this. Cross-group drops in the center append and select the panel in the target group. Content edges create a 50/50 horizontal or vertical split; the tab strip itself accepts center moves. Edge zones extend up to 32 logical pixels (20% for small groups); left/right take precedence at corners. A six-pixel pointer movement starts the gesture. Escape, pointer cancellation, dropping outside the workspace, disabled sources and disabled workspaces cancel it.
- Split handles use existing Resizable pointer capture, keyboard stepping, focus and semantics. Aggregate child minimum sizes constrain feasible layouts; infeasible minima are proportionally compressed and reported by `result.fit_minimum == false`. Split handles overlay the zero-gap model layout rather than consuming a separate gutter.
- `focus_content` optionally moves focus to the selected panel's declared `focus_target` after its callback submits controls. With this option disabled, selection keeps tab focus.
- `disabled` prevents workspace interactions and marks content inert. The panel callback still renders its contents.

Selection and resizing update the model during the call. Structural close/reorder/move/split changes commit after scene submission and invalidate the next frame; redraw before inspecting their new geometry. At most one structural action commits per view call, with close taking precedence over a cross-group/edge drop, then same-group reorder. Rejected drops return a model error and preserve the layout; the initial pointer-down selection remains an ordinary selection. `on_event` receives selected/closed/reordered/resized/moved/split notifications; callbacks must not mutate or destroy the model/view during rendering. Do external model edits between frames. Serialization persists model layout, active keys and ratios, not registry callbacks or transient view state; see [Dock model](DOCK_MODEL.md).

`cui_dock_split_layout` exposes the same pure split geometry/minimum calculation used by model layout. It accepts a split node and finite nonnegative rectangle/gap, returns its two child rectangles and aggregate minima, and leaves output untouched on failure. Its `fit_minimum` reports fit along the split axis; the view's result reports the whole workspace's two-dimensional minimum fit.

During a drag, an inert title preview follows the pointer inside the workspace and an outlined translucent region shows a center or half-group edge target. Default colors come from the supplied theme. `custom_palette` enables `drag_palette.accepted`, `rejected`, `background` and `text`, including alpha. `disable_preview` suppresses these portals; `disabled` suppresses gestures and previews. Preview geometry changes immediately without animation, including when `reduced_motion` is true. The underlying Tabs adapter retains its own same-group insertion marker. The target indicates host-policy permission; model/view capacity can still reject the release.

`allow_operation(policy_user, request)` is a reusable veto for `CUI_DOCK_CLOSED`, `MOVED`, `SPLIT` and `REORDERED`. The request carries stable panel/source IDs and operation-specific destination/index/axis/before fields. For close there is no destination; for reorder `index` is the final index and `source` identifies the group. Move uses `SIZE_MAX` to append; split uses a 50/50 ratio. NULL allows every operation. The callback must be pure and must not mutate the model/view: it can run during preview and runs again immediately before commit. A denial sets `result.denied`, emits no structural event and leaves layout unchanged; normal pointer-down tab selection may already have occurred. Programmatic model calls intentionally bypass view policies, allowing the host to implement its own trusted actions.

`cui_dock_move_split` stages removal and split creation in the model's bounded scratch storage and commits only on success. Rejection preserves layout, node generations and output. The new leaf inherits the source group's minimum sizes; target constraints remain unchanged. A sole panel cannot split itself. View storage must accommodate two additional nodes for an edge drop (a conservative check even when a source collapse could free nodes).

For generated standalone distribution use, `CUI_DOCK_BASE_ONLY` opts out of the styled Dock implementation so the pure model can be embedded without the component library. Define it before `CUI_DOCK_IMPLEMENTATION` and including `dist/cui_dock.h`. The normal styled bundle requires its documented CUI component dependencies; this macro does not remove public model declarations.

## Active-panel toolbars and zoom

An optional `cui_dock_panel.toolbar` callback submits ordinary controls in a clipped row below the tab strip. It receives the same key/user data as content. Only the active panel in each visible leaf runs this callback. `toolbar_height` defaults to 32 logical pixels when the row is present; content receives the remaining height. Disabled panels/workspaces render the toolbar inert.

`panel.zoomable` exposes a maximize control. Its stable ID is `cui_dock_zoom_id(view_id, panel_key)`. A view retains a stable maximized-panel key and renders its leaf across the workspace, hiding other split leaves. Tab selection within that leaf updates the retained key. Zoom changes presentation, not split ratios or model serialization. `cui_dock_view_zoomed` queries it; `cui_dock_view_set_zoom` allows trusted host actions and activates the requested panel. Zoom-in consults `allow_operation` with `CUI_DOCK_ZOOMED` and emits that event on success. Restore emits `CUI_DOCK_RESTORED` and is always available while maximized, even when zoom capability, policy or workspace interactivity is later revoked. It bypasses veto to avoid trapping the user. Restoring requests focus on the panel tab; disabled tabs follow the core's normal focus eligibility rules.

Node generations can change during model restore while the maximized key survives. Removing the maximized panel automatically restores the workspace and requests a surviving tab's focus on the next frame. View reset clears zoom. Toolbar callbacks and all policy/event callbacks must respect the rendering ownership contract.

## Independently resizable edge regions

`cui_dock_workspace` wraps four independent model/view/registry descriptors ordered `CENTER`, `LEFT`, `RIGHT`, `BOTTOM`. Left/right run the full workspace height; bottom sits below the center between the side regions. Each open region can itself contain nested splits and tabs. Stable `cui_dock_region_id` values namespace controls, and `cui_dock_edge_resize_id` exposes keyboard-focusable resize handles.

```c
cui_dock_workspace *area = cui_dock_workspace_create();
/* Create and check distinct models/views for every region you will open. */
cui_dock_region_content regions[4] = {
    {.model = center_model, .view = center_view,
     .panels = center_registry, .panel_count = center_count},
    {.model = left_model, .view = left_view,
     .panels = left_registry, .panel_count = left_count},
    {.model = right_model, .view = right_view,
     .panels = right_registry, .panel_count = right_count},
    {.model = bottom_model, .view = bottom_view,
     .panels = bottom_registry, .panel_count = bottom_count}
};
cui_dock_workspace_set_edge(area, CUI_DOCK_LEFT, true, 200);
cui_dock_workspace_set_edge(area, CUI_DOCK_BOTTOM, true, 160);
/* Within a frame; an unopened descriptor may be empty. */
cui_dock_workspace_options area_options = {
    .view = cui_dock_view_options_default()
};
area_options.view.width = available_width;
area_options.view.height = available_height;
cui_dock_workspace_result area_result = cui_dock_workspace_render(
    ui, 100, &theme, area, regions, area_options);
/* Destroy area and separately destroy all owned models/views when done. */
```

Use `cui_dock_workspace_set_edge` between frames to open/close a region and assign its preferred size; a closed region runs no panel callbacks. Actual sizes respect aggregate model minima and optional region minima, defaulting to 64 logical pixels for center and 48 for edges. Infeasible minima are proportionally compressed inside the viewport and reported by `fit_minimum == false`. Preferred sizes survive closing and temporary viewport compression. Real handle adjustments update them and invoke `on_resize` with the edge and new logical-pixel size. Disabled/motion settings propagate to all handles and views. The wrapper performs descriptor/capacity validation before submitting its root. Configured region models/views must be distinct, including closed regions prepared as drop destinations.

`cui_dock_workspace_save` and `restore` copy a portable field-based snapshot containing three open flags and preferred sizes. Restore validates every positive finite size before changing state. Encode these fields in the host's persistence format alongside each region model's ordinary serialization; the snapshot is not a specified binary file format. Model callbacks, panel registries and transient focus/drag state are not serialized. Mutate neither workspace state nor region models from render/policy/resize callbacks.

Zoom is local to the selected region's view; other edge regions remain present. Drag/drop can transfer between configured region models as described below. The wrapper supplies layout and callbacks, not built-in toggle menus or native floating windows.

## Cross-region transfer and closed-edge landing zones

A workspace observes tab drags using clipped core focus-scope hits. Dropping into another visible region appends/selects the panel, or creates a 50/50 leaf when released at a content edge. A closed left/right/bottom region has an outer landing zone up to 24 logical pixels (20% of a small viewport), with left/right precedence at corners. A successful drop inserts into its first leaf and opens the edge using its retained preferred size. An empty closed model is a valid destination. Opening commits after transfer; cancellation, disabled input, policy rejection, duplicates or capacity failure never open it. `disable_cross_region` suppresses these transfers; `disable_edge_opening` removes closed-edge landing zones. Regular in-region reordering/splitting retains its existing behavior. Modals/higher interactive layers block workspace gestures through core hit eligibility.

Both source and destination descriptors must already register enabled metadata for the panel key. Models are separate containers; registries, strings, callback user data, views and models remain host-owned and must survive the frame/renderer and notifications. Nothing copies/frees registry data. After success, only the destination active content callback runs for that key; duplicate panel keys in the destination are rejected. Internal Dock IDs are namespaced by region, but callback-created control IDs must still be globally unique in the CUI frame.

`cui_dock_transfer` stages source removal and destination insertion/splitting in each model's bounded scratch storage, then swaps both states only on success. Failure preserves both serialized layouts, node generations and output. Split leaves inherit source-group minima. Ordinary model APIs and direct transfer calls bypass host view policies. Workspace gestures also restore the source group's pre-drag active key on cross-region rejection/cancellation, so dragging an inactive tab cannot leave a serialization change behind. If incidental selection occurred, the view emits a corrective `CUI_DOCK_SELECTED` notification for the restored key; no transfer notification is emitted. Ordinary clicks and same-group gestures retain their selection behavior.

The source view's `allow_operation` checks MOVED/SPLIT intent. Its destination node belongs to the other model; use workspace `allow_transfer(transfer_user, request)` for policies needing both models/region IDs. This pure callback can also run during target preview and cannot mutate model/view/workspace state. A denial sets workspace `result.denied`. `on_transfer` runs after both models commit and receives source/destination regions/models, stable key, final destination leaf and whether the edge opened. The request pointer is borrowed only during the callback; the source node ID can be stale after removal collapses its former group. Redraw before inspecting new geometry; destination tab focus is requested on the following frame, or the destination's `focus_target` when `focus_content` is enabled. Zoomed destination views reject drops, matching the frozen reference's zoomed-group safeguard.

The frozen reference distinguishes panel drops (`base/src/dock/tab_group.rs`) from dock resize listeners (`component/src/dock/dock.rs`). This implementation supports panel-drop opening of configured closed edges. GPUI's resize-handle gesture that itself reopens a closed dock is not yet implemented, and no native floating-window behavior is implied. Accepted/rejected cross-region landing indicators use the same custom palette and preview settings as the view; actual model capacity remains checked at release.

The Dock clips callback paint/input to each allocated pane. A nested GROW callback box with a long intrinsic line can still report raw bounds wider than that pane; those bounds do not enlarge the pane or allow hits into its neighbor. Hosts displaying semantic geometry should use clipped visible bounds or cap/wrap their own content boxes. Headless oversized-content tests verify pane allocation and clipped hover isolation.

## Panel action menus

Each active leaf displays an SVG ellipsis menu button by default; `hide_menu` opts out. The adapter composes the existing Menu tree reducer, popup painter and ghost icon buttons. The toolbar zoom button is also an SVG icon with an accessible action name. Custom borrowed `cui_menu_entry` trees are supplied through `panel.menu_items/menu_count`; selected custom IDs invoke `panel.menu_action` after scene submission using `panel.user`. Trees must have unique nonzero interactive IDs, valid descriptors and at most `CUI_DOCK_CUSTOM_MENU_LIMIT` entries across all descendants. Builtin action IDs are reserved. Keep descriptors and callback data alive across frames; do not mutate model/view state from callbacks.

Builtins provide Close, Close Others, Close All, Maximize/Restore, horizontal/vertical split and Balance parent split. Close Others/All operate on the current leaf and skip protected, disabled or unregistered panels. Every eligible key passes the close policy before an atomic bulk removal; one veto preserves the complete serialized layout. `result.closed_count` reports the number removed and each successful close emits its usual notification. `cui_dock_remove_many` exposes the same transactional model primitive without view policies. Split moves the active tab into a new 50/50 leaf and requires at least two tabs; Balance sets the immediate parent ratio to 0.5. Existing split/resize policies apply. Disabled actions cannot activate, while restore stays reachable for an already maximized panel. `cui_dock_menu_id` and `cui_dock_menu_action_id` provide stable semantic IDs.

The frozen component `dock/tab_panel.rs` composes host dropdown entries with zoom and close actions, using ellipsis and maximize/minimize SVG controls. CUI follows that composition; Close Others/All, split and balance are explicit CUI extensions, not claims about the frozen menu. Customizable zoom-control placement and native floating hosting remain separate work.

## Validation

The headless styled test exercises active-only callbacks, stable keys including zero/`UINT64_MAX`, disabled controls, keyboard focus/navigation, close shortcuts, collapse, same-group pointer reorder, nested pointer/keyboard resize, infeasible minima, restore/reset and capacity rejection. The model unit test checks split geometry against leaf layout and transactional validation failures. Cross-group and all four edge drops are tested with queued pointer events, including cancellation, outside drops, disabled/reorder-off and full-capacity rejection. Draw-command tests verify accepted/rejected custom preview palettes, preview suppression, motion settings and cancellation cleanup. Host veto tests cover close buttons/shortcuts, reorder, move and split. Toolbar/zoom tests cover active callbacks, controls, focus, persistence and removal recovery. Edge-region tests cover independent geometry, real pointer/keyboard resize, callbacks, hide/reopen, model minima, disabled controls, constrained viewport preference retention and rejected snapshot restoration. Two-model transfer tests cover queued region/edge gestures, callback ownership/focus, empty closed destinations, cancellation/modal blocking, duplicate keys, disabled/missing metadata, policy veto and full model capacities, with serialized/snapshot equality on rejection. Menu tests cover SVG draw commands, custom and disabled actions, restore, atomic bulk vetoes, protected tabs and split/balance actions. No native window or browser-specific behavior was tested for this adapter.

Until the integrator wires the new component source and test into CMake, reproduce from the repository root:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -Iinclude -Ithird_party/clay src/components/cui_dock.c \
  tests/unit/test_dock_styled.c build/libcui.a -lm -o build/test_dock_styled
./build/test_dock_styled
```

After integration, applications link the usual CUI library containing the styled adapter; no added dependencies are required.

## Remaining reference work

The frozen panel presentation trait exposes `toolbar_buttons` and `zoom_control` separately from base capabilities such as closable/zoomable. The active-panel toolbar callback and retained zoom support now cover that composition, with tests for focus, persistence, model edits/removal and toolbar ownership. The `zoom_control` field supports Both, Menu, Toolbar and None. Its zero default preserves the original CUI both-controls behavior; choose Menu for the frozen presentation default. None prevents user-initiated zoom, while an already zoomed panel always offers restore. The programmatic view setter has no registry and intentionally bypasses this presentation gate.

The reference's edge docks are independently openable/resizable left, right and bottom regions around the center Dock area. The workspace wrapper now implements those retained regions, minimum allocation, pointer/keyboard resizing, toggles and field-based persistence. Remaining parity work includes the collapsed bottom strip and its continuous resize/snap behavior. Native floating windows require a host window-management contract and are not implied by portable layout/render support.

## Frozen source and export audit

This audit uses the frozen commit above, both Dock `mod.rs` export lists, `website/base/dock.md`, `website/component/dock.md`, `base/src/dock/{dock_area,dock_placement,panel,registry,state,state_convert,tab_group,drag}.rs`, the layout modules, and component `dock/{panel,tab_panel,dock}.rs`. Names below group every public export family; mapping is behavioral, not Rust ABI compatibility.

| Frozen exports / presentation | CUI coverage and precise limits |
| --- | --- |
| `DockLayout`, `PaneTree`, `PaneNode`, `PaneRef`, `NodeId`, `PanelId`, `RootKind`, `InsertTarget`, `EditResult` | Bounded model, stable keys, node inspection, insert/move/split/reorder/remove/activate, transactional restore and normalization. Binary splits replace Rust n-ary children; no identical builder or optional measured-size vector API. |
| `DockArea`, `DockAreaRenderer`, `DockContext`, `DockEvent` | Styled view and workspace own no panels; callbacks compose ordinary CUI controls, region handles and post-edit notifications. No arbitrary renderer trait override per chrome element. |
| `Dock`, `DockSizing`, `DockPlacement` | Retained left/right/bottom open flags and preferred sizes, bounds/minima clamping and pointer/keyboard resizing. Frozen component deliberately does not re-export base `Dock`. Per-edge collapsible flags and collapsed bottom strip/live-size snapping remain gaps. |
| `TabGroup`, `TabGroupConstraints`, `TabGroupContext`, `TabGroupEvent`, `TabGroupRenderer` | Active selection, close/zoom, focus, drag eligibility, policy vetoes and preview paint. No sealed-group constraints object, isolated per-group lock, or collapsed-strip presentation API. Global disabled state is stronger than a drag-only lock. |
| `AnyDrag`, `DragPanel`, `DropIndicator`, `DropPlaceholderBounds`, `DropTarget`, `DragPanelPreview` | Portable queued tab gestures, cross-group/region transfer, content-edge splits, accepted/rejected target indications and previews. No GPUI entity drag payload or opaque host drag union is needed for CUI events. |
| Base `Panel`, `PanelView`, `PanelEvent` (component aliases `BasePanel`, `BasePanelView`) | Stable metadata, render callback, closable/zoomable/disabled state, focus target and edit events. Hosts retain user data; no dynamic visibility predicate, automatic added/removed/active lifecycle hook or panel-specific dump callback. |
| `PanelRegistry`, `PanelBuildContext`, `register_panel`, `PanelBuilder`, `PanelSource` | Host-supplied borrowed descriptor registry and placeholder for absent keys. Model serialization and edge snapshot are available; host reconstructs content and separately persists panel payloads/schema versions. No named factory registry or automatic panel construction from payload JSON. |
| `DockAreaState`, `DockState`, `PanelInfo`, `PanelState` | Portable model text plus validated edge snapshot. Not a compatible encoding of frozen serde states; callback data and focus are transient. |
| Component `Panel`, `PanelView`, `PanelControl`, `PanelStyle`, `TitleStyle`, `panel_handle`, `DockSkin` | Themed title strings, tabs, toolbar callback, custom Menu trees, SVG controls, four zoom placements and portable scene composition. Arbitrary title/title-suffix elements, per-title colors, Auto-vs-TabBar mode, `title_bar`/`inner_padding` overrides remain gaps. |
| Component `ToggleZoom`, `ClosePanel` | Real controls and close shortcut with focus/policy gates; user zoom is withheld when controls are None, and restore stays reachable. No Rust action dispatch registration. |

Native floating is not exported or described as a Dock feature in the frozen modules/docs. The occurrences of detach refer to panel removal/reinsertion or detached asynchronous tasks, not native window creation. A future CUI multiwindow host example is an optional extension and cannot stand in for the source gaps above.

The frozen `CLOSED_BOTTOM_STRIP` is 29 pixels. `dock_extent` gives closed side docks zero extent and explicitly leaves reopening to the application. The bottom strip retains a reachable handle; `resize_dock` follows the pointer below the minimum and `end_dock_resize` snaps to closed or minimum by the midpoint. Therefore the remaining source obligation is collapsed **bottom** strip resize/open/close, not invented invisible side-handle reopening.
