# Dialog slot composition V1: Base/Styled contract checkpoint

Status: proposal only; no Base/Styled implementation approved by this checkpoint.
Base: `6c69bb1d`. This replaces the withdrawn prior-frame/settling geometry in
`44e182d4`; it uses the landed Core contract in `docs/SCROLL_LAYOUT.md`.
The runtime proof remains `3d9d8544`; Core production originated at `b3e03bc9`.
Existing surface, options, events adapter and Root public structs do not grow.

Subsequent scrollbar design authority is `SCROLLBAR_TRACK_LAYOUT_V1.md`: real
fixed track/thumb nodes staged before the existing second calculation, with
independent visibility/interactivity policy. The earlier paint-hook and semantic
projection discussions below are preserved review history, not the recommended
final scrollbar representation. Its test-only proof does not approve Dialog
production or close the Root/host/five-facade scope.

## Authority and scope

CUI authority: `include/cui/cui_surface.h`, `include/cui/cui_surface_events.h`,
`src/base/cui_surface.c` (`cui_base_surface_update`, begin and body submission),
`src/components/cui_surface.c` (styled begin/footer/end and scroll skin), and
`src/base/cui_overlay.c` (focus capture/restore). Existing tests are in
`tests/unit/test_surface.c`; Core admission/failure tests are in
`tests/unit/test_scroll_layout_contract.c` and `test_scroll_layout_geometry.c`.

Pinned comparison: GPUI component 0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, `dialog.rs` render and
`base-dialog.rs` render/action handlers; `header.rs`, `title.rs`,
`description.rs`, `content.rs`, `footer.rs`, `dispatch_anchor.rs`, `root.rs`.
The earlier signed `acc877f24` audit records full paths and hashes. Required raw
files already exist locally; this checkpoint needs no dependency installation.
Source header/title/description/content/footer admit children and refinement;
body is the scrolling children region, footer remains outside it. Header/content
builders and body children are distinct. Source title/description visual content
must not be confused with accessible name/description. Source base actions veto
synchronously. Source Root defers confirmed close and closes cancellation
immediately; this C slice retains C's existing immediate close and observer order.

This slice supplies all six visual slots and owner-scoped C actions. Root/host
registration, source global Enter dispatch, source placement/animation fidelity,
and all five foreign builders are separate work. No claim of complete Dialog
parity follows from landing this slice alone.

## Concrete additive declarations

New portable header `include/cui/cui_dialog_composition.h` includes surface and
scroll headers. Names below are the proposed exported names, not existing APIs.
Enums use C enum parameters; record discriminants/masks use fixed uint32_t.

