# Dialog component after Core region V2

Design checkpoint against signed `d4a1d847`; no component implementation is
approved by this document. It supersedes obsolete single-flex-region, exclusive
Header and whole-Outer defaults in earlier proposals. Core layout-record V2 is
independent of draw ABI2; ordinary draw-list frames remain supported.

## Authority and concrete remaining scope

Pinned GPUI Kit0.7.1 revision `87d10ae5e1299d1be18670a37570c5402f952e34`:
`crates/component/src/dialog/dialog.rs:560-745`, `base-dialog.rs:568-635`,
`content.rs`, `header.rs`, `title.rs`, `description.rs`, `footer.rs`. Existing
verified raw cache and executed28-row Taffy receipt remain authoritative; no
new dependency or engine execution was needed for this checkpoint.
C authority: `include/cui/cui_surface.h`, `include/cui/cui_scroll.h`,
`include/cui/cui.h`; `src/base/cui_surface.c:361-458`, `src/base/cui_scroll.c`,
`src/components/cui_surface.c`. Ledger status is not evidence of API absence.

| Concern | Available foundation | Component work still required |
| --- | --- | --- |
| Independent Header/Title/Description | Ordinary wrappers/semantics | Six typed slots; source Description is an explicit composable Header child, not a source automatic sibling |
| Content plus Body | Public144-byte V2,24-byte policies | Actual callback tree/default policies and conditional spacers |
| AUTO, min/max, Footer overflow |28 production source/dist oracles | Current-font composition, owner bounds and descendant dismissal policy |
| Body bar | Public48-byte real-node track projection | Shared Base behavior, skin/fade, queued wheel/touch/drag/page/key/CANCEL |
| Close/cancel/confirm | Existing synchronous decide/close | Explicit owner actions, recursive-call guard, exact event ordering |
| Open/teardown | Existing overlay focus/state | New-route reset helper, retained geometry lifetime, omission/reopen tests |

## Proposed API correction, before declarations are frozen

Keep the exported names and callback-only top-level ownership proposed in
`DIALOG_SLOTS_V1.md`: layout_init/open, Base/Styled compose, manual slot wrappers,
and Base/Styled owner-action helpers. No existing public struct grows; no old
surface/alert helper delegates to compose. The earlier candidate byte constants
are NOT approved ABI. The next implementation must measure actual LP64/ILP32
layouts and assert offsets before publishing constants.

Use new `cui_dialog_sizing_v1` containing size/version/reserved plus width and
Panel AUTO/EXPLICIT requested/min/max, followed by the two exact public
`cui_scroll_region_sizing_v1` records. Panel authority exists only here;
`surface_options.width/height` are inactive for the new route. Content default
FLEX1+INTRINSIC minimum; Body default FLEX1+ZERO minimum. Explicit presence maps
to OMIT versus present-empty; absence never depends on counting callback output.
Content or Body absent requires its policy OMIT; defaults resolve that before
validation. Present+NULL callback remains an authored empty region.

Use the corrected caller-owned layout with surface pointer, context binding,
reentry flag, sequence marker, outer-zero destination and ten retained rectangles
(Panel, Header, Title, Description, Content, BodyViewport, BodyContent, Footer,
Outer, Column). It and surface must outlive end_frame; never retain stack callback
tokens or borrow a temporary offset/bounds array. Slot/style descriptors are read
only during compose and values copied into submitted nodes. No per-dialog hidden
heap or registry; context-owned overlay drag/fade resources remain bounded and
must be measured separately from the caller layout and Core15416-byte plan.

Header, Title and optional explicit Description callbacks run independently in
order, then Content, Body, Footer, each exactly once. Custom Header does not
suppress Title. Application may put a Description manual wrapper inside Header
and omit the independent Description callback. Name/description semantic strings
remain explicit; visual children do not implicitly replace them. Manual wrappers
cannot cross callbacks. Compose owns all structural begin/end balance; no manual
top-level begin/end route. Distinct nested dialogs are allowed, same-layout
recursion/duplicate active submission rejected. Inert/measurement sequence0 is
not a unique frame marker and must not suppress every subsequent submission.

## Geometry and paint route

Submit Panel -> Outer -> Column -> BodyFrame -> BodyViewport -> BodyContent;
optional Content precedes BodyFrame in Column, optional Footer follows Outer in
Panel. Header/Title/explicit Description are intrinsic Column siblings. Column
gap0 with conditional explicit fixed spacers; no phantom spacing around OMIT.
Source defaults:16px horizontal/vertical padding, inner gap8px at explicit rem16,
Panel-to-Footer gap max(top padding,8). Refinement uses explicit logical values,
not an undocumented rem inferred from theme font size. Padding belongs in slot
children, not the zero-padding structural viewport/frame.

