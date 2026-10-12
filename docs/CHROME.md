# Window and pane chrome

`cui_status_bar` implements the frozen GPUI Kit 0.7.1 StatusBar composition:
left and right regions pin to their respective ends; the center is centered in
the remaining space when both ends are present, right-aligned with only a left
region, and left-aligned otherwise. Regions accept borrowed text or arbitrary
callbacks with stable unique IDs. Callbacks receive 12px muted text defaults at
the standard density and retain ownership of their input and semantics.

The default bar has an independent `status_bar` background and
`status_bar_border`, an upper 1px border, 8px horizontal and 4px vertical padding,
and 8px gaps. Spacing scales with the theme's base font density. `style` replaces
the outer box; explicit content spacers remain. It is a passive group, not a
live announcement region; custom interactive children keep their own Tab stops.

Headless tests cover all four end-presence combinations, two same-frame widths,
all four themes, callback sizing/semantics, and duplicate-ID rejection.
Remaining source gaps: independent region overflow clips, inherited rich font
styles, and the full Styled refinement API. The outer bar clips overflow; custom
content can cross its own region boundary. Browser/native visual audit and
TitleBar host integration remain pending.

## Title Bar

`cui_title_bar` uses the source's fixed 34px height, 12px leading inset (80px
for native traffic lights), full-height 34px window-control slots, small window
SVGs, and vertical background gradient. The gradient starts at 55% title-bar /
45% application background and ends at the title-bar color; immutable bitmap
underlays preserve its position beneath children on the first frame. Fullscreen
native content receives the extra 12px leading inset. Multiple content items
share free space between them.

A portable `cui_window_host` in the core platform interface reports decoration,
maximized/fullscreen state and supported actions. Browser, server-decorated and
native-traffic-light hosts omit duplicate window controls. Client-decorated hosts
render only supported controls. The optional close callback overrides host close.
Buttons expose explicit accessible names and keyboard activation. Title-area
pointer capture requests one move on the first motion, clears on release/cancel
or an outside press, and leaves child controls independent. Double clicks use
configurable time/distance thresholds because portable events do not yet carry
native click-count/preferences. Client-decoration secondary clicks request the
host window menu. The return value reports a performed action.

The SDL adapter exposes `cui_sdl3_window_interface` and
`cui_sdl3_set_decorated`. Window close requests stop the next poll without
invalidating the current frame. Native drivers expose minimize/maximize/restore;
window managers may complete these asynchronously. Dummy/offscreen hosts expose
close only. Decoration changes return false when unsupported. Non-macOS native
hosts map title-bar double-click to maximize/restore. These service contracts,
unsupported actions and decoration flags run in the SDL tests, including X11;
portable mock-host tests cover the complete pointer/keyboard routing and paint.

Remaining TitleBar gaps: SDL does not expose drag initiation or a system-window
menu through this bridge; a host can provide those callbacks. macOS traffic-light
placement and system double-click preferences, Windows native hit-test control
areas, and window-manager-specific capabilities are not fully integrated. The
close override is portable rather than Linux-only. Custom box style replaces the
outer source style rather than implementing arbitrary incremental refinements.
Actual native window-manager behavior and browser story still need broader audit;
this is tested partial support, not full platform parity.
