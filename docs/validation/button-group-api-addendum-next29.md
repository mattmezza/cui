# ButtonGroup declaration and staging addendum

This is a read-only proposal supplementing `button-group-proposal-next29.md`.
Base remains `021d553`; the subsequent integration correction `8e701b3` preserves
activation-only blockers while excluding them from SCROLL scope-hit queries.
No production edits are authorized by this document. Core production is currently
assigned to another worker; the acceptance seam below requires coordinated ownership.

## Proposed declarations

The following declarations are portable C11 and use existing public Core and theme
types. They are proposed source API, not a promise of binary compatibility for a
new struct. Put Base declarations in new `cui_button_group.h`; put styled declarations
after Button options in `cui_components.h` or a new styled header included there.

```c
#include "cui/cui.h"
#include "cui/cui_components.h" /* Only needed for the styled declarations below. */

enum { CUI_BUTTON_GROUP_MAX_ITEMS = 256 };
typedef enum {
    CUI_BUTTON_GROUP_SOURCE_POINTER_CALLBACKS,
    CUI_BUTTON_GROUP_ALL_ACTIVATIONS
} cui_button_group_activation_policy;
typedef struct {
    cui_id activated_id;
    size_t item_index; /* SIZE_MAX means actual collection-background activation. */
    size_t event_index;
    const size_t *selected_indices;
    size_t selected_count;
} cui_button_group_request;
typedef void (*cui_button_group_change_fn)(void *, const cui_button_group_request *);
typedef void (*cui_button_group_child_click_fn)(void *);
typedef struct {
    cui_id id;
    cui_str label;
    bool disabled, loading;
    bool skip_tab, prevent_pointer_focus;
    bool custom_cursor;
    cui_cursor cursor;
    cui_role role; /* Zero derives Button; accept Button or Link only. */
    cui_button_group_child_click_fn on_click;
    void *action_user;
} cui_button_group_item;
typedef struct {
    bool multiple, disabled, vertical;
    cui_button_group_activation_policy activation_policy;
    cui_str accessibility_label;
    cui_button_group_change_fn on_change;
    void *change_user;
} cui_button_group_options;
typedef struct {
    bool valid;
    size_t accepted_activations, callbacks;
} cui_button_group_result;
typedef struct {
    /* Caller-owned bounded transient workspace, not retained widget state. */
    cui_context *context;
    uint64_t frame_sequence;
    const cui_button_group_item *items;
    bool *selected;
    size_t count, registered_count;
    cui_id id;
    cui_button_group_options options;
    cui_interaction interactions[CUI_BUTTON_GROUP_MAX_ITEMS];
    bool registered[CUI_BUTTON_GROUP_MAX_ITEMS];
    size_t requested_indices[CUI_BUTTON_GROUP_MAX_ITEMS];
    cui_button_group_item styled_items[CUI_BUTTON_GROUP_MAX_ITEMS];
    bool active, failed;
} cui_button_group_workspace;

bool cui_base_button_group_begin(cui_context *, cui_id,
    const cui_button_group_item *, bool *selected, size_t count,
    cui_button_group_workspace *, cui_button_group_options);
cui_interaction cui_base_button_group_item(cui_context *,
    cui_button_group_workspace *, size_t index);
cui_button_group_result cui_base_button_group_end(cui_context *,
    cui_button_group_workspace *);

typedef struct {
    cui_id id;
    cui_str label;
    cui_button_options options;
} cui_button_group_styled_item;
typedef struct {
    cui_button_group_options behavior;
    bool override_size, override_variant;
    cui_control_size size;
    cui_design_size design_size; /* INHERIT leaves per-child design_size intact. */
    cui_button_variant variant;
    bool compact, outline;
    cui_box box; /* Explicit portable layout constraints; orientation supplied above. */
} cui_button_group_style_options;
bool cui_button_group_ex(cui_context *, cui_id, const cui_theme *,
    const cui_button_group_styled_item *, bool *selected, size_t count,
    cui_button_group_workspace *, cui_button_group_style_options,
    cui_button_group_result *result);
```

