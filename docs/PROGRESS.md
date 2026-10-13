# Progress

Progress uses a controlled normalized fraction (`0..1`). Submit it each frame;
its numeric semantic value is the target fraction even while the visible bar
animates. Indeterminate progress is busy and has no numeric value in the browser
accessibility bridge. It never adds a Tab stop.

```c
#include <cui/cui_components.h>

void draw_download(cui_context *ui, const cui_theme *theme,
                   cui_progress_motion *motion, float fraction, double now) {
    cui_progress_options options = cui_progress_defaults();
    options.name = cui_string("Downloading assets");
    options.size = CUI_CONTROL_SMALL;
    options.motion = motion;
    options.now_seconds = now;
    cui_progress_ex(ui, cui_id_from(ui, "download"), theme,
                     fraction, 240, options);
}
```

Keep a zero-initialized `cui_progress_motion` in application state and supply a
monotonic host clock in seconds. The first submission displays the value
immediately. Later changes animate over 180ms by default, and reversing a target
continues from the currently displayed value. Set `duration` to customize it;
zero selects the default. `reduced_motion`, or context reduced motion, snaps to
the target. The return value reports whether another animation frame is needed;
CUI also invalidates the context while moving.

For a loading bar, set `indeterminate = true`. The defaults use the frozen
one-second cycle: the segment grows from the left, then contracts toward the
right using quadratic ease-in-out. Reduced motion produces a static centered
35% segment. Invalid or negative clock values also select that static segment.
A zero-initialized options struct preserves the earlier CUI 1.6-second,
fixed-width 30% loading cycle; explicitly set `source_motion = true` or use the
constructor to select the source-style cycle.

Extra-small/small/medium/large heights are 4/6/8/10px; positive `height` overrides
them. The bar radius is half its height, except that a square theme remains
square. `custom_corners` and `corners` refine each corner on both track and fill.
`custom_color` and `color` override the solid fill, including transparent color;
the track uses 20% of its alpha. Zero/negative/nonfinite width selects 160px.
Nonfinite fractions become zero; finite fractions clamp to `0..1`. CUI uses
fractions rather than the reference's `0..100` percentage API.

## Base composition

`cui_progress_measure` is allocation-free, independent geometry in normalized
track units. It returns the clamped fraction, segment start/length and whether
loading needs another frame. An application can scale the geometry to pixels and
paint its own track, indicator or arbitrary child content with Core boxes.

Inside the root box, call `cui_base_progress` with the same ID to register
progress semantics without painting. The styled adapter uses this same base
behavior. There is no independent interaction reducer in the styling layer.

`cui_progress_animate` is a caller-owned deterministic value transition, also
usable by custom styling. Its duration is explicit, with zero meaning instant;
invalid arguments preserve the state and output. A regressing clock is held at
the last observed time. The move easing is cubic-bezier `(0.2,0,0,1)`; arbitrary
easing/theme-motion-token overrides remain a separate API gap.

The loading formula and sizes follow the [frozen Progress source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/progress/progress.rs).
The quadratic easing was checked against its pinned `gpui-pre 0.3.8` dependency
(crate SHA-256 `d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360`).
No engine or runtime dependency is added. Base and styled tests cover geometry,
cycle boundaries, reduced motion, colors/sizes, reversal continuity, clock
regression, invalid-state transactionality and controlled semantics.
