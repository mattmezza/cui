# Canvas gallery mesh adapter

The gallery currently routes mesh command 6 through its rectangle fallback.
Implement a host-only JavaScript raster consumer before exposing Popover arrows.
No Core API, public API, native adapter or existing legacy surface changes belong
to this slice. Gallery controls follow in a separate slice.

Decode mesh count, origin, scale, linear premultiplied color and XY/ST through
additional `gallery_command` fields. Check the count before allocating a vertex
array. Copy values during the current frame; never retain WASM memory views.
The renderer prepares every mesh before target dimension assignment or clear,
alongside the existing shadow preflight. Preparation owns independent triangle
rasters and expires on the next preparation. Failure leaves the target intact.

The supported host transform is finite positive uniform scale plus translation,
which covers the gallery's DPR mapping. Reject other matrices explicitly. Require
source-over and finite global alpha in 0..1; verify state at drawing. Convert XY
to physical pixels; leave ST unchanged. Use the software adapter's double edge
equations, orientation correction, top-left inclusion, pixel-center clipping,
physical ST derivatives, .001 solid threshold and zero-gradient coverage rule.
Unpremultiply finite linear RGB, clamp, then encode sRGB; alpha zero produces
zero RGB. Out-of-gamut input is valid. Quantize coverage alpha to nearest byte.

Prepare one tightly bounded Canvas raster per visible nondegenerate triangle.
Use reusable ImageData scratch and retain only uploaded canvases. Draw triangles
in command order through Canvas source-over, preserving overlap rather than
flattening a mesh into an intermediate image. The prepared pixel mask owns
nested clips and backing-viewport clipping; temporarily remove gallery clip
states exactly as the existing shadow path does. Draw at identity transform,
without smoothing, then restore caller state. Mesh color already includes Core
ancestor opacity; the host must not apply that opacity again.

Bound a frame to 131072 commands, 262144 vertices, 64 nested clips, 4096 prepared
triangle canvases, 100 million candidate pixel visits, and 32 MiB accounted
owned raster bytes (all retained canvas backing plus peak reusable ImageData
scratch). Validate finite derived arithmetic before integer conversion or
allocation. Browser/driver object overhead is outside this byte accounting and
is constrained separately by the canvas-object bound. Reject resource exhaustion
before clear. Degenerate and offscreen geometry is valid empty output but still
consumes input-count budget.

Headless Canvas tests cover DPR1/2, analytic ST including zero gradient, winding,
shared edges, degenerate triangles, color conversion, transparent alpha zero,
ordered translucent overlap, fractional nested clips, translation, transform
and capacity rejection, payload lifetime, expired prepared frames and unchanged
target after preparation failure. Require exact edge/clip/coverage masks; compare
color compositing against explicit Canvas reference vectors with only measured
per-vector rounding allowances. A real Core-emitted ancestor-opacity mesh must
prove opacity is applied once. Later Popover gallery tests consume actual CUI
mesh commands, never substitute DOM geometry. Exact GPUI raster parity remains
outside the claim.
