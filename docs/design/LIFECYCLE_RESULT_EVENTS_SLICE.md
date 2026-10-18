# Lifecycle and result observer checkpoint

Design-only audit at `ea5a3e40`; no production API, metadata promotion or new
manifest family. Read with the [remaining audit](COMPONENT_EVENTS_REMAINING.md),
[contract](COMPONENT_EVENTS_CONTRACT.md) and [scroll checkpoint](SCROLL_EVENTS_SLICE.md).
Existing headers remain authoritative. No build/test was run for this document;
linked assertions are existing source evidence, not fresh verification.

## Disposition without erasing gaps

| Family | Current useful contract | Remaining absence and recommendation |
| --- | --- | --- |
| Focus Trap | Caller-owned overlay open state plus Core focus-scope bracketing and input-close bool. | No typed overlay open/close observer. Scope begin/end itself is a justified primitive exception, not a retained lifecycle. Optional operation observers would be real additive work; do not call existing bool an event facade. |
| HoverCard | Caller `cui_hover_state`, delayed open policy, paired begin/end when composed; child controls own their events. | Genuine gap: no typed model-open observer. Propose separate Base then Styled observation, explicitly not visibility/render completion. |
| Tooltip | Same delayed state but no interactive content hold; complete-call text composition and trigger description. | Genuine gap: no typed model-open observer; share typed payload with HoverCard while preserving distinct timing/children. |
| VirtualList | Typed error/view/scroll_changed/window_pending results and visible-row callback. | No final-offset observer for the whole virtual operation, but no demonstrated missing asynchronous completion lifecycle. Keep window completion result-only; a future offset adapter is separate, not implemented by the Scrollable facade automatically. |
| MessageScroller | Typed double-precision view, changed/jumped/opacity/wake results and explicit model operations. | No precise final-offset/tail-transition observer. This remains an optional real gap; current result is sufficient for immediate inspection, but is not equivalent to a notification. No message-delivered/read or render-complete lifecycle exists. |

The user objective does not require identical event categories on every family.
These distinctions justify preserving useful result APIs; they do not relabel
missing callbacks as implemented. No generic queue, scheduler or Core lifecycle
registry is proposed.

## Recommended decision for the original event objective

**Implement bounded HoverCard/Tooltip model-open observers next** if closing the
remaining interactive lifecycle gap is part of this batch. Their delay, suppression
and placement policy changes state inside the component; a caller should not need
to reconstruct that transition from composition bool. This is a real missing API,
not a render-only exception. Base then Styled can be independently reviewed; no
visibility, animation-completion or foreign callback claim is required.

**Accept Focus Trap scope bracketing and virtual-window completion as explicit
exceptions**, subject to root review. Scope begin/end already express caller-owned
composition, and virtual result pending describes this call's rendered window;
neither owns a lifecycle request to complete. Do not manufacture callbacks merely
for symmetry. Overlay operation metadata is a reasonable later enhancement,
outlined below, but is not required to make the trap primitive usable or to cover
the original Button/Input/control/menu/dialog objective. Accepting this exception
must not be described as an implemented typed overlay facade.

**Keep MessageScroller result-only for this batch**, explicitly recording the
missing exact offset/tail observer as deferred, not implemented. Its broader
`changed` result cannot substitute for that observer. A separately requested scroll
notification objective would require the double-precision slice below; it does
not require a fictional message/render completion lifecycle. Virtual final-offset
observation is likewise a separate deferred scroll feature, not closed by the
Scrollable pair. These are documented scope decisions, not parity promotion or a
claim that all lifecycle categories now exist across 78 families.

## Focus Trap: explicit overlay operations versus scope lifetime

[`cui_overlay.h`](../../include/cui/cui_overlay.h) is the public Focus Trap surface.
[`cui_overlay.c`](../../src/base/cui_overlay.c) implements these exact seams:

- `open`: nonnull context/state, nonzero overlay ID, currently closed; remembers
  explicit trigger or current focus, sets open, clears previous bounds, invalidates.
  Already-open calls do nothing. It does not begin a scope or paint anything.
- `close`: currently open; clears open, restores eligible trigger focus or clears
  focus, invalidates. It does not end an active scope, destroy UI or prove that a
  panel was painted. No retained listener exists.
