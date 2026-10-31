# Control surface shadow gallery

On Button, Checkbox, Radio, Switch and Slider routes, **Surface shadows** enables
an additive example. It starts off on each route; turning it off uses the existing
legacy call. Button collection mode retains its existing route and disables this
standalone demonstration. Component size, design scale, disabled state, alignment
and existing event handlers remain available.

Button's Run action uses `cui_button_surface_ex`: normal DROP, a larger hovered
DROP, pressed INSET and an explicit empty list while disabled. The pure callback
checks `info.disabled` even in its instance phase: the captured instance patch
replays after disabled, so clearing only in a disabled callback would be wrong.
Activation still uses the existing `on_click` exactly once. No shadow is implicit
when the feature is off.

Checkbox and Radio use explicit `cui_selection_indicator_parts` with their
`*_with_parts` facades. Their decoration follows the selected value; it targets
the indicator, not the label row or glyph. Switch uses the public track/thumb
IDs; thumb DROP remains clipped by the track. Slider uses its public legacy
track ID and an INSET layer; it does not acquire a named thumb or fill. Indicator
radii follow current size/design metrics and actual shape; shadow radii are
clamped by Core to the current caster bounds. Identity is not a presence check:
submit first, then check the current-frame setter before end_frame.

The copyable snippet contains real public-header C functions and follows the
selected size/design tokens. Application state remains caller-owned. The examples
use built-in control artwork; they introduce no text glyph icons. These C
callbacks and named-part seams do not imply corresponding language-builder
callback support. All78 component/portal/glyph/binding shadow coverage remains
open beyond the explicitly implemented and tested slices.

Implementation and the focused fixture are staged for the integrator's combined
WASM build. **No actual browser proof is claimed at this source checkpoint.**
After that build, run the existing Playwright environment headlessly:

```sh
nice -n 19 taskset -c 0 python tests/web/test_control_surface_shadow_gallery.py \
  --directory /home/matteo/.cache/cui-current-validation/web \
  --report /home/matteo/.cache/cui-current-validation/control-surface-shadow-gallery.json
```

The fixture compares actual canvas pixels with the legacy route at unchanged
bounds, checks Button hover/press/disabled policy, performs real component
clicks/Slider key input, checks values and design sizes, and restores the off
route. Wide/DPR1 and narrow/DPR2 cases are included. `--compile-usage` additionally
syntax-checks copied snippets with strict C11; it requires the integrator to
allocate a compiler slot. GPU/desktop tests are unnecessary and are not proof
provided by this fixture.
