# Dialog/AlertDialog current-height checkpoint

Status: source-only on integrated139695; no production edit or compiler run.
The Table touch production branch is preserved separately. This checkpoint does
not transfer the unpublished scope cancellation/marker or other raw reducers.

## Revalidated authority and actual causes

Pinned GPUI Kit0.7.1 is87d10ae5e1299d1be18670a37570c5402f952e34. All ten raw
cache files under `~/.cache/cui-dialog-audit-87d10ae5` were rehashed against the
original acc877f24 source anchors and match. No dependency/fetch was required.
Actual inspected authority: `dialog.rs:554-752`, `alert_dialog.rs:295-335`,
`header.rs`, `title.rs`, `description.rs`, `content.rs`, and existing28-row
executed Taffy receipt/golden under `tests/reference/dialog_flex`.

| Current integrated C behavior | Consequence | Source behavior |
| --- | --- | --- |
| `cui_surface_geometry`: nonpositive height ->260, capped to usable viewport | Short plain Dialog stays unnecessarily tall if caller expects intrinsic content | Panel AUTO with min96/max usable height |
| `cui_alert_dialog`: nonpositive height becomes **prior** body_content.h+padding | First presentation is260; first-open/content/font/width changes can lag | Current layout determines Panel size |
| Alert opens Body6/Content7 before title/description/actions and centers everything | Title and actions are inside one scrolling body; no independent Header/Footer | Alert supplies Header containing Title/Description, Footer outside Inner |
| Dialog title row/description are fixed siblings above one GROW Body | No independent custom Header/Content allocation | Header and standalone Title are independent; Content and Body are separate flex regions |
| Existing Alert test warms4 frames; wrapping test inspects after4 frames | Does not prove first-open or same-frame resize | Need strict first-presented-frame witnesses |

C sources: `src/base/cui_surface.c:223-244`,
`src/components/cui_surface.c` body begin, begin_internal, footer_begin/end and
alert_dialog; `tests/unit/test_surface.c:846-909`. The legacy unspecified Dialog
height is genuinely a fixed fallback, while Alert's current docs explicitly
promise eventual measured fitting. Do not silently redefine all surface kinds.

Source Panel has16px vertical padding, inner gap8 at explicit rem16, min96 and
current usable max. Header/Title/Content/Body are conditional children of a
clipped flex Inner; Footer is outside Inner. Content is FLEX1 with intrinsic min,
Body FLEX1 with zero min; missing Body is not a present-empty flex participant.
Header does not suppress standalone Title. Alert's automatic Header nests an
icon/text row and Title/Description text column with gap4; it does not manufacture
a separate standalone Title as well. Close is overlaid independently.

## Minimal compatible implementation direction

Reuse the already reviewed six-slot composition contract in
`DIALOG_COMPONENT_AFTER_CORE_V2.md` and current public Core144-byte layout V2;
do not invent a new Core solver or use previous bounds for present sizing.
Use the existing source-placement tree proved in DIALOG_SOURCE_PLACEMENT_PROOF.
Current96-min normalization when usable max<96 remains explicit; do not clamp
away that source minimum or fabricate a fixed readable spacer.

Keep `cui_surface_begin` and its manual end/footer ownership, pure surface
geometry, explicit positive heights and existing state/options ABI unchanged.
Introduce the additive Base/Styled composition route already proposed as
`cui_base_dialog_compose_v1` / `cui_dialog_compose_v1`, with caller-owned retained
layout storage and a separate descriptor; do not resize existing public structs.
The next test-only fixture should exercise its actual tree before declarations
are finalized. All six slots remain in scope: Header, Title, Description,
Content, Body, Footer/actions. Conditional presence and present-empty are
explicit; callbacks run once in source order, measurement may repeat.

Tree: current-size Mount -> optional bounded top spacer -> Panel -> Outer/Inner
-> Column -> Header/Title/Description/Content and BodyFrame -> BodyViewport ->
BodyContent; optional Footer is a direct Panel child after Outer. Conditional
fixed spacers reproduce source gaps without phantom omitted-Body space.
Register current public V2 after balanced callback submission. The second
stored-tree calculation allocates Content/Body and Panel from current intrinsic
sizes and clamps scroll before final positioning. Reuse public real-node track
projection for any visible Body bar; no prior-bounds paint fallback.

An Alert convenience adapter can fill Header and Footer slots from its known
strings/buttons, omitting Body unless caller children exist. Initially this is
an additive source-layout entry, not an implicit rewrite of the old Alert tree.
A separate old-helper first-frame fitting correction is compatible with its
AUTO intent only if it preserves the old all-content-scroll policy; do not claim
it supplies independent Header/Footer or source parity. Prefer the one complete
new tree rather than shipping a warmed-bounds workaround under a new API.

## Public compatibility and explicit remaining gate

Use existing surface owner, synchronous decide/on_close and focus helpers. Keep
old exported reducer/wrapper timing untouched and retain its independent frozen
trace. No new Enter/global dispatcher, queued veto, cancellation marker, contact
ledger or input replay. Supported callback close must avoid registering geometry
for a dismissed scope and must retain borrowed storage through end_frame.

The new geometry route must not claim stronger close/reopen press-lifetime
protection than integrated Core provides. The separately audited held-input
problem remains open; do not smuggle its prototype into this height slice or
claim full lifecycle parity. Before production, integrator must explicitly
accept the new route's legacy-compatible input scope or require the independent
Core lifetime operation first. This is a real remaining contract gate, not a
reason to substitute first-frame blank/lagged Body geometry.

## Independent focused headless witnesses to prepare

1. Capture current old Alert on its first presented frame with a short title,
   description and actions, then change all strings/action widths and viewport
   in the next frame without priming. Preserve old fixed260 behavior as a
   compatibility oracle, not the new route's expected result.
2. Actual current-font provider: short/long title, description and body at wide,
   narrow and wide-again widths; each transition is asserted immediately after
   one end_frame. Line counts come from an independent provider/text-style
   oracle; no expected height recaptured from candidate bounds. Use verified
   Inter regular/bold or report exact fallback font coverage honestly.
3. Assert short AUTO Panel is intrinsic (source min96), never always viewport
   height. Independently compute padding+current fixed/intrinsic regions using
   the pinned28-case flex golden where applicable. Explicit height remains exact.
4. Header+standalone Title both present; description nested in Header versus
   independent slot; Content+Body intrinsic-min freeze, Body omitted versus
   authored-empty, Footer omitted/present/oversized. Verify FooterY and Inner
   clipping separately, preserving actual Content-overflow source policy.
5. Current tracked Panel/region bounds, semantic clips/ranges and software
   pixels must agree on first-open and every resize. Long Body scrolls while
   fixed Header/Footer remain stationary; current shrink clamps offset in that
   frame. UI slot/callback counters once; borrowed bounds storage outlives end.
6. Existing callback veto/accepted-close/on_close/focus ordering and frozen
   legacy traces; no input-event fabrication. Held-stream reopening stays an
   explicit separate gate, with no all-consumer/full-family/binding claim.

No new runtime result is claimed by this checkpoint. The next authorized step
is a bounded test-only current-tree/current-font fixture against139695, not an
arithmetic-only proof or an immediate public contract transfer.
