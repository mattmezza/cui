# TitleBar events: host dispatch checkpoint

Design only, audited at `ad11bb38`. No proposed API below exists yet. The frozen
78-family inventory is unchanged. Read with [the shared contract](COMPONENT_EVENTS_CONTRACT.md)
and [coverage audit](COMPONENT_EVENTS_COVERAGE.md), whose host-service exception
remains necessary even if an optional observer is added.

## Existing ownership and timing

[`cui_chrome.h`](../../include/cui/cui_chrome.h) exposes caller-owned gesture state
(`move_armed`, `moved`, pending click/time/position), borrowed options/items, a
`cui_window_host`, and legacy `on_close(close_user)`. There is no confirmed window
model owned by TitleBar and no disabled/readonly option. Public functions return a
boolean, not a typed event/result or guaranteed window-state change.

| Path | Actual behavior and return meaning |
| --- | --- |
| [`cui_base_title_bar`](../../src/base/cui_chrome.c) | Registers pointer-activatable GROUP with no tab stop, then scans original events in order. Primary down arms move; first targeted move disarms and dispatches MOVE, consumes that event even if unsupported/false. Secondary down dispatches MENU only for client decoration and consumes even if unsupported. Two accepted primary releases within distance/time dispatch TITLEBAR_DOUBLE_CLICK. Cancel/outside down clears gesture state. Boolean ORs successful host returns only. |
| [`cui_title_bar`](../../src/components/cui_chrome.c) | Validates arguments/items, queries host once, executes Base before opening bar layout, renders custom items in order, then renders minimize, maximize-or-restore, close controls in that order. The queried info snapshot remains fixed for the submission even if a host callback changes external state. Boolean ORs Base and control results. |
| Private `cui_title_control` | Calls `cui_interact(BUTTON)`, paints control/icon, then on aggregate `input.activated` invokes close override or host perform. Close override takes precedence, returns no decision, and makes boolean true; it does not also call host CLOSE. Other controls return capability-gated host bool. Multiple accepted activations for one control in a frame coalesce into this one old dispatch. |

Client decoration alone shows portable controls: capability bits gate minimize and
current maximize/restore; CLOSE bit **or** old `on_close` enables close. Browser,
server-decoration and native-traffic-lights modes do not render these controls.
Custom item renderers own their own input/semantics and are not TitleBar actions.
The old `on_close` is synchronous inside control painting, before the outer bar
ends, not a window-destroyed callback. Base host calls precede custom item painting.

[`test_chrome.c`](../../tests/unit/test_chrome.c), `title_bar`, proves fake-host
close versus override, keyboard Enter maximize/restore, first-move dispatch,
double click, secondary menu, child-target isolation, decoration semantics and
paint. It does not prove OS service availability or all accessibility paths.

## Recommendation and explicit service exception

Retain host query/perform and close override as services, not rejectable value
requests or lifecycle events. `perform=true` is only the adapter's reported handling;
window state must be queried later. A notification after dispatch cannot cancel
work already performed. In particular CLOSE may only set a host quit flag.

An additive observer is worthwhile for uniform logging/application follow-up, but
must observe **actual synchronous dispatch**, not pretend to replace host policy.
Propose a separate `cui_chrome_events.h` with Base/Styled `*_title_bar_events`
facades returning `{bool acted; size_t action_count;}` and a callback-local typed
payload containing common metadata, `cui_window_action action`, an explicit
`HOST_PERFORM`/`CLOSE_OVERRIDE` route, and `bool handled`. Coordinates, if exposed,
need a separate `has_position` flag: Base forwards real x/y, control services use
old synthetic 0/0 and must not advertise those as pointer evidence. No fabricated
before/after maximized state, CHANGE, COMMIT, destroyed lifecycle or rejection.
Declare only reviewed implemented fields; do not change old options/state layouts.

Use narrow private common hooks rather than wrapping host callbacks: hooks see the
actual action, target and input seam without replacing shared host/close userdata
or accidentally observing query/custom renderer operations. Existing entry points
use the same code with no observer. Observe immediately after the original
`perform` returns (including false), or after original `on_close` returns. Only a
real call counts: unsupported actions/missing perform have no dispatch callback;
observer presence must not enable controls or services. Nil observer preserves
all legacy writes/host calls and yields zero actual callbacks. No allocation,
collector workspace, event replay, input consumption or new dispatch system.

Implement Base service observations first and Styled control observations second
as independently verified slices. Base emits per actual event dispatch, including
repetition; Styled controls retain their legacy zero/one dispatch per control,
fixed category order and activation coalescing. This is a documented aggregation
exception, not a claim of per-input control callbacks.

## Evidence, passive work and lifetimes

