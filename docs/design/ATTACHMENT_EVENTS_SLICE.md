# Attachment action observation checkpoint

Design only, audited at `46506c9d`. Proposed names below are not implemented.
Preserve all 78 manifest entries, the legacy options/result/state layouts and the
existing callback timing. This is a direct C action facade, not foreign callbacks.

## Existing contract and seams

The public [media API](../../include/cui/cui_media.h) separates a pure Base
attachment layout planner from the Styled interactive attachment. The planner
has no interaction or action to dispatch. Styled owns no upload/request state:
status/progress/options belong to the caller. Image loader and render hooks are
resource/presentation callbacks, not action handlers.

[Styled implementation](../../src/components/cui_media.c) validates first, plans,
begins a focus scope, registers a pointer-activatable GROUP for the card only when
identified and `on_click` is present, then submits content/actions/media portals.
Remove/retry visibility depends on the corresponding legacy function pointer.
Retry can originate in a media control or a description LINK; description retry
IDs derive from the metadata subtree, not the card retry ID. Custom action
children have their own controls and callbacks, outside these three channels.

The three result flags are aggregate booleans. After original paint submission
and tooltip work, callbacks run in fixed order: click, remove, retry, followed by
focus-scope end. Each category invokes at most once per submission even with
multiple inputs. This is category order, not global input order. Reading the
result never invokes callbacks. Loading is not a universal disable flag: the old
planner decides available actions by status/identified/touch/hovered state.
There is no existing editable attachment value, owner rejection or submit event.

## Proposed compatible facade

A new `cui_media_events.h` may include media plus shared metadata and declare:

```c
typedef enum {
    CUI_ATTACHMENT_ACTION_OPEN,
    CUI_ATTACHMENT_ACTION_REMOVE,
    CUI_ATTACHMENT_ACTION_RETRY
} cui_attachment_action;
typedef struct {
    cui_component_event_metadata event;
    cui_attachment_action action;
} cui_attachment_action_event;
typedef void (*cui_attachment_event_fn)(void *, const cui_attachment_action_event *);
typedef struct {
    cui_attachment_event_fn on_activate, on_remove, on_retry;
    void *user;
} cui_attachment_handler;
typedef struct {
    cui_attachment_result control;
    size_t activation_count, remove_count, retry_count;
} cui_attachment_event_result;
cui_attachment_event_result cui_attachment_events(
    cui_context *, cui_id, const cui_theme *, cui_attachment_state *,
    cui_attachment_options, cui_attachment_handler);
```

Call the complete legacy Styled operation exactly once. Synchronous stack thunks
replace only categories having a new observer; each invokes the original callback
with its original userdata, then the new observer. Other existing callbacks still
need their original userdata through corresponding forwarding thunks if the shared
`action_user` is replaced. Never replace one category and accidentally pass thunk
userdata directly to another old callback. No retained registration/global state,
heap allocation, input replay or new reducer is required.

A new typed handler also enables that category when its old callback is absent,
using the same existing planner/interaction path. An absent typed handler leaves
the category availability unchanged. Missing all typed observers calls the old
operation directly. Do not manufacture an observer from a true paint-valid flag.
Count actual callbacks only; each category is zero or one, retaining coalescing.
The old API is unchanged. Its booleans remain observations, not accepted requests.

Use ACTIVATE metadata for each action; removal/retry are requested application
actions, not proof that host deletion/upload happened. No CHANGE, COMMIT, SUBMIT
or lifecycle event is inferred. New callbacks cannot retroactively consume input,
stop propagation, prevent a platform default or veto an already observed action.

## Evidence and lifetime rules

Snapshot evidence before the first legacy action callback can mutate application
state. Map only actual possible control IDs: card, remove, media retry and every
eligible description retry. Prefer capturing at the existing actual interaction
seam if scanning cannot identify the real category without ambiguity. A unique
accepted activation with a genuine snapshot retains its original source/index
and activation target; metadata target remains the attachment compound. Multiple
causal activations coalesce and cannot be represented by an arbitrary first/last
snapshot: report proven frame with UNKNOWN source and no index/activation then.
Do not guess a description retry ID from the card retry ID. Unsupported native
GROUP/LINK accessibility actions remain documented legacy limitations, not a new
allowlist or invented keyboard support.

