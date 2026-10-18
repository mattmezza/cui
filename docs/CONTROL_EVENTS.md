# Button and Check event facades

`cui_control_events.h` and the Styled declarations in `cui_components.h` add typed synchronous event facades for Button and Checkbox/Check. Existing entry points keep their behavior. [Switch/Toggle facades](BOOLEAN_EVENTS.md) are also implemented; group controls and language request bindings remain deferred; this is not uniform coverage of all 78 families. See the [design checkpoint](design/COMPONENT_EVENTS_CONTRACT.md), [coverage inventory](design/COMPONENT_EVENTS_COVERAGE.md) and [binding limitations](design/COMPONENT_EVENTS_BINDINGS.md).

## Controlled Checkbox requests

The value is an immutable entry snapshot. The application accepts by assigning its own value, or rejects by leaving it unchanged. Subsequent events, painting and semantics in this invocation still use the entry snapshot; the next submission observes accepted owner state. No notification of an external assignment is synthesized.

```c
#include <cui/cui_components.h>

typedef struct {
    cui_check_state checked;
    bool allow_changes;
} account_state;

static void request_check(void *user, const cui_component_check_request *request) {
    account_state *account = user;
    if (account->allow_changes)
        account->checked = request->requested;
}

static cui_component_action_result account_checkbox(cui_context *ui, cui_id id,
                                                     const cui_theme *theme,
                                                     account_state *account) {
    return cui_checkbox_events(ui, id, theme, cui_string("Receive updates"),
        account->checked, (cui_checkbox_options){0},
        (cui_component_check_handler){request_check, account});
}
```

`cui_component_check_request` contains typed `current` and `requested` states plus `event` metadata with kind `REQUEST`. Unchecked or mixed requests checked; checked requests unchecked. Every accepted activation produces a separate request from the same entry value. Two activations in one submission therefore propose the same next value, even if the first is accepted. Missing handler is read-only: no value write and zero requests, although accepted activation attempts are still counted.

`cui_base_check_events(ui, id, name, value, options, handler)` supplies drawing-free behavior inside a caller box with the same ID. Base preserves `cui_check_options.no_pointer_focus`; Styled `cui_checkbox_events` selects source Checkbox behavior, preserving existing focus on pointer activation. Keyboard tab/focus policies, role overrides, required/invalid metadata, indicator/style slots and motion remain the existing Checkbox policies. Omitted options default as before except that these new by-value entry points are always controlled. Old `on_change` and `change_user` must both be NULL; conflicting handler configuration fails preflight rather than being ignored or dispatched twice.

## Button activation

`cui_button_events(ui, id, theme, name, options, handler)` uses `cui_component_activation_handler` with a callback `(void *user, const cui_component_event_metadata *event)`. Its event kind is `ACTIVATE`, not value change. A missing new handler still reports accepted activation counts.

Legacy options remain usable for migration. Existing hover processing precedes click dispatch. Per accepted event the order is legacy `on_click`, old `on_click_event`, then the new handler, followed by existing paint/tooltip work after iteration. Existing click evidence is frozen before the legacy callback; a callback consuming accessibility input does not erase evidence delivered to the new handler. Original `options.action_user`, including hover, stays separate from the new handler's user. Prefer one callback or frame-result path for each side effect; performing the same action from both dispatch and result duplicates it.

## Results and lifecycle

`cui_component_action_result` contains:

| Field | Meaning |
| --- | --- |
| `valid` | Arguments/options passed preflight; later Core/paint errors remain possible. |
| `submitted` | Ordinary active submission reached, including disabled controls; false outside submission or in inert/layout-only ancestry. Passive visuals may still compose. |
| `activation_count` | Accepted activations observed by this invocation. |
| `request_count` | Check request callback invocations; zero for Button. |
| `count_overflow` | A count saturated at `SIZE_MAX`; dispatch continues in event order. |

Results aggregate one invocation; copying/reading a result never dispatches or mutates a value. Counts do not report owner acceptance. Disabled/inert controls emit no user callbacks. Button loading also suppresses activation; Checkbox gains no loading state. Missing/omitted immediate calls emit nothing. No COMMIT, SUBMIT, CHANGE, propagation control or default-prevention promise is added to these discrete APIs.

Payload/handler/user are borrowed through synchronous submission only. Metadata and Check request structs are pointer-free and copyable by value, including beyond frame/context lifetime. Appearance strings, icon resources and caller state retain their existing boundaries; some paint resources must live through renderer completion. [Shared metadata](COMPONENT_EVENTS.md) explains real input evidence, absent fields, context-local identities and safe copying of other borrowed UTF-8 payloads. Copying an arbitrary `cui_event` is not a text deep copy.

Callbacks may update unrelated application state and the external value owner. Changing a caller descriptor affects a later invocation, not the copied descriptor in flight. Consuming later input can change its later eligibility; callbacks are not prequeued. Request view removal or application shutdown in application state, then perform removal/freeing/teardown after the active submission and frame finish. Do not recursively submit the same component/frame, destroy the active context, or free active users/state/resources during dispatch. **There is no runtime context-destruction guard** in this slice. Bridge runner deferred destruction does not imply such a Core guarantee.

The header's `cui_control_events_private_*` identifiers are forwarding implementation details, not application entry points. No allocation, registration, worker, scheduler, Core event routing or backend changes are introduced. New source APIs are not automatically exposed by existing SDK/binding event queues. The integrator owns module registration, generated distributions/API indexes and consumer human/LLM resources.

The focused `tests/unit/test_control_events.c` covers per-event legacy/new order and users, pointer/touch/key/accessibility evidence, rejected and accepted owners, immutable multi-request snapshots, next-submission semantics, read-only/missing handlers, Base versus Styled focus, disabled/loading/passive suppression, invalid handler conflicts, outside-frame calls and saturation bookkeeping. Existing Button and Checkbox suites remain compatibility checks. Run registered tests without desktop windows:

```sh
python scripts/test_local.py --build-dir build -- -R 'control_events|button_click|button_callback|checkbox_source'
```

## Run a complete native example

The [settings example](../examples/control_events/README.md) builds a real SDL window with an owner-controlled Checkbox, an Apply button and deferred panel removal. It demonstrates acceptance and rejection, passive result inspection without duplicate side effects, and the next-submission snapshot boundary. Its README includes exact dependency, compiler, font, build and invisible-test commands. The separate [async fetch example](../examples/async_fetch/README.md) demonstrates returning promptly from a synchronous handler and applying worker completions on the UI thread.
