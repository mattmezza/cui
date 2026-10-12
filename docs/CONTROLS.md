# Foundational controls

## Switch

`cui_switch` retains the bool-pointer convenience interface and renders a real track and
thumb. `cui_switch_ex` adds sizes, disabled state, accessible-name override, optional
custom label content and tooltip, label placement, Tab exclusion, focus-ring suppression,
custom checked color, and persistent spring motion. Zero options select Medium and the
source defaults. Track sizes are 28×16 (Extra Small/Small), 36×20 (Medium), and 44×24
(Large); thumbs have a two-pixel inset. Theme radius below four preserves square themes.
Disabled tracks halve their alpha, while the thumb remains opaque.

Without `on_change`, activation writes the caller's bool. With it, the callback receives
the requested value and the bool stays controlled by the application. It may update the
bool synchronously; painting and checked semantics use the resulting actual value.
Callbacks and content userdata are borrowed for the call. Enter, Space release, pointer
activation on either track or label, and normal Tab traversal use the base Switch.
`skip_tab` removes sequential focus without removing programmatic/pointer focus.

A caller-owned, zero-initialized `cui_switch_state` enables the theme's `spring_move`
response/damping/epsilon policy. Supply monotonic `now_seconds`; the first paint is
immediate, changes retain position and velocity, and settled controls request no frames.
`next_wake_seconds` exposes the next suggested wake while core invalidation requests a
follow-up frame. Reduced motion, zero response, and disabled travel settle immediately.
The convenience function (and an extended call with null state) paints without animation.
The state also owns the optional tooltip reducer; its options need the normal viewport
and elapsed-time inputs described in the surface documentation.

The source's wrapped labels use explicit 16/20/24-pixel line heights. Core text currently
has no line-height option: this implementation reserves that height for a single-line
label, but wrapped text still uses core's measured line height. Tooltip hit ownership
covers the complete control, rather than only the source track. Font weight and native
SVG details remain dependent on the host text renderer. These are tracked gaps, not a
claim of complete source parity. `label_left` is a C convenience extension (the frozen
source has the field but exposes no public setter).
