# Overlay behavior and focus scopes

`cui_overlay.h` provides persistent application-owned open/dismiss state. It
uses core focus scopes; it does not implement an independent hit-test or focus
traversal system. The panel's layout and appearance remain application-owned.

Initialize a stable scope ID with `cui_overlay_init`, then open it with
`cui_overlay_open`. Supply an explicit trigger ID, or zero to remember current
focus. After each `cui_begin_frame`, call `cui_overlay_update` before submitting
controls, processing nested overlays innermost first. If `cui_overlay_begin`
returns true, submit the panel and its controls, then call `cui_overlay_end`.
After layout, save the panel's resolved bounds with `cui_overlay_set_bounds`.
Outside dismissal arms only after bounds are supplied for the current opening,
so opening a popup cannot dismiss against stale or absent panel geometry.
The existing `cui_popup_place` model can determine an overlay's desired bounds;
absolute positioning/portals are not currently supplied by this module.

A modal scope confines forward and reverse Tab traversal to enabled descendants.
The core establishes initial focus inside a newly submitted modal. Nested modal
scopes trap to the innermost scope and restore focus to their remembered trigger
when dismissed. Pointer input outside a modal is blocked from underlying
controls and routed to the modal scope, allowing outside-click dismissal.
Nonmodal popups permit the underlying outside click to activate while dismissing.
Escape dismisses the top open scope. Dismissal policies are independently
configurable. Focus restoration validates that the trigger was enabled in the
previous completed frame; final submission validation also clears focus if the
trigger has since become disabled or disappeared.

Input is routed against the preceding completed frame and its scope ancestry.
Opening affects dispatch from the next frame. Dismissal consumes the routed
scope event; later queued events retain the routing established at frame begin.
There is no post-Tab focus repair. A scope supplies keyboard and pointer
isolation, but currently adds no platform accessibility modal relation,
semantic dialog role, scrim, z-order, animation, scroll lock, or native window.
These are reusable behavior primitives, not a complete styled Dialog.

`test_overlay` verifies nested scopes, initial focus, forward/reverse traversal,
disabled descendants, Escape, trigger restoration, disabled/removed triggers,
and modal versus nonmodal outside-click activation.