The styled wrapper uses the fixed `styled_items[256]` conversion array in the caller
workspace, retained through its synchronous end call. No large stack allocation,
Core resource or platform handle is required. The declaration is explicit
enough to review storage/ownership; fields are implementation workspace, not application
selection. Initialize workspace to zero and use one workspace per concurrent group.

`begin` validates all items and borrowed buffers before registering anything, then
registers the collection semantic. Caller already opened its layout box with group ID;
Base does not paint or open hidden children. Call `item` exactly once for every index
while each child's intended logical scope/layout parent is active. Ordinary Base
callers submit their child boxes themselves. Styled code does this internally.
`end` requires all items registered and invokes callbacks only if every registration
succeeded and the frame is valid. No callback occurs on invalid input or partial
registration. The workspace cannot be nested/reused until end and cannot cross frames;
the implementation must detect active reuse and invalid item/end order. A stale
cross-frame workspace also needs a frame token: no such existing public query was
found. Propose `uint64_t cui_frame_sequence(const cui_context *)`, returning the
current successful begin-frame generation, and zero outside ordinary submission
or for null/inert context. Nonzero generation increments on each successful begin,
including `dt=0`, skipping zero on wrap. Store context and generation at group begin;
reject item/end if either differs. Do not substitute accumulated time. Applications
must reset workspace before reusing it with a newly created context. This narrow
lifecycle seam requires approval alongside accepted-event evidence; it is not an
unrelated config/secret or host change.

The wrapper returns false for validation/registration failure; the result reports
accepted activations and actual handler deliveries, not accepted model changes.
On controlled rejection, callbacks can be nonzero while `selected` remains unchanged.

## Direct ownership and inheritance order

Children are borrowed array descriptors, not pointers to retained Button objects.
IDs belong to the application and survive reorder. Names, slot callbacks, bitmap/icon
providers and each Button's optional state are borrowed under existing Button lifetime
rules. Parent owns the group callback; child owns its `on_click` only if no group
callback is installed. Group handler replacement never calls both. Hover callbacks
remain per-child, and do not change ownership when a group click handler is installed.

Resolve each styled descriptor in this exact order:

1. Copy child Button options, preserving icon/content slots, state, tooltip, cursor,
   palette, label constraints, focus policy and child disabled/loading.
2. If `override_size`, replace legacy size. If group design size is non-INHERIT,
   replace child design size; otherwise preserve it. Individual explicit positive
   height/font/icon overrides remain strongest, matching existing Button precedence.
   If `override_variant`, replace variant. True group compact/outline enables them;
   false does not clear a child's true value.
3. Set effective disabled = group disabled OR child disabled. Effective interaction
   inert = effective disabled OR child loading; loading remains per-child and retains
   the existing busy semantic/loading icon. This intentionally fixes source builder
   order quirks; never overwrite a child's true disabled with group false.
4. Set selected paint and toggled semantic state from caller `selected[index]`, not
   from a stale copy of child `options.selected/toggled`. This is the group's sole
   selection authority. No coercion of multiple preselected values in single mode:
   source permits them until an activation requests a singleton.
5. Apply group edge/corner composition last for count > 1; count 1 preserves child's
   rounding. No equal-width override. Group box orientation controls row/column, gap
   is forced zero for segmented composition; caller padding/explicit bounds remain.

Source callbacks are pointer-bubbled only (`button_group.rs:243-261`), and upstream
test `legacy_keyboard_click_does_not_reach_the_group_callback` confirms no group
keyboard callback. Default SOURCE_POINTER_CALLBACKS reproduces that behavior when
a group handler is installed: keyboard child activation is recorded/cleaned up but
does not deliver the application's replaced child handler or a selection request.
ALL_ACTIVATIONS delivers equivalent requests for accepted pointer, keyboard and
accessibility activations. Without a group handler, child handlers receive every
accepted activation regardless of the group policy. No new roving keyboard behavior.
The policy applies to callback eligibility, not focus, capture or pressed paint.

