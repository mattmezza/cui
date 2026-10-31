# Compound shadow parts: proposed identity contract

Design checkpoint against `08a00f0b`, following the
[78-family ownership audit](ELEMENT_SHADOW_COVERAGE.md). **Proposal only:** no API,
painter, binding, generated file or runtime evidence is added by this document.
Review this contract before implementation. This slice covers rectangular painted
surfaces and their existing controls; it does not finish all 78 families.

## Constraints and scope

Expose additive typed getters for existing IDs, then make the corresponding
painters call those getters/the same private mapping functions. Do not re-key any
box, callback argument, interaction, focus target, semantic parent or tracked
bounds. Do not add wrapper boxes, shadow masks, callback passes, allocations,
retained state, portal layers or styles. Unstyled command streams and node/command
capacity behavior must remain byte-for-byte unchanged. No public struct grows.

The existing [Core element setter](../../include/cui/cui.h#L679) remains the only
styling operation: submit the owner, then apply a copied list before `end_frame`,
outside measurement/prepaint/diagnostics. Getters are side-effect-free identity
computations. They do **not** look up current bounds, initialize state, open an
overlay, submit paint, report visibility or promise a part exists. An ID returned
for an omitted/virtual/closed/layout-only owner does not authorize styling it;
Core must continue rejecting unavailable targets. `cui_bounds` alone may describe
prior layout and must not become a presence gate.

Return `0` for root/scope `0`, unsupported enum, invalid index, null scroller state,
or invalid logical notification key `{0,0}`. Getter failure emits no diagnostic
and changes no output/state. Do not add a generic ID-space registry or claim
collision-free hashes. Preserve a legacy derived zero rather than remapping it:
that result cannot be targeted by the element setter. Existing nonzero-remapping
behavior in the media helper stays exactly as it is.

## Proposed public surface

Proposed declarations below are illustrative C11, using only existing portable
public types. Enum values should be explicit contiguous values starting at zero;
no platform/Clay types enter the headers. There is deliberately no generic
integer `part` escape hatch exposing all internal padding/hash slots.

```c
/* cui_content.h: applies to composed cui_bubble(), not cui_begin_bubble(). */
typedef enum {
    CUI_BUBBLE_PART_SURFACE, CUI_BUBBLE_PART_REACTIONS
} cui_bubble_part;
cui_id cui_bubble_part_id(cui_id bubble, cui_bubble_part part);

/* cui_message.h */
typedef enum {
    CUI_MESSAGE_PART_AVATAR, CUI_MESSAGE_PART_HEADER, CUI_MESSAGE_PART_FOOTER
} cui_message_part;
cui_id cui_message_part_id(cui_id message, cui_message_part part);
cui_id cui_message_body_id(cui_id message, size_t body_index); /* index < 256 */

typedef enum {
    CUI_MESSAGE_SCROLLER_PART_VIEWPORT, CUI_MESSAGE_SCROLLER_PART_JUMP
} cui_message_scroller_part;
cui_id cui_message_scroller_part_id(cui_id scroller, cui_message_scroller_part part);
cui_id cui_message_scroller_row_id(cui_id scroller,
    const cui_message_scroller_state *state, size_t index);

/* cui_status.h: scope is the ID passed to cui_notifications[_checked](). */
typedef enum {
    CUI_NOTIFICATION_PART_CARD, CUI_NOTIFICATION_PART_CONTENT,
    CUI_NOTIFICATION_PART_ACTION, CUI_NOTIFICATION_PART_CLOSE
} cui_notification_part;
cui_id cui_notification_part_id(cui_id scope, cui_notification_id logical_id,
                                 cui_notification_part part);

/* cui_media.h */
typedef enum {
    CUI_ATTACHMENT_PART_CARD, CUI_ATTACHMENT_PART_MEDIA,
    CUI_ATTACHMENT_PART_MEDIA_OVERLAY, CUI_ATTACHMENT_PART_METADATA,
    CUI_ATTACHMENT_PART_ACTIONS, CUI_ATTACHMENT_PART_REMOVE,
    CUI_ATTACHMENT_PART_MEDIA_RETRY
} cui_attachment_part;
cui_id cui_attachment_part_id(cui_id attachment, cui_attachment_part part);
typedef enum {
    CUI_ATTACHMENT_CHILD_PART_ROW, CUI_ATTACHMENT_CHILD_PART_RETRY
} cui_attachment_child_part;
cui_id cui_attachment_child_part_id(cui_id attachment, size_t child_index,
    cui_attachment_child_part part); /* index < CUI_ATTACHMENT_MAX_CHILDREN */
```

Keep existing `cui_message_scroller_jump_id`, `cui_attachment_remove_id` and
`cui_attachment_retry_id` symbols and results. New aliases must agree with them for supported nonzero roots;
no deprecation or behavior change is needed. Existing callers passing zero to
those old helpers retain their old behavior; the new zero-validation contract
must not be retrofitted onto legacy helpers.

## Exact mappings and ownership

In this table `content_child`, `message_child`, `status_child`, `media_id` and
`notification_element` mean the **current private functions**, not public hash
recipes for consumers. Implementations must reuse them. These expressions are
review evidence, not a recommendation to copy constants into applications.

| Facade / proposed part | Existing owner to expose | Submission and callback limits / exact source |
| --- | --- | --- |
| `cui_bubble`, SURFACE | `content_child(root,3)` | Actual rounded surface inside row/width wrappers; [composition](../../src/components/cui_content.c#L636), [begin surface](../../src/components/cui_content.c#L485). `cui_begin_bubble(id)` already paints directly at `id`; applying the composed getter to that ID is incorrect. |
| `cui_bubble`, REACTIONS | `content_child(root,5)` | Same pill ID in flow and measured absolute placement; absent without reactions callback. Do not target child4 alignment or child6 spacer. [Both branches](../../src/components/cui_content.c#L539). |
| `cui_message`, AVATAR / HEADER / FOOTER | `message_child(root,1/3/7)` | Avatar is painted clipped round box; metadata getters identify slot rows, not callback-owned descendants. Slots may be omitted by the existing plan. [Avatar/slots](../../src/components/cui_message.c#L51), [composition](../../src/components/cui_message.c#L109), [footer](../../src/components/cui_message.c#L157). |
| `cui_message`, body index | `message_child(root,20+index)` | Typed body: actual `cui_begin_bubble` surface. Untyped body: slot row, not an invented bubble. Custom slot renderer receives a different child namespace and owns its own boxes. `index < 256` matches existing admission; getter cannot know current body count/type. [Body loop](../../src/components/cui_message.c#L127), [limit](../../src/base/cui_message.c#L25). |
| `cui_message_scroller`, VIEWPORT | `message_child(root,100)` | Existing clipped scrolling content viewport; offset includes virtual before/scroll projection. Not the outer root or a particular message. [Viewport](../../src/components/cui_message.c#L229). |
| `cui_message_scroller`, JUMP | Existing `cui_message_scroller_jump_id(root)` | Actual painted/interactive jump box, **not** positioning portal child600 or callback namespace child601. Appears only when enabled with positive jump opacity. [Portal and button](../../src/components/cui_message.c#L289). |
| `cui_message_scroller`, row(state,index) | `message_child(root,(size_t)state->rows[index].key+200)` | Side-effect-free **state-derived** getter: validate `index < state->count`, read retained key, never create/measure/reconcile a row. Exists as a Core box only for rendered range/overdraw. Key follows existing splice bookkeeping; index is a lookup coordinate, not identity. [Row paint](../../src/components/cui_message.c#L235), [key allocation/splice](../../src/base/cui_message.c#L163). |
| `cui_notifications[_checked]`, CARD | `notification_element(scope,logical_id)` | Actual card portal; no box exists at the supplied scope ID. Fully hidden cards may be `layout_only`; system-only notifications have no card. [Derivation](../../src/components/cui_status.c#L476), [portal/layout-only](../../src/components/cui_status.c#L769). |
| Notification CONTENT / ACTION / CLOSE | `status_child(card,302/301/300)` | CONTENT is the real clipped content column. Default ACTION is a button only with action label; custom `action` receives this namespace but need not submit a box. CLOSE is actual button inside positioning portal313, emitted only hovered/expanded. Do not target portal313 or glyph315 instead. [Content/action](../../src/components/cui_status.c#L842), [close](../../src/components/cui_status.c#L867). |
| `cui_attachment`, CARD / MEDIA | `root` / `media_id(root,3)` | CARD is actual chip/card inside outer overhang wrappers; MEDIA is clipped thumbnail/glyph frame only with media enabled. [Card/media](../../src/components/cui_media.c#L397). |
| Attachment MEDIA_OVERLAY | `media_id(media_id(root,3),5)` | Existing positioning portal for optional `media_overlay`; callback receives child6 namespace, not this owner. Requires usable tracked media bounds; may be absent on first layout. [Overlay](../../src/components/cui_media.c#L477). |
| Attachment METADATA / ACTIONS | `media_id(root,4/5)` | METADATA column only when content enabled. ACTIONS is actual cluster box or portal with the same ID in horizontal/vertical forms. Custom action callback receives `media_id(root,6)` and owns any real buttons. [Metadata](../../src/components/cui_media.c#L211), [action cluster](../../src/components/cui_media.c#L503). |
| Attachment REMOVE / MEDIA_RETRY | Existing remove/retry getters | REMOVE is button inside positioning portal8. MEDIA_RETRY is the existing retry button used by failed media, not a universal textual retry link. [Legacy getters](../../src/components/cui_media.c#L16), [media retry](../../src/components/cui_media.c#L454), [remove](../../src/components/cui_media.c#L536). |
| Attachment child ROW | `media_id(media_id(root,4),index+1)` | Actual title/description row; CUSTOM child receives same namespace without an automatic row box. Do not fabricate a wrapper around the callback. [Child loop](../../src/components/cui_media.c#L211). |
| Attachment child RETRY | `media_id(cui_attachment_retry_id(media_id(root,4)),index+1)` | Actual description retry link only when existing first-eligible failed-description logic emits it. Index is original child-array index, not filtered/visual order. The metadata helper receives `media_id(root,4)` as its own local root: neither `cui_attachment_retry_id(root)` nor a child of that card-level ID names these links. [Link derivation](../../src/components/cui_media.c#L259). |

No getter is proposed for spacers, fade strips, legacy shadow-ring portals,
progress raster strips, icon paths, text glyphs, title/status tint regions or
custom callback namespaces without a known automatically submitted owner. Their
styles/callbacks remain separate concerns. Native OS notification cards are host
owned and cannot acquire a Core shadow through an in-app getter.

## Notification identity, collisions and lifetimes

Three different identities must stay explicit:

- **Logical notification ID** `{type,key}` belongs to model replacement/dismissal,
  callbacks and optional OS delivery. `{0,0}` is already invalid for push
  ([validation](../../src/base/cui_status.c#L88)). Never cast its key to paint ID.
- **Scope ID** is the caller's namespace for one in-app stack rendering. The same
  logical notification rendered in another scope derives a different card; scope
  does not itself imply an emitted paint node.
- **Card paint ID** derives from scope plus the full existing logical-ID mixing
  function. Part IDs derive from that card. Getters must preserve current
  `size_t` conversion and arithmetic, including existing architecture limits;
  do not promise identical cross-architecture IDs or 128-to-64-bit injectivity.

The logical pair is not collision-free after folding into a `cui_id`. Two model
entries can be distinct while deriving the same card or folded Core ID. Tests
must demonstrate existing duplicate-ID rejection and no partial decoration of
another owner, not silently salt/remap/merge them. Hash redesign would alter
legacy identities and requires a separate review.

A computed ID owns no memory. It is useful only with the same scope/root and
logical record/current composition. Model replacement retaining a logical ID
retains the paint identity; dismissal completion removes presence, not the pure
mapping. Scroller row IDs instead depend on live bookkeeping state and retained
row keys: unaffected rows keep IDs across insertion/removal shifts; reset/newly
inserted rows get the existing new keys. Do not cache an index as stable identity
or call a state-derived getter after destroy. Existing collection/state lifetime
through `end_frame` remains unchanged. Numeric body/child positions identify
positions, not application-level stable keys after reordering.

## Implementation seams and review gates

Use the [content child helper](../../src/components/cui_content.c#L23),
[message child helper](../../src/components/cui_message.c#L36),
[notification/card helper](../../src/components/cui_status.c#L476),
[status child helper](../../src/components/cui_status.c#L278) and
[media helper](../../src/components/cui_media.c#L8) as the single derivation
sources. Do not reproduce their salts in public headers, tests or bridge code.
For row lookup, reuse the existing private row-key representation; Base and
component currently mirror the private scroller state. If extraction is needed,
share one internal header/accessor rather than introducing a third layout copy
or moving platform/theme dependencies into Base. Public prototypes belong only
to the four owning headers above; no Core API change is proposed.

Painters should consume the semantic mapping at the same call sites, without
moving callbacks or changing `cui_interact` order. Notification custom action and
Attachment custom child/action callbacks retain their existing namespace IDs and
phase; no post-paint callback is added. After a compound painter returns, callers
may style its actual completed named boxes before `end_frame`. Pure getters may
also be used outside a frame, but the element setter may not. Legacy root helper
behavior, duplicate diagnostics, callback count/order and fixed array limits
remain authoritative.

## Required focused evidence before accepting implementation

| Area | Headless acceptance fixture |
| --- | --- |
| Exact ownership | Record legacy emitted IDs by observing draw/layout/semantics and callback arguments, then compare before/after getter integration. Check facade root versus surface boxes and independently tracked bounds; do not derive expected values by copying salts or merely compare getter to painter using the same new helper. Freeze representative baseline IDs or compare a retained pre-change fixture. |
| Getter validity | Zero roots/scopes, unknown enums, body index256, child index128, null/out-of-range scroller state, logical notification `{0,0}`; no diagnostics/state mutation or new allocations. Valid-but-not-present IDs still compute without a fake presence result. Preserve old zero-input helper behavior and any derived-zero edge. |
| Bubble / Message | Composed surface versus row, direct begin/end surface, reaction in-flow versus absolute placement retaining ID, optional slots, typed/untyped/custom bodies, 0/256 bodies and rejection257; alignment and custom renderer arguments unchanged. Apply/clear mixed shadows to surface without decorating row or sibling. |
| Scroller | Prepend/splice/reset and changed indices retain the correct row-key identity; offscreen row getter still returns identity but setter rejects absent row. Only current visible/overdraw rows submit. Jump disabled/fading/visible cases, callback namespace versus row box, viewport clip/offset and current-frame scroll geometry. Destroy/lifetime contract must be enforced by caller, not dereferenced in a test. |
| Notification | Same logical ID in distinct scopes; replacement/dismissal/ending/full removal; in-app/system-only/both delivery; collapsed hidden `layout_only` cards; hover/expanded conditional close; default action label versus custom action with/without a submitted box; same callback count/phase and logical ID payloads. Explicit logical-pair and folded-ID collisions reject decoration transactionally. |
| Attachment | Horizontal/vertical card, media omitted, loading/failed/complete, first-frame and resolved-bounds media overlay, actions in flow versus portal with identical ID, custom metadata/action renderers with no box, multiple description rows and first eligible retry mapping, remove hover/touch conditions. No added wrappers to make absent parts styleable. |
| Core style integration | Current-frame move/resize, open/closed boxes, ordinary/absolute/portal owners, prior-frame/omitted/layout-only rejection; copied eight-layer DROP/INSET list, clear and invalid transaction, ancestor clip/opacity, existing underlay/outline/prepaint ownership, unchanged interaction/semantics. No new command/node use on unstyled path; opt-in command exhaustion follows existing rollback and diagnostic behavior. |
| SDK/docs/bindings boundary | Matching canonical headers, consumer examples, coverage metadata, generated SDK/amalgamation/skill resources and offline skill checks. Do not implement or claim a retained binding facade as part of getter tests. Native/browser/GPU proof, if later needed, remains separately declared and isolated. |

Tests should be focused portable/headless fixtures; there is no reason to open a
native desktop window to prove ID mappings. No build or test ran for this design
checkpoint; link/count checks are documentation checks only.

## Future bridge mapping: MAIN is not automatically ROOT

This is a mapping proposal, **not a new binding API or claim of support**. The
retained facade should resolve an explicit semantic target after the relevant
painter has submitted it, with per-frame deferred application for portals. Never
use previous bounds as proof of presence or retain pointers into immediate paint.
Distinguish unsupported/absent targets from success; do not decorate a fallback
row when the requested surface was omitted. Callback-only namespaces are not
paint targets unless the callback actually submits the corresponding box.

| Current facade | Future ROOT / MAIN decision |
| --- | --- |
| Bubble | ROOT = node ID (outer row); MAIN = composed SURFACE getter. REACTIONS explicit and conditional. Current facade calls composed `cui_bubble` ([site](../../src/bridge/cui_bridge.c#L4648)). |
| Message | ROOT = node ID stack; MAIN = body0 **only for the bridge's current known typed single body** ([site](../../src/bridge/cui_bridge.c#L4675)). General C multi-body content requires explicit index; no arbitrary first-body rule becomes a C guarantee. |
| MessageScroller | ROOT = node ID clipped shell; MAIN aliases that shell, not an arbitrary message or the translated content canvas. VIEWPORT/JUMP explicit. A child bridge Item already renders Message at **its own node ID**, ignoring callback namespace; its MAIN should use that Message's body0. Scroller row(state,index) is not a substitute for child Item identity ([callback](../../src/bridge/cui_bridge.c#L3633), [facade](../../src/bridge/cui_bridge.c#L4497)). |
| Notification | There is no per-node Core ROOT: the bridge tree case submits nothing ([site](../../src/bridge/cui_bridge.c#L4486)). Reject ROOT as unsupported rather than silently target the shared stack/scope or alias another node. MAIN = CARD at existing shared notification scope and logical `{1,node->id}`. Apply after deferred shared-stack rendering ([logical ID](../../src/bridge/cui_bridge.c#L2133), [stack site](../../src/bridge/cui_bridge.c#L4954)); explicit CONTENT/ACTION/CLOSE follow emitted owners. Changing ROOT to an alias would need an explicit separate facade contract. |
| Attachment | ROOT and MAIN both = actual card node ID; MEDIA/MEDIA_OVERLAY/METADATA/ACTIONS/REMOVE/MEDIA_RETRY/child parts explicit. Current limited bridge options do not expose all C callbacks/media; do not infer binding option parity from getters ([site](../../src/bridge/cui_bridge.c#L4687)). |

Follow-on bridge work must specify authoring validation, retained style ownership,
when absent targets retry or remain unavailable, and atomic frame failure before
shipping MAIN/ROOT or typed subparts. It must keep existing facade APIs/default
paint unchanged and preserve the six-kind worker's independently owned contract.

## Implemented C slice

The additive typed getters described above are implemented without public struct
layout changes. Existing painters keep their raw zero-owner derivations; the new
getters reject zero owners. Message's private derivation now lives in Base and is
shared by its painter and getters; the row getter uses the existing Base state,
with no additional private layout copy.

Focused warning-enabled tests pass through the private-Xvfb wrapper:
`compound_shadow_parts`, `compound_shadow_legacy` and `element_shadows`.
The independent legacy fixture was captured/replayed before rebuild from the
preserved pre-change library; its provenance and 64-bit little-endian scope are
in [the consumer guide](../COMPOUND_SHADOW_PARTS.md). It verifies unchanged
IDs, commands, semantics and callback ordering for nonzero and zero-owner scenes.
The getter fixture checks current-frame owners, closed/conditional/custom parts,
retained-row identity, capacity boundaries, Notification collision/layout-only
behavior and Attachment flow/portal identity. SDK/amalgamation freshness and the
18 offline agent-skill tests also pass. The expanded 12-test headless slice
includes existing family/Base-only regressions and selective/umbrella generated-
header C11 declaration/link checks against the rebuilt library. These checks do not establish browser/GPU
rendering or language-builder target parity. The future bridge table remains a
proposal only.

Implementation sign-off: `/root/hidden_windows`, 2026-10-31. Additive C getters
only; legacy fixture unchanged; no public struct/ABI layout change, no bridge or
Core input-architecture changes. Root integrator owns merge/publication.
