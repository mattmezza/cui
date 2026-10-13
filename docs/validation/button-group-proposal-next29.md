# ButtonGroup source contract and implementation proposal

Read-only audit base: `021d553c23d066473db0b8e2bc67ef9ab525efb0`.
Pinned reference: v0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, local checkout
`/tmp/cui-gpui-reference`. The public contract below comes from
`crates/component/src/button/button_group.rs`, not from unrelated stories.
No production implementation or completion certification is included.

## Exact source surface

The group owns an element ID, generic style refinement and an ordered collection
of actual Buttons. Its public builders are `new`, `child`, `children`, `multiple`,
`layout`, `compact`, `outline`, `on_click`; trait methods are `Disableable::disabled`,
`Sizable::with_size`, `Styled::style`, and `ButtonVariants::with_variant` and its
variant conveniences. Defaults are horizontal, single selection, enabled,
noncompact, nonoutlined, with no group size or variant override and no callback.
Size supports XSmall/Small/Medium/Large and explicit pixels (`sizing.rs:6`).
There is no group loading builder, density builder, cursor builder, roving focus,
arrow navigation, mandatory selection, selection limit or error state.

Source render lines 153–264 establish these concrete contracts:

| Feature | Pinned behavior | CUI status at audit base |
| --- | --- | --- |
| Children | Ordered Buttons, retaining labels, icons, slots and Button behavior | Button available; distinct ButtonGroup absent |
| Selection | Read each child's `selected` at render, advertise it with `toggled(selected)` | Button has separate selected paint and toggle semantics |
| Single click | Request exactly `[clicked_index]`, including clicking an already selected child | No group collection API |
| Multiple click | Scan selected children in ascending index order; remove clicked selected index, otherwise append it | ToggleGroup uses independent booleans; no equivalent ordered result contract |
| Controlled ownership | Callback receives request; group does not persist or mutate the application's selected state | Controlled ToggleGroup exists, but is a different control |
| Horizontal shape | First has both left corners; middle has none; last has both right corners | Toggle segmented composition exists; Button has only uniform rounding |
| Vertical shape | First has both top corners; middle has none; last has both bottom corners | ToggleGroup is horizontal only |
| Shared border | First all edges; later horizontal children omit left, later vertical children omit top | Toggle painter supports per-edge borders; Button does not |
| One child | No corner/edge override | Must preserve ordinary Button shape |
| Size/variant | Override only when group option supplied, otherwise retain each child | All Button options available; group override absent |
| Compact/outline | Group true enables on every child; false does not erase child true | Button fields available |
| Child callback | Preserved when group has no callback; replaced when group has callback | Button callback available; group replacement absent |
| Group callback | Enabled group listens to a bubbled click; vector is the request above | Absent |
| Collection background | Root click with no recorded child returns the rendered selected indices unchanged | No equivalent collection click target |
| Focus | Ordinary child Button focus, Tab and Enter/Space; no group focus algorithm | Core/Button supports this; no new roving requirement |

`ButtonVariant` has Default, Primary, Secondary, Danger, Info, Success, Warning,
Ghost, Link, Text and Custom (`button.rs:146`). CUI additionally has its compatible
legacy variants; a group should reuse the existing enum rather than introduce a
second palette system. Source group inherits individual Button rendering, including
loading/icon behavior, focus, disabled paint and slots. It does not add an equal-width
rule, wrap algorithm or spacing builder. Generic source style refinement can change
layout; that unrestricted GPUI style system is not equivalent to a small portable
options struct, and must not be silently certified as implemented.

## Source quirks that need an explicit policy

The pinned tests deliberately name these legacy behaviors:

* Lines 346–359: installing a group callback replaces the child callback; without
  it, the child callback survives.
* Lines 362–370: initial selected child 1 produces single `[0]`, multiple `[1,0]`
  when child 0 is clicked. This order is observable; a sorted output would differ.
* Lines 373–378: keyboard activation does not reach the group callback. The child
  callback was replaced with a state-recording closure, so it also does not reach
  the application's original handler. This is not evidence for keyboard parity
  of a new selection-changing callback.
