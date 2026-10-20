# HoverCard desktop and tap policy

Include `cui/cui_hover_card_extended.h`. This additive C API combines explicit
tap-toggle policy, bounded outside taps and optional semantic relationships.
Existing HoverCard/Tooltip functions retain their behavior, including global
Escape dismissal for consumed, repeated and modified Escape. There is no new
input dispatcher, Core schema, AT adapter or foreign binding API.

Submit the trigger first. Pass its actual Button return value as
`accepted_trigger_activation`; the extension trusts this application assertion
and does not rescan consumed trigger events. A false assertion means no toggle.
Do not substitute pointer-up coordinates, raw focus or a queued activation request.
Keyboard/accessibility activation accepted by that trigger can toggle too.

Choose `CUI_HOVER_CARD_TAP_TOGGLE` explicitly for touch/mobile behavior. Hover,
focus and delays do not open or close this mode. One accepted trigger assertion
sets open to the inverse of entry open; outside release is applied first and the
trigger assertion uses that frozen entry value. Disabled or global Escape closes
last. Placement can subsequently close for invalid geometry. A zero/NULL policy
retains the existing desktop reducer and placement. No platform is auto-detected.

Keep a zero-initialized `cui_hover_card_contact` next to the caller-owned
`cui_hover_state`. Submit at most once each frame. Reset the workspace when the
context is recreated; neither structure owns callbacks or input pointers. Mode
change, frame omission, closed model and protected/frame-zero work discard a
pending contact. Do not share one workspace across simultaneous cards.

When `outside_tap` is enabled, the workspace must be provided. A primary down
outside both previous valid card and trigger bounds arms one contact identified
by type, device and contact ID. A matching unconsumed primary up outside both
regions closes only when movement stayed below eight logical pixels. Movement
at the threshold, even followed by a return to origin, cancels the pending tap.
Include displacement at release. Consumed matching input, cancellation, scroll
or gesture takeover cancels it. Unknown bounds, unmatched release and secondary
buttons never close. Ambiguous simultaneous contacts disable outside dismissal
until explicit workspace reset, card close or matching/global cancellation.
No event is consumed and no outside pointer capture is installed.

A child click retains the card. A press inside followed by a drag outside also
retains it. This is intentionally stricter than the frozen upstream mouse-up-out
handler, which does not require an outside press or a movement threshold. It
supports ordinary outside taps without claiming every host gesture or full
mobile parity. Modal permission uses existing Core policy; geometry and modal
checks do not establish exclusive ownership among unrelated overlays. Call the
helper after relevant leaf consumers; later consumption cannot undo an already
applied decision.

Enable `relationship` only when this card owns the submitted trigger's
`controls`/`expanded` fields. The extension copies all other semantic details.
An admitted panel sets controls to its ID and expanded true; rejection/closed
sets controls zero and expanded false. This reflects composition admission, not
only `state.open`: resource rejection may leave the model open. Missing current
trigger semantics yields `relationship_applied=false`; no node is fabricated.
The card retains its GROUP role, explicit name and parent=trigger. Tooltip
description behavior stays unchanged. Omission does not patch caller semantics.

Only `result.lifecycle.control_result` determines whether to compose children
and call the existing `cui_end_hover_card`, exactly once. The lifecycle result
uses existing observer meaning: changed is a final model transition independent
of handler; change_count counts actual callbacks. Callback runs after final
relationship work and before arbitrary children, with UNKNOWN source and genuine
eligible frame only. No input index, activation evidence, dismiss cause, veto,
completed visibility or physical AT announcement is claimed. Copy the payload
to retain it. Do not mutate/reenter the active model or input/options from its
callback; handlers are borrowed only for the call. Inert/layout-only work keeps
maintenance but emits no callback or relationship patch.

Core semantic snapshots can prove controls/expanded and preserved details. The
current AccessKit adapter emits expanded false for TREEITEM, but not ordinary
buttons, so explicit collapsed AT state remains unsupported by this slice.
Live screen-reader announcements, arbitrary overlay ownership, raw outside
mouse-up semantics and Python/Node/TypeScript/Go/Rust exposure remain separate.

The compiling headless example is
[`examples/hover_card_extended/main.c`](../examples/hover_card_extended/main.c).
