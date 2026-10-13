# Loading motion contracts

The pinned GPUI Kit 0.7.1 source at
`87d10ae5e1299d1be18670a37570c5402f952e34` defines Spinner in
`crates/component/src/spinner.rs`, Skeleton in `skeleton.rs`, and linear Progress
in `progress/progress.rs`. Its GPUI dependency is exactly `gpui-pre=0.3.8`;
`src/elements/animation.rs` defines quadratic `ease_in_out` and the forward/reverse
`bounce` used here.

Spinner defaults to a medium 16-pixel Loader in foreground color, with a
repeating .8-second quadratic rotation. Its source
public customization is icon, color, easing and size; speed is private. Skeleton
is a decorative styled rectangle using the theme skeleton color (secondary by
fallback), a two-second bounced quadratic opacity from 1 to .5, and an optional
secondary half-opacity. It has no loading boolean, busy/status role, accessible
name, completion event, or focus stop. Adding those semantics to the placeholder
would misrepresent the source. Associate loading status with the actual operation
in the application's semantic tree.

CUI's legacy `cui_spinner` dot-ring and `cui_skeleton` cosine/phase APIs retain
existing dimensions, semantics and invalidation. The opt-in `cui_spinner_ex`
and `cui_skeleton_ex` paint the source-style Loader and bounced placeholder.
Neither adds semantics. CUI has no dedicated skeleton theme token, so the new
placeholder uses `theme.secondary`; the secondary option halves alpha again.
Progress's existing determinate cubic-bezier transition and one-second
indeterminate quadratic segment are unchanged. Reduced-motion indeterminate
Progress retains its centered 35% segment and busy progressbar semantics.

## Ownership and scheduling

Use one zero-initialized caller-owned `cui_loading_motion` per independent
animation; it has no allocations. Zeroing it explicitly restarts the timeline.
The first successful sample starts at the supplied clock value. Subsequent
backward time clamps to the last sample. Time must be finite and in [0,1e12];
period must be finite and in (0,60]. For the component options, duration zero
selects the source .8/2-second period; negative, nonfinite and over-60 durations
reject atomically. Changing duration applies the new period to the same origin.

The component returns `active` and an absolute `next_wake_seconds` in the
**supplied clock's domain**. Moving primitives return the next 60 Hz sample;
reduced motion returns phase zero, full base opacity, false activity and deadline
zero. Context reduced motion also overrides the option. Custom easing is not
called while reduced. These APIs do not request perpetual Core invalidation.
Schedule the returned deadline and paint again then, as the gallery does:

```c
options.now_seconds = cui_frame_time(ui);
if (cui_spinner_ex(ui, id, &theme, &motion, options, &result))
    host_deadline = result.next_wake_seconds > 0
        ? host_now + fmax(0, result.next_wake_seconds - cui_frame_time(ui)) : 0;
```

If supplying host time directly, use the result directly. Repeating loading has
no inherent completion; stop submitting it when the operation stops. Activity
means submitted motion is enabled, not that pixels are visible: clipping and
parent visibility do not automatically stop caller scheduling. Hosts may omit
hidden primitives and restart/reset state as appropriate.

Custom easing receives normalized phase (Skeleton first applies its triangular
bounce). It must return finite [0,1]; invalid output preserves state/result and
paints nothing. The custom Spinner paint callback receives color, size and
center rotation in radians; its userdata is borrowed for the synchronous call
and never retained. The default uses the existing bounded context-owned SVG icon
cache, with draw-list bitmap lifetime unchanged. A paint callback returning false
must leave its own paint unchanged; CUI cannot roll back arbitrary user callback
side effects. State/result commit only after success. General Core frame-capacity
errors still follow Core's existing frame error contract.

## Validation

`tests/unit/test_status_motion.c` checks clock start/reset/rewind, default and
invalid durations, callback rejection, finite wakes, reduced quiescence, no
invented Skeleton semantics, custom paint pixels, default SVG/software pixels,
and unchanged Progress transition/indeterminate behavior. It links `cui`,
`cui_software` and native text dependencies. Root owns CMake registration.

`tests/web/test_status_motion.py` runs real WASM in Playwright 1.58.0 with pinned
Chromium 1208 by default (`--chromium` may select 1228). It checks animated frame
progress with finite gallery wake deadlines and no perpetual redraw, actual
canvas/mask pixels, reduced-motion frame and pixel stability, decorative
semantics, and existing Progress busy/reduced-motion behavior. Distribution
regeneration is integrator-owned; stale distribution checks are expected until
that step.
