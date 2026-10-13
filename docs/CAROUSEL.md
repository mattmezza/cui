# Carousel

The module follows the frozen GPUI Kit 0.7.1 Carousel state, content, item,
scroll mask, navigation controls and pagination contracts. Its pure base model
requires only its source and `libm`; styled rendering uses existing Core,
Theme, Icon and Tooltip components. There are no persistent allocations.

Initialize `cui_carousel_state` with caller-owned geometry storage for 1–1024
items. The state and storage must remain valid through `cui_end_frame`.
`count` may be zero; the selection is then `CUI_CAROUSEL_NO_SELECTION`.
The initial nonempty selection is zero. Styled rendering synchronizes the
model count to its supplied descriptor count on each call. Programmatic count/orientation/looping
setters cancel interaction; count changes clamp selection, and an empty-to-
nonempty transition selects zero. `cui_carousel_set_selected` clamps silently.
User selection and navigation emit `on_change` only for a valid, different index.
Callbacks execute synchronously; their user pointers remain application-owned.
The callback may inspect state, but must not re-enter a render or replace its
geometry workspace during a call.

The drawing-free geometry API accepts finite, positive, sorted nonoverlapping
item bounds along the primary axis. It validates the entire input before writing
state or storage. Viewport extent is positive. Nonlooping snaps align item starts
and clamp to the scrollable extent. Navigation skips items that share the same
clamped snap, while explicit controlled/user selection can still select them.
Nearest-snap ties choose the earlier logical item. The cycle includes a trailing
gap matching the first inter-item gap. Runtime looping requires more than one
item and a cycle at least as large as the viewport; a short cycle falls back to
ordinary bounded navigation. Before geometry is available, configured looping
supports logical wrapping, as in source.

The styled root is a focusable, named Region with a clipped frame and optional
controls/pagination callbacks. Each logical item renders exactly once every
frame, including offscreen items. Item wrappers expose Group semantics, an
accessible label and position/set-size metadata. Looping repositions the one
instance of each item around the visible cycle; it never duplicates callback
state, child control IDs or content. Adjacent first/last navigation targets the
neighboring cycle and silently rebases after settling, without another change
callback. Long active drags periodically normalize their coordinate so the
workspace and animation distance remain bounded.

`cui_carousel_default` supplies growing width, a 200px frame height and 16px
leading item padding. The root has a 16px gap between composed parts. This is
an explicit C layout default; source content height is supplied by its host
layout. Item `extent=0` fills the frame along the primary axis and includes the
leading padding. A positive extent is the complete outer item size, including
that padding, and must exceed `gap`. Custom item appearance comes from `box`;
primary/cross dimensions are controlled by its extent and the frame. Arbitrary
callbacks do not receive an automatic intrinsic-size measurement pass. Hosts
translate source variable widths/heights into explicit extents, or use the pure
geometry API for separate presentation. This boundary is deliberate and is not
a claim of arbitrary GPUI style/layout parity.

Frame bounds are retained through `cui_track_bounds`. Growing width starts with
a 300px estimate until the first completed layout; fixed width uses its declared
value. A changed measured frame schedules a follow-up frame and recalculates
snaps. Geometry changes cancel active gestures and align the current selection.
The host drives frames until `cui_needs_redraw` clears. `now_seconds` is an
explicit monotonic host clock; backwards motion timestamps hold their last value.
The default movement spring has a 280ms response, damping .85 and settling
precision .5px. Styled rendering uses Theme `spring_move` response/damping;
reduced motion or disabled travel settles immediately. Critical and overdamped
settings also have an analytic solution. Settled motion does not request frames.

