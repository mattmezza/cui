# Proposed activation and discrete-value event slice

Status: design only, based on `71b72f96`. No declarations below are shipped or approved by this document. Read the [contract](COMPONENT_EVENTS_CONTRACT.md), [78-family inventory](COMPONENT_EVENTS_COVERAGE.md), [bindings audit](COMPONENT_EVENTS_BINDINGS.md) and [implemented metadata utility](../COMPONENT_EVENTS.md) together. Preserve all 78 families. This slice covers direct portable C Button, Checkbox, Switch and Toggle only, not groups, binding requests, continuous editing or global routing.

## Source facts and compatibility decisions

| Control | Existing observable behavior | Consequence for this slice |
| --- | --- | --- |
| Button | [`cui_button_ex`](../../src/components/cui_components.c) freezes each click payload, invokes legacy `on_click` then `on_click_event` per accepted event, then paints. Hover metadata/callback work precedes click iteration. Disabled/loading block activation. Inert/layout-only ancestry paints without normal lifecycle mutation. | Adapt the existing per-event typed callback; preserve both old callbacks and their order. Reuse its already-frozen `evidence`, not a new lookup after legacy callbacks. |
| Checkbox | [`cui_base_check_ex`](../../src/base/cui_base.c) snapshots the value and requested tri-state once per call, iterates accepted events and invokes requests individually. Callback controls acceptance; missing callback with `controlled=true` is read-only. Source Styled checkbox suppresses pointer focus; Base defaults to pointer focus. Legacy simple Checkbox still uses aggregate mutable behavior. | New by-value controlled facade can reuse extended Check with local snapshot and forwarding callback. Preserve mixed -> checked, snapshot-once multiplicity and source/Base focus differences. Leave all legacy functions unchanged. |
| Switch | [`cui_switch_ex`](../../src/components/cui_components.c) calls legacy Base binary reduction on a local bool, then emits at most one aggregate `on_change`; it reads the external owner value again for semantics and painting after that callback. | A new per-event request API cannot be a wrapper around that callback. Add a separate Base reducer and split out paint-only Styled composition without changing the old path. New snapshot behavior must be explicit. |
| Toggle | [`cui_toggle_ex`](../../src/components/cui_components.c) likewise collapses activation to one local bool request; paint-only `cui_toggle_paint` already exists. | New Base request reduction plus existing paint is feasible. Never silently upgrade the legacy aggregate callback to per-event dispatch. |

Evidence tests inspected, not rerun for this documentation-only change: [Button event multiplicity](../../tests/unit/test_button_callback_events.c), [Button evidence snapshots and mutation](../../tests/unit/test_button_click_snapshot.c), [Button passive lifecycle](../../tests/unit/test_button_passive_lifecycle.c), [Checkbox controlled/source contracts](../../tests/unit/test_checkbox_source.c), and [controlled siblings](../../tests/unit/test_controlled_siblings.c). Existing test coverage is not a general reentrancy or teardown guarantee.

## Exact additive declarations proposed

Put typed request/activation declarations and Base entry points in a new `cui_control_events.h` depending on Base and the existing shared metadata header. Styled declarations belong beside existing component entry points in `cui_components.h`. No platform types or component dependency in Base. These handler types contain context pointers, never untyped value payloads:

```c
typedef void (*cui_component_activation_fn)(
    void *user, const cui_component_event_metadata *event);
typedef struct {
    cui_component_activation_fn callback;
    void *user;
} cui_component_activation_handler;

typedef struct {
    cui_component_event_metadata event; /* kind REQUEST */
    bool current, requested;
} cui_component_boolean_request;
typedef void (*cui_component_boolean_request_fn)(
    void *user, const cui_component_boolean_request *request);
typedef struct {
    cui_component_boolean_request_fn callback;
    void *user;
} cui_component_boolean_handler;

typedef struct {
    cui_component_event_metadata event; /* kind REQUEST */
    cui_check_state current, requested;
} cui_component_check_request;
typedef void (*cui_component_check_request_fn)(
    void *user, const cui_component_check_request *request);
typedef struct {
    cui_component_check_request_fn callback;
    void *user;
} cui_component_check_handler;

typedef struct {
    bool valid, submitted, count_overflow;
    size_t activation_count, request_count;
} cui_component_action_result;

typedef struct {
    bool disabled, skip_tab, no_pointer_focus;
    int tab_index;
} cui_boolean_event_options;

cui_component_action_result cui_base_check_events(
    cui_context *, cui_id, cui_str name, cui_check_state value,
    cui_check_options, cui_component_check_handler);
cui_component_action_result cui_base_switch_events(
    cui_context *, cui_id, cui_str name, bool value,
    cui_boolean_event_options, cui_component_boolean_handler);
cui_component_action_result cui_base_toggle_events(
    cui_context *, cui_id, cui_str name, bool value,
    cui_boolean_event_options, cui_component_boolean_handler);

cui_component_action_result cui_button_events(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_button_options, cui_component_activation_handler);
cui_component_action_result cui_checkbox_events(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_check_state value, cui_checkbox_options, cui_component_check_handler);
cui_component_action_result cui_switch_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, bool value,
    cui_switch_state *, cui_switch_options, cui_component_boolean_handler);
cui_component_action_result cui_toggle_events(
    cui_context *, cui_id, const cui_theme *, cui_str name, bool value,
    cui_hover_state *tooltip_state, cui_toggle_options,
    cui_component_boolean_handler);
```

All value arguments are immutable per-call snapshots. A callback may assign the application's external value, but the new call keeps its original snapshot for subsequent requests, semantics and painting. The next submission observes the owner assignment. No getter hook, borrowed model pointer, hidden retained value, application acceptance bool or synthesized CHANGE notification is introduced. A future same-call owner-readback API would be a separate decision, not an undocumented callback effect.

For Check options, `on_change` and `change_user` must be NULL; likewise the old change callback/user fields in Checkbox/Switch/Toggle options. Reject conflicts before submission with `valid=false`; never silently discard or double-dispatch value handlers. Check's `controlled` setting is superseded by the immutable signature; this entry point is always controlled. Styled Checkbox always selects existing source focus behavior, regardless of `source_behavior`; Base preserves its supplied `no_pointer_focus`. Other appearance, required/invalid, role, slot and tab settings retain their documented meanings.

Button may retain legacy click callbacks for migration. New dispatch order is existing hover processing, then for each accepted event: freeze existing payload, legacy click, old typed click, new metadata handler; then existing paint/tooltip work. Forward original `action_user` independently from new handler user (including hover), and preserve artwork descriptor snapshotting. Document one side-effect path as recommended; using both handlers is an explicit migration/debug choice. No new hover metadata API is needed here.

## Result, timing and request rules

`valid` means arguments/options passed preflight; it does not promise that later rendering cannot fail. `submitted` means ordinary active submission was reached, including a disabled control; it is false outside submission or in inert/layout-only ancestry. Passive visual composition may still occur. No result claims that an external owner accepted a request.

`activation_count` counts supported accepted activations observed by this invocation, in original Core event-index order. `request_count` counts actual typed request callback invocations (zero for Button). Both use saturating `size_t` increments with a shared overflow flag; counts are per invocation, not accumulated across frames. Callback delivery continues in reducer order on counter saturation. Result reads are passive. Do not add an event log or last-value field to this small slice.

Each accepted event invokes Button's new handler once when present. Each accepted Check/Switch/Toggle event proposes the same inverse of the entry snapshot; mixed Check proposes checked. For two accepted activations in one call, both controlled requests carry the same current/requested pair, even if the owner accepts the first. This matches extended Check's current snapshot rule, while the old aggregate Switch/Toggle functions retain their single callback. Never derive per-event identity from aggregate `cui_interaction.activated`.

A missing request callback is read-only: no mutation and no request dispatch, but accepted activation count may still report an attempt, as existing controlled Check's bool does. Do not claim rejection unconsumes input or permits parent activation. Disabled and inert controls emit no user activation/request callbacks; Button loading has the same suppression. Switch, Toggle and Checkbox gain no invented loading state. Omitting the call emits nothing. No new commit, submit or lifecycle events are manufactured for discrete toggles.

Metadata uses `cui_component_event_from_activation` on a genuine frozen snapshot; Button uses existing payload evidence. Do not fabricate a pointer for keys/accessibility or reconstruct evidence after mutable callbacks. Payload pointers, handlers and users are borrowed during synchronous submission only; request/metadata structs are pointer-free and may be copied by value. Appearance resources/state keep existing longer lifetimes, including deferred label paint. There is no callback registration or end-frame queue.

## Implementation boundaries and the teardown decision