New notification eligibility requires an active, paint-enabled, non-inert frame.
Preserve passive/layout/omitted and status-dependent legacy work; never globally
suppress loading actions that the old component allows. Preserve consumed/cancelled
activation behavior at the actual old seam. Custom child actions do not become
attachment action events merely because they are inside the card.

Metadata/action payload is pointer-free, copyable and independently owned after
copying. Handler/userdata are borrowed during this call only. Original callbacks
retain old order and side effects. They must leave resources needed by the rest
of the original operation and the new observer live. The new observer cannot
mutate/free/rebind active state/options/children/context, recursively submit UI,
or destroy the context. Unrelated flags and copying payload are supported; defer
removal and teardown until outer return plus renderer consumption. A local thunk
must never escape into retained state.

## Review and verification boundary

Before production, verify all callback enablement, actual ID/evidence and suppression
seams against the current source and existing media tests. If faithful causal
capture needs a narrow internal observation hook, review that implementation
choice before editing the reducer/presentation loop. Do not silently convert the
old three aggregate callbacks into per-input callbacks.

Proposed worker ownership: new public header, the Styled media source, new focused
Attachment event test, new guide, this checkpoint status. Root owns build/WASI/docs
registration, generated distribution/SDKs and resource generation. Base planner,
Core, binding schemas and other families remain outside this slice.

Meaningful headless proof must cover old/new owner/options/result/semantic/paint
compatibility; original userdata and click/remove/retry order; missing old/new
handlers; typed-only category availability; repeated activations and category
coalescing; unique and ambiguous pointer/keyboard/accessibility evidence where
actually supported; media versus description retry; status/identified/touch/hover
availability; custom child action isolation; inactive/passive/inert/omitted calls;
callback/result coexistence without replay; copied payload and deferred removal.
Use strict C11/C++ header checks and the private native wrapper. Existing image
and attachment/group tests remain regressions; no visible desktop tests. Foreign
bindings retain their existing queued generic notifications unless separately
implemented and tested; including this header in SDKs is not a language facade.

## Approved implementation refinement: private common hook

The stack-thunk strategy above is superseded by a narrow private common entry in
Styled media. Legacy `cui_attachment` passes no descriptor; the new facade passes
its borrowed handler/result. No public old layout changes. Availability is the
original matching callback OR new matching observer. The final fixed click/remove/
retry blocks invoke each original callback with the untouched `action_user`, then
the typed observer. Missing all typed observers uses the exact old path.

Use three fixed stack collector slots, no allocation/workspace/input replay.
Collect only when the actual old `interaction.activated` is true, immediately at
that seam before later custom rendering or legacy action callbacks can change
focus/evidence. Record a unique genuine accepted snapshot when available; multiple
accepted inputs/control origins coalesce to proven-frame UNKNOWN without index.
Missing accepted evidence also gets frame-only UNKNOWN, never a fabricated index.
Freeze category metadata before the first legacy action callback; dispatch still
occurs at the original category callback point before `end_focus_scope`.

Actual IDs and eligibility audited in current Styled/Base media:

- Card uses compound ID, identified and (old click OR typed activate). Capture
  after existing GROUP interaction/details; loading does not disable it.
- Remove uses `cui_attachment_remove_id(card)` only when planner remove_visible
  and prior tracked card width >0; planner requires identified, remove handler
  availability and touch/hover. Loading does not disable removal.
- Media retry uses `cui_attachment_retry_id(card)` only when retry_media and prior
  tracked media width >0. Retry requires identified, FAILED, retry availability.
- Description retry uses
  `media_id(attachment_retry_id(media_id(card,4)), child_index+1)`, not the card
  retry ID. Only the first non-TITLE/non-CUSTOM child when retry_description gets
  the LINK; a child status override does not independently gate it. Pass the
  collector to this existing private metadata helper rather than duplicating its
  child eligibility logic in an outer scan.

The collector observes accepted activation evidence only, never reducers or input
consumption. It does not add support for GROUP/LINK accessibility ACTIVATE. Tests
must include media/description retry, unchanged shared userdata/order, and a late
legacy callback that changes focus after evidence was frozen, plus payload copying.
Production is authorized only after this refinement is committed.
