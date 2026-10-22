# Popover source anchors

Semantics are audited against GPUI reference commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, especially component Popover,
base Popup live anchor positioning and Positioner corner clamping.

`cui_popover_begin` is an additive C API for source-style Popover positioning and
arrows. Existing `cui_surface_begin` retains its legacy side placement, 6px gap,
opposite-side overflow selection and size reduction. Existing public state and
options structs have unchanged ABI. Recompile/link against the new library to
use the added symbols. This API is C-only until binding coverage is explicitly
updated; family catalog presence does not imply binding option parity.

```c
cui_surface_options options = {
    .viewport = {0, 0, 640, 480},
    .anchor = {120, 80, 100, 32}, /* current-frame trigger rectangle */
    .width = 240,
    .height = 160,
    .margin = 4,
    .hide_close = true,
    .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE,
};
cui_popover_position position = {
    .mode = CUI_POPOVER_POSITION_ANCHOR,
    .anchor = CUI_POPOVER_TOP_LEFT,
    .reference_rem = 16,
    .arrow = true,
};
if (cui_popover_begin(ui, theme, state, cui_string("Details"),
                      (cui_str){0}, options, position)) {
    cui_text(ui, cui_string("Popover content"), theme->font_size, theme->text);
    cui_surface_end(ui);
}
```

Read `include/cui/cui_surface.h` for the complete contract. Initialize state with
`CUI_SURFACE_POPOVER` and open it with `cui_surface_open` as for existing surfaces.
Always balance a successful begin with `cui_surface_end`; a false begin has no
caller-owned boxes to close. Supply updated trigger bounds each submitted frame.

Anchor names identify the popup's own anchor: TopLeft/Center/Right open below the
trigger, BottomLeft/Center/Right open above it. LeftCenter opens to its right;
RightCenter opens to its left. Alignment follows the named trigger edge/corner.
Anchor mode clamps without flipping or shrinking the chosen dimensions. Width/
height <=0 select 420/260; invalid active geometry is rejected before overlay state
updates. `cui_popover_surface_geometry` exposes these defaults and placement as a
pure helper with unchanged output on failure.

`reference_rem` is a logical size from app/host root sizing configuration. Zero
selects 16. Never infer it from a theme font. Default offset is reference_rem/4;
arrow depth is reference_rem*3/8 and is added to surface distance. With
`custom_offset=true`, signed offset including 0 replaces the default gap. Finite
negative gaps may overlap the trigger. Clamping may alter the apparent gap. Source
logical placement retains fractional coordinates; host pixel snapping is separate.

Arrow base inset uses the theme radius, even if the surface radius is refined.
Fill follows the effective refined background. Appearance NONE without explicit
background refinement uses theme popover for the arrow. The 1px outline uses theme
text at alpha 26/255 and is absent for NONE. Arrow triangle and ring seam patch
paint after panel contents at its layer, inherit panel opacity and clip to both
the supplied viewport and host viewport. Decoration does not enlarge the panel
hit/dismiss bounds, create semantics or receive focus. It remains visible with
an inert ancestor and while another modal owns input. A separate inert paint
portal uses local prepaint so visual painting does not require input eligibility.

Geometry and vertices are copied into context-owned frame storage; do not retain
draw-list pointers across the next begin-frame. Arrow payload/paint-command
capacity failure balances internal boxes and
suppresses the final draw list. Ordinary layout-node capacity follows Core
semantics: failure remains balanced but ordinary paint may be partial. Retry on
a later adequately sized frame.

The portable outline uses a bounded 2px miter and triangle meshes. GPUI's external
PathBuilder tessellation/raster AA is not reproduced exactly. Source fill, anchor,
tip and seam equations do not establish exact pixel parity. Software, SDL_GPU and
Sokol support Core mesh commands. SDL3 now supports bounded analytic meshes
through prepared CPU coverage textures; see `design/SDL3_MESH.md` for its limits.
The browser Canvas adapter has exact headless mesh-mask and opacity evidence at
DPR 1 and 2. The gallery additionally tests all eight anchors, signed offsets,
clamping, opacity, inert paint and modal ordering with desktop and touch input.
Physical GPU hardware checks remain separate from Xvfb/software validation.
This slice does not cover
Popover transition, right-click trigger or focus/control parity.

## Browser gallery

The Popover story starts with its existing legacy account card. Select **Source
anchors (opt in)** to use `cui_popover_begin` and a real centered CUI trigger.
All eight names identify the popup's own anchor. Controls map directly to
`mode`, `anchor`, `custom_offset`, signed scalar `offset`, `arrow` and
`reference_rem`. The latter is explicit application/host root sizing: zero means
16 logical pixels; the sample also offers 16, 24 and 32. It is never inferred
from the theme font. Inactive controls are disabled in legacy mode.

Edge clamp moves the actual trigger to the lower corner. The gallery schedules
one settling frame when the newly laid-out trigger differs from the bounds used
for composition, then returns to idle. It retains popup size and anchor direction.
Opacity and inert controls wrap the real stage. Open the card first to observe
paint persisting under an inert ancestor while child semantics/input disappear.
The modal demonstration uses a separate ordinary dialog at explicit layer 200,
above the card's default layer; it does not implement automatic overlay ordering.
Closing that dialog updates its control. Route changes close both surfaces and
reset the demonstration controls. The complete C11 snippet reflects the current
positioning and stage settings.

`tests/web/test_popover_anchor_gallery.py` drives actual mouse/CDP touch input,
checks all eight source equations against real CUI bounds and mesh vertices,
compiles the displayed snippets with strict C11 warnings, and reads actual Canvas
pixels for centered arrow joins, opacity, inert persistence and modal backdrop
ordering. Fractional clamped Canvas rectangle edges can still antialias the join
differently from mesh pixel-center coverage: a dark fill value 10 can read 9 at
an adjacent join pixel. Clamped geometry is checked separately; exact centered
join assertions retain exact values. This known raster difference is not claimed
as GPUI pixel parity. Browser font, accessibility and focus limitations remain.
