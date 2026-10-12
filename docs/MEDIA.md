# Image and Attachment

The comparison is frozen to gpui-component 0.7.1 and its local source/docs snapshot.
Image is a re-export of GPUI's image element, rather than a kit-defined widget.
Attachment is a composed, application-controlled presentation of file lifecycle.

Image source descriptors distinguish embedded asset keys, file paths, URIs,
encoded bytes and custom loaders. CUI supplies the portable host-loader boundary,
not a decoder, filesystem reader or HTTP client. Assign a stable source identity;
change it when the source or retry attempt changes. The host callback must reuse
its request/cache. Resource success and failure are retained until identity
changes or state is explicitly reset. Custom loaders are polled each frame and
may supply a new frame collection. Source descriptor memory is borrowed for the
callback. Ready frame arrays and pixels must remain alive while retained by state,
and through completion of every renderer consuming their draw commands.

A load returns Pending, Ready or Failed. Ready validates up to 4096 borrowed
RGBA8 frames with positive dimensions, stride of at least four bytes per pixel,
finite nonnegative durations and no addressable stride overflow. Pixels are
straight alpha; changing them requires changing the revision. CUI keeps loading
and failure replacement callbacks within the reserved image box. Identified
pending images show the loading replacement after 200ms; unidentified images do
not show it. A fast success skips that replacement. Failure callbacks do not
initiate retries. Returned `next_wake_seconds` lets the host schedule delayed
loading or animation. Call the update/render function when host loading completes.

Image geometry supports centered Contain (default), Cover, Fill, ScaleDown and
None. Cover and overflowing None crop the source, preserving real bitmap pixels.
A zero dimension means automatic sizing: an explicit opposite dimension and ratio
resolve it, otherwise decoded intrinsic dimensions resolve it. An explicit aspect
ratio reserves height before decoding. Without reserved dimensions, decoding can
change layout. Grayscale, opacity and independent corner radii are passed to the
bitmap renderer. Informative descriptions create a passive semantic label;
interactive image commands belong inside a real application Button or Link.

Animation advances decoded frames only when identified, active and not reduced
motion. Inactive/reduced-motion periods do not accumulate catch-up time. A zero
frame duration uses a 10ms minimum to prevent an unbounded advancement loop. Frame
selection is bounded by the supplied frame count even after long elapsed times.

Attachment defaults to Complete, Medium and Horizontal. The application controls
Pending, Uploading, Processing, Failed and Complete; CUI never starts an upload or
mutates lifecycle state. Named sizes follow the reference rem geometry (at root16:
40/48/56/64px chip heights, 176/200/232/272px content widths and 28/32/38/44px media).
Custom density scales Medium geometry from an explicit pixel base. Vertical cards
without metadata are flush square media tiles; vertical metadata cards use a
120px default width and independently configurable media aspect ratio.

Typed metadata retains ordering, inherits lifecycle unless explicitly overridden,
and supports custom child callbacks. Typed children may override color/font size
and supply Marker shimmer duration/spread/highlight/direction options. Uploading/Processing titles use Marker
shimmer; Failed descriptions use destructive color. Known Uploading progress is
clamped to 0–100, appears as a determinate media ring and horizontal bottom bar,
and appends a rounded percentage to typed descriptions. Processing ignores known
progress and remains indeterminate. Media uses real image rendering when a host
loader/state is supplied. Custom media children and centered overlays compose
existing controls. Image status scrims are 35% while in progress and 55% on failure.

Stable identified cards may expose a pointer-only whole-card action, leaving focus
and keyboard activation to real child controls. The actions cluster captures its
gaps so pressing them does not arm the card. Remove is a 20px control with 6px
upper/trailing overhang, visible on desktop hover and always on explicit touch
mode. Failed retry appears over media and after the first typed description.
Callbacks report actions; ownership, removing records and retry requests remain
with the application. Keep states and borrowed child arrays alive through frame
completion. Tooltip composition reuses the existing Tooltip and requires the
caller to provide viewport and timing options.

AttachmentGroup retains a horizontal scroll offset, or uses an explicit external
scroll state. Its default gap is 12 and vertical padding is 4. Dominant horizontal
wheel input updates it, while ties remain vertical for the parent chain. Optional
24px edge fades appear only while content is hidden, with independent 200ms cubic
transitions and reduced-motion settling. Each fade uses 24 bounded strips and
claims no pointer target. Retained bounds measure overflow on a follow-up frame.

Current integration limits: no built-in raster/SVG decoding, remote transport,
HTTP cache, shared/scoped decoded asset cache or encoded animation parser. Host
adapters supply those; decoded-frame playback is implemented here. True font
weight and automatic ellipsis are absent in the existing text primitive; metadata
is clipped to a single line. The progress ring uses 36 bounded samples; the bottom
bar samples rounded clipping with six strips. Remove/retry shadows and automatic content ellipsis are not implemented.
Pending dashed borders are bounded
samples rather than a general dashed-path primitive. Arbitrary source style
builders are exposed as composition callbacks and explicit geometry, not a full
GPUI style language. These limits preclude a full-parity claim.

Integration: compile `src/base/cui_media.c` and `src/components/cui_media.c` into
the styled library. The independent base test needs only its test source, media
base source and math, with `CUI_MEDIA_BASE_ONLY`; no core/theme implementation is
linked. Styled code needs core bitmap commands, pointer-only semantic activation,
focus-scope hover ancestry, Theme, content helpers/Tooltip, Marker, and scroll base.
Each Attachment state is bounded by 128 metadata entries; Marker state is retained
per entry so its tracked text bounds remain valid through frame completion. State
may be heap allocated by the application to avoid large automatic objects.