```c
#define CUI_DIALOG_COMPOSITION_VERSION_1 1u
typedef enum {
    CUI_DIALOG_HEADER, CUI_DIALOG_TITLE, CUI_DIALOG_DESCRIPTION,
    CUI_DIALOG_CONTENT, CUI_DIALOG_BODY, CUI_DIALOG_FOOTER,
    CUI_DIALOG_SLOT_COUNT
} cui_dialog_slot_kind;
typedef enum {
    CUI_DIALOG_CONFIRM, CUI_DIALOG_CANCEL, CUI_DIALOG_CLOSE
} cui_dialog_action_kind;
typedef struct cui_dialog_frame cui_dialog_frame; /* borrowed callback token */
typedef void (*cui_dialog_slot_fn)(cui_context *, const cui_dialog_frame *, void *);
typedef struct { cui_dialog_slot_fn emit; void *user; } cui_dialog_slot;
typedef struct {
    uint32_t fields;
    cui_color foreground;
    float font_size, padding, gap;
    uint32_t align_x, align_y, reserved;
} cui_dialog_slot_style_v1;
typedef struct {
    uint32_t struct_size, version, panel_fields, reserved;
    cui_dialog_slot_style_v1 slots[CUI_DIALOG_SLOT_COUNT];
    cui_color background, border;
    float radius, padding, gap, border_width;
} cui_dialog_refinement_v1;
typedef struct {
    uint32_t struct_size, version, present, reserved;
    cui_str name, description;
    cui_dialog_slot slots[CUI_DIALOG_SLOT_COUNT];
    const cui_dialog_refinement_v1 *refinement;
} cui_dialog_composition_v1;
typedef struct {
    uint32_t struct_size, version, mode, reserved;
    float width, requested_height, min_height, max_height;
} cui_dialog_sizing_v1;
typedef struct {
    uint32_t struct_size, version, reserved[2];
    cui_surface_state *surface;
    float outer_y;
    uint32_t reserved2;
    cui_rect completed[8];
    uint64_t last_submit_frame;
} cui_dialog_layout_v1;
typedef struct { bool valid, submitted; } cui_dialog_submit_result;

bool cui_dialog_layout_init_v1(cui_dialog_layout_v1 *, cui_surface_state *);
bool cui_dialog_composition_valid_v1(const cui_dialog_composition_v1 *);
bool cui_dialog_sizing_default_v1(cui_surface_options, cui_dialog_sizing_v1 *out);
cui_dialog_submit_result cui_base_dialog_compose_v1(
    cui_context *, cui_dialog_layout_v1 *, cui_surface_options,
    const cui_dialog_sizing_v1 *, const cui_dialog_composition_v1 *);
cui_dialog_submit_result cui_dialog_compose_v1(
    cui_context *, const cui_theme *, cui_dialog_layout_v1 *, cui_surface_options,
    const cui_dialog_sizing_v1 *, const cui_dialog_composition_v1 *);
bool cui_dialog_slot_style_v1_get(const cui_dialog_frame *,
    cui_dialog_slot_kind, cui_dialog_slot_style_v1 *out);
bool cui_dialog_slot_begin_v1(cui_context *, const cui_dialog_frame *,
    cui_dialog_slot_kind, cui_id wrapper_id);
void cui_dialog_slot_end_v1(cui_context *, const cui_dialog_frame *);
cui_interaction cui_base_dialog_action_v1(cui_context *, const cui_dialog_frame *,
    cui_id, cui_str name, cui_dialog_action_kind, bool disabled);
bool cui_dialog_action_v1(cui_context *, const cui_dialog_frame *, const cui_theme *,
    cui_id, cui_str name, cui_dialog_action_kind, cui_button_options);
```

Record masks: six `present` bits correspond to slot enum values; no other bits.
Slot fields are FOREGROUND=1, FONT_SIZE=2, PADDING=4, GAP=8, ALIGN_X=16,
ALIGN_Y=32. Panel fields are BACKGROUND=1, BORDER=2, RADIUS=4, PADDING=8,
GAP=16, BORDER_WIDTH=32. Align values are the public Core alignment values.
All reserved fields zero; exact struct_size/version required. Active floats finite;
font_size positive; padding/gap/radius/border_width nonnegative. Sizing mode is
Core AUTO or EXPLICIT and follows Core requested/min/max rules; width finite
positive. No infinity sentinel. Unknown masks/modes fail before callbacks.

Candidate LP64 layouts: slot=16, slot_style=32, refinement=232,
composition=152, sizing=32, layout=168 bytes. These are field-layout calculations,
not compiler measurements; implementation must measure LP64/ILP32 and add offset
assertions before declaring an ABI byte constant. No existing options ABI growth.
`completed` order: Panel, Header, Title, Description, Content, BodyViewport,
BodyContent, Footer. Absent parts clear their retained rectangles on submission;
completed geometry becomes usable only after a successful end_frame.
Outer/column ranges can be obtained through named IDs and Core scroll semantics.

## Callback and manual ownership

Compose is the sole owner of panel, structural boxes, focus scope and end balance.
There is deliberately no public manual top-level begin/end API in V1: the complete
closed tree must be registered with Core before compose returns. Each selected
callback runs once in declared order. No layout/measure pass re-enters application
composition. Text measurement can repeat, exactly as documented by Core.

The frame token is valid only during its callback; it borrows the layout, options
and resolved styles. A callback may emit arbitrary ordinary CUI children, including
nested dialogs with a different layout/state, and manually balanced slot wrappers
using begin/end above. A wrapper styles the requested slot using an explicit
caller ID; it does not invoke another slot callback or change slot presence.
No automatic anonymous wrapper is silently substituted for a failed caller ID.
Manual wrappers cannot span callbacks. Leaked boxes, reused owner/layout in the
same frame, structural IDs reused by callback children, and foreign frame tokens
are errors. Public helpers cannot make arbitrary malformed application trees safe.

