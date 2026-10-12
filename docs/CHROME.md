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
