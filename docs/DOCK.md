# Styled Dock

The Dock view composes the portable Dock model, Tabs, Buttons and Resizable into a themed workspace. Each leaf shows an overflow-scrolling tab strip and calls only its active panel's content renderer. Nested horizontal and vertical splits use pointer and keyboard resize handles; changed ratios or opt-in n-ary slot sizes are written back to the model and therefore included in its serialization.

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

`cui_dock_workspace_save` and `restore` copy a portable field-based snapshot containing three open flags, preferred sizes and noncollapsible flags. Restore validates every positive finite size before changing state. Encode these fields in the host's persistence format alongside each region model's ordinary serialization; the snapshot is not a specified binary file format. Model callbacks, panel registries and transient focus/drag state are not serialized. Mutate neither workspace state nor region models from render/policy/resize callbacks.

Zoom is local to the selected region's view; other edge regions remain present. Drag/drop can transfer between configured region models as described below. The wrapper supplies layout and callbacks, not built-in toggle menus or native floating windows.

## Cross-region transfer and closed-edge landing zones

A workspace observes tab drags using clipped core focus-scope hits. Dropping into another visible region appends/selects the panel, or creates a 50/50 leaf when released at a content edge. A closed left/right/bottom region has an outer landing zone up to 24 logical pixels (20% of a small viewport), with left/right precedence at corners. A successful drop inserts into its first leaf and opens the edge using its retained preferred size. An empty closed model is a valid destination. Opening commits after transfer; cancellation, disabled input, policy rejection, duplicates or capacity failure never open it. `disable_cross_region` suppresses these transfers; `disable_edge_opening` removes closed-edge landing zones. Regular in-region reordering/splitting retains its existing behavior. Modals/higher interactive layers block workspace gestures through core hit eligibility.

Both source and destination descriptors must already register enabled metadata for the panel key. Models are separate containers; registries, strings, callback user data, views and models remain host-owned and must survive the frame/renderer and notifications. Nothing copies/frees registry data. After success, only the destination active content callback runs for that key; duplicate panel keys in the destination are rejected. Internal Dock IDs are namespaced by region, but callback-created control IDs must still be globally unique in the CUI frame.

`cui_dock_transfer` stages source removal and destination insertion/splitting in each model's bounded scratch storage, then swaps both states only on success. Failure preserves both serialized layouts, node generations and output. Split leaves inherit source-group minima. Ordinary model APIs and direct transfer calls bypass host view policies. Workspace gestures also restore the source group's pre-drag active key on cross-region rejection/cancellation, so dragging an inactive tab cannot leave a serialization change behind. If incidental selection occurred, the view emits a corrective `CUI_DOCK_SELECTED` notification for the restored key; no transfer notification is emitted. Ordinary clicks and same-group gestures retain their selection behavior.

The source view's `allow_operation` checks MOVED/SPLIT intent. Its destination node belongs to the other model; use workspace `allow_transfer(transfer_user, request)` for policies needing both models/region IDs. This pure callback can also run during target preview and cannot mutate model/view/workspace state. A denial sets workspace `result.denied`. `on_transfer` runs after both models commit and receives source/destination regions/models, stable key, final destination leaf and whether the edge opened. The request pointer is borrowed only during the callback; the source node ID can be stale after removal collapses its former group. Redraw before inspecting new geometry; destination tab focus is requested on the following frame, or the destination's `focus_target` when `focus_content` is enabled. Zoomed destination views reject drops, matching the frozen reference's zoomed-group safeguard.

The frozen reference distinguishes panel drops (`base/src/dock/tab_group.rs`) from dock resize listeners (`component/src/dock/dock.rs`). This implementation supports panel-drop opening of configured closed edges. The opt-in collapsed bottom strip also supports handle-driven reopening as documented below; no native floating-window behavior is implied. Accepted/rejected cross-region landing indicators use the same custom palette and preview settings as the view; actual model capacity remains checked at release.

The Dock clips callback paint/input to each allocated pane. A nested GROW callback box with a long intrinsic line can still report raw bounds wider than that pane; those bounds do not enlarge the pane or allow hits into its neighbor. Hosts displaying semantic geometry should use clipped visible bounds or cap/wrap their own content boxes. Headless oversized-content tests verify pane allocation and clipped hover isolation.

## Panel action menus

Each active leaf displays an SVG ellipsis menu button by default; `hide_menu` opts out. The adapter composes the existing Menu tree reducer, popup painter and ghost icon buttons. The toolbar zoom button is also an SVG icon with an accessible action name. Custom borrowed `cui_menu_entry` trees are supplied through `panel.menu_items/menu_count`; selected custom IDs invoke `panel.menu_action` after scene submission using `panel.user`. Trees must have unique nonzero interactive IDs, valid descriptors and at most `CUI_DOCK_CUSTOM_MENU_LIMIT` entries across all descendants. Builtin action IDs are reserved. Keep descriptors and callback data alive across frames; do not mutate model/view state from callbacks.

Builtins provide Close, Close Others, Close All, Maximize/Restore, horizontal/vertical split and Balance parent split. Close Others/All operate on the current leaf and skip protected, disabled or unregistered panels. Every eligible key passes the close policy before an atomic bulk removal; one veto preserves the complete serialized layout. `result.closed_count` reports the number removed and each successful close emits its usual notification. `cui_dock_remove_many` exposes the same transactional model primitive without view policies. Split moves the active tab into a new 50/50 leaf and requires at least two tabs; Balance sets the immediate parent ratio to 0.5. Existing split/resize policies apply. Disabled actions cannot activate, while restore stays reachable for an already maximized panel. `cui_dock_menu_id` and `cui_dock_menu_action_id` provide stable semantic IDs.

The frozen component `dock/tab_panel.rs` composes host dropdown entries with zoom and close actions, using ellipsis and maximize/minimize SVG controls. CUI follows that composition; Close Others/All, split and balance are explicit CUI extensions, not claims about the frozen menu. Customizable zoom-control placement and native floating hosting remain separate work.

## Validation