Absent bit selects fallback; present+NULL callback means deliberately empty.
Header fallback emits Title then Description; custom or authored-empty Header
suppresses both automatic children, and simultaneous Header plus Title/Description
presence is rejected. CONTENT fallback empty; BODY fallback empty; FOOTER fallback
empty. Title/Description fallback renders semantic strings only when nonempty.
Custom visual content never replaces `name`/`description` implicitly. Header,
Content, Body and Footer are invoked in that order; Title/Description callbacks
occur inside fallback Header. Title/Description wrappers remain independently
styleable. Theme defaults resolve before panel/slot explicit fields; Base uses
neutral unpainted defaults and never fabricates a theme. BODY padding applies
inside BodyContent, never to the zero-padding structural clip/frame.

## Geometry, constraints and bars

New named parts via existing public `cui_surface_part_id`: preserve Close=1,
Confirm=2, Cancel=3, Title=4, BodyViewport=6, BodyContent=7; propose Header=9,
Description=10, Content=11, Footer=12, OuterViewport=13, Column=14,
BodyFrame=15, Mount=16. Validate all full IDs and folded aliases using Core's
normal rules, including custom child collisions; no copied hash or allocator.

Mount is a transparent portal over options.viewport inset by options.margin;
Panel (owner ID) is its ordinary centered child, with current intrinsic height.
The existing backdrop remains separate. Panel has fixed constrained width
`min(sizing.width, usable_width)` and may have padding. Nonpositive usable extent
fails instead of inventing a readable spacer. Final current-frame centering follows
normal layout of Mount; no previous height, guessed height or first-frame blank.
This preserves C's centered Dialog placement. GPUI's preferred y=viewport/10,
layer offsets and oversized snap-up policy remain an explicit placement gap,
not a claim of this composition API or an alteration of the old route.

Exact Core chain: Panel -> OuterViewport -> Column -> BodyFrame ->
BodyViewport -> BodyContent. Outer/Viewport have only the named flow child;
BodyViewport attaches to BodyFrame with inherited clip, GROW at offset zero.
Header, Content and Footer are fixed intrinsic siblings of BodyFrame in vertical
Column. Outer/BodyViewport clip; Column/Frame do not; structural clip/frame
padding zero. BodyContent FIT Y, width fixed from the constrained width minus
resolved horizontal padding. GROW descendants under a width-constrained ancestor
are allowed; height-dependent widths, percentage/aspect feedback and reserved
scrollbar gutters are not admitted. Ordinary nested independent-width constraints
follow Core validation, not a general constraint closure promise.

Let P be panel vertical padding, F measured fixed regions plus gaps and I current
intrinsic BodyContent height. AUTO H=clamp(P+F+I,min,max); EXPLICIT H=requested.
Effective max=min(sizing.max_height, usable viewport height); reject min>effective
max, or EXPLICIT outside that interval, before composing. Styled convenience
sizing initializer (proposed `cui_dialog_sizing_default_v1`) selects width420,
AUTO/min0/max usable height. A source-like minimum96 must be explicit, not an
unconditional spacer for short content. Base requires a sizing record.
If F<=H-P, Body receives remaining height. Otherwise Body receives I and the whole
Column scrolls in OuterViewport, preserving reachable fixed controls. This is
Core's admitted oversized-fixed-region policy, not footer clipping or silent
header/footer shrinkage. Empty Body has intrinsic zero; authored-empty callbacks
still run exactly once. Both scroll offsets clamp using current final dimensions.

Overlay bars only: no reserved width feedback. NULL options.scrollbar_style
resolves theme for Styled; Base emits no paint. Explicit invisible style omits
paint while scrolling and semantics remain. Body/outer bars appear only for a
positive current range; omit Body bar in the oversized-fixed policy when its body
is fully exposed. Current final rectangles/ranges are read at one registered
prepaint paint hook, never from last-frame scroll-skin geometry. No model mutation,
input replay, semantic duplication or child composition in that hook. Its bounded
frame descriptor must live through end_frame. The existing legacy scroll skin
remains untouched. Validate hook/resource capacity before application callbacks
where possible; a later failed opt-in frame follows Core failure semantics.

## Owner actions and close compatibility

