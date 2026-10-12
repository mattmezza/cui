# Icon

Icon renders named, host-loaded or raw SVG as a monochrome antialiased mask through
CUI's portable RGBA bitmap draw path. It is decorative: it contributes layout and
paint, without a semantic role, focus stop or pointer target.

```c
#include "cui/cui_icon.h"

cui_icon_state check = {0};
cui_icon_set_name(&check, cui_string("check"), NULL, NULL);
/* Inside a frame, with a unique id and an existing theme: */
cui_icon(ui, 42, &theme, &check, (cui_icon_options){.size = CUI_ICON_MEDIUM});
/* After the renderer has consumed the returned draw list: */
cui_icon_state_collect(&check);
/* After the final draw list has finished rendering: */
cui_icon_state_destroy(&check);
```

Use one caller-owned state per widget. A zero-initialized state is ready to use;
`cui_icon_state_init` also initializes fresh storage. Do not copy an initialized
state or initialize it again without destroying it. State operations are not
thread-safe. Setters copy path/raw source bytes and replace the previous source.
`cui_icon_set_empty` retains the requested layout but paints nothing.

## Sources and assets

`cui_icon_set_name` accepts catalog names such as `check`;
`cui_icon_set_path` accepts paths such as `icons/check.svg` or host-specific paths.
An optional loader runs on the next preparation and again after
`cui_icon_reload`. Its returned SVG bytes must remain valid until preparation
returns. Returning false tries the embedded bundle. CUI performs no implicit
filesystem or network access. Raw data uses `cui_icon_set_data`.

The complete 1,830-name/path catalog remains available through
`cui_icon_catalog_*`. The default build embeds the 106 frozen upstream default
SVGs. Define `CUI_ICON_ALL_ASSETS=1` when compiling the implementation to embed all
1,830; other catalog paths require a host loader in the default build.
`cui_icon_asset` returns a borrowed static SVG string when embedded.

`scripts/icon_assets.py` regenerates the bundle offline from the pinned JSON
catalog in `third_party/gpui_icons`; `--check` verifies reproducibility. Generated
assets retain the Lucide ISC, Feather MIT and upstream Apache notices. The private
NanoSVG parser/rasterizer is pinned and unmodified under its Zlib license.
Its external symbols use a CUI prefix, so an application may link its own
NanoSVG implementation in a separate translation unit. Implementation macros
introduced by CUI are removed afterward. Including both implementations in the
same translation unit is unsupported because their private types and static
functions share names.

## Size, color and transforms

Zero options inherit `theme.font_size` and `theme.text`. Hosts with their own text
scopes can supply `inherited_text_size` and `custom_inherited_color`/
`inherited_color`. General automatic text-style scope inheritance is not yet
provided by the core.

XS, S, M and L are 12, 14, 16 and 24 pixels at a 16-pixel theme font size, scaled by
`theme.font_size / 16` to represent source rem units. Custom size is an unscaled
pixel square. With inherited size, `custom_width`/`custom_height` enable independent
layout dimensions, including zero; an unspecified dimension becomes zero when the
other is explicit. An explicit preset or custom size overrides those dimensions.

SVG aspect ratio is scaled from the requested width and centered within the layout
height. Rotation or a center-relative affine transform changes paint bounds while
leaving layout unchanged, and may paint outside the icon's box subject to ancestor
clipping. `cui_icon_rotation` returns a replacement transform in radians.

SVG RGB colors are discarded; shape alpha becomes the mask. `custom_color` uses an
explicit tint, and `custom_opacity` clamps an independent paint opacity to [0, 1].
Changing tint or opacity reuses the raster. Source content, dimensions and affine
transform select raster cache entries. The counters in state expose parsing and
rasterization for diagnostics.

## Draw lifetimes and bounds

Preparation retains immutable masks: changing a source, size or transform does not
invalidate bitmap pointers already borrowed by a draw list. Call
`cui_icon_state_collect` only after **all** earlier draw lists using that state have
finished rendering. It frees retired masks and retains the active mask. Destruction
has the same renderer-completion requirement. Collection after each rendered frame
keeps steady-state memory small. Reusing one state for several icons in a frame is
supported, but consumes additional retained cache entries until collection.

`cui_icon_prepare` also works without a UI context. After it returns `CUI_ICON_OK`,
`cui_icon_state_bitmap` and `cui_icon_state_paint_bounds` expose the current borrowed
raster and relative paint bounds. Empty or singular transforms produce no bitmap.
After an error, do not paint a previously exposed bitmap as the new source.

Default limits are 256 KiB SVG input, 1,048,576 pixels per raster, 8 MiB retained mask
bytes and 16 retained versions. `cui_icon_set_limits` accepts smaller or bounded
larger limits; zero fields use defaults. Dimensions and raster sides are capped at
4,096 pixels, paths at 4,096 bytes, SVG dimensions/coordinates at 1,000,000, shapes
at 4,096 and total path points at 65,536. Affine coefficients and translation are
also bounded. Exhaustion returns `CUI_ICON_LIMIT`; allocation failure returns
`CUI_ICON_ALLOCATION`. Borrowed masks are never silently evicted to satisfy a new
request. Collect completed draws or increase the configured budget before retrying.

## SVG coverage and remaining differences

This implementation uses NanoSVG's supported SVG subset, rather than upstream
resvg. It supports the frozen catalog's paths, fills, strokes and opacity, including
tiny round-cap dots. It does not promise full SVG/CSS, text, filter, external image
or resvg feature parity. The parser is permissive for unsupported constructs;
resource checks bound accepted inputs and geometry but are not a complete XML
validator. DOCTYPE/entity declarations and embedded NUL bytes are rejected. Vendor
internal allocation failures may appear as parse failures or missing geometry;
explicit CUI buffer allocation failures are reported separately.

Headless tests inspect actual alpha geometry, aspect positioning, affine overflow,
size precedence, tint/opacity cache reuse, host reload/fallback, source replacement
and borrowed draw lifetimes. Both default and full-catalog configurations parse and
rasterize every embedded asset. Build-system, amalgamation and adapter wiring are
maintained by the integrator.

## Context-owned convenience icons

`cui_icon_named(ui, id, theme, name_or_path, options)` uses a bounded context
resource instead of caller-owned state. Equivalent name/path spellings retain the
same source and raster. `cui_icon_spinner` renders Loader with a repeating
0.8-second quadratic ease-in-out rotation; zero options use Medium size. Its clock
starts on first appearance and resets after absence for a complete frame. The
spinner overrides the paint transform, retains layout dimensions and requests
redraw while active. Host reduced motion makes it static and stops those requests.

Convenience resources use a 1 MiB/four-version raster cache budget per widget,
subject to the context's resource count and payload budget. Finish rendering before
`cui_begin_frame`: that call collects retired masks and eventually destroys omitted
widgets. Context destruction releases all caches. Source/raster preparation errors
are returned; context resource exhaustion also reports `CUI_ERROR_CAPACITY`.