The headless styled test exercises active-only callbacks, stable keys including zero/`UINT64_MAX`, disabled controls, keyboard focus/navigation, close shortcuts, collapse, same-group pointer reorder, nested pointer/keyboard resize, infeasible minima, restore/reset and capacity rejection. The model unit test checks split geometry against leaf layout and transactional validation failures. Cross-group and all four edge drops are tested with queued pointer events, including cancellation, outside drops, disabled/reorder-off and full-capacity rejection. Draw-command tests verify accepted/rejected custom preview palettes, preview suppression, motion settings and cancellation cleanup. Host veto tests cover close buttons/shortcuts, reorder, move and split. Toolbar/zoom tests cover active callbacks, controls, focus, persistence and removal recovery. Edge-region tests cover independent geometry, real pointer/keyboard resize, callbacks, hide/reopen, model minima, disabled controls, constrained viewport preference retention and rejected snapshot restoration. Two-model transfer tests cover queued region/edge gestures, callback ownership/focus, empty closed destinations, cancellation/modal blocking, duplicate keys, disabled/missing metadata, policy veto and full model capacities, with serialized/snapshot equality on rejection. Menu tests cover SVG draw commands, custom and disabled actions, all four zoom placements, restore, atomic bulk vetoes, protected tabs and split/balance actions. Collapsed-bottom tests exercise real pointer/keyboard opening and snapping, same-tab reopening, protected close shortcuts, no content/actions while closed, disabled gestures and snapshot rollback on cancellation or a blocking modal. No native window or browser-specific behavior was tested for this adapter.

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

The reference's edge docks are independently openable/resizable left, right and bottom regions around the center Dock area. The workspace wrapper now implements those retained regions, minimum allocation, pointer/keyboard resizing, toggles and field-based persistence. The opt-in collapsed bottom strip now covers continuous resize/snap behavior; per-edge collapsibility is retained in the workspace snapshot. Native floating windows require a host window-management contract and are not implied by portable layout/render support.

## Frozen source and export audit

This audit uses the frozen commit above, both Dock `mod.rs` export lists, `website/base/dock.md`, `website/component/dock.md`, `base/src/dock/{dock_area,dock_placement,panel,registry,state,state_convert,tab_group,drag}.rs`, the layout modules, and component `dock/{panel,tab_panel,dock}.rs`. Names below group every public export family; mapping is behavioral, not Rust ABI compatibility.

| Frozen exports / presentation | CUI coverage and precise limits |
| --- | --- |
| `DockLayout`, `PaneTree`, `PaneNode`, `PaneRef`, `NodeId`, `PanelId`, `RootKind`, `InsertTarget`, `EditResult` | Bounded model, stable keys, node inspection, insert/move/split/reorder/remove/activate, transactional restore and normalization. The default constructor preserves binary v1 layouts. The opt-in constructor supports canonical n-ary children, optional sizes, same-axis normalization and transactional v2 persistence. Explicit source-style root policy is available; no identical fluent builder. |
| `DockArea`, `DockAreaRenderer`, `DockContext`, `DockEvent` | Styled view and workspace own no panels; callbacks compose ordinary CUI controls, region handles and post-edit notifications. No arbitrary renderer trait override per chrome element. |
| `Dock`, `DockSizing`, `DockPlacement` | Retained left/right/bottom open flags and preferred sizes, bounds/minima clamping and pointer/keyboard resizing. Frozen component deliberately does not re-export base `Dock`. Opt-in bottom collapse includes its strip/live-size snapping. Per-edge collapsibility is retained and guards closing and bottom drag snapping. |
| `TabGroup`, `TabGroupConstraints`, `TabGroupContext`, `TabGroupEvent`, `TabGroupRenderer` | Active selection, close/zoom, focus, drag eligibility, policy vetoes and preview paint. The view has a collapsed-strip mode. Sealed/in-split constructors and a pure per-node constraint callback cover lock, close permission, last-panel preservation and collapse. These optional policies differ from global disabled state, which also blocks ordinary controls. |
| `AnyDrag`, `DragPanel`, `DropIndicator`, `DropPlaceholderBounds`, `DropTarget`, `DragPanelPreview` | Portable queued tab gestures, cross-group/region transfer, content-edge splits, accepted/rejected target indications and previews. No GPUI entity drag payload or opaque host drag union is needed for CUI events. |
| Base `Panel`, `PanelView`, `PanelEvent` (component aliases `BasePanel`, `BasePanelView`) | Stable metadata, render callback, closable/zoomable/disabled state, focus target and edit events. Hosts retain user data; dynamic visibility predicate; caller-owned next-tick lifecycle reconciliation and factory payload dump callbacks; no automatic view-owned lifetime management. |
| `PanelRegistry`, `PanelBuildContext`, `register_panel`, `PanelBuilder`, `PanelSource` | Host-supplied borrowed descriptor registry and placeholder for absent keys. Model serialization and edge snapshot are available; host reconstructs content and separately persists panel payloads/schema versions. Named caller-owned factories reconstruct live descriptors and preserve unknown payloads; typed frozen `PanelState` conversion is available; JSON text parsing remains host-owned. |
| `DockAreaState`, `DockState`, `PanelInfo`, `PanelState` | Portable model text, validated edge snapshot and typed `PanelState`/`PanelInfo` import/export. No complete serde `DockAreaState` JSON reader/writer; callback data and focus are transient. |
| Component `Panel`, `PanelView`, `PanelControl`, `PanelStyle`, `TitleStyle`, `panel_handle`, `DockSkin` | Themed title strings, tabs, toolbar callback, custom Menu trees, SVG controls, four zoom placements and portable scene composition. Title and active title-suffix callbacks compose ordinary CUI controls. Auto/TabBar modes, single-panel title-bar opt-out and multi-panel top-padding override are supported. Optional per-panel simple-title background/foreground match the frozen `TitleStyle` scope. |
| Component `ToggleZoom`, `ClosePanel` | Real controls and close shortcut with focus/policy gates; user zoom is withheld when controls are None, and restore stays reachable. No Rust action dispatch registration. |

Native floating is not exported or described as a Dock feature in the frozen modules/docs. The occurrences of detach refer to panel removal/reinsertion or detached asynchronous tasks, not native window creation. A future CUI multiwindow host example is an optional extension and cannot stand in for the source gaps above.

