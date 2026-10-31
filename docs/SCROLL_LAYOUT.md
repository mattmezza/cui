# Core intrinsic scroll-body layout

`cui_layout_scroll_body_v1` and `cui_layout_scroll_clamp_v1` are opt-in C Core
constraints. Read their exact declarations in `include/cui/cui.h`. They do not
change the existing Scrollable/Base/Dialog APIs or expose foreign-builder slots.
Python, JavaScript, TypeScript, Go and Rust builder composition support remains
future work; the C API does not establish full Dialog or component parity.

## Supported relationship

A body registration names this exact current-frame chain, after all six boxes close:

```text
Panel (fixed positive current width)
  Outer viewport (clip)
    Column (ordinary vertical flow, gaps and fixed siblings)
      Body frame (ordinary flow)
        Body viewport (absolute attachment, clip inherited from frame)
          Content (intrinsic flow or explicit content height)
```

Panel/Outer/Viewport each have exactly the named single flow child. Frame has no
flow child; its attached viewport grows from the frame origin. Outer, Frame and
Viewport have zero padding; Outer/Viewport align content from the top. Column is
vertical and may contain headers, descriptions and footers alongside Frame. Compose these and
all content once. The relationship allocates current intrinsic content height to
the Body, capped by the panel's available height. No previous-frame extent or
minimum readable spacer is used. AUTO chooses the smaller of natural demand and
max_height, respecting min_height. EXPLICIT uses the validated requested_height.
When fixed regions exceed the available height, the Body exposes its natural
height and the whole column scrolls inside Outer. Both Y offsets clamp to the
completed frame's ranges before final drawing, hits, semantics and caret geometry.

Widths must be determined independently of the new Y allocation. Ordinary GROW
children under a fixed/constrained ancestor are supported; every child need not
have a literal fixed width. Hidden/overlay scrollbars preserve that width.
Reserved-gutter feedback is outside V1. Core cannot infer an application's gutter
policy; the caller must satisfy the width-independent contract. The adapter rejects
aspect-ratio/percentage-height dependencies and foreign floating roots in the
registered subtree, checks the stored-tree width fingerprint and final predicted
ranges, and fails instead of iterating. Width independence comes from these structural
constraints and stable provider/X inputs; equality of a 64-bit diagnostic fingerprint
is not a proof of arbitrary constraint closure. This is not a general solver.
Fixed-size absolute decorations, including caret boxes, may attach to their actual
parent with inherited clipping. Canonical explicitly fixed-zero axes stay zero.
Nested registered bodies solve inside-out. No portal/root fallback is inferred.
Ordinary Core portals detach from their lexical parent and remain independent
roots; they do not contribute intrinsic demand or inherit this clip relationship.
The constraint does not certify or change those separate roots.

A standalone clamp names a directly parented clipped viewport and its content,
with a fixed positive viewport height. It changes Y translation only, preserves
sizes and cannot contain a registered body. Standalone and body-owned identities
cannot overlap. X/mixed axes are rejected, not ignored. The APIs create no semantic
role or input handler: applications retain their existing behavior/scroll owners.
For an existing matching scroll semantic, Core updates its completed range/value.
Input routing continues to use the existing dispatch and preceding-frame geometry.

## Registration and ownership

The 80-byte `cui_scroll_body_layout_v1` and 32-byte `cui_scroll_clamp_v1` records
contain no pointers. Set struct_size to sizeof(record), version to
CUI_SCROLL_LAYOUT_VERSION_1 and all reserved fields to zero. Heights and initial
offsets must be finite; 0 <= min_height <= max_height and max_height > 0.
AUTO requires requested_height = 0. EXPLICIT requires a positive request between
min_height and max_height. Clamp axes must equal CUI_SCROLL_CLAMP_Y.

```c
/* After composing/closing the six boxes, using stable application-owned IDs. */
cui_scroll_body_layout_v1 layout = {
    .struct_size = sizeof layout,
    .version = CUI_SCROLL_LAYOUT_VERSION_1,
    .mode = CUI_SCROLL_BODY_AUTO,
    .panel = panel_id, .outer_viewport = outer_id, .column = column_id,
    .body_frame = frame_id, .viewport = viewport_id, .content = content_id,
    .max_height = available_panel_height
};
bool accepted = cui_layout_scroll_body_v1(ui, &layout, &model.outer_y, &model.body_y);
```

Records copy during registration and can leave scope. Borrowed writable floats
must remain aligned, valid and stable through end_frame, including inside child
callbacks. Distinct destinations cannot overlap each other, another constraint's
destinations or retained tracked-bounds rectangles. Alias checks repeat after
composition so later bounds registrations cannot evade the rule. Record storage
may overlap a destination because its values have already copied; the destination
itself must still outlive end_frame. Core cannot validate arbitrary C pointer lifetime.
Register every frame; there are no persistent links or replacement semantics.
Inert/layout-only ancestry receives effective geometry clamping without writing
borrowed model scalars or publishing synthetic change events.

At most 32 body links and 64 total clamps are accepted. Each body reserves two
clamp slots atomically; standalone clamps use remaining slots. Repeated links,
missing/ambiguous IDs, incorrect ancestry, open referenced boxes, conflicting
identities and aliases reject the frame. No last-writer-wins or silent fallback.

## Errors and phase behavior

Ordinary-composition registration opts the frame in even if it fails. Invalid
records/topology report INVALID_ACTION; identity ambiguity reports DUPLICATE_ID;
resource/link/clamp limits report CAPACITY; unresolved/unsupported final geometry
reports LAYOUT. Wrong-phase calls retain existing lifecycle behavior and do not
retroactively opt a frame in. The first sticky error remains until explicitly
cleared. Clearing within a failed opt-in frame cannot salvage it: end_frame then
reports LAYOUT if no earlier error remains, and returns an empty draw list.

An erroneous opt-in frame has no valid draw, prepaint, tracked-bounds or semantic
snapshot promise. Discarded trial geometry never reaches prepaint. All active
borrowed offsets commit together only after every final constraint validates;
failures before that commit preserve them. This transaction does not roll back
application/input mutations or a successful commit followed by a later prepaint
error. Clear the sticky error and rebuild on the next frame to recover. A frame
with no opt-in attempt keeps the original layout, callbacks and error behavior.

The adapter calls real Clay_EndLayout once, then one additional stored-tree
calculation before Core prepaint/conversion. Public composition, input dispatch
and prepaint do not repeat. Text measurement/shaping can repeat under their
existing provider contracts. Vendored Clay is unchanged; the private adapter is
pinned to the repository version and must be reviewed when that dependency moves.

Storage is lazy within existing Core resource budgets: one 7216-byte payload on
the measured LP64 build, plus an 8-byte opaque-context pointer even when idle and
one shared immutable 16-byte failure header. No public config or existing public
struct grows. Other ABIs may have less padding; a compile-time 16 KiB payload bound
and the same 32/64 capacities apply. No resource scan or extra calculation runs
on a zero-opt-in legacy frame. Resource omission/collection follows the existing
Core lifetime contract. Tests cover public APIs, fixed/narrow/wide/current-font,
nested/empty/oversized regions, once-only callbacks, scroll/caret coherence,
capacity/allocation failure, late aliases, context isolation and legacy behavior.
