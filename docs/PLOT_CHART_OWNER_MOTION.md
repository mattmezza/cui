# Owner Plot and Chart motion

`cui_plot_motion_from_theme` projects a standalone `cui_theme_motion_tokens` value into a copied `cui_plot_motion_policy`. The source projection uses the fast duration for enter/exit and pointer response, the respective enter/exit Bezier curves, source spring damping 1/travel with epsilon 0.1, and a distinct one-second quartic-out appearance. Move/control springs, move easing and rem distances do not drive Plot motion. This matches pinned 0.7.1 `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/theme/mod.rs::plot_motion`.

```c
cui_theme_motion_tokens tokens = cui_theme_motion_default();
cui_plot_motion_policy motion;
if (cui_plot_motion_from_theme(&tokens, &motion) == CUI_PLOT_OK) {
    chart_options.custom_motion = true;
    chart_options.motion = motion;
    plot_options.custom_motion = true;
    plot_options.motion = motion;
}
```

Policies contain values, including Bezier control points; they do not borrow Theme storage or contain self-referential callback userdata. Edit a copied policy for one owner and keep each owner's Chart/Plot state separate. Zero/default `custom_motion=false` preserves the existing Chart one-second appearance/120ms hover and Base Plot explicit zero-duration policies. `cui_chart_theme` continues to project colors only; it does not silently install motion.

PlotElement opt-in replaces its legacy enter/exit/appear policies. Appearance still requires its existing explicit identity/generation/frame/scope contract. Chart opt-in consumes enter/exit, appearance and pointer policy internally for line/area, bar/candlestick and pie channels. Positive legacy Chart `appear_duration` overrides policy appearance duration; positive `hover_duration` overrides enter/exit durations and pointer response. Zero legacy durations leave the explicit policy unchanged, so explicit policy duration 0/response 0 means instant. `disable_appear` and owner/Core reduced motion win. Manual reveal remains caller supplied. Changing a policy retimes the existing appearance; changing `appear_key` explicitly restarts it. Same-frame explicit render followed by a changed policy is re-evaluated rather than reusing stale motion.

Custom marks decide which datum coordinates to move. In a Plot marks callback, call `cui_plot_motion_follow` with the owner, a zero-initialized caller-owned pointer state, `options.motion.pointer`, the target coordinate and `hover->entering`. Use the returned coordinate when painting. This consumes Core time/reduced motion, requests redraw while moving and resets when the owner changes. Use independent states per coordinate/channel. The helper invalidates Core while moving; the host advances frames using `cui_needs_redraw`, including requests made by marks callbacks. It does not infer arbitrary host mark positions or replace the existing live-pointer/current-bounds host seam.

The low-level transition adapter borrows the explicitly supplied transition **only for synchronous stepping**. Do not retain an adapter. Opt-in Plot `on_appear` receives a sampler valid only during that callback. Published Plot state retains time/progress/flags and clears that temporary sampler's pointers. For later stagger sampling, use `cui_plot_motion_appear_stagger(&copied_policy.appear, &copied_state.appearance, ...)`; both values may safely be copied to other addresses. Legacy caller-supplied borrowed samplers retain their prior lifetime contract.

Validation rejects invalid enums, nonfinite/out-of-range timing and spring parameters before opt-in widget mutation. Factory, sample, stagger and follow failures preserve their outputs/state. Timing/delay is bounded to 3600 seconds in magnitude; response to 3600, damping to 100, epsilon to 65536. Negative delays are supported. Bezier overshoot is sampled as supplied; Chart reveal/hover and Plot hover opacity clamp to 0..1 for bounded rendering. Undamped springs may keep moving; positive damping settles. Appearance sampling does not rewrite caller data. Host data/callback/bitmap lifetimes remain unchanged.

The new native regression demonstrates two owners with different appearance/fade/pointer policies, actual Plot mark composition, line/bar/pie channel independence and settling, reversal, explicit instant/reduced overrides, legacy duration precedence, same-frame policy changes, invalid rollback and copied-state later sampling after original storage is overwritten. The standalone WASM browser probe paints real Chart bitmaps and Plot marks, drives actual mouse input and proves different reveal/pointer motion plus reduced completion. It owns fixed host bounds. No automatic current-frame prepaint/occlusion, OS long-press or native screen-reader parity is claimed.

## Integrator registration

Register `src/components/cui_plot_motion.c` in the styled library and web source lists; add `tests/unit/test_plot_chart_owner_motion.c` as a strict target linked to `cui`. In `scripts/amalgamate.py`, register `cui_plot_motion` with this component source and include it as an implementation dependency of Chart/PlotView and the aggregate Components header, alongside existing Plot and Theme dependencies. Regenerate distribution headers. Standalone probe `tests/web/test_plot_chart_owner_motion.py` builds its own scoped WASM fixture and requires pinned Zig 0.16.0/Playwright 1.58.0. No CMake/web/distribution registration was edited by this worker.

Validation on base `137e1c5`: strict native build passed without warnings; CTest 112/113 passed, with the sole distribution check reporting stale PlotView/Chart headers (root regeneration). The new behavior target was registered locally using an external CMake include. ASAN/UBSAN with leak detection and GCC fake-stack/use-after-return mode passed; Clang was unavailable. Standalone pinned Zig/WASM Chromium1208 canvas/mouse proof passed. Logs are `/home/matteo/.cache/cui-validation/owner-motion-next23-{handoff-build,commit-ctest,commit-asan,opacity-browser}.log`.
