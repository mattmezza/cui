# Tabs scroll ownership and boundary handoff

Scrollable Tabs now consume each wheel or touch-converted SCROLL event when their bounded offset actually changes. A refused event is offered to an eligible ancestor through the existing `cui_defer_scroll_event` API. Public options, zero-initialized behavior and selection callbacks are unchanged.

This matters to browser page fallback: before the fix, a vertical touch move changed the Tabs offset by 30 pixels but left the event unconsumed. The existing `web/touch_page_host.h` policy consequently made the page eligible and the browser scrolled the page by the same 30 pixels. A parent following Core targets did not automatically double-scroll; the observed leak was the host page fallback. Wheel events had the same missing consumption signal, although the production gallery's wheel preflight can prevent page scrolling independently of that signal.

A second case occurs when several events share one frame. Core initially routes them using the previous layout snapshot. If the first reaches the Tabs edge, the second now defers to the parent rather than disappearing with the retained child target. An event that moves the child partially is consumed entirely, matching shared Scrollable behavior; this change does not recover its residual delta. Boundary handoff therefore happens on a later refused event. No inertia or momentum behavior is added.

The source change is restricted to the Tabs scroll loop in `src/base/cui_choice.c`. Core pointer presence, capture, modal routing, native adapters, browser production handlers, DataTable and shared scrollbar visibility/timing are unchanged.

## Regression evidence

`tests/unit/test_choice.c` adds an actual styled Tabs scene inside a shared Scrollable. Both orientations and precise/line deltas assert child movement and consumption, no parent movement for accepted deltas, ignored already-consumed input, disabled child routing, and two events in one frame with the second moving the parent exactly once. The accepted-event assertion fails on the original source; the failure is preserved in `/home/matteo/.cache/cui-validation/tabs-before.log`.

`tests/web/tabs_scroll_probe.c` composes public styled Tabs and Scrollable APIs and the unchanged page-host policy. `test_tabs_scroll_chaining.py` builds this source as strict C11 WASM with pinned Zig 0.16.0, then drives actual Chromium mouse-wheel/Shift-wheel and CDP touch contacts. It asserts current first-tab bounds and changed canvas pixels, child-to-parent boundary handoff, page remaining stationary, disabled wheel/touch behavior, cancellation without selection, and one selection from the next real touch tap. No test invokes a model mutation or reducer export to substitute for user input. Reset is fixture initialization between independent cases.

Before-fix CDP evidence is preserved in `tabs-before-touch.log`: Tabs offset 30, parent offset 0, consumption 0, page scrollY 30, one fallback. After-fix tests report PASS in `tabs-final-browser.log`; the screenshot is `tabs-final-browser/tabs-scroll.png`, under `/home/matteo/.cache/cui-validation/`. This is a controlled real browser fixture, not a claim that every gallery mode or physical device was tested. The Canvas2D fixture draws ordinary Tabs rectangles, borders and text; it is not a general CUI renderer conformance test.

Run the browser proof from the repository:

```sh
TMPDIR=/home/matteo/.cache/cui-validation/tmp-storage/tabs \
  uv run tests/web/test_tabs_scroll_chaining.py \
  --output /home/matteo/.cache/cui-validation/tabs-final-browser
```

Playwright is pinned to 1.58.0 and defaults to its installed Chromium 1208 executable. `--case touch` isolates the touch diagnostic; default execution includes the full suite. The browser fixture uses the same Shift-plus-vertical-wheel translation as the gallery.

The warning-enabled native build completed without warnings. Full CTest passed 119/120; only `distribution` failed because generated distribution sources are integrator-owned and stale after this source edit. Focused `test_choice` passed with AddressSanitizer, UndefinedBehaviorSanitizer and leak detection. Logs are `tabs-native-build.log`, `tabs-native-ctest.log`, and `tabs-asan.log` in the validation directory. No physical OS touch, native window-leave or full Tabs/scrollbar parity certification is inferred from these tests.

## Native SDL_Renderer Shift-wheel

The SDL3 adapter routes a vertical-only wheel delta horizontally while its
window has observed Shift held. It uses keyboard snapshots in dispatch order
and clears the retained Shift state when the window loses focus. SDL supplies
no modifier snapshot with a wheel event. If Shift is already held when the
window gains focus, remapping starts after the adapter receives a key event
reporting Shift. The adapter preserves simultaneous axes, fractional ticks,
and the signs supplied by SDL, including natural scrolling.

The Shift flag is appended after the SDL V2 guards. Compile-only Linux x86_64
and WASM32 probes preserve the 37 original field offsets, guard offsets,
alignment and constructor allocation size of the
[measured guard layout](validation/sdl-v2-guard-layout.json).

`tests/integration/test_sdl_tabs_wheel.c` passes window-addressed SDL events
through the wait/poll path on private X11. It verifies that Shift-wheel moves
the first tab by 40 units and consumes the delta, that release in the same
batch leaves the following wheel vertical, that diagonal input preserves both
axes, and that loss of focus clears Shift. An ordinary wheel still moves a
bounded vertical strip. The original adapter failed the 40-unit assertion.
Physical wheel/trackpad, Cocoa and Wayland input remain unverified by this test.
