# Design sizes

Styled controls accept an appended `cui_design_size design_size`. Zero
(`CUI_DESIGN_INHERIT`) keeps their previous legacy size and custom metric behavior.
Existing zero initializers remain valid. Public structs have grown: rebuild callers
when updating the library.

The shared scale is xs, sm, base, md, lg, xl, 2xl, 3xl, 4xl, 5xl, 6xl, 7xl,
8xl and 9xl. Base and md deliberately share metrics. Explicit sizes resolve the
existing public `cui_design_measure` table, including font, line height, control
height, horizontal/vertical padding, gap and indicator size. Normal styled density
is `theme.font_size / 16`; compact density changes those metrics once.

The newly supported options are Badge, Progress (including circular Progress),
Tag, Attachment model, DropdownButton, Tabs, DescriptionList, Icon, Toolbar,
Settings, Questionnaire and Speech button/waveform. These join the previously
supported Button, check/radio/switch/toggle, Input, Textarea, NumberInput, OTP,
List, Select, Combobox, Menu and Form families.

## Geometry and composition

Explicit design size replaces legacy size-derived metrics. Caller-provided widths,
height/diameter overrides, custom boxes and artwork transforms keep their existing
precedence. Invalid enum values submit no UI and perform no input reduction or
callback. Invalid density and bounded bitmap capacity failures remain explicit
failures. All public API types remain portable C11.

A Badge sizes its count/dot/icon indicator; the application still owns its wrapped
content dimensions. Linear Progress uses one quarter of the scale control height
unless a height is supplied. Circular Progress uses the scale control height unless
a diameter is supplied; raster storage remains caller-owned and capacity-checked. The bounded circle
geometry limit is 4096 pixels; raster scale is at most four (16384-pixel extent).
Insufficient capacity rejects before touching storage or motion state.
Tag padding also accommodates the enlarged font's vertical padding. DescriptionList
labels retain their 7/8 text ratio. Attachment preserves caller card width and
media aspect; its text, glyph, height and padding follow the scale.

Toolbar presentation metrics carry the resolved design size to nested groups and
custom children. Settings' built-in field controls use the original theme plus
the explicit size, avoiding a second density multiplication. Custom render callbacks
receive the original control theme and the design size in their render options;
they must apply that size to their own child controls.

Questionnaire root options propagate size and density to composable parts. Its
`density` defaults to the root theme density, or one for standalone parts. A part
with `custom_size` keeps its explicit size choice; an explicit positive density
overrides inherited density. Custom choice views carry the design size. Speech
waveform width defaults to six font units and height to the scale control height;
explicit waveform dimensions remain authoritative.

## Gallery and Tabs

The gallery exposes the shared fourteen-size selector for these families and hides
their old four-size selectors. A named size changes submitted canvas geometry and
text; it is not a label translation. Supersized controls can require scrolling or
more application layout space.

The Tabs overflow trigger has square geometry, a centered SVG chevron and a minimum
32-pixel hit area even at compact xs. Its semantic Button is named “All tabs”.
Horizontal and vertical overflow use the existing menu, keyboard and selection
reducer. This change does not alter shared host wheel routing.

## Validation

Headless regressions compare the private Base-only table with the public resolver
through all fourteen sizes at normal, compact and doubled density, then check
actual submitted dimensions, text fonts, invalid-input atomicity and compound
propagation. `tests/web/test_design_sizes.py` runs a built gallery in real Chromium,
checks all twelve newly exposed story families and both circular Progress variants at
every size, compact density,
and both overflow axes with centered SVGs and actual menu activation.

Run the ordinary warning build and CTest, build the gallery with
`python3 scripts/build_web.py`, then use
`uv run tests/web/test_design_sizes.py --directory build/web --chromium /path/to/chrome`.
Distribution regeneration belongs to the integrator; source-only worker validation
may report the expected stale-distribution check.