The frozen `CLOSED_BOTTOM_STRIP` is 29 pixels. `dock_extent` gives closed side docks zero extent and explicitly leaves reopening to the application. The bottom strip retains a reachable handle; `resize_dock` follows the pointer below the minimum and `end_dock_resize` snaps to closed or minimum by the midpoint. CUI covers collapsed **bottom** strip resize/open/close through the opt-in behavior below; it does not invent invisible side handles.

## Collapsible bottom strip

Set `workspace_options.collapsible_bottom = true` for the frozen closed-bottom behavior. The default remains false for compatibility with clients expecting a completely hidden closed region. A configured closed bottom model retains a 29 logical-pixel strip, compressed if the viewport cannot fit region minima. Its tabs show no active highlight, content, close buttons/shortcuts, toolbar/menu or panel drag source. The strip still supports roving keyboard focus. A pointer click or Enter/Space on an enabled tab opens the bottom dock at its retained preferred size. Closed left/right docks retain zero extent.

The bottom handle permits continuous resizing down to the strip, temporarily showing content above it. Release snaps to the strip or natural bottom minimum according to their midpoint; an open snap never leaves content below that minimum. Preferred size survives closing, so clicking the closed strip restores it. Keyboard handle resize also settles to a valid state. `on_resize` reports live changes and completion with logical size; save the validated workspace snapshot for persistence. Cancel/Escape, disablement or a blocking modal restores the complete pre-drag edge snapshot. Model content and serialization are untouched by this resize gesture. This adds no native window ownership.

`view_options.collapsed` independently requests tab-only presentation for an ordinary styled view; it is the workspace's building block, not an automatic reopen controller. The caller owns open/close behavior when using it directly. Multiple/nested groups retain the model topology and each visible leaf's tabs; CUI exposes optional sealed/locked constraints through the policy described below. Optional arbitrary title rendering hooks are described below.

## Per-edge collapsibility

`cui_dock_workspace_set_collapsible(workspace, edge, false)` withholds closing for that edge; it preserves its current open flag and preferred size. This matches frozen `DockArea::set_dock_collapsible` and the open-edge guard in `toggle_dock`. An already closed edge may remain closed and can still be opened. Calling `set_edge` to close an **open** noncollapsible edge returns false before changing any state. Making the edge collapsible again permits closing. Center is not an edge and is rejected.

Snapshots append `noncollapsible[3]` in left/right/bottom order; zero-initialized old host snapshots preserve the previous collapsible default. Persist these flags by field, together with open flags and sizes. A closed noncollapsible state is valid, as it is in the frozen reference. Restore remains atomic on invalid sizes. For an open noncollapsible bottom dock, resize respects the normal region minimum instead of snapping shut. A closed noncollapsible bottom still retains the opt-in strip; opening/resize returns it to a valid normal size. Headless checks cover all three flags, close rejection without snapshot mutation, valid closed-state restoration, invalid snapshot rollback and real keyboard resizing.

## Panel title composition

The frozen component Panel trait exposes `title` and `title_suffix`; `tab_panel.rs` paints every tab title and places the active panel's suffix outside the scrolling strip. CUI supports those hooks through `panel.title_render` and `panel.title_suffix`. Both callbacks receive the context, stable panel key and existing `panel.user`; descriptors and user data stay host-owned. The descriptor's `title` string remains the accessibility name, even when its visual body is an icon, badge or custom text. Do not mutate model/view/registry data from presentation callbacks, and keep callback-created control IDs globally unique across regions.

A title callback requires a positive finite `title_width`; its body paints inside that fixed clipped slot. Without custom title callbacks, the original built-in title drawing is unchanged. A group mixing custom and ordinary titles draws ordinary labels with the same default tab metrics and palette. Oversized callback descendants can report raw bounds beyond the slot, but paint and input are clipped and cannot expand the pane. This hook replaces the body, not all tab chrome or its selected/hovered palette.

A title-suffix callback requires positive finite `title_suffix_width`. Only the selected panel's callback runs, including the frame in which selection changes. The group reserves its largest declared suffix width (capped below pane width) and subtracts that space from the scroll viewport, preventing selection changes from shifting the strip or overlapping its trailing controls. The suffix wrapper is inert for a disabled workspace/panel; a collapsed group paints no suffix. Title callbacks still paint collapsed tab labels. Widths are validated before submitting the scene, and zero widths without callbacks preserve existing callers.

```c
static void title(cui_context *ui, uint64_t panel, void *user) {
    (void)panel;
    const cui_theme *theme = user;
    cui_text(ui, cui_string("Editor • modified"), 14, theme->text);
}
/* Append to the ordinary panel descriptor. */
panel.title_render = title;
panel.title_width = 140;
panel.user = &theme;
```

Headless tests verify real custom text/clip commands, unchanged semantic names, active-only suffix ownership and immediate selection routing, clickable suffix controls, disabled/collapsed gates, 160px pane allocation/hit isolation, stable key behavior after restore and atomic descriptor rejection. Per-title chrome colors remain separate audited work; modes and padding are described below.

## Auto title mode and panel-owned chrome

Set `view_options.panel_style = CUI_DOCK_PANEL_AUTO` to choose a 30 logical-pixel simple title when a group contains one panel. It retains the panel's semantic name, stable title key, focus/drag source, custom title/suffix callbacks and SVG close control. When groups merge to hold multiple panels they automatically return to real tabs. `CUI_DOCK_PANEL_TAB_BAR` (zero) preserves the original always-tabs presentation. A simple title clips long content; it does not replace it with a synthesized shortened semantic name.

In Auto single-panel mode, `panel.hide_title_bar = true` lets a self-chromed panel use the whole allocated pane: no title, close/menu or ordinary toolbar row is submitted. Multiple-panel groups still show tabs so every panel remains reachable; collapsed groups still show their reopening strip. An already maximized panel always retains its restore control even if it hides its own title bar. This is view presentation only; serialization and the panel key are unchanged.

Frozen `TabGroupSkin::panel_frame` applies its `inner_padding` hook as an 8px **top** spacer only in groups with more than one panel. Set `view_options.inner_padding = true` for that behavior; `panel.omit_inner_padding = true` removes it for the active panel. Single-panel content receives no spacer, regardless of title mode. Defaults preserve the original CUI zero-padding behavior. Choose Auto and inner_padding explicitly when adopting the frozen presentation defaults:

```c
options.panel_style = CUI_DOCK_PANEL_AUTO;
options.inner_padding = true;
/* A panel that paints its own header can use: */
panel.hide_title_bar = true;
panel.omit_inner_padding = true;
```

Headless tests assert simple-label versus tab semantics, exact content-origin changes for title hiding and top spacing, active-panel padding override, collapsed reachability, custom-title/suffix invocation in Auto mode, close-policy veto and real cross-group dragging from a simple title. Mode validation rejects invalid enum values before callbacks or scene submission.

## Group constraints and protected last panels

Frozen `TabGroupConstraints` combines alone, dock-locked, collapsed and closable state. A draggable group is unlocked and does not hold the last visible panel in an alone container; closing additionally requires container and panel permission. `sealed()` starts locked, alone and unclosable. CUI provides `cui_dock_group_sealed()` and `cui_dock_group_in_split(alone)` plus an optional pure `view_options.group_constraints(user, model, node)` callback returning `locked`, `unclosable`, `preserve_last` and `collapsed`.

A missing callback or all-zero result preserves prior CUI behavior. `locked` suppresses tab reordering, source dragging, incoming drops, splitting and user closing while selection, content controls and zoom remain usable. `unclosable` alone suppresses closing without blocking moves. `preserve_last` protects a group's sole panel against drag/close; with several panels, Close All retains the active key rather than emptying the group. `collapsed` paints that leaf as a tab-only strip without suppressing siblings. User close buttons, shortcuts, menu actions and transaction release checks all apply the constraints.

The callback receives the actual model and tab-node ID, including configured closed-edge targets. Cross-region constraints are checked against both models; foreign node IDs are never interpreted as source-model nodes. A denied closed-edge drop leaves both serialized layouts and open flags intact and does not fall through to an unrelated in-region split. Locked workspace destinations paint a rejected target indication. A source that becomes locked while dragging cancels the gesture. Model APIs remain independent of UI constraints and bypass them.

```c
static cui_dock_group_constraints constraints(void *user, const cui_dock *model,
                                               cui_dock_node group) {
    (void)user;
    /* Derive node identity from a stable panel key after every restore/edit. */
    cui_dock_node editor_group;
    if (cui_dock_panel_find(model, 42, &editor_group, NULL) && editor_group == group)
        return cui_dock_group_sealed();
    return cui_dock_group_in_split(group == cui_dock_root(model));
}
options.group_constraints = constraints;
```

Callbacks and user data remain host-owned; evaluate deterministically and do not mutate model/view/registry data. Constraints are presentation policy, not serialized model data. Restore can invalidate node IDs, so derive them from stable keys or refresh host mappings. `preserve_last` counts visible panels through the optional visibility predicate. The frozen lock originates at DockArea; CUI's per-node callback is a portable host seam rather than a claim about a second frozen pinned-group primitive. It protects panel rearrangement, not splitter geometry; hosts can separately veto `CUI_DOCK_RESIZED` through `allow_operation`.

Headless checks exercise constructors in model-only linkage, sealed-source selection/content, blocked close/shortcut/menu and drag, locked targets with byte-equal model serialization, unclosable-but-movable groups, sole-panel preservation, isolated collapse, preserved active key after bulk close, cross-model locked-source/target rejection, closed-edge non-opening, target permission revoked after preview and successful transfer after unlocking.


### Dynamic panel visibility

Set `view_options.panel_visible` and `visibility_user` to filter panels without removing them from the model. The callback receives the owning model and stable panel key; a null callback means every panel is visible. It must be deterministic during a frame and must not mutate model, view or registry data. Visibility is host state and is not serialized.

```c
static bool panel_visible(void *user, const cui_dock *model, uint64_t key) {
    const bool *show_inspector = user;
    (void)model;
    return key != 42 || *show_inspector;
}

/* Keep show_inspector alive while these options are used. */
options.panel_visible = panel_visible;
options.visibility_user = &show_inspector;
```

`cui_dock_visible_count(model, node, predicate, user)` counts visible members of a tab group. `cui_dock_displayed_panel(model, node, predicate, user, &key)` returns the stored active panel when visible, otherwise the first visible member; it returns false without changing the output when no member is visible. Neither query changes the active key or layout. Invalid groups return zero/false.

The styled adapter uses that same fallback, omits hidden tab/title/content callbacks and semantics, and moves focus from a hidden tab to the displayed panel. Showing the original active panel restores its presentation unless the user explicitly selected another panel. Auto mode uses the visible count; the optional inner-padding rule retains the frozen source's total group-member count. Filtered reordering maps visible anchors back to model indices, retaining hidden members. Close-all and close-others affect visible panels only, and preserve-last protects the displayed visible member. An all-hidden group gives up its geometry slot; an all-hidden zoomed group restores the normal layout.

Workspace transfers evaluate visibility against each region's model. Hiding a dragged source cancels the gesture; hiding the incoming key at the destination rejects release with `CUI_DOCK_INVALID`. Both layouts remain unchanged on rejection. Hosts should update visibility between frames, including after a drop preview. Headless tests exercise fallback, focus, unchanged serialization, Auto mode, visible-count protection, filtered reorder, bulk close, zoom restoration, hidden-slot geometry and real queued cross-region gestures.


### Concrete multi-slot sizing

The frozen `layout/node.rs` exposes n-ary split children and optional pixel sizes; `layout/edit.rs::set_sizes` rejects a vector whose length differs from the child count. The default constructor offers concrete measured sizing while preserving binary topology and v1 persistence. The opt-in canonical model below implements immediate n-ary children and optional sizes.

`cui_dock_split_slots(model, split, slots, capacity, &count)` flattens descendants with the same axis in visual order. A tab group or opposite-axis subtree is one slot. Pass null output to query the count; insufficient capacity reports the required count and leaves output untouched. Tab nodes reject the query.

`cui_dock_set_slot_sizes(model, split, sizes, count, gap)` atomically converts positive finite slot extents into the existing split ratios, including internal gaps. The vector must match the projected slot count. Invalid nodes, dimensions, counts or unrepresentable ratios leave the serialized layout unchanged. It preserves node IDs, active panels, topology and opposite-axis ratios. No host callback or view wiring is needed: the existing layout, styled adapter and splitter handles consume those ratios.

