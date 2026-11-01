# Typed Base Slider (unreleased, C only)

Include `<cui/cui_base_slider_model.h>`. This additive route owns its fixed-size
root, track, fill and thumb boxes. It implements Single/Range double-valued input
and plain Base painting, independently of legacy `cui_slider`,
`cui_base_slider_events` and their snapshot/callback behavior. The separate
[Styled C presenter](STYLED_SLIDER_MODEL.md) adds palette/source styling, motion
and focus rings. Direct C model/state/result structs are not foreign-language
records; the [Python bridge-owned controlled subset](PYTHON_SLIDER_MODEL.md)
exposes copied double Single/Range proposals and explicit receipt acceptance.
The [Rust bridge subset](RUST_SLIDER_MODEL.md) adds owned copied requests and
a restricted callback context; Linux dynamic and matching bundled-static checks
passed on1.99.1, with MSRV1.85.1 pending. Native numeric host delivery remains pending. The whole Slider family remains partial.

The complete, independently runnable C11 example is
[`examples/slider_model_controlled.c`](../examples/slider_model_controlled.c).
It defines persistent application/reducer state, the actual `on_request`
callback, `compose`, and a headless `main`; there are no undefined callback or
userdata placeholders. Initialize once, then call the same compose function from
your host's frame loop. The example renders no native window and needs no font.
Its two queued Left keys explicitly accept .625 then .5 while both requests keep
current=.75. Expected output: `Selection [0.250, 0.500]; accepted 2 requests`.
A GUI host consumes the real draw list before the next begin. Use the private
Xvfb wrapper for automated Linux execution, including this windowless example.

The callback accepts UPDATE into external `confirmed` storage only; it never
mutates the active reducer/options/UI. It deliberately does not accept CANCEL:
that request can contain the captured pair from an old domain or kind.


Initialize state once, not inside each frame. Exact v1 sizes/tags and zero
reserved fields are required. The domain and value rules are in
[SLIDER_MODEL_VALUES.md](SLIDER_MODEL_VALUES.md). No allocation belongs to the
model; opting writable semantics into Core numeric capabilities can lazily
allocate Core's separately bounded numeric storage. Strings, callback and user
are borrowed through the synchronous call. Copies of requests own all fields
and survive the context. The callback may mutate external owner/userdata and
enqueue application work; it must not mutate active UI/state/options/layout or
borrowed strings, recurse, destroy the context or free active storage.

`current` is the immutable confirmed entry for the whole call. `requested`
evolves in original event order, independently of acceptance: two Right keys from
.25 with step .125 propose .375 then .5 even if neither is accepted. Each idle
key/numeric command emits BEGIN(previous intention), UPDATE(next), COMMIT(next).
Pointer down emits BEGIN and UPDATE; each move/up emits UPDATE, and up then
COMMIT. Accept values explicitly; the reducer never writes or rereads the owner.
Idle intention is discarded at return. Only an ongoing pointer preview persists.
Metadata retains the actual Core target/index/source; lifecycle cancellation
without a genuine event has unknown source and no fabricated input.

Layout extents are finite positive floats, `cross_extent >= max(track,thumb)`
and `0 <= axis_inset < length/2`. Root main extent is length; cross extent is
cross_extent. Track is centered across the axis and spans inset..length-inset;
thumb centers traverse that same interval. Inset zero permits half a thumb to
extend outside root; inset thumb_extent/2 keeps it inside. Vertical values increase
bottom to top. Reverse-fill changes only Single fill (endpoint toward maximum),
not value mapping, input direction, or Range fill. Only box appearance fields
participate: background, border, active border widths/corners and opacity.
Caller geometry fields are inactive. Single ignores the entire start-thumb box.
Disabled thumbs retain semantic geometry but paint transparently; complete
source disabled presentation belongs to the pending Styled route.

