# Spinner

Use `cui_spinner_ex` for the frozen source-style decorative loading icon. It
paints the Loader SVG at 16 logical pixels in `theme.text` by default, rotating
through one turn every 0.8 seconds with quadratic ease in/out. Sizes 12, 14, 16
and 24 correspond to the source presets; any positive size up to 4096 is allowed.
The application owns whether loading is happening and when to stop submission.
Spinner adds no status role, busy relationship, focus stop or completion event.
Announce the actual operation using the application's semantic tree when needed.

```c
/* Persistent application state: one instance per independent spinner. */
cui_loading_motion loading_motion = {0};

/* During an active frame: */
cui_spinner_options options = cui_spinner_options_default();
options.now_seconds = host_now_seconds;
options.size = 24;
options.custom_color = true;
options.color = (cui_color){220, 40, 90, 255};
options.reduced_motion = user_reduced_motion;
cui_loading_motion_result result;
if (cui_spinner_ex(ui, loading_id, &theme, &loading_motion, options, &result)) {
    /* Absolute deadline in host_now_seconds' clock domain; zero means idle. */
    next_wake_seconds = result.next_wake_seconds;
} else {
    /* Report submission failure; inspect the Core frame error as appropriate. */
}
```

Keep motion state between frames. Initial zero state starts at the first
successful supplied time; explicitly zero it to restart. Time is finite [0,1e12],
rewinds clamp to the last sample, and duration zero selects 0.8 seconds. Explicit
durations must be finite [0,60]. Context or option reduced motion freezes phase
at zero, skips custom easing, returns inactive/deadline zero and requests no
perpetual invalidation. Moving results request the next sample at 60 Hz. Hidden
or clipped submission does not automatically cancel application scheduling; omit
hidden spinners when appropriate. Stop scheduling when the operation completes.

## Custom icons and styling

A synchronous paint callback receives the selected size, color and center
rotation in radians. Use any available named icon or a caller-owned Icon state.
Host-loaded icons retain their existing loader/asset lifetime requirements.

```c
static bool paint_loading_icon(cui_context *ui, cui_id id, const cui_theme *theme,
    float size, cui_color color, float radians, void *user) {
    (void)user;
    return cui_icon_named(ui, id, theme, cui_string("loader-circle"),
        (cui_icon_options){.size = CUI_ICON_CUSTOM, .custom_size = size,
            .custom_color = true, .color = color,
            .custom_transform = true,
            .transform = cui_icon_rotation(radians)}) == CUI_ICON_OK;
}
/* Before submission: */
options.paint = paint_loading_icon;
```

`paint_user` and `ease_user` are borrowed only for the call and never retained.
`ease(phase,user)` receives normalized phase and must return finite [0,1]. A
failed easing/paint call preserves motion state/result; paint callbacks must
leave their own submission unchanged on failure. Core capacity errors retain
Core's existing failed-frame contract. Borrowed bitmap data in successful draw
lists expires at the next frame as with other Icon commands.

Compose a parent `cui_box` for placement, padding, backgrounds, borders, opacity
and clipping. Supported Icon style fields can also be supplied inside a custom
callback. Icon `.style.padding` currently rejects nonzero values; arbitrary
inherited source Icon styling therefore remains an explicit parity limitation.
This guide does not claim complete Icon styling or complete renderer/platform
parity from Spinner tests. Parent composition is the supported CUI padding path.
Explicit transparent color remains transparent; it is not a fallback sentinel.

## Source boundary and legacy API

The reference is GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/spinner.rs`
and its `Icon`/`Sizable` implementations. Spinner exposes icon, color, easing
and size. Its source duration is private; CUI additionally exposes a bounded
duration option. Spinner itself has no general Styled implementation, but its
custom Icon can carry style. Default easing comes from pinned GPUI 0.3.8.

The older `cui_spinner` is a compatible eight-dot phase ring with legacy Loading
status semantics and continuous invalidation while moving. It remains available;
use `cui_spinner_ex` for the source-style SVG, decorative semantics and explicit
wake scheduling. The two APIs intentionally have distinct contracts.

## Gallery and verification

The existing **Spinner** gallery entry offers source preset/custom sizes,
Loader/Loader circle/Arrow right, theme/custom/transparent colors, and default
quadratic/linear easing. Copy its example and open this guide from the component.
No extra catalog family is introduced.

`tests/unit/test_spinner_contract.c` verifies all source pixel sizes plus custom
64, default color, transparent color, actual rotated custom SVG pixels, static
reduced-motion restoration, supported Icon background/radius styling, ancestor
opacity and clipping, and decorative/quiescent behavior. Existing
`test_status_motion.c` covers clocks, callbacks, custom easing and rejection.
`tests/web/test_spinner_gallery.py` loads real WASM and exercises controls,
bitmap/pixel changes, reduced-motion quiescence, desktop/390px and DPR 1/2,
copyable usage and guide navigation. Native tests run through
`python scripts/test_local.py --build-dir <build>`; browsers stay headless.
These tests are evidence for the listed behavior, not hardware/GPU acceptance.
