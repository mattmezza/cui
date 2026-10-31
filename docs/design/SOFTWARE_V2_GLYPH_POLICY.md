# Proposed software V2 glyph coverage policy

Review checkpoint before text painting, not an implemented guarantee. Keep V1
font selection, shaping, fallback, tracking and color-cluster rules unchanged.
Existing scaled line/colored/layout drawing accepts one source scale <=16; V2
supports independent positive target densities [1/65536,64].

Proposed explicit policy `software V2 glyph coverage v1` (private, tied to the
software V2 implementation; no change to CENTER_V1 background sampling): choose
source glyph raster density q=min(16,max(1,scale_x,scale_y)). Shape using original
logical font size/paragraph ranges. Ask existing scaled text APIs to emit coverage
at q, with original logical glyph origins; target origin is not folded into shaping.
This keeps font hinting independent of scroll/origin and provides at least unit
source coverage even for downscaling. Densities above16 upscale the source bitmap;
this is an explicit bounded quality policy, not a claim of native64x hinting.

Each callback bitmap is transient: copy its rows into the bounded transaction
owner before callback return. Callback (gx,gy,w,h) describes source pixels covering
[gx,gx+w) x [gy,gy+h). Source pixel centers are gx+i+.5,gy+j+.5.
For each destination center P, inverse-map logical L=(P-origin)/target_scale,
then source S=q*L. Bilinearly interpolate four source coverage bytes about source
pixel centers; outside that glyph's bitmap coverage is zero. Round the resulting
coverage once to nearest byte (ties upward). Do not clamp to edge pixels, move
origins to integer target pixels, rerun shaping at anisotropic font sizes, or
rescale paragraph ranges. This permits fractional phase and unequal densities.

For exact isotropic target=q, integer source-to-target translation, use direct
coverage bytes (the bilinear equation gives identical values). In particular,
unit-scale origin0 ordinary text must equal frozen V1 pixels. Keep glyph callbacks
in original paint order; overlapping glyph coverage is blended per glyph, never
max-merged. Colored callbacks use the selected cluster color; ordinary spatial
text paint is sampled at destination's inverse logical center. Then apply existing
byte alpha*coverage/255 truncation exactly once and existing straight-RGBA blend.
Clip is tested once at destination center; no extra bitmap shape mask.

The bilinear policy is bounded, deterministic coverage resampling, NOT exact area
integration for severe minification. Independent tests must expose this quality
limit, including densities below1/16 and above16. SOURCE_038/CSS brush models do
not affect glyph coverage. Native GPU consumers need not share this text policy;
no cross-adapter text-pixel identity is implied.

## Budget prerequisite, not hidden allocation

Coverage copies, callback descriptors, row padding and transaction scratch all
charge CPU/metadata budgets before publication. Existing font draw APIs can also
allocate shaping/FreeType/Bidi temporaries internally; they do not currently take
an adapter allocation envelope. Merely limiting copied glyphs cannot prove a
joint 32MiB owned-transient bound. Before full text support, audit these paths and
add a private counted allocation/session seam or explicitly report the already
owned font-session memory separately with a reviewed budget contract. No new
public font ABI is proposed here. Until resolved, slice(a) structural validation
must not report text resources fully prepared, or advertise a complete renderer.
External provider callbacks may have side effects even on failure; only the
published target/adapter ownership is transactional.

Tests: independently supplied tiny asymmetric coverage arrays, fractional origin,
unequal/extreme densities, transparent padding, adjacent and overlapping glyphs,
colored clusters and gradient ink; actual bundled-font unit-scale V1 equality and
scaled paragraph/fallback/letter-spacing tests; allocation failure and callback
failure after earlier glyphs leave target unchanged. This policy requires review
before any text painter is implemented.
