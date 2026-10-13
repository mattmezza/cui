# Button pointer activation barriers

Disabled and loading Buttons now prevent a pointer press on their visible area
from activating an underlying parent. This applies to `cui_base_button` when
disabled and to styled `cui_button` when disabled or loading. Existing callback,
keyboard and controlled state contracts are unchanged.

`cui_semantic_details.blocks_pointer_activation` is an append-only, opt-in Core
flag. Zero preserves the previous behavior of other controls. The previous-frame
hit graph considers a flagged element even when it is disabled, using the same
clip, modal, layer and submission-order rules as other pointer targets. A topmost
barrier consumes primary DOWN without moving focus or assigning capture. Primary
UP over a barrier cannot create an activation, including an activation of a
previously captured ancestor. Existing capture still receives motion and UP for
cleanup; CANCEL still clears capture. A later unobstructed press works normally.

This flag blocks pointer activation and pointer focus only. It does not disable
Tab, programmatic focus or keyboard activation on an otherwise enabled element.
It does not consume wheel events or prevent touch panning an actual scroll owner.
It also does not clear an existing text composition when the blocked press occurs.
Hover, cursor and current-frame prepaint occlusion are independent: callers that
need a passive hover occluder must use the existing hover/prepaint participation
contract. The flag alone is not a general visual or hover occlusion policy.

The focused regressions are `test_pointer_activation_blocker.c` (Core-only) and
`test_button_activation_blocker.c` (real Base and styled Buttons and Scrollable).
They cover legacy zero behavior, disabled/loading mouse and touch, focus retention,
release activation counts, capture cleanup, cancellation, subsequent clicks,
keyboard activation, clipping, later siblings, elevated layers, modal eligibility,
composition retention, hover independence and ancestor wheel/touch scrolling.

`tests/web/test_button_activation_blocker.py` independently builds its real CUI
WASM fixture and drives Chromium mouse, keyboard, wheel and CDP touch contacts.
The unfixed frozen source failed both disabled mouse and touch assertions with
parent count 1 and parent focus; the corrected source preserves the textbox focus
and leaves both callback counts at zero. The browser fixture also checks enabled
clicks, retained capture, cancellation and scrolling without page fallback. Run it
with pinned Playwright 1.58.0 and Chromium 1208; its `--source-root` option permits
the same regression against an earlier source checkout.

This is a scoped correctness fix, not certification of the complete upstream
Button family, every host, accessibility platform or physical touch device. The
separate Button completion audit still records remaining API and evidence gaps.

Worker validation on the frozen `6e36b5a` base: warning-enabled native build had
no warnings; existing CTest passed 121/122, with only the expected generated
distribution mismatch pending integrator regeneration. Both new standalone tests
passed `-std=c11 -Wall -Wextra -Wpedantic -Werror`, and passed ASan/UBSan with
leak detection. The existing Button behavior test also passed the sanitizer run.
The independent strict WASM build and full Chromium 1208 suite passed after the
fix. These results do not substitute for the integrator's generated distribution
and platform CI checks.