Smallest independently reviewable order: **(1) Button and Check**, using existing per-event seams; **(2) Boolean Base reducer and Toggle**; **(3) Switch paint extraction and facade**. Do not implement all four with an aggregate callback adapter. Proposed ownership for a later implementation approval: new control-events header, narrow additive declarations in Base/Components headers, Base event reducer source, Components forwarding/paint extraction, dedicated unit fixtures, guide and integration example. Root retains CMake/module/export/binding/generator ownership.

The new Boolean Base reducer must register each role's normal semantics/focus policy once, iterate accepted snapshots without duplicate reduction, then preserve controlled snapshot semantics. Do not implement it by publicly calling Check with a different role: Check adds required/focusable/pointer-activation details and has a distinct disabled-parent behavior. Share private accepted-event iteration only after semantic-policy tests establish equivalence. Extract Switch painting mechanically and prove the old mutation/callback-before-paint sequence unchanged.

Callback mutation supported here: change unrelated application state or the owner's external value, request view removal/close in application state, and omit later siblings through ordinary caller control flow. Descriptors are passed by value; changing the caller's descriptor affects later invocations only. Active state/resources/users/labels must remain alive through their existing boundary. Consuming later events can change their later eligibility; do not pre-queue every future callback merely to simplify iteration.

**A reliable context-destruction guard is not feasible in a facade-only slice.** [`cui_destroy`](../../src/core/cui.c) calls `cui_core_mutation`, whose guards cover diagnostic/measurement/prepaint phases, not arbitrary ordinary component callback execution. It can free the context while a submitting component still holds it. A local `busy` flag or checking frame sequence after return cannot detect that safely; reading the freed context would itself be invalid. Different wrapper workspaces also cannot enforce per-context recursive-entry safety.

Recommendation: keep recursive same-component submission, recursive frame entry, destruction of the active context, and freeing active storage explicitly unsupported preconditions in this slice, as the committed contract already states. Do not advertise a runtime guard. The application marks pending teardown and performs it after submission/frame completion. If root requires deterministic rejection or deferred teardown instead, stop before production implementation and authorize a separate Core-owned per-context dispatch-depth/destruction design, with legacy compatibility tests. Bridge runner deferred destruction is not a portable Core guarantee. No test should deliberately execute undefined use-after-free and count a non-crash as proof.

## Required proof before integration

- Multiple accepted pointer, touch, Enter, Space and accessibility events: callback order, exact count, no repeat/cancel activation, real frozen metadata and retention after context destruction performed **after** frame completion. Pointer clicks preserve raw contact/device evidence.
- Button migration trace: hover, legacy click, old typed click and new callback order; separate old/new users; legacy callback consumes accessibility input or mutates artwork descriptors; new metadata remains frozen. Run existing click snapshot/artwork/passive lifecycle tests unchanged.
- Controlled value owner accepts/rejects and changes unrelated state; current/requested remain entry snapshots through several events; no hidden writes, CHANGE notification or duplicate frame-result effects. Missing handler is read-only; conflicting old/new request handlers fail before semantics, callbacks or paint.
- Check mixed transition, Base versus source pointer focus, required/invalid/role override/tab order/indicator slots and disabled parent bubbling. Run source Checkbox and controlled-sibling suites unchanged.
- Boolean role semantics, disabled parent routing, focus/tab/keyboard/a11y policy and occlusion match each family, rather than inheriting Check behavior. Legacy Switch/Toggle still dispatch once for multiple accepted events; new facades dispatch once per accepted event.
- Disabled/loading/inert/layout-only/outside-frame and measurement/prepaint calls: zero event callbacks/counts, correct submitted flag, preserved passive state behavior. Failure preflight leaves caller values/state untouched.
- Mutation requests remove a later sibling or schedule application close; no handler retains freed userdata, removal happens only after the active call returns, and teardown occurs after frame completion. Recursive misuse is documented unsupported unless the separately approved Core guard exists.
- Counter saturation helper tests without fabricating a huge event allocation; returned result copies never redispatch. Strict portable C11 builds and meaningful CTest through the invisible wrapper; browser tests stay headless. No changes to bridge queued notification timing or false binding coverage claims.

Human/LLM guide and generated resource/index updates must accompany an approved implementation, plus one real owner-accept/reject example. These additions do not provide synchronous rejection through existing language bindings; that remains separately reviewed work.