Input uses the previous completed track/thumb geometry; this call paints actual
current geometry after reduction. Current thumb semantics are preregistered,
then Core rebinds them to the actual named boxes at end-frame. Root track presses
choose the nearest previous visual thumb (exact midpoint chooses END). A newly
introduced Range START uses its entry fraction on the previous track. Root
capture stays on its original target; the chosen current thumb receives explicit
Core focus (`focus_visible=true`). Direct thumb pointer focus retains Core's
pointer-focus policy. ROOT-target keys from the original dispatch batch delegate
to active thumb (END initially), preserving ROOT in metadata. Home/End and
Left/Down versus Right/Up use additive domain steps (or span/100 when step=0),
including repeated keys. Logarithmic scale changes visual mapping, not additive
keyboard steps. Secondary contacts/buttons and IME-owned keys are not claimed.

Numeric capabilities belong only to actual current writable thumbs, never Root
or legacy Slider. SET_VALUE uses domain-normalized semantic [0,1], linearly mapped
to the domain even when visual scale is logarithmic; the raw double remains in
input evidence. Increment/decrement use the same domain step as keys. Claiming
input and accepting a proposal are independent. During a pointer gesture the
current mask is zero. An own-target numeric event legitimately queued from the
prior idle snapshot is explicitly consumed as busy rejection, without request,
count or value invention. Foreign, stale, disabled, read-only and modal-ineligible
input is not consumed by that shortcut. The real next dragging snapshot publishes
cap0, so later enqueue already rejects; tests never fabricate prior capabilities.

Same-call down→SET→up tests the busy path: restoring capabilities after up cannot
replay consumed SET. Domain/mode changes during a retained gesture emit CANCEL
with the unchanged captured pair and captured gesture domain. Following intention
uses that pair only if valid in the current kind/domain; otherwise it resets to
confirmed entry. Thus domain/mode cancellation can be followed by a real key;
SET becomes enqueueable after a fresh idle publication. An INPUT cancel followed
by SET is reachable from a prior idle snapshot, and preserves original order.
Source-generation-only cancellation keeps captured-start progression when valid.

Disabled, read-only, zero span, source replacement, kind/domain/axis change,
omission and capture loss end gestures with their typed reason (in that priority
order). Missing callback is read-only and silently clears reducer drag. Terminal
events in the original batch are processed before final capture-loss detection;
same-batch up must not become a synthetic cancel. This route never fabricates
physical releases or activation/focus events. Scope Escape remains owned by Core;
subsequent capture loss has unknown-input evidence.

Input requires `cui_frame_input_snapshot_ready`, ordinary non-inert/non-layout-only
composition and existing current/previous modal eligibility. This is prior Core
semantic/geometry validity, not renderer or host success. First frame paints
geometry without interaction. Existing states submitted during invalid-snapshot
recovery paint confirmed geometry but do not renew state frames, consume input or
emit requests. A cleared diagnostic cannot revive a failed frame. After a fresh
successful geometry frame, the next admitted state frame observes any omission
gap and cancels once; geometry-only recovery itself is not an admitted state frame.
A callback that causes a diagnostic stops later requests even if it clears it.
There is no rollback of already delivered requests/application effects after a
later Core failure. Fixed absolute child geometry works with Core V1/V2; this
slice introduces no dispatcher or global input policy.

`cui_base_slider_model_part_id_v1` returns ROOT=owner and stable TRACK/FILL/
START_THUMB/END_THUMB identities, not presence. Owner zero/unknown tag returns0;
Single omits START. Register shadows on emitted named parts after the facade and
before end-frame with `cui_element_set_box_shadows`; no glyph or shadow-mask
semantics are invented. Invalid admission leaves UI/state/callbacks unchanged;
late Core failure is reported separately. See exact ABI/geometry design records
in `docs/design/SLIDER_MODEL_ABI_V1.md` and
`docs/design/SLIDER_BASE_INTERACTION_V1.md`.

The separate [Node/TypeScript bridge-owned controlled subset](NODE_SLIDER_MODEL.md)
also supplies copied requests and explicit full-pair receipt acceptance; it does
not bind direct C custom Styled style or motion records.