```c
/* Three same-axis slots, with four pixels between adjacent slots. */
const double sizes[] = {100, 200, 300};
cui_dock_result result = cui_dock_set_slot_sizes(model, split, sizes, 3, 4);
/* At an axis extent of 608, these widths are exact if minima allow them. */
```

At the measured extent `sum(sizes) + (count - 1) * gap`, layout reproduces the requested sizes unless minima constrain them. At other extents the existing binary ratios govern resizing; use the same gap and adopt new measured sizes when appropriate. For a legacy model this remains ratio projection, rather than canonical optional-size storage. Persisted ratios round-trip through the existing v1 format and older layouts still restore unchanged. Headless tests cover three slots with an opposite-axis subtree, gap geometry, minima precedence, query output on insufficient capacity, invalid-vector rollback and persistence.


### Opt-in canonical n-ary models

Use `cui_dock_create_nary(config)` to select canonical splits. The existing `cui_dock_create(config)` keeps its binary edits and exact v1 serialization. Both use the same caller-specified bounded node/panel pools and stable panel keys. A same-axis split inserts one sibling instead of introducing another binary container; opposite-axis splits introduce a new container. `cui_dock_node_get` exposes `child_count` and `nary`; `first` and `second` remain the first two children for compatibility. Enumerate every immediate child with `cui_dock_child_at`.

```c
cui_dock *model = cui_dock_create_nary((cui_dock_config){0});
cui_dock_node first = cui_dock_root(model), second, third;
cui_dock_insert(model, first, 10, 0);
cui_dock_split(model, first, CUI_DOCK_SPLIT_HORIZONTAL, .5, 20, false, &second);
cui_dock_split(model, second, CUI_DOCK_SPLIT_HORIZONTAL, .5, 30, false, &third);
cui_dock_node split = cui_dock_root(model);
const cui_dock_slot_size sizes[] = {
    {.has_size = true, .size = 100},
    {.has_size = false},
    {.has_size = true, .size = 200},
};
cui_dock_result result = cui_dock_set_sizes(model, split, sizes, 3);
/* At width 400 and gap 4, these occupy 100, 92 and 200 pixels. */
```

Check allocations and operation results in application code. `set_sizes` requires exactly the immediate child count. Known sizes may be zero and must be finite, nonnegative and at most 1e9; unknown numeric fields are ignored. The complete vector is validated before mutation. With unknown children, known sizes reserve pixels and unknown children share remaining space. An all-known positive vector scales proportionally to the available extent; an all-zero vector shares it equally. Minima take precedence, and infeasible minima compress proportionally. `cui_dock_children_layout` returns each immediate child's bounds, aggregate minimum and visibility; a null output queries the count, and insufficient capacity leaves output untouched. Hidden children retain their stored sizes and IDs but consume no geometry or gap.

Same-axis normalization follows frozen `distribute_slot`: when an outer size and every inner size are known and the inner sum is positive, inner sizes are scaled to the outer size. If any relevant size is unknown, inner sizes pass through unchanged. Surviving child IDs remain stable; a flattened container ID expires. Explicit nonzero CUI split minima retain a constraint boundary instead of flattening it. Removing a singleton container promotes its child and preserves the container's minimum. Loading does not flatten same-axis descendants or infer pixel sizes; the explicitly selected root policy may wrap/promote the root. A split of a known slot divides its stored size using the requested ratio; splitting an unknown slot retains unknown sizes rather than inventing a measured viewport.

Canonical models serialize as `CUIDOCK 2`, using `X`/`Y` splits with child counts and explicit known-size flags; tab and legacy binary node encodings remain available in v2. Restore accepts both v1 and v2 transactionally and selects the encoded editing mode. Restoring v1 into a n-ary-created object therefore resumes legacy binary editing without migration, except when an explicitly pinned root requires canonical root preservation as described below. Malformed counts, duplicate keys, invalid sizes and capacity failures preserve the previous model and output values. Legacy ratio and two-child geometry APIs reject canonical splits with more than two children using `CUI_DOCK_INVALID`; use `set_sizes`/`children_layout` instead. `cui_dock_balance` assigns equal canonical weights, or the legacy .5 ratio.

The styled adapter submits one real Resizable handle per visible adjacent child pair; `cui_dock_resize_id_at` identifies a handle by its preceding child index. Pointer and keyboard changes persist accepted measured sizes; merely rendering at a new viewport does not overwrite stored optional sizes. Cancellation ends capture while retaining the last accepted live size, matching the source resize interaction. Disabled state and operation policies block changes. Topology/visibility changes invalidate held splitter state. View capacity checks remain conservative for split gestures: reserve two spare cache entries even when a canonical sibling insertion would need only one model node.

Headless checks cover three-child geometry, unknown/zero sizes, minimum compression, nested same/opposite-axis normalization, malformed v2 rollback, exact v1 round trips, full-capacity two-model rejection and 7,000 random edits in each mode. Styled tests exercise the second divider, cancellation, disabled/policy rejection, hidden siblings and edge split/merge with unchanged child counts.

### Frozen method-family mapping

The export-family table above is complemented by these operation mappings. CUI exposes portable operations rather than Rust entity types or method signatures.

| Frozen method family | CUI operation / limit |
| --- | --- |
| `PaneTree` node/panel lookup, root, insert/remove/move/split, active selection, sizes and normalization | `root`, `node_get`, `child_at`, panel queries, `insert`, `remove`, `move`, `move_split`, `split`, `activate`, `set_sizes`; canonical same-axis normalization is automatic after edits. `create_with_root` selects source-style root preservation. |
| `PanelBuilder` horizontal/vertical splits, child optional sizes, tabs and active index | Compose canonical model operations and `set_sizes`; typed schema conversion is available; no fluent builder or JSON parser. |
| `DockArea` center/edge installation, open/toggle/collapsible, add/remove/select/move, zoom, dump/load | Workspace descriptors and edge setters/snapshot plus model/view operations; hosts supply independent models and caller-owned factory payload records. |
| `TabGroup` sealed/in-split constraints, active/displayed panel, close/zoom and renderer | Group constraint constructors/callback, visibility-aware queries, real view controls, menu/policy callbacks and scene callbacks. |
| `Panel` visible, closable, zoomable, title/style/suffix, toolbar, menu, focus and lifecycle | Borrowed descriptors and presentation callbacks, visibility predicate, factory/dump helpers and caller-owned lifecycle reconciliation. Optional simple-title palettes match the source; custom title scenes choose their own explicit ink. |
| `DockSizing` bounds/minima, pointer resize, collapsed-bottom reopen | Retained workspace handles, minimum compression and collapsibility flags; no native window creation. |