Capture evidence **before** host/close/custom callbacks can mutate unrelated UI.
Base already has the genuine frame index/type/target. MOVE and MENU are raw
pointer dispatches, not accepted activations: use real indexed source facts only.
For double click copy the actual accepted second-release activation snapshot when
available. Styled controls register BUTTON before inspecting activation; enumerate
matching accepted activation evidence after interaction and before painting custom
icons/dispatch. Preserve a unique genuine snapshot; for coalesced multiple causes
use proven frame/UNKNOWN/no index rather than selecting an arbitrary last event.
Metadata target should identify the compound TitleBar; carry the actual control
ID separately and preserve snapshot's original target. No early registration.

There is no general disabled flag to forward. Capability/decorations are service
availability, not disabled state. Base has no explicit passive/inert check around
its event loop, unlike its `cui_interact` call: do not claim that all host callbacks
are suppressed in those scopes merely because semantics are absent. Before any
facade promises observer silence, test actual `cui_frame_events`/target behavior
under inert/layout scopes and preserve old gesture/host behavior. Observer policy
may suppress passive notification without silently suppressing old host effects;
any demonstrated legacy safety fix must be separately reviewed.

Payload scalars/metadata can be copied; any future text pointer must have explicit
borrow/copy policy (none needed here). Descriptors, host, state, UI and users must
survive the whole call and renderer consumption. Host callbacks already execute
synchronously and may block in platform services; no worker-thread delivery is
implied. New observers may copy payloads/set unrelated flags, but may not mutate
active state/items/host descriptors, replay input, recursively submit UI, destroy
context/window, or unwind foreign exceptions. Defer close teardown/omission until
outer return and renderer completion. Existing host side effects cannot be rolled
back; absence of a handler is observational, not readonly.

## Host and binding limits

[`cui_platform.h`](../../include/cui/cui_platform.h) is the authoritative service
contract. [`cui_sdl3.c`](../../backends/sdl3/cui_sdl3.c), window query/perform,
currently advertises CLOSE; conditionally minimize and resizable zoom actions;
non-mac resizable double-click. MOVE/MENU are not advertised/implemented there.
Dummy/offscreen drivers omit the conditional capabilities. macOS reports native
traffic lights. Do not infer real Cocoa/Windows move/menu support from fake-host
unit tests, or infer completion from an SDL function return. This audit makes no
native runtime claim and needs no external dependency or OS experiment.

[`bridge_render_chrome`](../../src/bridge/cui_bridge.c) constructs TitleBar with
only name, no window host or close callback, and ignores its bool. Python/JS/TS/Go/
Rust node factories therefore do not imply these C callbacks/services are wired.
Surface `on_close` APIs are a different domain; do not claim TitleBar support from
their name. New foreign callback registration, lifetime and exception containment
would require a separate bridge/binding slice.

## Verification required before implementation handoff

Extend focused Chrome tests with old/new comparisons of gesture state, returns,
host logs/order, semantics, commands and focus. Cover multiple Base actions in one
frame, same-control coalescing, cross-control fixed order, maximize snapshot versus
host mutation, false host results, missing capability/perform/query, override
precedence, unique and ambiguous evidence, callback mutation of unrelated flags,
copied payload/deferred omission, actual inert/layout scopes and child routing.
Use genuine pointer and Enter activation; separately test supported accessibility
activation through public Core API before claiming it. Do not invent a keyboard
shortcut for dragging/menu/double click. Strict C11/C++ headers and private-wrapper
Chrome/Core tests are required; fake-host success is not OS/GPU acceptance.

## Reviewed refinement: exact semantics and implementation decision

This section supersedes the earlier tentative recommendation to add a shared
metadata-envelope facade immediately. **Recommend retaining the explicit
host-service exception for this batch**, not manufacturing an event kind. Existing
host `perform` and `on_close` already give applications synchronous extension
points; application-owned wrappers can log dispatch/results without changing CUI
routing. No evidence here establishes a missing user-value notification or an
inability to execute an action. Consistency does not require renaming services.
A separate observer remains optional future work only if a concrete application
needs independent listeners without wrapping its host. Root review is required.

### Why the shared kind is not honest

[`cui_component_events.h`](../../include/cui/cui_component_events.h) has ACTIVATE,
REQUEST, CHANGE, COMMIT, SUBMIT and LIFECYCLE. A raw first move is not an accepted
activation. A post-`perform` callback is not a rejectable REQUEST, confirmed CHANGE,
COMMIT or lifecycle transition. Close override returning void is not acceptance.
Do not set ACTIVATE merely to obtain the existing metadata struct or add a global
DISPATCH kind for this one service. Existing host bool means exactly “returned true
from this invocation”; call it `host_returned_true`, not completed/succeeded/applied.

