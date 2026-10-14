# Notification source shadows

Set `settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE` before initializing
the notification state. Zero-initialized settings and
`cui_notification_settings_default()` retain `CUI_NOTIFICATION_SHADOW_LEGACY`.
SOURCE replaces the sixteen decorative rectangles with two copied drop-shadow
layers on each visible card's final, animated bounds:

| Order | Offset | Sigma | Spread | Color at rest |
| --- | --- | --- | --- | --- |
| 1 | (0, 10) | 7.5 | -3 | black, alpha 26/255 |
| 2 | (0, 4) | 3 | -4 | black, alpha 26/255 |

The caster uses `theme.radius_lg`. Core paints both shadows before the card
surface; content clipping and the ordinary border retain their existing order.
Source-invisible collapsed cards and system-only notifications emit no shadows.
The popover surface palette, custom content, stacking, dismissal controls,
deadlines and existing slide distance remain the Notification renderer's current
behavior.

The pinned source is gpui-component 0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/component/src/styled.rs:85–95` (ordered layers),
`crates/component/src/notification.rs:494–495,518–528` (transition ink), and
`crates/base/src/toast.rs:514–534` (independent stack visibility).
Source shadow ink uses transition alpha cubed, then element opacity adds another
transition-alpha factor. CUI projects the resulting ink as
`round(255 * 0.1 * transition_alpha^4 * stack_visibility)`.
Stack visibility is an independent ancestor factor, not raised to the fourth
power. External Core ancestor opacity applies once after this projection.
The source floating alpha 0.1 becomes 26/255 at rest; this is an explicit
8-bit projection rather than floating Hsla identity.

```c
cui_notification_settings settings = cui_notification_settings_default();
settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
/* Initialize caller-owned entries/state using these settings. */
cui_notification_advance next;
bool accepted = cui_notifications_checked(ui, overlay_id, theme, &state,
                                          viewport, now_seconds, &next);
/* End the active frame and check Core/renderer errors separately. */
```

`cui_notification_settings_valid` rejects unknown policies. Reducer operations
also reject invalid settings before changing state or invoking callbacks.
The checked render entry requires an active UI frame, valid initialized state,
independent output storage, finite viewport/time and, in SOURCE mode, valid
shadow radius and finite owner fade state. Rejected inputs leave its output,
owner reductions, callbacks and UI emission untouched. SOURCE setter failure
returns false without publishing the output. Already accepted reductions or
host/custom-content callbacks cannot be rolled back by later UI capacity,
`cui_end_frame`, or renderer-preflight failure. The compatibility wrapper returns
a zero advance on checked failure. This API does not certify frame submission.

The two local layer values are copied by Core; neither the stack array nor the
temporary theme is retained by a deferred renderer. Existing caller ownership
of notification strings, content callbacks, state entries and host userdata is
unchanged. Custom content must continue honoring the supplied opacity and its
existing borrowed-lifetime contract. Renderer budgets, supported shadow domain,
pre-clear rejection and copied cache keys use the shared drop-shadow capability.

Evidence includes meaningful headless geometry/order, invalid-policy/radius/fade
rejection, controlled exit, independent stack opacity, translucent surface and
ancestor composition, hidden/system delivery and custom disabled/loading
children. Software and actual SDL Renderer (OpenGL and software drivers), SDL
GPU (Vulkan), Sokol (Mesa GL), and Canvas exercise actual component commands in
light/dark themes at scales 1 and 2. Failed shadow preparation preserves previous
pixels; native SDL captures occur before Present because accelerated backbuffers
need not retain pixels after presentation.

This slice does not complete Notification parity: the source's 96px slide,
arbitrary source style refinement and exact GPUI native shader/AA/color identity
remain separate work. Source behavior is not inferred from the private shadow
helper's name. The unchanged LEGACY approximation remains available explicitly.

The native/unit/WASI/browser fixtures are registered by the integrator; generated
headers include the optional policy and checked entrypoint. The four GNU SDL
wrap flags capture actual completed drawing before presentation. Reproduce
Canvas with `uv run tests/web/test_notification_shadows.py --output build/notification-shadow-probe`.