A new action helper uses the ordinary Button reducer/activation and existing
surface decision and close functions; no action queue, dispatch graph or global
Enter interception. CONFIRM->ACCEPTED, CANCEL->CANCELLED, CLOSE->CLOSED (C legacy
reason). On accepted activation, decide runs synchronously in the same callback;
veto leaves open. Acceptance closes immediately, restores focus, then on_close,
matching existing C order. Consumed events, disabled controls and held key repeats
follow ordinary Button rules. Enter activates only the focused Button. There is
no source global confirm or implicit default action in this slice.

Important implementation requirement: refactor the private surface update reducer
into common dismissal/scroll handling plus optional legacy reserved-action handling.
Existing public update/begin call it with legacy handling unchanged. New compose
calls it with reserved action handling disabled; default/custom helpers own their
Button activation, so parts1/2/3 cannot trigger a second decide before helpers run.
This is a private reducer split, not a new input dispatcher. Pending public
request_close still resolves at update entry with existing next-update timing;
escape/outside still resolve there. Imperative surface_close still bypasses veto.
New helper closure does not stop already-entered callbacks from balancing boxes;
subsequent helpers cannot re-close a closed owner. No retry or replay after veto.

Default visible Close uses part1 and helper CLOSE, honoring hide_close and Alert's
show_alert_close. No automatic confirm/cancel buttons: authored Footer owns those.
Custom actions may use parts2/3 or distinct IDs but never a structural ID; kind is
explicit, not inferred from numeric ID. Alert uses identical slot topology and
existing Alert dismissal defaults; no implicit change to old alert_dialog helper.
No action helper is valid outside an active owner callback. During decide/on_close,
retain existing events adapter restrictions on reentrant mutation/destruction;
no stronger observer rollback guarantee is introduced. Existing observer wrapper
options can be passed unchanged. Synchronous foreign veto is not implemented by
this C helper; its language-specific exception/reentrancy contracts need review.

## Lifetime, allocation and failure

Layout is new caller-owned stable storage, one per surface; initialize while
closed, do not copy/move while a frame borrows it. Surface and layout outlive
end_frame and any attached events adapter. `outer_y` and surface.body_scroll are
borrowed writable Core clamp destinations. Retained completed rectangles live in
layout, never callback stack. Records/callback descriptors/refinements are borrowed
only for the compose call; emitted strings use ordinary Core text lifetime rules.
Frame-only style/paint copies must use bounded Core frame storage, not retained
callback pointers. No process-global active layout or hidden application heap.

Per compose consumes one Core body link/two clamps (32/64 global frame caps).
No per-dialog permanent heap; caller layout168 candidate bytes plus existing state.
Implementation must measure frame descriptor/paint-hook storage and resource cost;
reserve before callbacks and report capacity failure rather than truncate slots.
No numeric allocation claim beyond the landed Core7216-byte lazy plan is proven
here. Closed owner returns valid=true/submitted=false without callbacks/resources.
Invalid descriptors fail with INVALID_ARGUMENT before user callbacks; malformed
submitted topology/fold collisions or Core resource failure invalidate the frame.
Return submitted is admission only, not final geometry success. A failed opt-in
end_frame has non-OK error, empty draw and no prepaint/valid snapshot guarantee;
no stale completed rectangles may be advertised as current. Error clearing within
the same frame cannot salvage it. Clear/rebuild next frame recovers. Application
callback effects and delivered input/observer calls are never rolled back.

Teardown needs no new layout destructor: close through existing API, finish any
borrowed frame, detach event adapter under its existing closed-only contract, then
discard layout/state. Clearing or reinitializing only layout while open is invalid.
Reopen resets outer/body offsets to zero in the new route; legacy reopen behavior
stays unchanged. Switching old/new routes on an open surface is a caller contract violation: close
first and reinitialize explicitly. Same-frame duplicate new-route submissions are
diagnosed; without growing legacy state this V1 does not promise detection of
every cross-frame call to an old API. Do not claim an unavailable route registry.

## Independently reviewable implementation and proof

First slice: new Base/Styled header/implementation, private reducer split, bounded
current-geometry bar hook, C example, docs/metadata and all generators. No Root,
native host or bridge ABI changes. Keep legacy public structs and old begin/end,
footer and alert helper bodies behaviorally unchanged. No new Core/vendor phase.
Root/host follow-up owns mount/removal, focus restoration and confirmed deferred
close compatibility; it cannot be represented as completed by this C slice.