### Exact alternative, if separately approved

A future service-only header can define the following portable additive API without
changing shared kinds or old struct layouts. These names remain a proposal:

```c
typedef enum {
    CUI_TITLE_BAR_DISPATCH_HOST,
    CUI_TITLE_BAR_DISPATCH_CLOSE_OVERRIDE
} cui_title_bar_dispatch_route;
typedef struct {
    cui_id target;             /* compound TitleBar */
    cui_id control;            /* actual control ID, or 0 for title-area gesture */
    cui_window_action action;
    cui_title_bar_dispatch_route route;
    bool host_returned_true;   /* meaningful only for HOST route */
    bool has_position;
    float x, y;               /* meaningful only when has_position */
    unsigned evidence_flags;  /* existing HAS_FRAME/INDEX/SOURCE/ACTIVATION bits */
    uint64_t frame_sequence;
    size_t event_index;
    cui_component_event_source source;
    cui_activation_snapshot activation;
} cui_title_bar_dispatch_event;
typedef void (*cui_title_bar_dispatch_fn)(void *, const cui_title_bar_dispatch_event *);
typedef struct {
    cui_title_bar_dispatch_fn callback;
    void *user;
} cui_title_bar_dispatch_handler;
typedef struct {
    bool acted;               /* unchanged old function result */
    size_t dispatch_count;    /* actual observer calls, not host calls if callback absent */
} cui_title_bar_dispatch_result;
cui_title_bar_dispatch_result cui_base_title_bar_dispatch_events(
    cui_context *, cui_id, cui_title_bar_state *, cui_window_info,
    cui_title_bar_options, cui_title_bar_dispatch_handler);
cui_title_bar_dispatch_result cui_title_bar_dispatch_events(
    cui_context *, cui_id, const cui_theme *, cui_title_bar_state *,
    const cui_chrome_item *, size_t, cui_title_bar_options,
    cui_title_bar_dispatch_handler);
```

The custom evidence fields intentionally have **no shared event kind**. Same flag
presence rules apply; absent evidence is zero-filled, UNKNOWN omits HAS_SOURCE.
Never infer source from action alone. Copy real unique accepted snapshots before
painting/host callbacks; retain their original target even though `event.target`
is compound and `control` names the affected button. Base indexed pointer facts
are copied directly. No pointer coordinates for keyboard/a11y controls or host
control arguments `(0,0)`. CLOSE_OVERRIDE fixes action CLOSE and route override;
`host_returned_true=false` is explicitly inapplicable, while legacy `acted=true`.
No extra “accepted”, attempted, pending, completed or destroyed field.

No callback when no old function is invoked: capability suppression, absent host
or perform, missing controls and passive work do not become dispatch events.
Callback absence leaves `dispatch_count=0`; old calls/results still occur.
Count only after actual observer invocation. Base invokes at most once per input
event (branches dispatch on mutually exclusive types); Styled adds at most three
control dispatches. Core bounds event count, and existing `CUI_CORE_EVENT_MAX` is
far below `SIZE_MAX-3` on supported targets; implementation must check that bound
with a compile-time assertion or checked counter, never wrap/saturate silently.
No dynamic workspace, unbounded dispatch, callback reentrancy or invented overflow
status. If the invariant cannot be established without exporting private Core
limits, prefer a local checked increment with pre-proven `event_count <= SIZE_MAX-3`
before any dispatch; failure policy needs review rather than changing legacy calls.
This complexity further weakens the case for an unrequested service facade.

### Passive evidence resolved from source, remaining test gap

`cui_frame_events` returns the event buffer even inside inert/layout scopes, but
`cui_event_target` and `cui_event_activation` return zero when `cui_core_inert(ui)`.
Thus inert Base cannot match its nonzero ID; CANCEL still clears gesture state and
any primary UP clears arming even without a target. Preserve those maintenance
writes. `cui_interact` also returns empty interaction in inert scope, so Styled
controls have no activation there. **Layout-only differs:** target lookup does
not check layout-only, whereas `cui_frame_sequence` returns zero. Source therefore
does not justify claiming layout-only host-dispatch silence; a TitleBar Base loop
can still match a previous target. Do not silently change that old behavior in an
observer patch. New observations, if approved, should be admitted only when
`cui_frame_sequence(ui) != 0`, while preserving any old passive calls/writes;
this is an explicit notification policy, not proof that service effects stopped.
The exact actual-scope regression remains required before production.

The current signed checkpoint is design/source audit only. No tests were run for
this refinement, no host capability was added, and neither the shared event API
nor foreign bindings now expose these proposed declarations.
