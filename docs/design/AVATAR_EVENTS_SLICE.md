# Avatar activation and render-family coverage checkpoint

Status: **proposal only; no Avatar event facade is implemented by this document**.
Source checkpoint: `faa07960`. This is a documentation-only review of the current
C implementation and twenty N rows in
[remaining coverage](COMPONENT_EVENTS_REMAINING.md). That matrix and the older
[coverage inventory](COMPONENT_EVENTS_COVERAGE.md) are intentionally unchanged
pending integrator review. The shared
[contract](COMPONENT_EVENTS_CONTRACT.md) still applies. No production API, routing,
capture/bubble, Core dispatch, legacy callback timing or binding behavior changes.
Tests below were inspected, not newly executed for this checkpoint.

## Coverage correction for review

The historical Avatar row is incorrect. `cui_avatar_ex` owns an interactive
BUTTON, consumes matching activation events and returns an aggregate activation
bool. `cui_avatar_stack` owns the visible children's composition and aggregates
an activated item index. These are existing family behaviors, not merely arbitrary
controls supplied by a render callback. The legacy `cui_avatar` void helper is
presentational, but cannot justify classifying every Avatar entry point as N.
Recommended current disposition: **G, direct activation observer gap**, with
legacy initials/image rendering separately described. A proposal does not make
this A or establish complete Avatar parity.

Alert also needs a narrower correction. `cui_alert` constructs its own Dismiss
button when dismissible, returns that button's aggregate activation bool, and
writes `*visible = false` when a visibility pointer exists. Its return is not
successful submission. Suggested disposition: **G, built-in dismiss observation
gap plus arbitrary child composition**. A separate Alert proposal must preserve
its post-content dismissal order, optional owner write, and child button evidence;
it must not call a successful composition result a lifecycle close.

The other eighteen N entries remain justified for current standalone entry
points, with the explicit distinctions below. Merely calling `cui_interact` to
register a semantic label/group/table does not establish an activation channel.
Custom role overrides and application-supplied callbacks can compose interactive
controls; these do not automatically become a family reducer or observer.

| N family | Current source seam inspected | Proposed disposition and reason |
| --- | --- | --- |
| Alert | `src/components/cui_content.c:cui_alert` | G: intrinsic Dismiss result and optional visibility mutation; arbitrary action/content callbacks stay separate. |
| Avatar | `src/components/cui_content.c:cui_avatar_ex`, `cui_avatar_stack` | G: direct opt-in activation and aggregate bool/index. Legacy void Avatar remains presentational. |
| Badge | `src/components/cui_components.c:cui_badge_ex` | N: invokes caller content and paints a marker; true is valid composition, not badge activation. |
| Bubble | `src/components/cui_content.c:cui_bubble`, `cui_bubble_reactions` | N: reaction/content functions are supplied by the application; the pill helper calls that renderer, not an intrinsic reaction reducer. Measurement invalidation is separate. |
| Empty | `src/components/cui_content.c:cui_empty_ex` | N: typed slots and optional children compose controls; successful submission is not activation. |
| Focus Trap | `src/base/cui_overlay.c:cui_overlay_begin`, `cui_overlay_end`; `include/cui/cui_overlay.h` | N primitive exception: caller-owned scope begin/end and explicit overlay operations; completed Surface close is a different API. Preserve the existing reviewed exception. |
| GroupBox | `src/components/cui_content.c:cui_begin_group_box`, `cui_end_group_box` | N: presentation and child scope only. |
| Kbd | `src/components/cui_content.c:cui_kbd` | N: chord presentation, not keyboard input capture or command activation. |
| Label | `src/components/cui_components.c:cui_label`; `src/components/cui_label.c:cui_label_source` | N: text/label composition and relationships, no activation observer owned here. |
| Marker | `src/components/cui_status.c:cui_marker` | N: text/icon/custom-child presentation and loading motion; children own their controls. |
| Message | `src/components/cui_message.c:cui_message` | N: sender/avatar/body/footer slot composition, true means valid submission; MessageScroller scroll events are separate. |
| Progress | `src/components/cui_components.c:cui_progress_ex`; `src/components/cui_components.c:cui_progress_circle` | N: value projection/motion/presentation; no owner value mutation or completion notification inferred. |
| Shimmer | `src/components/cui_status.c:cui_shimmer_text` | N: time-driven text paint; role registration is not activation or animation completion. |
| Skeleton | `src/components/cui_content.c:cui_skeleton_ex` | N: caller loading motion is sampled and committed after successful paint, not an input action or loading-completion event. |
| Spinner | `src/components/cui_content.c:cui_spinner_ex` | N: easing/paint functions are policy/presentation; motion result and successful paint do not mean activation or completion. |
| Tag | `src/components/cui_status.c:cui_tag`, `cui_begin_tag` | N: label semantics/style, optional child composition; no built-in click/remove path. Hover styling alone is not activation. |
| Form | `src/components/cui_form.c:cui_form` | N: field rendering/association and layout error result; child validation, input events and application submit buttons remain separate. Input Group is independently covered. |
| DescriptionList | `src/components/cui_description.c:cui_description_list` | N: labels/value callbacks; no direct selection/activation reducer. |
| StatusBar | `src/components/cui_chrome.c:cui_status_bar`, `cui_chrome_region_draw` | N: supplied render callbacks own input; default items are LABELs. TitleBar's built-in controls are a separate family. |
| Table | `src/components/cui_table.c:cui_table_begin_ex`, row/cell/head/caption helpers | N for basic table composition: semantic registration and validity checks, no sorting/selection reducer. DataTable's built-in selection/sort paths remain separate. |