* Lines 381–423: group disabled state depends on builder order. `disabled(true)`
  before `child` disables that child; after `child` suppresses only the group
  callback and leaves its child callback runnable. `children` extends the vector
  without applying the `child` method's disabled propagation. `child` also writes
  the group's false disabled value over an individually disabled child's value.

These are actual source behavior, not desirable portable API requirements. Propose
consistent effective disabled = group disabled OR child disabled/loading, for
pointer and keyboard, irrespective of array construction order. This is an explicit
source divergence and must be approved and documented. Loading is child-local;
there is no need to invent a group loading API.

The render-local `Cell<Option<usize>>` also retains the last child index and is not
reset after a callback. Thus a later background click before rerender can reuse that
index, and a keyboard activation can set it even though the group callback did not
run. This follows the implementation, not a focused upstream test. The proposal
below intentionally identifies each actual background activation separately and
requests unchanged selection instead of reproducing this stale-index behavior.

For keyboard, expose a named policy with zero/default `SOURCE_POINTER_CALLBACKS`
and opt-in `ALL_VALIDATED_ACTIVATIONS`. The source policy preserves the legacy
no-group-callback result for keyboard with a group handler; the usable policy
requests the same selection transition for Enter/Space and accessibility activation.
Keep ordinary child Tab stops; a RadioGroup's single tab stop and arrow algorithm
would be an invented ButtonGroup requirement. No pointer-focus restoration hack is
needed; retain the Button's existing per-child policy and Core capture barriers.

## Smallest proposed public API

Use a new portable Base header with Core-only dependencies and a separate styled
wrapper. Names below are a proposal, not declarations already present:

* `cui_button_group_item`: stable nonzero child ID, accessible label, disabled and
  loading inputs. Styled items additionally carry the existing `cui_button_options`.
  Group ID and all child IDs must differ; duplicate IDs reject before callbacks.
* Controlled `bool *selected` array is always supplied by the caller. A callback
  receives a borrowed, synchronous `const size_t *requested_indices`, count,
  activated stable ID/index and user pointer. Applications accept by updating their
  selected array, or reject by leaving it unchanged. The group never applies a
  hidden default mutation. Without a group callback, ordinary child handlers run
  and selected state stays caller-owned.
* Caller supplies request scratch of at least item count plus bounded event scratch
  for pending activations. Reject null/overlapping storage, insufficient capacity,
  invalid IDs/enums/nonfinite geometry and invalid strings before callbacks. No
  per-frame heap allocation or retained platform handles. Propose an explicit
  initial maximum of 256 items; document it independently of Core context capacity.
* Base options: multiple, effective disabled, orientation and callback policy.
  Styled options: those plus optional size, design-size and variant overrides,
  compact, outline, accessible collection name and explicit finite layout bounds.
  Separate override booleans are needed because zero-valued medium/default variants
  cannot mean both inherit-child and override-child. Density remains theme-owned.
* Return a result separating validated activation count from request callback count;
  it must not call a rejected controlled request a model change. Preserve source
  background clicks on explicit collection bounds: callback index `SIZE_MAX` and
  activated ID = group ID request unchanged selection. An empty collection can
  request `[]` when its explicit bounds are clicked; intrinsic zero-area empty
  collections have no hit. There is no background keyboard tab stop.

For each request, enumerate the currently accepted model in ascending index order,
then perform remove/append or replacement exactly as source does. A second queued
activation starts from the model accepted by the preceding callback. If that callback
rejects, the next request starts from the unchanged model. This transactional policy
avoids hidden optimistic state. Source tests prove only one event per render; queued
multi-event behavior is a deliberate portable contract requiring new tests.

Reduce and register all child behavior first, deliver accepted events in input order,
then paint all children from the final controlled model. This avoids stale selected
pixels for already-built siblings and keeps semantic checked/value coherent in the
same frame. The collection itself is not a focusable duplicate button; its optional
  explicit background bounds are pointer-activatable without focus. Children
retain Button/link roles with explicit toggled metadata. No invented radio role or
group activation target. Callback payloads/scratch are frame-local and cannot escape.
Callback reentrant frame submission must follow existing Core phase restrictions.