Required C tests: callbacks-once (including nested dialogs and present-empty),
rich/custom Header/Title/Description and semantic names, Content versus Body versus
Footer order, refinement defaults then overrides, first-open short AUTO and long
AUTO, narrow/wide wrapping and height resize, explicit/min/max and invalid sizes,
oversized fixed controls reachable by outer scroll, same-frame shrink clamp with
draw/hit/semantics/caret, empty/hidden/overflow bars, inert/measurement no offset
commit, allocation/capacity failure and next-frame recovery, late retained aliases,
custom structural collisions, nested focus and independent owner actions. Test
same-frame decide/veto once, close reasons, focused Enter/no global Enter, disabled
and consumed events, deferred public request_close, observer-after-focus order.
Legacy oracles compare old begin/footer/alert draw geometry, event timing, semantics,
focus and state transitions before/after the private reducer split. Headless and
portable C11; native wrapper/private Xvfb only; no claim from arithmetic-only tests.

Second slice: Root/native host sidecar and real ownership/teardown tests, separately
reviewed. Third slice: bridge plus Python, JavaScript, TypeScript, Go, Rust typed
slot trees and owner actions. Preserve each runtime's rollback and app-selected
library authority, old missing-symbol loads, copied bounded node state and node
reuse. Native C callbacks are not foreign synchronous veto support. Before that
slice, specify NAPI/cgo/Rust/Python callback lifetime, exceptions/panics, reentrancy
and thread restrictions with tests; never substitute queued foreign veto. Typed TS
must match JS. All five remain planned, not implicitly satisfied by C examples.

## Review correction: two viewport paint owners, no cross-part bounds query

The single-hook wording above is withdrawn. `cui_bounds` checks previous semantic
bounds first; retained tracking publishes after prepaint. Neither is a current
cross-part geometry seam. Proposed replacement needs no new public API:

* Register one local prepaint callback on OuterViewport13 and one on BodyViewport6,
  while each actual box is open. Each uses only its own `info.bounds`, `info.clip`
  and `info.visible_bounds`. Never use `cui_bounds` or retained rectangles there.
* Submit a current scroll semantic for each viewport during ordinary composition.
  In each callback call public `cui_semantics(ui,&count)` and copy the matching
  own-owner details.scroll_y/scroll_max_y to locals. Do not retain the returned
  array or use its bounds, parent links or eligibility as completed snapshots.
  Require exactly one matching owner with finite nonnegative range and finite
  offset in range; otherwise return false and diagnose an internal composition
  invariant before advertising a successful frame (implementation must choose
  existing error reporting, not silently hide mandatory missing semantics).
* Current phase authority: `cui_core_scroll_layout_end` commits both clamp ranges
  to ui->semantics before returning to end_frame; then
  `cui_core_prepaint_prepare` invokes callbacks. `cui_semantics` directly returns
  that current array and has no phase ban. This is a read-only current submitted
  semantic view, not final accessibility publication: semantic filtering/bounds
  assignment and previous snapshot commit occur later. The new consumer guide
  should explicitly document this narrowly tested read contract; no generic
  current-part geometry query or observer is introduced.
* Infer own content height only as own viewport height + its current maximum;
  compute thumb with public cui_scrollbar_thumb_ex. Own zero range means no paint.
  The body's zero range naturally omits its bar in oversized-fixed-region mode.
  Local stream clips to own visible bounds; no deferred paint or caller clips.

Proposed copied payload (private component type, not public ABI):

```c
typedef struct {
    cui_context *ui;                 /* borrowed through end_frame */
    cui_id owner;
    cui_scrollbar_style style;       /* full value copy, no pointers */
    float opacity;                  /* ordinary-submission fade result */
} cui_dialog_bar_paint;
```

Candidate LP64: offsets0/8/16/60, sizeof64, align8. Style candidate sizeof44
(enum4, five bools, padding3, six floats, two colors). Verify these with actual
compiler before implementation handoff; ILP32 ABI alignment is not assumed.
Two payloads=128 bytes plus at most30 alignment bytes (Core max_align_t=16 candidate) in existing frame storage;
no tail, theme pointer, layout pointer or retained semantic pointer. Two hook
registrations consume two max_elements prepaint slots and reserve eight command
units total: per hook at most track RECT + thumb RECT + two Core-owned clips.
Hidden/zero-range paint emits nothing; no callback-driven budget expansion.
Existing Core per-element prepaint metadata/command backing is already allocated
by configuration; these consume capacity, not an additional persistent allocator.
Measure metadata sizeof and actual frame-copy accounting in the test fixture.
The hook never invokes opacity reducer or application callback: resolve skin and
existing fade state during normal submission, copy opacity, then geometry alone
changes at prepaint. VISIBLE is the deterministic first-open proof case; AUTO_HIDE
must separately test existing fade scheduling/hover policy, without claiming that
this geometry seam changes that policy or adds current-frame scrollbar input.