This is all twenty N rows, recommending two corrected gaps and eighteen retained
render/child/primitive classifications. It does not re-audit the other fifty-eight
families or promote any of the original seventy-eight manifest entries.

## Existing Avatar timing, aggregation and evidence

The authoritative [header](../../include/cui/cui_content.h) and
[implementation](../../src/components/cui_content.c) establish these distinctions:

1. `cui_avatar` draws initials and IMAGE semantics; it has no activation result.
   Cache preparation, palette and metrics helpers are resource/pure operations.
2. `cui_avatar_ex` validates UI/theme/nonzero ID, prepares the current image,
   begins its box, calls `cui_interact` with BUTTON only when `interactive`,
   submits photo/icon/initials, ends its box, updates semantic/focus/cursor
   details, then scans frame events in increasing Core event-index order.
3. Each event satisfying the existing `interactive && !disabled && !consumed &&
   cui_event_activation(ui, index) == id` gate is consumed. The returned bool
   becomes true and remains true. Multiple matching inputs therefore produce
   one true bool while every matching event is consumed. The bool is neither a
   submission-success result nor proof that a host opened a profile.
4. No existing Avatar action callback runs. Image/icon preparation and any existing
   resource-loader policy still occur at their original earlier point. Do not
   move them after new notifications. The caller acts after return; there
   is no old action userdata to forward. The new facade must preserve this old
   path and its consume/paint order, rather than add callbacks to old options.
5. Stack children are submitted in increasing visible item order. Each child
   runs the complete Avatar operation, and the stack replaces its result index
   whenever a child reports true. Multiple activated items yield the highest
   activated visible index, **not the index of the chronologically last input**.
   An overflow badge is a passive IMAGE and never an activation target here.
   Hidden items are not submitted. Later painted overlapping children retain
   their existing hit-test priority.

Current Core routing is evidence to preserve, not an Avatar-specific rewrite:

- Pointer/touch activation comes from the routed accepted release/capture path;
  cancellation, consumed input, hit priority and prior bounds matter. Do not
  infer activation from hover, pointer-down, or successful bitmap preparation.
- Avatar does not set `release_key_activation`. Its current default Core policy
  activates Enter on a nonrepeat key-down and Space on the matching key-up after
  capture; do not silently replace it with source Button's release-key policy.
  Focus changes, modifiers, cancellation and competing consumers need explicit
  regression tests rather than broad claims based on role alone.
