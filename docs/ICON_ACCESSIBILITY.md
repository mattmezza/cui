# Icon accessibility and source audit

Frozen reference: GPUI Kit commit
`87d10ae5e1299d1be18670a37570c5402f952e34`,
`crates/component/src/icon.rs`.

The reference Icon itself declares no accessible name, role, focus or input
behavior. Existing CUI icons remain decorative by default. CUI adds an optional
`accessible_name` application label: a nonempty label submits a noninteractive
`CUI_ROLE_IMAGE` semantic with the icon's layout bounds, no focus and no Tab stop.
`decorative=true` suppresses that semantic, including when a name was supplied.
Asset names, paths, SVG title elements and raw SVG bytes are never used as labels.
An empty source may still be intentionally labeled; its semantic uses layout
bounds although it paints no SVG. Parent inert/layout-only behavior remains Core's.

The audit covered all reference branches:

- Legacy and full-catalog names convert to an Icon path; entity view constructors
  build the same source. CUI retains catalog/path APIs and context-owned named cache.
- Default/empty path, path replacing data and data replacing path are handled by
  existing owned source state. Data is copied; masks are immutable across source
  replacements until collection after renderer completion. CUI does not expose
  Rust Entity or Arc cloning; initialized state must not be copied.
- `same_layout` excludes source/color/transform. CUI layout uses the resolved
  numeric border-box size; transformation changes absolute paint bounds rather than semantic size.
- `into_svg` inherits text size unless either style dimension exists; explicit
  size replaces both dimensions. Existing width/height/custom flags preserve
  these rules, including an explicit zero and a missing opposite dimension.
- Presets use .75/.875/1/1.5 rem. New positive `rem_size` separates rem metrics
  from inherited text size; zero keeps legacy theme-font-based preset scaling.
  Custom pixels, inherited text size, dimension overrides and design metrics retain
  their existing precedence. Explicit size wins over style dimensions.
- Explicit text color overrides fallback. RenderOnce falls back to inherited text
  color; entity Render falls back to theme foreground. CUI's existing
  `custom_inherited_color` bridges RenderOnce scopes; omitting it selects theme
  foreground as the entity branch does. `custom_color` wins over both. SVG masks
  are tinted with this current color; the supported parser does not preserve a
  multicolor SVG palette.
- Rotation and affine transformation replace one another via options. Existing
  center-relative transforms preserve layout and allow paint overflow.
- Source `Styled` accepts generic refinements. New optional `style` supports Core
  box decoration (background, border, radius, clip, opacity, inert/layout-only,
  offsets), while explicit icon dimensions remain authoritative. Uniform or per-edge padding participates in the border-box sizing floor.
  SVG paint covers the complete resulting bounds, without a content inset. This does not reproduce arbitrary GPUI flex properties, margins, every
  SVG feature or automatic ambient style inheritance. Callers supply inherited
  metrics/colors explicitly; no global ambient style stack is introduced.

Validation of the new fields occurs before state preparation, cache allocation,
layout or semantics. Decorative labels are ignored entirely, so they need not
carry valid text. Nondecorative nonempty labels require a valid borrowed pointer.
Core copies the label into frame storage; the `style` pointer is synchronous and
not retained. Existing opacity normalization and source parser errors keep their
legacy behavior.

`tests/unit/test_icon_accessibility_source.c` checks actual draw tint/background,
layout and semantic dimensions, color precedence, independent rem presets,
explicit sizing priority, label privacy/decorative suppression, pointer/focus
ineligibility, invalid preflight atomicity and owned custom-SVG replacement
lifecycle. Existing icon tests cover catalog assets, raster geometry, affine
transforms, parser limits, explicit-zero dimensions and loader caching. The
the new test is registered in CMake and generated distribution headers include the API.

## Numeric styled bounds

`cui_icon_options.constraints` optionally supplies `cui_icon_constraints` with
explicit per-axis minimum and maximum flags. Zero is a valid explicit maximum.
Unset descriptor maxima use positive `style.max_width`/`max_height`; zero legacy
style caps remain unbounded. Minimum constraints win over maxima, then sums of
padding and border edges floor the resulting axes. A cap smaller than the
padding/border sum therefore does not shrink the final box below that sum.
Explicit Icon size still replaces both style dimensions before constraints.

The resolver runs before parsing/rasterization or semantics. It accepts finite
nonnegative enabled constraints/selected padding/border edges up to 4096 and
rejects a resulting dimension above 4096. Descriptor and style pointers are
borrowed synchronously. These numeric constraints do not imply percentage, flex,
grid, margin or automatic ambient style support.

`tests/unit/test_icon_style_source.c` covers padding/border floors, numeric caps,
explicit zero and conflicting min/max, full-box paint, unchanged named semantic
bounds, aliases and immutable simultaneous raster versions from one caller-owned
state. `tests/web/test_icon_style.py` compiles the assertion-enabled contract into
a dedicated real-WASM fixture and checks visible software-rendered SVG pixels,
clipping and named semantics at desktop/390 and DPR1/2. This is not a proof of
resvg equivalence or density-aware native SVG rasterization.
