# Styled Dock

The Dock view composes the portable Dock model, Tabs, Buttons and Resizable into a themed workspace. Each leaf shows an overflow-scrolling tab strip and calls only its active panel's content renderer. Nested horizontal and vertical splits use pointer and keyboard resize handles; changed ratios are written back to the model and therefore included in its serialization.

This implementation was checked against the frozen GPUI Kit 0.7.1 reference, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, including `crates/component/src/dock/dock.rs`, `tab_panel.rs` and `website/component/dock.md`. It implements a specific subset, not full Dock family parity. Edge docks, floating windows, zoom/maximize and panel toolbars/menus are not implemented. Programmatic `cui_dock_move` and `cui_dock_split` remain available. No platform or Clay types appear in the public API.

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

## Validation

The headless styled test exercises active-only callbacks, stable keys including zero/`UINT64_MAX`, disabled controls, keyboard focus/navigation, close shortcuts, collapse, same-group pointer reorder, nested pointer/keyboard resize, infeasible minima, restore/reset and capacity rejection. The model unit test checks split geometry against leaf layout and transactional validation failures. Cross-group and all four edge drops are tested with queued pointer events, including cancellation, outside drops, disabled/reorder-off and full-capacity rejection. Draw-command tests verify accepted/rejected custom preview palettes, preview suppression, motion settings and cancellation cleanup. Host veto tests cover close buttons/shortcuts, reorder, move and split. No native window or browser-specific behavior was tested for this adapter.

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

The frozen panel presentation trait exposes `toolbar_buttons` and `zoom_control` separately from base capabilities such as closable/zoomable. A next portable increment can add an active-panel toolbar submission callback and a retained maximized-panel key, preserving the ordinary split model while showing only that leaf. Restore must remain available even if a host later revokes zoom permission, matching the reference's safeguard against trapping a zoomed panel. Headless tests should cover focus restoration, model edits/removal while zoomed and toolbar ownership.

The reference's edge docks are independently openable/resizable left, right and bottom regions around the center Dock area. They need explicit retained region models, visibility/size state and bounded minimum-size allocation, rather than interpreting the current center split tree as edge-dock parity. This can follow toolbar/zoom with tests for all regions, toggles, constrained layouts and persistence. Native floating windows require a host window-management contract and are not implied by portable layout/render support.