- Accessibility ACTIVATE requires prior supported semantics. Interactive BUTTON
  and its pointer-activatable details are supported by the current Core
  allowlist. Queue admission before first semantic registration can fail;
  current role/disabled/modal eligibility must still hold during submission.
  Passive Avatar IMAGE/no-focus is not an activation promise. Browser semantic
  evidence is not a tested native screen-reader integration.
- `cui_interact` records accepted activation snapshots; the later explicit
  `cui_consume_event` can clear them, especially accessibility and release-key
  slots. Freeze the genuine snapshot immediately before consumption at the
  existing event-loop seam. A wrapper scanning only after `cui_avatar_ex`
  returns loses information and cannot faithfully recover multiplicity.
- A raw legacy gate match is not automatically proof of an accepted snapshot.
  If supported active submission reaches that gate without accepted evidence,
  retain only proven frame and target, with UNKNOWN source and no index or
  activation flags. Do not fabricate a source/index from the aggregate bool or
  change the legacy gate in order to make metadata convenient. Dedicated tests
  must establish when this fallback can occur.

Inspected regression evidence:
[content tests](../../tests/unit/test_content.c) cover stack overlap pointer
selection, Enter, cancelled touch, disabled suppression, named/passive semantics,
cache failure recovery and real software pixels. They do **not** establish
Avatar multi-input counts, Space timing or accessibility metadata snapshots.
[Button events](../../tests/unit/test_control_events.c) and
[Attachment events](../../tests/unit/test_attachment_events.c) illustrate proven
per-event versus coalesced evidence capture, but are not Avatar proof.

## Proposed additive C API, not declarations currently available

Use a new `cui_avatar_events.h` including Content and the existing Control event
vocabulary. Do not modify existing option/result layouts. Proposed phase one:

```c
cui_component_action_result cui_avatar_events(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_avatar_options, cui_component_activation_handler);
```

Reuse the existing `{callback, user}` typed activation handler and
`cui_component_action_result`: `valid` is preflight only; `submitted` is ordinary
active paint-enabled, non-inert submission (including disabled/passive Avatars);
`activation_count` counts events accepted by the observed legacy loop, whether
or not a handler is installed; `request_count` is always zero. Counts saturate
with `count_overflow` while dispatch continues. The old bool API remains exactly
as before. In this new API `activation_count > 0` is its aggregate activation
observation; reading/copying the result never redispatches. A handler does not
implicitly enable `interactive`: options continue to control role and routing.
Missing handler still preserves legacy consumption and result counts.

Add a narrow private common Avatar entry with an optional observer/result.
Legacy `cui_avatar_ex` passes neither. The facade calls that operation once;
never submit twice, replay input, or infer per-event counts from `interaction`'s
aggregate flag. Inside the existing loop: freeze evidence, perform original
consumption and bool update, increment the count, then call the observer
synchronously before continuing to the next event. Metadata kind is ACTIVATE,
target is the Avatar ID, and a genuine snapshot supplies its original
frame/index/source/contact/key evidence. Photo/icon/initials painting has already
been submitted at this point. No CHANGE, COMMIT, SUBMIT, resource-loading or
host-profile-open event is implied.

The later stack addition is separately reviewable after that seam is tested:

```c
typedef struct {
    cui_component_event_metadata event;
    size_t index;
    uint64_t key; /* Original application key, including zero. */
} cui_avatar_stack_activation;
typedef void (*cui_avatar_stack_activation_fn)(
    void *, const cui_avatar_stack_activation *);
typedef struct {
    cui_avatar_stack_activation_fn callback;
    void *user;
} cui_avatar_stack_handler;
typedef struct {
    size_t control; /* Legacy aggregate index, or SIZE_MAX. */
    cui_component_action_result action; /* request_count remains zero. */
} cui_avatar_stack_event_result;
cui_avatar_stack_event_result cui_avatar_stack_events(
    cui_context *, cui_id, const cui_theme *, const cui_avatar_item *, size_t count,
    cui_avatar_stack_options, cui_avatar_stack_handler);
```