Pointer processing happens before child callbacks using Core's previous-frame
logical scope ownership and the measured slide-frame bounds. Navigation and pagination controls outside that frame never start a drag, so clicking them during spring motion cannot first snap to a different slide. A tap retains ordinary child press/release/activation.
Movement greater than 2px locks to the dominant axis; ties lock to the Carousel
axis. A stronger cross-axis move cancels the drag so ancestors can handle it,
and suppresses that sequence's click. A locked release selects the nearest snap.
Both locked drags and cross-axis cancellation cancel only their pending release
activation, leaving the release and capture cleanup available to the child.
Controls can be clicked normally again after the gesture. Primary-axis pointer
motion/release are cooperatively consumed; unrelated controls and overlays keep
Core hit priority. Disabled navigation cancels gestures and ignores keys/wheel;
host callbacks remain responsible for disabling any child controls they own.

Wheel delta signs match Core and Scroll: a positive delta moves backward,
negative forward. A discrete wheel burst steps once and remains owned until
28ms silence; a burst beginning at an edge belongs to its ancestor. Diagonal
wheel motion uses its dominant axis, with ties vertical. Precise input uses
`scroll_precise` and `scroll_phase` to maintain a gesture-wide dominant-axis
lock and continuous pixel offset. Started/Moved gestures retain ownership at
an edge, including overshoot. Ended snaps once; Cancelled restores the original
offset and emits no change. Zero-delta Ended/Cancelled still finish an active
gesture. Moved-only streams settle after 28ms silence. Host adapters must supply
reliable precise/phase metadata where available; SDL/DOM wheel inference is not
provided by this module.

Scroll handoff is cooperative. Pair a parent `cui_scrollable_begin` with
`cui_scrollable_end_ex` and its mutable state, so end-phase reduction receives
child-deferred events, avoids replay, and updates the same frame's layout offset.
The tests cover this actual styled Scroll composition as well as an independent
ancestor reducer after children. The older const-state `cui_scrollable_end` cannot
revisit a dynamically deferred quiet-burst event. Carousel still observes a
raw-hit edge event already reduced by the parent begin phase so a reversed delta
in that same burst remains deferred rather than selecting a slide.

Arrow keys navigate only the configured orientation. Home/End select the first
or last item. Children process input before Carousel navigation, so a consuming
text editor or inner component retains its key handling. Changed navigation
consumes the key; an invalid boundary action remains available to ancestors.
The root and controls preserve ordinary Tab stops. A pointer activation of a
navigation/pagination control focuses the root and suppresses its focus ring;
a keyboard activation preserves control focus. Root pointer focus keeps arrow
navigation without drawing the ring; losing focus restores it. The ring can be
hidden explicitly.

Navigation controls default to Medium 32px round outline buttons with real
16px Chevron icons, chosen for axis/direction. They sit 16px outside the frame
and use fixed dimensions immediately, or retained/estimated growing width.
A growing frame's first placement is provisional and corrects after layout. `size` selects XS/Small/Medium/Large (20/24/32/32px); their icons are
12/14/16/24px. Custom uses `custom_size` and a 75% icon dimension; `custom_box` and `content` replace presentation.
Previous/next disable themselves when no distinct reachable snap exists.
Optional caller-owned Tooltip state/options compose the real delayed Tooltip;
its host viewport must be supplied. Labels default to “Previous slide” and
“Next slide”; hosts can provide localized/accessibility labels.

Pagination items default to XS 20px compact buttons with selected metadata.
They deliberately have no invented dot/number content: applications provide
`content` and `custom_box` to match their chosen pagination presentation. Invalid
indices disable the item. The default accessible label includes the slide
number. `cui_carousel_pagination_begin/end` supplies the source named Group and an
8px-spaced, centered row, with an optional custom box. All callback arrays,
strings and Theme pointers are borrowed for their call. Wrapper IDs must be
unique and differ from IDs submitted by callback-owned controls.

Captured touch scrolling uses `cui_carousel_touch_scroll`: precise pixel deltas retain their dominant axis and live offset until an explicit ended/cancelled phase. Pauses between finger movements do not trigger the 28 ms wheel-burst timeout. Styled Carousel selects this path for Core events whose pointer type is touch. Hosts using the headless model must deliver a terminal phase or call `cui_carousel_cancel` on capture/focus loss. Mouse wheels and ordinary precise trackpad bursts retain their documented timing policy.