Styled composition needs narrow Button paint overrides for per-corner radii and
per-edge borders, with the source horizontal/vertical first/middle/last masks above.
Implement these once in Button's painter and reuse its palette, hover/pressed/focus,
icons, loading, slots and custom cursor. Do not emulate Buttons with Toggle paint.
Do not claim full Custom style or gradient parity: the Button completion audit still
records source style limitations that a wrapper cannot solve.

## Separate queued activation callback multiplicity gap

`cui_interact` (`src/core/cui.c:2000`) visits all pending activations for an ID but
collapses accepted activations into one boolean and clears their slots. Styled
`cui_button_ex` calls `on_click` once when that boolean is true. The prior audit's
two-click single-frame probe observed two validated event activations but one
callback. This is separate from inert ancestor blocking, which is already fixed.
Pointer presence, capture, clipping, modal eligibility and cancellation must remain
Core-owned; counting raw UP events in a widget is not an acceptable fix.

Propose an append-only accepted `activation_count` in `cui_interaction`, computed
by the existing acceptance loop. Existing `activated` remains any-count for source
compatibility; `cui_button_ex.on_click` delivers once per accepted activation, not
once per frame. Return boolean still means at least one. If a first callback changes
external disabled/loading state, subsequent queued activations are already accepted
against this frame's submitted inputs and still deliver; they affect subsequent
frames. Document this deterministic snapshot contract, rather than rereading arbitrary
callback-owned options.

A chronological Group needs accepted event identity as well as counts. Propose a
bounded Core event query recording the accepted target after `cui_interact` validates
and consumes each activation slot (`cui_event_accepted_activation`). It returns zero
for pending, cancelled, inert, rejected accessibility-role/scope or unsubmitted
targets. It is readable during the frame after all children register. This avoids
using `cui_event_activation`, whose contract describes generated pending activations
before widget consumption, as if it guaranteed acceptance. Retain fixed existing
event capacity, no allocation, and clear records at frame start. This seam needs
separate exclusive Core ownership approval and meaningful Core tests before use.

The queued test matrix must distinguish two clicks on one child, alternating children,
Enter/Space/accessibility, repeated key suppression, cancelled pointer/Space,
disabled/loading transition, modal/clipped release, rejected controlled requests and
callback replacement. Each accepted activation delivers exactly one eligible handler,
in event order, with no child+group duplicate. No handler runs for cancelled events.

## Implementation and proof slices

1. Core accepted count/event evidence plus Button callback multiplicity, isolated
   from group drawing. Own narrow `cui.h`, `cui.c`, Button callback region and new
   focused tests; preserve Boolean compatibility and current blocker/presence APIs.
2. New Base group reducer/header and new styled group wrapper, narrow Button painter
   corner/edge additions and new tests. Integrator owns CMake, umbrella, WASI,
   amalgamation, exports and manifest. No Core changes by the group worker afterward.
3. Actual native and WASM geometry/callback proof before any completion claim.

Headless proof must include 0/1/2/many children, both orientations, first/middle/last
corners and one-pixel nonduplicated separators, mixed child sizes when no override,
4 legacy and all 14 user design sizes in comfortable/compact density, every group
variant with selected/unselected, content/icon/loading geometry and disabled paint.
Test default selection, repeated selected clicks, multiple removal/append order,
accept/reject, sibling semantic/pixel coherence, item reorder/removal and focus cleanup,
group/child disabled and loading, callback ownership, Tab/Enter/Space policy, mouse and
touch cancellation and custom cursor preservation. Overlay/modal/clip barriers and
ancestor wheel/pan propagation need independent regressions. Browser actions must be
real input, not direct reducer calls; actual SVG/pixels and semantic geometry must
be checked. Native queued SDL input evidence is separate from physical device claims.

This audit inspected exact source and current implementation; it adds no runtime
test and claims no new passing platform result. Existing ToggleGroup or Button tests
do not prove a currently nonexistent ButtonGroup. No ledger entry should become
complete on the strength of this proposal alone.