An actual background pointer activation uses group ID/index SIZE_MAX and requests
unchanged selection. The group background is pointer-activatable without Tab/focus;
enable this target only for an enabled group with an installed group callback.
Without that callback, the collection is passive and background clicks retain
ordinary ancestor routing. Disabled/loading child surfaces retain their individual
activation barriers; do not silently turn the whole passive container into a barrier.
Empty intrinsic bounds have no hit; explicit empty bounds can request an empty array.
Do not reuse a stale last-child index from an earlier source `Cell` activation; this
intentional source divergence is documented separately from the mouse-only policy.

## Missing acceptance evidence and exact staged dispatch

Keep the full requirement: one eligible handler per accepted activation, independently
of how many arrive in a frame, in original frame-event order across children. Boolean
per-child/frame coalescing is not the final Group contract.

Existing `cui_event_activation` exposes the generated target before widget consumption.
It cannot prove acceptance after current-frame registration. `cui_interact` consumes
pending activation slots and collapses accepted slots to `interaction.activated`;
current-disabled handling can consume every slot without setting that Boolean.
Accessibility has extra previous-role/current-role and scope checks in that same
loop, and end-frame rejection also clears pending evidence. Cancelled pending slots
already become zero, but consumed accepted slots also become zero in the activation
array. Reconstructing accepted identity from slot zero, raw UP, a child Boolean or
event order is ambiguous. Counting a previously saved generated target would bypass
current disabled/loading/accessibility rejection. This is the precise missing seam.

Propose only an append-only read-only event query:

```c
cui_id cui_event_accepted_activation(const cui_context *, size_t event_index);
```

During successful semantic registration, Core records the accepted target for each
event whose activation slot actually passed existing acceptance checks. Record after
capacity/duplicate checks succeed; zero remains pending/unsubmitted/cancelled/rejected.
Storage is one ID per existing bounded dispatch event; reset at begin frame and clear
on rejection. Query only during ordinary frame submission, not prepaint/measurement
callbacks. Before returning, check current submitted target remains enabled and, for
accessibility, current final role/scope eligibility after metadata refinement. The
query must not consume an event or manufacture an activation. It preserves previous-
frame pointer/modal routing. Later scene submissions cannot retroactively undo an
already delivered callback; dispatch after current group semantics/scopes are final,
using the same frame snapshot principle as existing controls. Tests must cover a
current modal registered before dispatch and role refinement before the query.

This is an event-acceptance evidence seam, not a broad configurable activation engine.
No new raw event or Core config field is needed. Existing bool interaction/return
values stay compatible. Accepted counts can be calculated by scanning these records;
an `activation_count` struct field is unnecessary for the Group proposal. Fixing the
standalone Button's currently collapsed callback behavior can use the same evidence
in a separate narrow dispatch patch after approval, without changing its Boolean API.

Staged group handling:

1. Validate borrowed inputs, workspace, IDs/count and enum/finite/style constraints.
2. Register collection and all child semantics using each child's final Button/link
   role and effective inert state. Refine busy, toggled, pointer focus, Tab and cursor
   metadata before dispatch. Consume no callback twice. Reuse a private Button
   metadata/hover helper extracted from existing Button code, not duplicated policy.
3. Iterate `cui_frame_events` once in index order; query accepted identity. Match
   group or stable child ID, apply callback policy and skip zero/unsubmitted targets.
   Produce request from caller's currently accepted selected array. Multiple scans
   selected indices ascending, then removes or appends clicked index; single requests
   its singleton. Invoke one group or child handler per eligible accepted event.
4. After each callback, reread caller values for the next event: accept/reject is
   explicit. Then update every child's checked/value to the final model.
