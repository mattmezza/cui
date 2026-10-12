# Progress

`cui_progress` paints an application-owned fraction, clamped to 0–1. It is an immediate component: keep the operation state in your application and submit the current value each frame.

Use `cui_progress_ex` for an indeterminate operation whose completion fraction is unknown:

```c
bool moving = cui_progress_ex(ui, progress_id, &theme, fraction, 300,
    (cui_progress_options){
        .name = cui_string("Uploading files"),
        .indeterminate = uploading && !total_known,
        .now_seconds = monotonic_seconds,
        .reduced_motion = user_prefers_reduced_motion,
        .height = 8});
```

The segment moves back and forth over 1.6 seconds. Supply a monotonic host clock; the component requests redraw while moving and returns `true`. The host must service that request through its normal frame loop. Both the option and the context's reduced-motion preference stop the animation and display a centered segment. The default determinate helper keeps its previous dimensions and painting behavior.

The component exposes a progressbar semantic with a 0–1 range. Indeterminate progress marks it busy; adapters should omit the numeric current value when the total is unknown. Progress has no activation callback: completion and cancellation belong to the operation's model and surrounding controls.

The website's Progress mode selector demonstrates determinate and indeterminate forms. Changing the operating system's reduced-motion preference updates the browser demonstration immediately.