Lifecycle reconciliation is explicitly deferred and net-state based; it does not reproduce every synchronous Rust entity arrival or intermediate zoom event. Panel payload envelopes are caller-owned and are not a compatible implementation of the frozen serde JSON schema. These remaining differences prevent a full Dock-family parity claim.

### Caller-owned panel reconstruction and payloads

The frozen `registry.rs` registry is keyed by persisted panel name, and returns a rich panel view whose `dump` can write updated payloads. An unknown name must retain its original state across load/save. CUI's `cui_dock_panel_factory` provides this seam without a global registry or platform types.

A `cui_dock_panel_state` carries a stable key, factory name and opaque payload bytes. Pass records plus a borrowed factory array to `cui_dock_panels_reconstruct`. Records must cover every model key exactly once; duplicate keys/names, missing keys, incomplete coverage and insufficient output capacity reject before invoking builders. A builder receives the owning model, current tab ID and record, and fills a live `cui_dock_panel` with the same key. Copy each successful instance's `panel` into the contiguous descriptor array passed to `cui_dock_view_render`.

```c
/* build_editor/destroy_editor/dump_editor use the factory signatures above. */
cui_dock_panel_factory factories[] = {
    {cui_string("Editor"), host, build_editor, destroy_editor, dump_editor}
};
cui_dock_panel_state records[] = {
    {42, cui_string("Editor"), cui_string("{\"document\":7}")}
};
cui_dock_panel_instance instances[1] = {0};
cui_dock_result result = cui_dock_panels_reconstruct(
    model, records, 1, factories, 1, instances, 1);
if (result == CUI_DOCK_OK) {
    cui_dock_panel descriptor = instances[0].panel;
    /* Render with &descriptor/count=1 while all borrowed data stays alive. */
    (void)descriptor;
    cui_dock_panel_state saved;
    if (cui_dock_panel_dump(&instances[0], &saved) == CUI_DOCK_OK) {
        /* Copy saved.name/payload into the host's persistence envelope. */
    }
    cui_dock_panels_release(instances, 1);
}
```

Factories supply `build`, `destroy` and optional `dump`. Successful instances retain a borrowed factory pointer. Unknown names produce a descriptor with the original key/title and no content callback, while retaining the exact name/payload bytes, including embedded NULs. A dump callback returns borrowed updated payload bytes; without one, `cui_dock_panel_dump` returns the original payload. A failed dump leaves output untouched. The factory name and key remain the original identity.

Reconstruction stages descriptors before publishing output. On a failed builder or mismatched returned key, successful staged instances are destroyed and output/model remain unchanged. A builder that returns failure must clean up its own partial resources. Builders, destroy and dump callbacks must not mutate the model, registry, records or output arrays, or recursively invoke reconstruction on the same storage. Arbitrary external callback side effects cannot be rolled back.

The caller owns instance storage, descriptors, registry, payload bytes and all factory-created user data. Keep borrowed registry/strings alive until `cui_dock_panels_release`; that function destroys each successful known instance once and clears the array. Do not overwrite live instances with reconstruction. Release descriptors only after the UI no longer references them. Dump payloads remain valid according to the factory's documented lifetime and should be copied into host persistence before it expires.

The v1 layout remains unchanged. Restore the layout first, reconstruct its records into new storage, then publish the model and descriptors together; preserve the old model/instances until both steps succeed. Hosts can wrap the layout, panel records and application schema version in their own JSON or other format. This factory seam does not parse JSON, manage application schema migration or deliver lifecycle notifications. The typed schema converter below handles frozen tree structure.

Headless tests reconstruct and render a real control, dump updated known payloads, retain byte-exact unknown payloads, reject invalid coverage/duplicate registry names before callbacks, roll back a later failed builder and wrong-key builder, preserve serialized layout, and verify destruction occurs once.


### Deferred lifecycle reconciliation

The frozen `panel.rs`, `active.rs` and `tab_group.rs` distinguish group arrivals from genuine removals. Initial reconciliation announces only the active panel; inactive panels never receive an initial `false`. Active changes use frame-end net state, with deactivation before activation. A hidden stored-active panel still receives `true`, independently of visible rendering fallback. Moves preserve prior notified active state and re-add to the destination group without removal. Removal itself is the deactivation signal.

CUI exposes those rules through caller-owned `cui_dock_lifecycle` bookkeeping. Append `on_added`, `on_removed`, `set_active` and `set_zoomed` callbacks to descriptors, using the existing panel `user`. Keep two distinct, disjoint `cui_dock_lifecycle_snapshot` arrays alive for the tracker's lifetime. Initialize once; capture the complete workspace after committing edits/end-frame, and dispatch on the next host tick. Include closed regions: closing an edge is not removing its panels. Missing descriptors are still tracked by key but have no callbacks.

```c
cui_dock_lifecycle lifecycle;
cui_dock_lifecycle_snapshot current[64], pending[64];
if (!cui_dock_lifecycle_init(&lifecycle, current, pending, 64)) {
    /* Invalid storage/capacity. */
}
/* At the beginning of the next host tick: */
(void)cui_dock_lifecycle_dispatch(&lifecycle);
/* After all region edits and rendering have committed: */
cui_dock_lifecycle_region regions[] = {
    {.model = center_model, .view = center_view},
    {.model = inspector_model, .view = inspector_view}
};
cui_dock_result result = cui_dock_lifecycle_capture(
    &lifecycle, regions, 2, descriptors, descriptor_count);
```

Pass each region's visibility predicate to determine its visible zoom recipient; active notifications use the stored active key. Set `collapsed` or a pure `group_collapsed` callback to mirror collapsed group constraints: collapse deactivates the stored active panel, and reopening activates it again. Ordinary closed edges should remain uncollapsed unless the host intentionally presents the collapsed bottom strip. The lifecycle capture itself does not change layout, focus, policies or visibility.

