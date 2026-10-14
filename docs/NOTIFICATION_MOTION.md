# Opt-in source Notification motion

After initializing the caller-owned state, select source motion explicitly:

```c
cui_notification_state state;
/* entries, settings and host are application-owned. */
if (cui_notification_init(&state, entries, capacity, settings, host)) {
    state.motion_policy = CUI_NOTIFICATION_MOTION_SOURCE;
}
```

Zero-initialized state and initialization retain `CUI_NOTIFICATION_MOTION_LEGACY`.
Reinitialization resets this opt-in. Motion and `settings.shadow_policy` are
independent: use SOURCE shadows as well when both source behaviors are wanted.
No Theme, Core, Base lifecycle or renderer API is changed by this policy.

SOURCE follows the pinned gpui-component 0.7.1 implementation at
`87d10ae5e1299d1be18670a37570c5402f952e34`:

| Transition | Duration | Translation | Opacity |
| --- | --- | --- | --- |
| Enter | 400ms | 96px outside the edge → 0 | 0 → 1 |
| Exit | 200ms | 0 → 96px outside the edge | 1 → 0 |

Top-left/center/right move upward outside the stack, bottom-left/center/right
move downward, and left/right-center have zero translation. Elapsed time is
mapped through CSS cubic-bezier `(0.25, 0.1, 0.25, 1)`, solving its x coordinate
before sampling y. SOURCE uses the source's bounded eight-step Newton and
32-step bisection solver; LEGACY retains its existing six-step solver and 16px
distance. Measured animated card bounds, custom content, interactions and copied
shadow casters all follow the same translated card.

The source locations are `crates/component/src/notification.rs:30–32,485–529`
and `crates/base/src/animation.rs:14–64`. SHA-256 respectively:

- `09010c94024ee6e2580ecf557d97dcf276512a52be8cf298e0936088df0afeca`
- `b17a0e92d239c4f947373f6ebf699e3fde7c83f13e2f96e979ada8b823eef44d`

Reduced motion samples the oneshot end state, matching GPUI's
`AnimationExt` contract and implementation at
`gpui-pre-0.3.8/src/elements/animation.rs:78–80,407–413`
(SHA-256 `117a4b1a580382b21accce30308f0480853c1e9372220566748dcb5f74d5793e`).
Thus enter is immediately at rest and fully opaque; exit is immediately at its
translated, transparent end state. Stack springs already snap under the existing
reduced-motion setting. SOURCE requests the remaining lifecycle deadline instead
of 16ms animation wakes. It does not invent immediate removal: the source
ToastManager retains its 400ms/200ms transitions independently
(`crates/base/src/toast.rs:234–266`). Presented results and close callbacks retain
those deadlines. Autohide deadlines and hover/focus pause behavior are preserved.

Render once per active UI frame through `cui_notifications_checked`, passing a
stable overlay ID and monotonic finite owner clock. Unknown motion enum values
fail before reductions, callbacks, output publication or UI emission. SOURCE
also checks the initialized owner's fade phase and finite timing/visibility
state. Existing shadow-policy validation and checked-render transaction limits
remain as documented in [NOTIFICATION_SHADOWS.md](NOTIFICATION_SHADOWS.md): later
UI/frame/renderer errors cannot undo previously accepted host callbacks. No new
borrowed buffers, sampler callbacks or resource lifetime are introduced.

The effective SOURCE shadow opacity remains transition opacity to the fourth
power, multiplied by independent stack visibility, then inherited Core opacity
once. Motion does not change the two copied source layers or their quantization.

Evidence:

- `test_notification_source_motion.c`: all eight placements at five actual enter
  times, an intermediate exit, exact completion/removal boundaries, legacy
  compatibility, reduced deadlines/no animation redraw, close callback timing,
  independent shadow policies, invalid policy/NaN rejection and Software pixels
  distinguishing 96px source movement from the old 16px card location. The exact
  pre-change component source fails its new source-distance assertion; the
  external reproduction keeps the appended header solely to select that policy.
- `test_notification_source_motion_sdl.c`: actual SDL Renderer OpenGL and software
  drivers, 32 controlled placement/enter/exit/scale frames per driver. Pixel
  capture happens before real Present; assertions abort rather than show modal
  ignore workarounds. A GNU/Clang native fixture needs `--wrap=SDL_RenderPresent`.
- `test_notification_source_motion.py`: actual WASM component commands and Canvas
  pixels for 64 controlled enter/exit/placement/reduced cases at DPR1/2, including
  legacy/source position distinction. Independent double-precision Bezier
  inversion checks geometry; the shared raster independently checks composition.
  No wall-clock sleeps drive these transitions.

Strict C11, scoped GCC ASan/UBSan and portable Wasmtime checks exercise the new
headless fixture. The worker does not register global tests or regenerate
headers; integrator regeneration is required for the appended state member.
The browser fixture takes `--output` and optional `--chromium` arguments.

This is a bounded motion slice, not Notification-family completion. Arbitrary
source style refinement and renderer-native GPUI AA/color/shader identity remain
separate boundaries. Native GPU/Sokol source-shadow coverage is from the prior
shadow slice; this motion fixture adds actual SDL Renderer and Canvas proof.