- `update`: caller runs after frame begin, innermost overlay first, before controls.
  It skips consumed events, scans in order and closes on the first eligible
  nonrepeat Escape targeted at the overlay, or eligible outside pointer down
  against previous panel bounds. Modal outside requires owner target; nonmodal
  outside does not. Returns true only for that input-driven close. It does not
  explicitly consume the closing event; preserve Core modal routing and focus
  restoration rather than adding consumption through an observer.
- `begin`: open flag plus successful Core focus-scope begin, with no layout;
  `end` pops that scope. Pair only if begin succeeded. Omitted begin cannot create
  a new trap, but does not clear application `open`. Frame omission is not close.

[Core scope implementation](../../src/core/cui.c) can register an inactive inert
scope while still requiring a matching end; focus behavior is separate from box
layout/paint. Do not globally promise inert/layout-only operations never mutate
application overlay state. `update`/`open`/`close` are explicit operations, and
policy/focus eligibility remains the existing Core contract. Zero frame evidence
cannot identify a unique passive frame or an OS focus event.

[`test_overlay.c`](../../tests/unit/test_overlay.c) meaningfully asserts nested
Tab/Shift-Tab and disabled-child skipping, inner-first Escape, consumed Escape /
outside suppression, modal opener isolation, nonmodal behavior, and restoration
to missing/disabled trigger fallback. First open has no previous panel bounds to
outside-dismiss. It does not prove every passive/protected phase or retained
lifecycle because no retained listener exists.

A narrowly reviewable **optional overlay-operation slice** could add
`cui_overlay_open_events`, `cui_overlay_close_events`, `cui_overlay_update_events`
with typed `{event, previous_open, open, reason}` and one borrowed handler argument.
Reasons would distinguish EXPLICIT_OPEN, EXPLICIT_CLOSE, ESCAPE and OUTSIDE, not
invent acceptance/veto/destruction. Each observes an actual transition after
existing focus restoration/invalidation. Preserve update's exact legacy bool as a
separate `control_result`; missing handler still mutates, repeated no-op stays
silent. A private update seam can freeze the matched event index/type before
closing; do not rescan after observer mutation or infer activation snapshots from
pointer down. Explicit operations have PROGRAMMATIC origin without invented input;
input update uses only genuine index/frame/source when available. Frame-zero
observation should report maintenance result but suppress callback, consistent
with the approved scroll policy. Out-of-frame explicit-operation policy must be
reviewed separately: preserve old allowed operations and diagnostics rather than
pretend zero means a known passive frame.

Do not add callbacks to scope begin/end: those calls bracket composition, do not
own the panel, and repeat every submitted open frame. An OPEN callback on each
successful begin would be false multiplicity; end is not CLOSE. Surface's retained
completed-close observer is a different API, not a second overlay notification to
chain automatically. No new Styled Focus Trap layer is necessary.

## HoverCard and Tooltip: model-open is not actual visibility

The [public hover contract](../../include/cui/cui_content.h),
[pure Base policy](../../src/base/cui_content.c) and
[Styled panel helper](../../src/components/cui_content.c) establish:

- State is caller-owned `{open,pending,target,suppressed,elapsed}`. Pure update
  returns only old-versus-final `open`; pending/elapsed-only changes return false.
  Positive finite dt advances delay; invalid/nonpositive dt does not. Invalid or
  negative delay becomes zero. Repeated calls advance according to supplied dt;
  there is no automatic once-per-frame time accounting.
- Disabled or Escape resets state immediately; Escape suppresses reopening until
  trigger, focus and content inputs are all absent. Otherwise target is trigger
  hover or focus, plus content hover only while already open. Pending reversal
  restarts elapsed. This is delayed state policy, not animated exit completion.
- Styled helper scans all frame key events for Escape without checking consumed,
  repeat, target or modifiers. Preserve this observable difference from Overlay
  update; an observer must not silently “fix” input filtering. Trigger focus opens;
  HoverCard also holds open for panel hover or focused descendant whose preceding
  bounds are contained by the previous card bounds. Tooltip does not use that hold.
- The trigger must already be submitted; placement uses preceding-frame anchor
  bounds. Invalid viewport or missing/empty anchor can force `state.open=false`
  after pure update. Measurement resource allocation can fail with `open=true`
  while helper returns false. Thus legacy false is not simply “closed”, and true
  means panel composition/pair admission, not completed presentation.