5. Paint all children from the final model with `cui_button_paint`; run the existing
   tooltip/hover lifecycle through a narrow shared private helper. Callback handlers
   must not register a second semantic or paint a child themselves. All temporary
   request pointers expire when their synchronous callback returns.

Per-child callbacks that disable external state affect the next submitted frame;
events already accepted against this frame's final submitted inputs remain eligible.
This deterministic snapshot rule also applies to child loading. Selection, however,
is controlled mutable model input between callbacks as described above. Cancelled
pointer/Space capture, key repeats or suppressed accessibility actions produce no
accepted record and no handler. Future pending events retain existing Core capacity.

## Existing paint capability: no Core extension needed

`cui_box` already has `custom_corners`, `corner_radii`, `custom_border_edges` and
`border_edges`. These become existing DrawCommand fields; adapters already render
them, as the segmented Toggle painter does. Add these four append-only fields to
Button options only and forward validated finite nonnegative values into its existing
box. Group derives flags and masks; no new DrawCommand, renderer, GPU or Core layout
API is needed. Border widths use the resolved Button border width, so ghost/link/text
styles do not accidentally gain an outline. All supplied per-edge widths must be
validated/clamped under the existing box integer-width contract.

For multiple horizontal children: first keeps TL/BL, last TR/BR, interiors no corners;
first retains all borders, subsequent children omit left. For vertical: first TL/TR,
last BL/BR, interiors no corners; subsequent children omit top. The allowed outer
radius is each child's resolved radius, rather than unconditionally theme.radius.
Zero-gap border masks avoid doubled separators. Count 1 receives no group override.
Source style/gradient limitations remain separately audited; this is not full GPUI
StyleRefinement certification.

## Exact ownership proposal and proof

* Acceptance slice: narrow `include/cui/cui.h` declaration and `src/core/cui.c`
  accepted event record/reset/query/validation only; new Core tests. Coordinate
  exclusive ownership after password Stage2 releases Core. No secret/config changes.
* Group slice: new Base header/source and styled source; Button declarations and
  Button-only painter/private metadata/tooltip helpers plus a new private header;
  new headless/browser tests and guide. No Checkbox/Radio/Switch changes. Integrator
  owns umbrella, CMake, WASI, amalgamation, export/manifest and gallery registration.
* Optional standalone callback slice: Button callback dispatch reads accepted records
  and invokes once per accepted event; same Boolean return. This is separate evidence
  from whether the Group dispatch is correct and needs dedicated before/after tests.

Acceptance tests must preserve zero-before-registration, two queued same-child clicks,
alternating children in opposite array/input order, current disabled/loading, partial
registration failure, cancelled capture/Space, repeat suppression, pending cancellation,
accessibility current role/scope rejection and modality before dispatch. Every accepted
record matches exactly one handler delivery; no raw-event heuristics.

Base group tests: 0/1/2/256 items, duplicate/missing/nonzero IDs, insufficient/invalid
workspace, begin/item/end misuse, controlled accept/reject, no implicit mutation,
single selected reclick, multiple ordered append/removal, source/default and all-input
policy, callback replacement, background click identity, stable reorder/removal and
focus cleanup, disabled/loading and current semantic checked/value coherence.

Paint tests: real laid-out children, 4 legacy and all 14 design sizes × both densities
× both orientations, mixed inherit/override, single untouched rounding, first/middle/
last corner and border command/pixel checks, no duplicate separator, all variants,
icons/loading/slots and text containment, focus/hover/pressed palettes and custom cursor.
Slots run exactly once per render; use final controlled model before drawing siblings.

Browser proof uses actual mouse, keyboard, wheel and CDP contacts on visible child
and background bounds, verifies selected semantics and actual separator/icon pixels,
callback counts/order, loading/disabled ancestor blocking, cancel/next click, clipped
and modal overlaps, and unaffected ancestor wheel/touch scrolling. Native queued SDL
events/cursors remain distinct from claims about physical devices or OS accessibility.
No production edit or new runtime success is claimed by this addendum.