Forward each visible child's observed activation immediately in existing
child-submission order, then that child's event-index order. Do not sort events
across children, coalesce per item, or mistake `control` for last input time.
The payload's metadata target is the compound stack ID; its genuine activation
snapshot retains the child target. Freeze index/key by value before dispatch;
key zero retains its documented index-derived identity behavior, and must not be
silently replaced in the payload with a synthetic key. The old stack function
uses no observer and preserves its aggregate result. Stack headers, semantics,
overflow behavior, visible-item planning and stable ID formula remain untouched.
A stack observer does not become an observer of arbitrary controls around it.

## Mutation, lifetime and unsupported operations

Handlers/users/payload pointers are borrowed only for synchronous submission.
Metadata and proposed stack payload are pointer-free, copyable by value beyond
frame/context lifetime. Rendering resources, source pixels, cache storage,
labels, typed icon state, item arrays and users retain their existing lifetimes
through submission and, where required, completed rendering. No callback is
registered on a retained Avatar or image cache.

As with Button, copying payloads, changing unrelated application flags and
requesting later omission/teardown are allowed. Consuming later events or
changing focus can change later eligibility; freeze one event at a time and do
not prequeue the whole submission. Changing caller-owned options affects later
calls; never read mutable options back after the call-local descriptor snapshot.
Do not mutate/rebind/free the active stack items, active resources or userdata,
recursively submit the same component/frame, or destroy the active context.
No local wrapper can provide a reliable use-after-free guard. Perform teardown
after outer submission and renderer consumption. The facade neither grants
capture/bubble control nor returns an acceptance/veto decision to Core.

Original APIs have no added callback or timing change. The reviewed
[Attachment approach](ATTACHMENT_EVENTS_SLICE.md) preserves old callbacks before
its observer and deliberately coalesces categories; Avatar must not copy that
aggregation rule. [Button's checkpoint](COMPONENT_EVENTS_ACTIVATION_SLICE.md)
provides the relevant per-event and mutation vocabulary. Existing callers may
continue to act on legacy bool/index; recommend one side-effect path when a
caller adopts new callbacks/results.

## Required proof before an implementation can be integrated

- Legacy versus no-handler facade equivalence: draw commands/software pixels,
  semantics, cache revisions, focus/tab/cursor state, consumed events and bool
  or stack index. Photo/initial/icon/palette/design-size options are unchanged.
- Several pointer/touch/Enter/Space/AT activations in one call: exact counts,
  callback order and copied genuine metadata, plus repeats/cancel/consumed
  origins. Enter down versus Space release and before-first-registration AT
  rejection need dedicated Avatar tests; inspect accepted snapshots before
  consumption and prove retained copies survive later event consumption.
- Disabled, passive IMAGE, inert/layout-only/prepaint/measurement/outside-frame,
  omitted component, invalid ID, modal/role changes and error cases: no new
  notification is invented. Preserve legacy return behavior even when no
  observer is eligible; do not silently repair Core/routing behavior here.
- Multiple stack items activated in reversed input order: callbacks retain
  child/event order and aggregate `control` retains the highest activated
  visible index. Verify stable/zero keys, overlap hit priority, narrowed width,
  hidden items and passive overflow; no extra callbacks for container paint.
- Handler consumes a later event, changes focus, copies payload, and marks a
  sibling/panel for later omission. Verify event eligibility is checked live,
  payload evidence remains frozen, original descriptors remain intact, and
  active storage is freed only after the documented boundary.
- Missing handler still yields counts without replay; one result read never
  redispatches. Counter saturation uses a small helper fixture, not an enormous
  input allocation. New API preflight distinguishes validity from paint success.
- Strict portable C11/C++ header checks and meaningful unit/software tests use
  `python scripts/test_local.py --build-dir <private-build>` for invisible Xvfb.
  Any browser test stays headless. No GPU/native assistive-technology claim
  follows from synthetic Core or Chromium evidence.

Production authorization is a later review. Proposed worker scope: new Avatar
events header, Avatar-only private common entry and stack forwarding, focused
unit tests, guide and a real owner example. Root owns event-family registration,
coverage-matrix correction, SDK/binding exposure, distributions and consumer
skill resources. Merely shipping C declarations through language SDKs is not a
working foreign-language callback facade. Keep all seventy-eight frozen manifest
entries and treat Video as its existing separate extension.