Prototype required before production approval: a TEST-ONLY public-API fixture on
the landed Core tree, two hooks with copied payloads, persistent tracked bounds,
current semantics and recorded emitted RECTs. After first-open long AUTO, resize
narrow/wide and shrink height/body from the bottom. Assert each hook's bounds
against its post-end tracked viewport and range against final semantics; assert
thumb geometry from those exact values, and that a previous-geometry sentinel
would produce different expected output. Include first-open short/empty (no bars),
oversized fixed regions (outer only), nested owners, hook/payload capacity failure,
no duplicate composition or input effects, and failed Core frame (zero hooks).
Do not count a static phase audit as runtime proof. No fixture has been run for
this checkpoint; compiler slot remains with root until explicitly released.

Reducer correction is intentionally narrower than a generic policy parameter:
only the three activation comparisons for parts1/2/3 are gated off for new
compose. Pending request processing, Escape, outside pointer, scroll, consumed
checks, focus restore and observer ordering retain their existing statements and
order. Legacy update/begin pass that gate enabled. Tests must compare both routes
for dismissal/observer timing and prove helper-owned activation is decided once.

### Test-only prototype results (portable Linux, pending production review)

`test_scroll_layout_geometry unused --probe-dialog-bars`, registered as
`dialog_scrollbar_prepaint_probe`, exercises the proposed public seam. The hook
itself uses only public APIs; existing test-only Core instrumentation separately
measures storage and confirms no resource/input/text mutation across layout.
No production source, public header or vendor change accompanies this prototype.

Measured LP64: copied payload64, shared style44, existing Core prepaint metadata192
bytes per registration. Two hooks consume384 bytes of existing metadata capacity,
128 payload bytes plus actual max_align_t padding, and reserve8 command slots.
The parent fixture's18 registrations used1339 frame bytes including187 alignment
bytes (not a constant per-frame allocator charge); two payloads can require up to
30 padding bytes when max_align_t alignment is16. Caller geometry storage remains
persistent through end_frame. Test payload stack is overwritten after registration
and the copied data still drives the final paint correctly.

The prototype emits track/thumb RECTs, checks their final geometry against
post-end retained viewport bounds, and reads each own current semantic range at
prepaint. Long first-open, narrow/wide and height resize, bottom-offset shrink,
short/empty Body, oversized fixed Header and nested independent owners exercise
both bars. The independent range oracle checks final tracked content-minus-
viewport dimensions and committed offsets. Final command alpha is50 for original
alpha200 × fade.5 × ancestor.5, and25 beneath two .5 ancestors. The payload never
multiplies prepaint_info.opacity; Core performs inherited opacity exactly once.
This is a draw-command alpha proof, not a raster/text/GPU comparison.

Command reservation exhaustion and frame-copy byte exhaustion reject registration,
produce no partial callback output, and recover the next frame after error clear.
A failed Core constraint frame invokes no prepaint callback. Ordinary composition
counts remain once per callback. Nested test marker colors were initially shared
and caused a parent-vs-child oracle mismatch; distinct markers fixed the fixture
without changing expected geometry or tolerances. No production fault was inferred.
AUTO_HIDE fade scheduling/interaction, production skin resolution and full Styled
integration remain unimplemented and unproven by this geometry prototype.

Validation on this checkpoint: focused warnings-as-errors Release target build,
then 4/4 selected CTests passed through private Xvfb, including the original
current-font geometry/AUTO and draw/hit/semantic/caret clamp tests. Reproduce in
an existing configured portable build (no native windows):

```sh
nice -n 19 taskset -c 0 cmake --build BUILD --target test_scroll_layout_geometry --parallel 1
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir BUILD \
  -R '^(dialog_scrollbar_prepaint_probe|scroll_layout_geometry|scroll_layout_scroll_resize|scroll_layout_auto_size)$' -V
```