- Tooltip composes text and closes its own portal, adding trigger description
  semantics. HoverCard begin opens portal plus inner content box and returns true
  before arbitrary children; end only closes them. No modal focus trap is added.
  Intrinsic measurement is context-owned, tracks child bounds and schedules a
  collision-placement redraw; no caller content callback is retained.

Omission freezes caller hover state: it does not advance delay, close, dispatch or
retain a synthetic late callback. A stale `open=true` after omission is model
state, not proof of a visible panel. Inert/layout-only work can still run policy
maintenance; do not infer user visibility from it or add observers in protected
measurement/paint phases. Existing public frame-zero limitations still apply.

[`test_content.c`](../../tests/unit/test_content.c) tests pure delay/suppression/
invalid dt; Styled pointer timing, card-versus-tooltip content hold, Escape,
trigger-focus opening, disable close, trigger description/parent/tab semantics,
explicit and intrinsic geometry, wrap/viewport collision, bounded height and
measurement redraw settling. Its closed/omitted-content assertions demonstrate
missing bounds/resource lifetime, not an automatic omission-close callback.
No current tests establish typed observer order, passive suppression, allocation
failure events, callback mutation or every consumed-Escape variant.

Recommend two independent slices, with no new shared Core mechanism:

1. **Pure Base hover observation:** `cui_hover_update_events(target, state, ...,
   handler)` preserving all current parameters and reducer bool in a typed result.
   Payload `{event, previous_open, open}` is pointer-free, kind LIFECYCLE, source
   UNKNOWN, without context/frame/index evidence: input booleans are caller facts,
   not physical events. One aggregate callback only for actual `open` transition;
   pending/elapsed changes remain result/state inspection. This is notification,
   not controlled rejection. Optional handler/user are borrowed only through call.
2. **Styled final-model observation:** `cui_tooltip_events(...)` observes after the
   whole old call; `cui_begin_hover_card_events(...)` observes after the old begin
   decision and before returning to arbitrary children. Result separates
   `control_result` (legacy pair admission) from actual open change and callback
   count. Caller ends only on true admission, even if no callback occurred. Capture
   entry state, invoke legacy once, then compare final model after placement/error
   fallbacks; never call a Base observer internally and notify twice. Suppress
   callbacks for zero-frame/protected work while preserving old maintenance.

Do not name payload “visible” or promise pixels/geometry settled. Closed→opened→
forced-closed in one call is aggregate net-zero, not a displayed open/close pair.
Already-open→composition-failure can be no model change despite false return.
A reason enum would require narrow internal policy instrumentation; omit reasons
initially rather than infer Escape/disable/geometry from final bool. Genuine frame
only, UNKNOWN source and no index/HAS_ACTIVATION for Styled aggregation. End has no
new lifecycle event. Content remains arbitrary child composition, not notification.

Callbacks may update unrelated app state only; active hover state/options/trigger/
input mutation, recursive submission or context teardown is unsupported. Handlers
are not retained across pending delay or omission. Pointer-free payload copies
outlive callback; borrowed text/children retain their existing frame obligations.
This adds no “will open” veto, confirmed close animation, disposal or OS event.

## VirtualList: result-only window state is a justified exception

[Public result](../../include/cui/cui_virtual_scroll.h),
[Base](../../src/base/cui_virtual_scroll.c) and
[Styled](../../src/components/cui_virtual.c) distinguish reduction from callbacks.
Base resolves extents, reduces scroll/owner keys and computes view. Styled renders
that window/overscan, then processes child-deferred scroll and translates current
viewport. Main-axis deferred movement sets `window_pending` and invalidates for
next-frame row composition; returned view still describes the rows just rendered.
Cross-axis change can set `scroll_changed` without main-axis pending.

[`test_virtual_styled.c`](../../tests/unit/test_virtual_styled.c) asserts precise
wheel/key/scrollbar behavior, disabled behavior, visible-row semantics/geometry,
software rendering and the pending-then-stable deferred sequence. Empty-model
normalization is also asserted. These do not establish async data or presentation
completion. Rows and measurement users are borrowed during composition; keys/
heights cannot change in row callbacks. Model state outlives use, not a retained
completion listener. Omitted widget produces no new result/callback and cannot
“finish” a previously retained request that does not exist.

Keep `window_pending` result-only: false is no new pending deferred change in this
call, not a completed generation or finished GPU frame. A retained completion hook
would require new request/ownership architecture. Repeating row composition in
finish would change callback multiplicity. A later useful final-offset observer
must wrap the whole virtual Base or Styled operation and retain the existing view/
error/pending result separately; generic Scrollable events miss virtual owner keys
or duplicate internal finish if naively composed. This observer remains absent;
no claim that result-only covers all application notification needs is made.

