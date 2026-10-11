# Base, themes, and components

Include `cui/cui_base.h` for behavior without a prescribed appearance. Begin a
box using a stable ID, call the base control with that same ID, render any
children, then end the box. Every value is application-owned: controls mutate
passed pointers on accepted input and report whether the value changed. Base
button returns the complete interaction state. Null value pointers do nothing.
Disabled controls retain values and reject input. Checkbox mixed state becomes
checked on activation. Radio activation selects its option without deselecting.

Core queues dispatch input against the preceding completed frame. Submit the
same IDs each frame. Space/Enter activation and tab focus come from core; the
base slider consumes only input explicitly routed to its ID. It supports
pointer drag/capture, arrow steps, Home/End, and a continuous value when step is
zero. Positive step snaps relative to minimum. Slider range must be finite and
ordered; a disabled slider never normalizes the application's value. Base
controls add no draw commands.

Include `cui/cui_components.h` for original styled controls. All interactive
wrappers call base behavior. Light, dark, high contrast, and amber/ink terminal
themes are value structs; callers can edit tokens to define another system.
Text indicators keep checkbox, switch, and radio understandable with the
portable text renderer. The terminal system uses square corners and a different
palette and spacing. Focus uses a visible border. Progress, badge, label, and
separator are presentational; progress has no accessibility range role yet.

Standalone model APIs provide popup side flipping and viewport fitting,
variable-height virtualization with overscan and spacer extents, and Gregorian
calendar arithmetic over years 1–9999. These are building blocks, not complete
popup, virtual-list, or calendar widgets. Virtualization scans heights in O(n)
and allocates nothing. Date arithmetic returns a zero date on invalid input or
out-of-range output; month navigation clamps the selected day. Popup placement
requires finite rectangles and sizes. No animation, tri-state accessibility
`mixed` flag, radio-group arrow navigation, range slider, logarithmic slider,
or integrated calendar widget is claimed.

`examples/unstyled` shows custom visuals consuming base checkbox state;
`examples/styled_settings` submits a complete settings form to the draw list.
The interactive gallery supplies the actual platform and renderer loop.