Instrumentation helper tests2/2, installed skill tests18/18, canonical skill
freshness, clang-format23.1.2 on changed C/header files and diff checks passed.
No whole-library rebuild, Windows/browser/GPU run or production API claim.

## Input review addendum: two owners and a remaining bar-hit seam

Static authority: `src/base/cui_scroll.c` functions
cui_base_scrollable / cui_base_scrollable_finish / cui_base_scrollbar_overlay /
cui_base_scrollbar; `src/core/cui.c` cui_core_touch_owner, cui_core_touch,
cui_core_scroll_hit and cui_defer_scroll_event; public `cui_scroll.h` and
`cui.h` comments. No input prototype or production change is included here.

### Wheel, touch and nested consumption proposal

Both OuterViewport13 and BodyViewport6 are actual semantic scroll owners. Each
uses vertical-only public cui_base_scrollable with hide_scrollbars=true before
its children, and cui_base_scrollable_finish after its children. Fixed header,
Content and Footer sit inside Outer, so their overflow is reachable. Body is a
semantic/layout descendant of Outer; nested callback-owned scroll owners remain
ordinary descendants. Do not give Outer an explicit parent that skips the modal
scope, and do not flatten nested parents. Core publishes final ranges from the
registered two clamps, as already proven by the paint prototype.

Input options use the *last successfully completed* retained viewport and content
extents, wheel_step48, vertical=true, horizontal=false, axis_lock=false,
gesture_lock=false, trap_horizontal=false, disabled=false. First-open has no
previous hit region; zero-range placeholder geometry must not advertise an
interactive scrollbar. Previous input geometry is intentionally allowed; it is
never passed to the current paint hook. Nonfinite stored model values are handled
under the Core registration error contract, not silently repaired after admission.

Use one stack-local cui_scroll_state per owner for wheel reduction, initialized
from its stable model Y (surface.body_scroll or layout.outer_y), with x=0 and no
persistent drag flags because hide_scrollbars=true. Copy Y back after begin and
finish; Core registration borrows the stable model destinations, never this stack
state. If an overlay drag helper changes Y between begin and finish, synchronize
the stack reducer Y from the model before finish so finish cannot overwrite it.
Update each clipped box via cui_set_box_scroll_offset after finish. The ordinary
Base helpers may invalidate on changes as appropriate; no event replay in layout.

Core input chooses a previous-frame eligible scroll hit and, for touch, a bounded
contact owner. Touch threshold/cancellation, capture and precise pan conversion
remain Core's existing path; Dialog adds no raw touch handler. A child wheel that
changes its value is consumed by the existing Base reducer. An unchanged child
calls public cui_defer_scroll_event to an eligible ancestor; the parent's finish
runs after the child, processes only still-unconsumed events now targeted to that
parent, and may defer onward. Do not loop until exhaustion or replay residual
partial deltas: existing Base behavior consumes the whole event if any value
changes. Core may route directly to Outer when Body cannot scroll in that direction.
Touch scrollbar priority is already role-based (SCROLLBAR suppresses competing
ancestor pan); no new touch dispatcher is proposed. Platform gesture-lock limits,
modal clipping and previous-frame ancestry remain exactly the Core contract.

A necessary correction to the earlier reducer gate: gating ONLY reserved action
parts1/2/3 while retaining the legacy Body6 wheel execution in the new route would
double-apply Body scroll before Base reduction (the old wheel branch does not
consume). That arrangement is rejected. New compose must select a private reducer
route with **both** legacy reserved-action reduction and legacy Body-wheel reduction
disabled, because public Base owns those operations there. The old public
update/begin route executes the existing wheel statements, arithmetic and ordering
unchanged. Pending request_close, Escape, outside dismissal, focus and observer
statements retain their exact ordering on both routes. This additional route gate
needs explicit review; it is not permission to change legacy wheel semantics or
move observer timing. Do not run the old branch and restore the value afterward:
a close observer could already have observed the transient mutation.

Retain OuterViewport/Column bounds separately for input. Amend the proposed NEW
layout record's completed array from8 to10 (append OuterViewport and Column),
adding32 caller bytes: computed LP64 layout200, not168. No old public struct grows.
Do not use raw cui_bounds(Column) mid-submission as a guaranteed retained snapshot.
A failed completed frame does not refresh the last-successful input geometry;
there is no valid current UI snapshot to interact with until recovery.

