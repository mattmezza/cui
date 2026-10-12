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

Switch labels use explicit 16/20/24-pixel line heights, including wrapping. Tooltip hit ownership
covers the complete control, rather than only the source track. Font weight and native
SVG details remain dependent on the host text renderer. These are tracked gaps, not a
claim of complete source parity. `label_left` is a C convenience extension (the frozen
source has the field but exposes no public setter).

## Toggle and ToggleGroup

`cui_toggle` now paints its label without an invented indicator. `cui_toggle_ex` adds
Ghost (default) and Outline variants, four source sizes, arbitrary child content,
accessible-name override, tooltip, Tab exclusion, optional focus outline, independent
corner radii/edge widths, and the same controlled boolean callback contract as Switch.
Pressed and enabled-hovered toggles use accent/accent-foreground; disabled presentation
keeps the source colors and suppresses interaction. Extra Small/Small/Medium/Large have
20/24/32/36-pixel minimum widths and fixed heights, with horizontal padding 2/4/8/12.
A composed icon or long label can grow the width. No automatic focus outline is painted,
matching the frozen source; `focus_ring` is an explicit C accessibility option.

`cui_toggle_group_ex` is the richer source composition. The existing Choice module's
`cui_toggle_group` remains available with its older selection options. Supply stable,
unique item IDs, caller-owned values, and per-item label/content/tooltip configuration.
The passive horizontal Toolbar group overrides child size and variant and combines
group/child disabled state.
Selections are independent; it does not introduce radio exclusivity or roving focus.
Ordinary groups have an eight-pixel gap; segmented groups have zero gap, only outside
corners, and no repeated left border. `custom_gap` permits an explicit spacing override.

Without a group callback, activation writes `values`. With a callback, provide a separate
`requested_values` scratch array of at least `count` elements. Each activation copies the
current rendered vector, flips its corresponding entry, and synchronously reports that
request. The callback may update the owner values immediately. Callback vectors are
borrowed only during the callback. Do not replace item/value storage during rendering.
Child callbacks are superseded by the group, matching the source grouping contract.
Pointer, Enter and Space all report the group request; keyboard delivery fixes a frozen
source limitation. The arrays and tooltip reducers stay application-owned, and rendering
allocates no persistent group state. Invalid sizes, variants, numeric geometry, duplicate
IDs, malformed strings, and insufficient/overlapping request scratch suppress the call.

Toggle labels use the source 1.25 line-height multiplier, subject to core integer layout
quantization. Hosts provide icon content and font weight; these remain rendering gaps.