Repeated successful captures replace pending state until dispatch, so within-frame selection churn produces only net active edges. Capacity, duplicate descriptors, repeated model regions or duplicate keys across models reject before changing pending notifications. All regions must be captured together to recognize cross-region moves. A failed or vetoed edit whose model stays unchanged yields no lifecycle edge.

Dispatch commits bookkeeping before invoking callbacks. It delivers group arrivals first, genuine removals next, then all active deactivations before activations. Moving the same instance to a different model/tab calls `on_added` with the new location; it never calls `on_removed`. An optional `lifecycle_identity` token distinguishes a newly constructed instance at an existing stable key: changing the token emits new-added, old-removed and initial activation for the replacement. With null tokens the stable key is the instance identity. Restore-invalidated tab IDs produce a fresh group arrival while retained instance active state remains seeded.

Group zoom-state edges notify only the currently visible displayed panel, including restore even when that panel never received zoom-in. Selecting another tab in an already zoomed group does not retroactively notify it. Capture reports net group zoom state; it does not reconstruct every intermediate zoom toggle within one tick. The frozen source dispatches zoom operation events asynchronously, so hosts requiring that event stream should retain their operation results separately.

Callbacks run outside rendering and can edit a model for the next capture. Recursive capture/dispatch on the same tracker is rejected. Do not reinitialize, directly modify or free tracker storage during dispatch. Callback/user/token resources from both old and new snapshots must remain valid until dispatch completes; callback edits cannot invalidate another scheduled callback's resources. Group/model pointers passed to callbacks are borrowed location identities; retain models through dispatch before destruction. Do not release old reconstructed instances until their removal callbacks finish. For shutdown, capture zero regions and dispatch removals before releasing factories/models; abandoning the tracker emits no implicit callbacks.

This explicit host seam batches group-add notifications with next-tick dispatch rather than invoking them synchronously from a Rust entity update. It provides portable net-state bookkeeping, not ownership of panel instances. Tests cover deferred initial activation, coalesced selection churn, ordering, hidden stored-active behavior, collapse/reopen, visible-only zoom delivery without retroactive notifications, cross-region moves, genuine removal without `false`, rejected-capture rollback, postcommit callback edits, reentry rejection and instance replacement.


### Hidden subtree geometry

Frozen `Panel::visible` specifies that an all-hidden container gives up its slot. `cui_dock_layout_visible` and `cui_dock_split_layout_visible` apply the same host predicate used by the styled adapter. Hidden leaves contribute no rectangle, minimum constraint or gap. A split with one surviving child assigns that child the whole parent bounds; if neither child survives, neither is rendered. Surviving split-node minimums still apply. A null predicate treats every panel as visible, but an empty tab group has no visible member and is omitted by these filtered APIs. The original unfiltered layout APIs retain their behavior.

Filtered leaf metadata counts visible panels and reports the displayed fallback key. The split query provides explicit first/second visibility flags, so zero-size allocated bounds are distinguishable from hidden children. Count queries and insufficient-capacity output follow the existing layout contract. The visibility callback must remain pure and deterministic throughout each query/frame.

The styled renderer omits hidden groups and their splitter semantics, cancels held splitters when a branch disappears, and uses surviving subtree minima. Open all-hidden workspace edges are omitted too. Ratios, node IDs, panel order, serialized layout and workspace open/preferred-size state remain unchanged; showing the panels restores their prior geometry. An entirely hidden root paints the workspace background. Center remains the host's workspace area when no edge is present.

Meaningful tests cover nested same/opposite axes, hidden minima/gaps, filtered metadata, all-hidden zero geometry, unchanged short output, equality with legacy geometry when every panel is visible, stored-ratio restoration, real held-pointer cancellation, omitted splitter semantics and open-edge omission/reappearance with unchanged preferred state.

### Per-panel simple-title palettes

Frozen `component/src/dock/panel.rs::TitleStyle` contains background and foreground colors. `tab_panel.rs::render_title` applies them to the Auto single-panel title; the tab-bar renderer does not read this override. CUI follows that scope with a borrowed optional `cui_dock_panel.title_style` pointer:

```c
cui_dock_title_style warning_title = {
    .background = {83, 47, 17, 255},
    .foreground = {255, 225, 177, 255},
};
registry[0].title_style = &warning_title;
options.panel_style = CUI_DOCK_PANEL_AUTO;
```

Keep the palette alive through the frame. Changing it between frames updates the header immediately without rebuilding the model or changing persisted data. A null pointer uses the theme. TabBar, multi-visible-panel groups, collapsed strips and a hidden title bar ignore it; content retains its ordinary palette. Disabled default titles retain the theme's muted ink. Explicit custom title/suffix callbacks submit their own colors because CUI scenes do not inherit Rust text styling. Ordinary close/menu controls retain their themed control palette. Tests inspect actual fill/text commands and check geometry, focus, disabled ink and all excluded presentation modes.

### Source-style root preservation

Frozen `layout/tree.rs::PaneTree::new` starts an empty horizontal split for both `RootKind::Any` and `RootKind::Split`. `normalize.rs::collapse_root` permits singleton promotion only for Any; `builder.rs::from_layout` wraps a tab root for Split. The new `cui_dock_create_with_root(config, kind)` follows those semantics using canonical storage. `cui_dock_root_policy` queries the retained policy. Existing `create` and `create_nary` continue starting with an empty tab root and unconstrained root policy for compatibility.

```c
cui_dock *center = cui_dock_create_with_root(
    (cui_dock_config){0}, CUI_DOCK_ROOT_SPLIT);
cui_dock_node root = cui_dock_root(center);
cui_dock_insert(center, root, 42, 0);
/* The root remains a split with one tab child. */
cui_dock_remove(center, 42);
/* The same root remains a split with zero children, ready for another panel. */
```

Check allocations/results. Root insertion into an empty canonical split creates a tab child; insertion into its sole tab child may use the root ID as a convenience. A multi-child split is ambiguous and requires the actual destination tab ID. In Any mode first insertion promotes the sole tab child, invalidating the former root ID. In Split mode closing/transferring the last panel preserves the empty split and closing one of two groups preserves a singleton root; ordinary nested singleton containers still collapse. Empty roots submit no title/content/splitter controls; singleton roots fill their viewport without a divider.

