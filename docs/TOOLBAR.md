# Toolbar and ToolbarGroup

Toolbar is a transparent horizontal command container. It exposes Toolbar
semantics, preserves ordinary Tab traversal through its controls, and handles
unconsumed Left/Right keys after its children render. Focus wraps among enabled
Tab stops in the scope, including nested groups. Nested toolbars handle their
own keys first. Hosted inputs retain caret navigation when their input behavior
consumes those events. The toolbar itself is passive. `disabled` suppresses only
its navigation; applications disable hosted controls individually.

This implementation follows the frozen GPUI Kit 0.7.1 `base/src/toolbar.rs` and
`component/src/toolbar.rs`. The website mentions vertical navigation, but the
actual source fixes orientation to horizontal and handles Left/Right only; CUI
follows that implementation.

`cui_toolbar_default()` selects Small. XSmall uses height 28, padding4, gap4,
font12, and sized wrapper 20. Small uses 32/4/4/14/24. Medium uses 48/8/8/14/32.
Large resolves to Medium. Custom preserves its numeric size for control
callbacks while using Medium bar geometry and a 24px sized wrapper, matching the
source's wildcard cases. Invalid enum/nonfinite/negative custom values fail
before submitting the container. Arrays are bounded at 1024 items and borrowed
for the call; no persistent allocations are required.

Each item has a render callback. Sized items receive `prepared=true`, final
metrics, and a centered fixed-height wrapper with a distinct nonzero ID. Their
callback implements the control-specific `prepare_for_toolbar` hook before
applying the final size; arbitrary C callbacks cannot be mutated automatically.
Unsized content gets `prepared=false` and no wrapper. Items render in source
order. A nested group callback forwards the presentation metrics into group
options to inherit the enclosing size. Group carries passive Group semantics
and an optional accessible label, lays out children horizontally and centered,
and adds no padding or gap. Its owner supplies spacing and disables controls.

`cui_toolbar_command_render` is the supplied sized-command callback: it renders
an actual compact ghost button, uses controlled selected/disabled state,
separate visible and accessible names, an optional icon callback, and validated
pointer/keyboard activation. A label-less command requires an accessible name.
Host tooltip display remains an application integration; the renderer does not
invent a tooltip from invisible text. Icon rendering and callbacks are borrowed
for the render call. Custom label commands retain natural content height and
20%-of-size horizontal padding; custom icon commands are square, as in source.

`custom_box=true` supplies a complete container layout/style replacement, rather
than guessing which zero fields in a C aggregate mean a style refinement. It can
add a background/border or override dimensions, but its owner must preserve the
source's intended horizontal layout when desired. Default containers have no
background/border and never shrink their fixed height. The surrounding header or
pane owns the surface.

The base begin/end API accepts application-owned layout and supports arbitrary
children without the styled layer. Pair end only after a successful begin. A
base toolbar opens a nonmodal focus scope; a base group adds semantics without
owning navigation. All pointer, focus, modal, clipping and inert rules come from
core. Arrays/callbacks/state belong to the application; no platform or Clay types
appear in the public API.
