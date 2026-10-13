# Checkbox motion, labels and tooltip

`cui_checkbox_ex` retains its immediate value/semantic update and single activation result. Optional caller-owned `cui_checkbox_state` adds reversible checked/mixed spring marks and a tooltip. A NULL state keeps the existing immediate mark behavior. A zero-initialized state snaps to the initial value, so mounting a checked control does not animate from unchecked.

```c
static cui_checkbox_state checkbox;
static cui_check_state checked;
void preferences(cui_context *ui, const cui_theme *theme, cui_rect viewport) {
    bool changed = cui_checkbox_ex(ui, 42, theme, cui_string("Receive updates"), &checked,
        (cui_checkbox_options){.state = &checkbox, .width = 240,
            .tooltip = {"Email notification preferences", 30},
            .tooltip_options = {.viewport = viewport, .open_delay = .2f, .close_delay = .1f}});
    if (changed) { /* Persist checked or invoke the application's callback once. */ }
}
```

Keep one state per stable Checkbox ID and retain it through `cui_end_frame`. State is bounded, allocates no memory and needs no cleanup; zero it to reset. Reusing it for another ID resets motion/tooltip. The two mark channels use existing disclosure spring integration with `theme.spring_control.response`, `damping`, `epsilon` and `travel`. They crossfade when checked becomes mixed, reverse from their current progress/velocity, and stop requesting frames when settled. Painted alpha is clamped even for underdamped overshoot. Response zero, travel false, Core reduced motion or `options.reduced_motion` snaps immediately. Logical selection never waits for paint.

`state.next_wake_seconds` is an absolute Core frame deadline while motion or a delayed tooltip is active; zero means quiescent. The component also invalidates Core. Hosts should honor deadlines together with normal input redraws. Nonfinite/exceptionally large Core clocks do not produce an invalid deadline. Disabled controls refuse activation; existing press cancellation and keyboard behavior remain in the Base reducer.

A positive `options.width` bounds a fit-height label column while keeping the indicator's width. Text wraps naturally; the existing `content` callback adds arbitrary rich child content below the label. Give custom content intrinsic height or an intentional constraint. The outer width has an indicator/gap minimum. Parent viewport/explicit child constraints can still clip; a word wider than its column needs enough width or a scrollable ancestor. No automatic ellipsis or arbitrary text word-breaking is added.

Tooltip text requires state and a valid `tooltip_options.viewport`. The component supplies the Checkbox trigger, frame delta and disabled flag, and delegates placement/delay/focus/Escape lifecycle to the existing tooltip. A targeted capture cancellation suppresses the tooltip until hover/focus leaves. Removing tooltip text clears its state. Touch activation focuses the same control, so focus can reveal help without changing the tap contract. Tooltip dimensions retain the existing explicit clipping contract; choose dimensions for the supplied text.

The appended state/width/tooltip/reduced-motion fields are Checkbox-only. `cui_radio_options` remains a compatible alias but does not implement these fields. Other Checkbox semantic flags and accessibility labels retain their previous meaning. This work does not certify complete Checkbox source/platform parity.

Validation uses a standalone strict C11 test for intermediate mark paint, immediate semantics, mixed reversal, two damping regimes, theme snap, reduced motion, quiescence, wrapped/custom-content geometry across all 14 design sizes, keyboard/disabled activation and touch cancellation. The browser fixture uses actual CDP contacts, actual WASM mark alpha, focus tooltip/Escape/Space, all design sizes/densities and a 390px wrapped label. Chromium is not physical mobile-device coverage.

Proof: warning-enabled native CTest 97/98 passes (only root-owned generated distribution freshness is stale); the new standalone test passes with `-std=c11 -Wall -Wextra -Wpedantic -Werror`. Its old-library comparison fails the actual partial-mark assertion. The new test passes ASAN/UBSAN with leak detection, and existing components/content/disclosure sanitizer tests pass 3/3. Actual Chromium headless-shell 1228 passes 34/34 strict cases. The documentation example compiles under strict C11. No family-wide or physical-device certification is implied.

Run the standalone test with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/unit/test_checkbox_motion.c build/libcui.a -lm -o build/test_checkbox_motion && build/test_checkbox_motion`. Run the browser proof with `uv run tests/web/test_checkbox_motion.py --chromium /path/to/chrome-headless-shell`.