The policy belongs to the caller's tree configuration, as in the frozen source, and is not encoded as a new persistence field. V2 permits zero/one-child canonical roots but continues rejecting such nonroot nodes. Restore retains the destination's policy: Split wraps a nonempty tab root with an unknown-size horizontal child, converts an empty tab root to an empty split, and retains an existing split root. Any promotes a loaded canonical singleton root. A pinned root selects v2 editing/serialization even when restoring legacy v1 data; legacy binary ratios remain intact until subsequent edits. No pixel sizes are inferred. Capacity/depth failures while inserting or wrapping reject atomically and preserve the model. Successful restore continues invalidating node IDs, so refresh the saved root ID afterward.

Structural tests cover empty and singleton construction/layout, nested opposite-axis collapse, stable pinned roots after remove/transfer, duplicate-transfer rollback, v1 wrapping and binary-root removal, v2 empty round trips and capacity/depth rejection. Styled tests close both actual groups, verify divider disappearance and focus repair, then reopen content beneath the retained root.

### Typed frozen persistence schema

`cui_dock_schema_node` mirrors the exported `PanelState` / `PanelInfo` data contract from frozen `state.rs`:

| Frozen serialized field | Portable typed field |
| --- | --- |
| `panel_name` | borrowed `panel_name` |
| `children` | borrowed contiguous `children` and `child_count` |
| `info: {"stack":{"sizes":[...],"axis":...}}` | `SCHEMA_STACK`, `sizes`/`size_count`, `axis` |
| `info: {"tabs":{"active_index":...}}` | `SCHEMA_TABS`, `active_index` |
| `info: {"panel":<application JSON value>}` | `SCHEMA_PANEL`, opaque borrowed `payload` |

This is a typed import/export seam, not a JSON text parser. The host decodes/encodes these fields with its own JSON library, keeps payload bytes and strings alive, and handles the outer `DockAreaState` envelope. That envelope has optional `version`, required `center`, and omitted optional `left_dock`/`right_dock`/`bottom_dock`; each `DockState` has `panel`, `placement` (`center`, `left`, `right`, `bottom`), `size` and `open`. Apply typed conversion separately to each host-owned model, and retain edge state with the existing workspace snapshot. The CUI API does not atomically publish a four-region serde envelope.

`cui_dock_schema_import` stages the complete model and commits only on success. A pure key callback maps each source leaf to a stable application key. The source schema contains no panel IDs: use host identity data or precompute unique traversal keys, rather than assuming names are unique. Do not construct live panels or mutate models in the callback; rejected imports may have called it for some leaves. Duplicate keys, invalid descriptors, nonfinite/out-of-bound sizes, callback failure and node/panel/depth limits preserve the old model and IDs. Import selects canonical v2 editing, while ordinary v1/v2 restore stays available.

The converter follows frozen `state_convert.rs`: it wraps bare leaves in tabs, recursively flattens nested tabs, ignores the old `TabPanel` carrying a Panel variant defect, clamps an excessive active index, drops empty child containers, promotes nonroot singleton splits and normalizes same-axis stacks. Axis zero is horizontal and every other value is vertical. Short size vectors and nonpositive entries become unknown; excess entries do not create children. Values must be finite and bounded to CUI's 1e9 dimension limit. The caller's RootKind policy applies after construction. Source descriptor traversal is bounded by the configured node plus panel capacities and a maximum depth of 64, with model construction also respecting its configured depth; oversized input can reject before normalization would shrink it.

`cui_dock_schema_export` takes a pure dump callback equivalent to frozen `PanelSource::dump`, returning complete borrowed leaf states. This supports unknown plugins retaining their original names, payloads and child states. Hosts can use the existing factory `dump` result as the opaque payload and retain original unknown descriptor children. Output container names are `StackPanel` and `TabPanel`; active selection is unconditional even for an empty tab group. Known canonical slot sizes are emitted directly and unknown slots emit zero, matching the frozen writer's sentinel. Real known zero sizes also emit zero and import as unknown, an ambiguity of that source schema. Legacy binary ratios have no absolute pixel value, so their exported sizes are zero; retain v1 serialization or resolve measured canonical sizes when ratio-preserving persistence is required.

The existing panel factory records can supply ordinary leaf dumps without giving the model ownership of instances:

```c
typedef struct {
    const cui_dock_panel_instance *instances;
    size_t count;
} schema_host;
static bool dump_leaf(void *user, uint64_t key, cui_dock_schema_node *out) {
    const schema_host *host = user;
    for (size_t i = 0; i < host->count; ++i) {
        if (host->instances[i].state.key != key)
            continue;
        cui_dock_panel_state state;
        if (cui_dock_panel_dump(&host->instances[i], &state) != CUI_DOCK_OK)
            return false;
        *out = (cui_dock_schema_node){.panel_name = state.name,
                                     .payload = state.payload,
                                     .kind = CUI_DOCK_SCHEMA_PANEL};
        return true;
    }
    return false;
}
```

A host retaining an unknown full leaf descriptor can instead return that descriptor unchanged, including its borrowed children. The JSON adapter interprets the opaque payload as the Panel variant's value; the base converter does not validate application JSON.

Query required capacities with null `nodes`; provide node and double buffers to export. The root is `nodes[0]`, and container child/size pointers point inside those buffers. Callback-supplied leaf strings, payload and descendants remain borrowed. Failed callback/validation leaves counts and buffers untouched; insufficient capacity reports required counts and leaves both buffers untouched. Do not alias buffers with live descriptor data that callbacks read, and keep all borrowed leaf data alive while using the exported tree.

Tests transcribe all 16 center story payloads and exact two-tab `[704,263]` vertical geometry from the shipped `fixtures/layout.json`, check typed round trips, query/short-buffer rejection, callback rejection, duplicate IDs, malformed descriptors, capacity failure, nested tabs, unknown children, size sentinels and nonzero-axis compatibility. Styled tests import a selected panel, exercise its real tab/content callbacks, export changed active selection and reimport it. This covers tree conversion; full source JSON parsing and application version migration remain host obligations.