### Visible overlay drag: public reducer reuse and precise limitation

After each owner's Base begin and before child composition, call public
cui_base_scrollbar_overlay with that owner's previous positive viewport/content
extents, vertical-only options, a copied/resolved VISIBLE or AUTO_HIDE skin, local
x=0 and stable modelY. The helper owns persistent dragging/grab state in the
context via its existing resource key; no private resource lookup by Dialog.
Use public cui_scrollbar_id(owner,true) for its semantic ID. The helper handles
track page click, thumb grab/move/release, keyboard Home/End/Page/arrow, primary
button selection and CANCEL through existing cui_base_scrollbar. Hidden bars still
run the scroll-owner begin/finish path but reset/omit overlay controls through
existing helper policy; wheel/touch scrolling stays enabled. Opacity is computed
in ordinary submission only and copied as fade, never ancestor opacity. Do not
call the old overlay paint helper with previous dimensions; current paint remains
the two proven own-viewport hooks.

However this reuse alone is NOT a complete visible control. The Base overlay
helper submits scrollbar semantics but creates no layout box. Its drag reducer
uses cui_bounds(bar_id), i.e. previous completed bar bounds. The old Styled overlay
paint helper creates fixed geometry boxes during submission. A prepaint RECT does
not create a semantic hit region. Emitting those old fixed boxes from retained
geometry would leave the NEXT hit snapshot stale relative to the current painted
bar after resize/shrink. That is beyond ordinary previous-frame input routing:
next-frame input should hit the bar actually painted in the completed frame.

Existing public absolute boxes allow parent-fill height, but the admitted Core
scroll layout explicitly rejects nonregistered floating descendants with nonfixed
width/height (`cui_core_sl_floating`). It also has no public right-edge attachment
or semantic-only current hit-rectangle setter. A nonflow GROW track is therefore
not a currently admitted shortcut, and a fixed previous-height track is not
current geometry. Prepaint cannot submit semantics/layout or mutate drag state.
The passing prototype proves current paint, not this missing hit projection.

The remaining narrow seam for review is a bounded opt-in **scrollbar semantic
projection associated with an already registered viewport clamp**, not a generic
observer or arbitrary hit rectangle API. It would associate public-derived bar ID
with its viewport, resolved thickness and visible/fade eligibility; compute the
current right-edge track from final viewport bounds; publish current clipped bar
bounds, owner parentage and maximum/value to the completed semantic hit snapshot;
and expose that same projected hit region to prepaint occlusion. Next input then
uses that completed bar via the unchanged Base overlay reducer. Zero range or
hidden/zero-opacity eligibility withdraws the control and capture according to
existing disappearance rules; current paint is omitted as today. Inert/layout-only
and errored opt-in frames must not publish it. Projection must not compose a
callback or dispatch input, and must not update state during prepaint.

This requires a separate concrete Core contract/implementation review: final
semantic bounds alone are insufficient if current prepaint occlusion still uses
the viewport-wide layout node. Both consumers must use the same clipped projected
region; no fake passive whole-viewport hitbox, private Clay access from components,
second action node, or source ID hash copy. Capacity must reserve at most two
projection associations per composed Dialog, with full/folded alias validation,
atomic registration and resource budget before callbacks. Exact record/storage
and phase placement are intentionally not invented as approved APIs here.

The no-new-Core alternative is an explicitly limited wheel/touch-only Dialog with
noninteractive decoration bars. It does not satisfy the requested full visible
bar controls and is not the recommended completion claim. Therefore Base/Styled
production remains gated on the narrow projection decision (or a separately
proved currently admitted layout representation), even though the two-owner
wheel/touch route can reuse the existing public reducers.

Required follow-up proof: queue real wheel and touch frames against completed
snapshots; long Body versus oversized fixed Outer; nested child consumes, edge
defers and outer consumes once; hidden bars preserve scrolling; track click and
thumb drag/cancel/release for BOTH owners; current resize and shrink while dragging;
painted track equals completed hit region and next-frame target; no stale bar after
range vanishes, inert/failed frame publication and recovery; legacy surface wheel
and observer byte/state/timing oracles unchanged. All Root/host/global-Enter/source
placement and five-facade gaps listed above remain open.