Register public V2 after balanced callback submission. Preserve automatic
Content minimum overflow inside clipped Inner and Footer outside Inner, even
Footer400 beyond bounded Panel260. Outer is NOT a scroll owner in source policy;
range/model0. Body alone scrolls. Old whole-Outer reachability policy remains old
route behavior, not an implicit fallback. Mandatory internal Body chain remains
for OMIT, but submits no Body semantics, callback, input helper or bar.

Use real fixed track/thumb nodes plus Core track registration. Core projects
current bounds/clip/culling/shadow ownership; no prepaint geometry bridge,
previous-bounds current paint, private Core access or renderer negotiation.
INTERACTIVE authoritatively enables a placeholder bar; disabled policy clears it,
while owner/ancestor disabled and bar readonly still gate. Visible disabled paint
is retained. Hidden/zero-range bar semantics are omitted, wheel remains enabled.
Fade opacity applies once; no opacity multiplication in a private paint hook.

## Input, close and lifecycle sequence

Extract a narrow PRIVATE surface reducer entry taking a new-route policy bit;
public old update calls it with legacy behavior. Gate ONLY legacy Body6 wheel
statements and reserved1/2/3 activation handling for compose. Preserve pending
request_close, resize, Escape/outside checks and ordering, synchronous decision,
accepted close, focus restoration, on_close exactly. No exported new dispatcher.
This requires explicit ownership of the small `src/base/cui_surface.c` helper
hunk in addition to new Dialog files; do not duplicate the full reducer.

For source route reduce Body with public Base scroll begin/finish discipline:
children first, then changed-only wheel consumption; once consumed never replay
or pass residual delta. Use previous successful semantic/bounds input routing,
Core current layout/clamp for publication. Overlay Base helper owns drag state;
use previous viewport/content dimensions for input only, synchronize its model
back before finish/Core registration. Do not call old overlay paint. No Outer
input/bar helper in source route. A separately approved whole-Outer policy would
need separate two-owner tests; it is not part of this default.

Owner actions validate token/context/open/eligibility then reduce the actual
button interaction once. Accepted activation calls existing synchronous decide
with ACCEPTED/CANCELLED/CLOSED, then surface_close immediately; veto keeps open.
No queued veto, synthetic Enter or second consumption. If a callback closes the
owner, later slots/actions are skipped and internal boxes still balance. Earlier
callbacks are not rolled back. Recursive compose/double-close must not notify
twice. Legacy request_close keeps next-update timing. Source global Enter,
Root deferred-confirm and host removal remain explicitly separate work.

New open helper is required for EVERY closed-to-open transition, including
between-frame close/reopen; validate context/layout before changing state. Reset
Body scroll/max and retained geometry plus outer-zero destination, then delegate
surface_open. Already-open succeeds without reset. Close/request_close stay
existing APIs. Rebinding layout to another context requires explicit reinit
while unmounted; no dereference of a dead former context. Omission does not
implicitly notify close; resource collection follows existing Core rules.

## Decisions and test gate before component production

1. Approve source placement mode now or preserve centered CUI placement as an
   explicit mode. Recommended additive placement enum SOURCE_TOP/CENTERED;
   source preferred y=viewport/10 plus explicit layer offset, snap up to margin
   when oversized. Do not claim full placement parity from a centered Mount.
   Current Core can center an intrinsic child, but source min(preferredY,
   availableHeight-currentPanelHeight) needs a current-height alignment strategy;
   prove that tree before approving SOURCE_TOP, never use previous height.
2. Approve the narrow private reducer ownership above. No legacy statement may
   be retimed merely to accommodate new action helpers.
3. Keep full slot/refinement scope, including styles on independent wrappers and
   named Panel/track/thumb ownership; no fixed-height/demo-only shortcut.

Component gate: callbacks-once and actual-font narrow/wide/AUTO resize; all28
source allocations through real slots; Header+Title+Description/manual children;
omitted versus empty; Footer overflow and actual eligible descendants; Body
wheel/child consumption/touch/drag/page/key/CANCEL, hide/fade/reappearance and
nonunit opacity; synchronous accept/veto/close/reopen including between frames;
nested/modal/inert/measurement/failure/recovery; frozen old route geometry,
input and observer timing. Source placement requires its own current-size proof.
No component production claim follows from Core28 geometry tests alone.
Root/native-host controlled state, mount/removal/deferred-confirm/Enter and all
five facades remain required subsequent slices. Foreign synchronous veto needs
explicit exception/panic/thread/reentrancy/lifetime contracts before bindings.
