# Current-frame Chart and Plot paint

This scoped contract uses Core prepaint rather than retained geometry. It addresses
GPUI 0.7.1 PlotElement's layout/prepaint split and visibility gate at pinned commit
`87d10ae5e1299d1be18670a37570c5402f952e34`; it does not establish full Chart/Plot parity.

Set `interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME` on a styled Chart.
Core resolves bounds, clipping, pointer position, passive barriers and modal scope
at `cui_end_frame`. The Chart resolves its datum against that snapshot, steps its
appearance/hover/pointer motion, paints the bitmap and local decorations, then
paints the default tooltip in a deferred stream. A stationary pointer therefore
uses the new geometry after scroll, resize or layout movement. Unknown pointers
supply no live datum. Source appearance gates by linear elapsed time even when
the quartic float paint progress has already rounded to one. Exit presentation is
retained until its transition settles; reduced/instant motion completes directly.

`AUTO` and `LEGACY` preserve existing Chart input behavior, including applications
that call `cui_chart_update_hover` with their own coordinates. There is no existing
option that unambiguously distinguishes those applications from host pointer
routing, so AUTO does not guess from whether Core knows a pointer. Host-driven
gallery/application stories should opt into CURRENT_FRAME explicitly. Layout
`tooltip_content` and `tooltip_measure` are supported on the legacy path;
CURRENT_FRAME rejects them before registering paint. Pure title, label, value,
color and number formatters remain supported during paint and must not submit UI.
Static axes/legends and custom Radar/Sankey label layout remain composition work.

For PlotElement, CURRENT_FRAME ignores its legacy bounds/pointer/hover inputs and
uses Core. New `paint_marks` and `paint_tooltip` callbacks receive a const snapshot,
a callback-local writer, copied theme values, retained hover values and borrowed
user data. They return false to discard their emitted stream. Emit marks with
`deferred=false` and tooltip commands with `deferred=true`; the latter clip to the
viewport rather than the plot. Apply `hover->progress` to custom paint opacity.
The writer also applies ancestor opacity. `paint_command_budget` bounds the combined
custom commands (1..4096 with callbacks); the component reserves bitmap and clip
overhead separately. CURRENT_FRAME rejects the old layout marks/children/tooltip
callbacks and touch input. Existing explicit geometry, touch/long-press state and
layout callbacks keep their legacy behavior under AUTO/LEGACY. AUTO selects Core
only for new paint callbacks with zero host bounds and no legacy callbacks.
Primary activation retains the view's existing release routing and uses the last
successfully painted bounds; this is not a new per-mark hit or gesture system.

Use the pure `cui_plot_motion_follow_at` with `info->frame_time` to step caller-owned
mark springs. It preserves state/output on invalid input and reports whether
another frame is needed. A paint callback requests that frame with
`cui_prepaint_request_frame(writer)`. It must not call layout, semantics, focus,
input mutation or `cui_invalidate`. The existing UI motion helper wraps the same
pure step and retains its composition-time invalidation behavior. Motion policy
projection and legacy zero/Chart defaults are unchanged. Reduced motion is copied
from Core at composition; set it before composing the owner.

Driver state, data arrays, pixels, callback user data and pointer-containing options
must stay live through end-frame and renderer consumption. Chart copies its name
into owner storage before deferred work. Writer text commands copy text; images
borrow pixel storage through consumption. The component prepares a local state and
publishes it only after successful emission. Callback side effects and mutations
to borrowed pixel buffers are outside draw-output rollback. Old callback borrowing
contracts remain unchanged. No OS long-press, native screen-reader certification,
GPU mesh equivalence or implicit pan/zoom is established by this slice.

## Validation and integration

`tests/unit/test_plot_chart_prepaint.c` covers current geometry with stationary
pointer and a routed wheel, unknown pointer, passive inert overlay/removal, modal
suppression, manual input compatibility, source motion rounding/completion and
quiescence, Bar/Pie/Candlestick/Radar hover paint, pure invalid-step atomicity and
custom budget exhaustion preserving driver state. Existing Chart tests check the
shared legacy motion/update path.

`tests/web/test_plot_chart_prepaint.py` compiles the actual pinned-Zig WASM sources
with a standalone probe. It uses real mouse/wheel/resize events, frame/phase signals
and actual Chromium canvas pixels for Chart geometry/occlusion and Plot local plus
deferred paint. It uses no fixed wall-clock sleeps. This proves that fixture, not
an exhaustive platform matrix or all gallery stories.

Integrator: register the new unit executable/test linked to `cui`; regenerate
Chart/PlotView/Motion distribution headers after merging. There are no new production
source modules. Browser invocation:

```
TMPDIR=/home/matteo/.cache/cui-validation/tmp-storage uv run --script tests/web/test_plot_chart_prepaint.py --chromium /home/matteo/.cache/ms-playwright/chromium_headless_shell-1208/chrome-headless-shell-linux64/chrome-headless-shell
```