## MessageScroller: retain exact results; precise observation is separate work

[API](../../include/cui/cui_message.h), [Base](../../src/base/cui_message.c),
[Styled](../../src/components/cui_message.c), and
[tests](../../tests/unit/test_message.c) establish double offsets/totals, explicit
append/prepend/splice/measurement, anchoring and following-tail. Application owns
message records; opaque state retains only bookkeeping. Row renderers run for
visible rows plus overdraw; collection/state must remain stable through end_frame.

Base learns prior rendered row bounds, lays out/anchors/clamps, then captures its
local old scroll; input/bar/jump actions and opacity transitions follow. `changed`
includes row measurements and jump/fade animation, so it is not a scroll event.
Styled width can remeasure before Base and tracked bounds feed later calls. There
is no virtual-style deferred finish/window_pending or data-delivery generation.
Tests assert prepend/measurement anchor behavior, tail following, bounded row work,
precise wheel, edge consumption, Home, jump action, reduced-motion/opacity/wake and
invalid options. They do not assert a missing offset observer or completed render.

An optional later slice could expose a separate double `{previous,value}` offset
observer around the whole model operation, plus a distinct `{previous,value}`
following-tail transition if applications need it. Snapshot must precede layout
reconciliation; never float-cast `view.scroll` or use `result.changed` as its
condition. Preserve existing result exactly. Programmatic append/tail/remeasure
changes, passive work and between-frame calls need an explicit notification policy
before implementation; handlers must not be invoked by retained row-bound writes.
No message-received/read, network, render-complete or paint lifecycle is implied.

## Review/test gates before production

Each proposed slice needs separate ownership approval and a signed, strict C11/
C++ header/test commit, with native CTest through the private-display wrapper.
For Overlay: repeated explicit operations, inner-first input, consumed/repeat/
outside/first-bounds rules, focus restore ordering, omitted/passive operations and
old/new behavior comparisons. For hover: timer/reversal/suppression, repeat calls,
consumed/global Escape preservation, focus/child hold, fallback final-state/net-zero,
allocation failure, no callback from omission or frame-zero work, callback ordering
before arbitrary HoverCard children, correct begin/end balancing, copied payload
and no retained user pointer. For any future virtual/message observer: exact final
state versus rendered view, deferred window and double precision/anchor/tail proofs.
Do not change Core input policy or retime callbacks to satisfy a generic vocabulary.

Bindings expose none of these proposed immediate handlers; existing queued bridge
allowlists stay unchanged. A new C payload alone does not add owning foreign views,
synchronous request hooks or async completion. Browser checks stay headless and
private Xvfb proof does not establish hardware/native-session behavior.

## Proposed ownership for the next reviewed work

Base hover slice: new `include/cui/cui_hover_events.h`, narrow
`src/base/cui_content.c` additive entrypoint, new
`tests/unit/test_hover_events.c`, new `docs/HOVER_EVENTS.md` and this checkpoint
status only. Prefer new-header discovery over modifying legacy public layouts.
Pure Base arguments remain exactly the existing `cui_hover_update` input values
plus explicit target and handler; no context or retained callback is added.

Styled follow-up after separate review: additive declarations in that new events
header, narrow `src/components/cui_content.c` entrypoints/common seam, new
`tests/unit/test_hover_styled_events.c`, guide/checkpoint status. Existing
`cui_content.h` discovery can be considered explicitly; no layout additions.
Preserve helper failure/begin-end behavior, measurement storage, trigger semantics
and old functions. Root owns CMake/source manifests/generated distributions and
bindings for both slices. Core/overlay/virtual/message code is outside ownership.

Base tests should prove exact legacy state/bool sequence equivalence across
opening/closing/reversal/suppression/disabled/invalid timing, actual callback counts,
missing handler, repeated same-state calls and copied metadata with no fake frame.
Styled tests add pointer/focus/inside-card behavior, global consumed/repeat Escape
legacy equivalence, placement/anchor failure and net-zero fallback, allocation
failure where deterministically injectable, passive suppression, composition
admission separate from open change, old/new paint/semantics/geometry, callback
order and borrowed lifetime. No platform desktop is needed for these tests.
