# RadioGroup selection notifications

`cui_selection_events.h` and `cui_radio_group.h` add synchronous Base/Styled RadioGroup CHANGE observers. [Select observers](SELECT_EVENTS.md) have their own guide; Combobox remains [proposed](design/SELECTION_EVENTS_SLICE.md). No bridge/binding selection allowlist or uniform family coverage is implied.

```c
#include <cui/cui_radio_group.h>

static void selection_changed(void *user, const cui_selection_change *change) {
    cui_id *last_seen = user;
    *last_seen = change->value;
}

/* Inside an active frame; items, selected and focus_state are app-owned. */
static cui_radio_group_event_result choices(cui_context *ui, const cui_theme *theme,
    const cui_radio_group_item *items, size_t count, cui_id *selected,
    cui_radio_group_state *focus_state, cui_id *last_seen) {
    return cui_radio_group_events(ui, 10, theme, items, count, selected, focus_state,
        (cui_radio_group_options){.vertical = true},
        (cui_selection_handler){selection_changed, last_seen});
}
```

`cui_base_radio_group_events` has the legacy Base arguments followed by a `cui_selection_handler`; it draws nothing. `cui_radio_group_events` similarly appends the handler to the Styled arguments. Return `.legacy` retains the original `valid/requested/changed/tab_stop` meanings. `.events` reports preflight `valid`, active `submitted`, observer invocation `change_count`, and `count_overflow`. Missing observer preserves normal value mutation and reports zero callbacks. Counts saturate at `SIZE_MAX`; dispatch continues. Disabled controls can submit but emit no notifications; inert/layout-only submissions emit none.

Selection remains caller-owned through `cui_id *selected`. Without old `options.on_change`, eligible input writes that value. With it, the old synchronous callback still requests an item and may accept, reject or assign another value. Its `change_user` remains independent of the observer user. The observer runs immediately after that old operation only when the actual value changed. Rejection without change, already-selected activation, focus-only changes, and external assignments emit no CHANGE. No new callback can veto the operation. Later eligible events read current owner state; this is not the immutable entry-value policy of controlled Check.

The pointer-free `cui_selection_change` copies `previous`, actual post-operation `value`, attempted `item`, reason ACTIVATION or NAVIGATION, and `cui_component_event_metadata event`. Metadata kind is CHANGE and `event.target` is the public RadioGroup ID. RadioGroup preserves legacy semantic registration after reduction, so Core accepted activation snapshots are unavailable before the old callback. This facade deliberately omits HAS_ACTIVATION for every path. It freezes only genuine frame/index/source facts from the triggering event before callback consumption; pointer-up, keyboard and accessibility event types establish their respective source, without copying/inventing pointer coordinates, touch details or an accessibility activation snapshot. Navigation likewise has no activation snapshot. [Shared metadata](COMPONENT_EVENTS.md) documents presence flags and context-local identities.

Dispatch follows the existing reducer event order, then existing semantics/paint. A payload captures the transition before the observer runs. Observer writes to the owner affect later input and painting, but do not recursively synthesize notifications. Legacy `.changed` retains its existing operation comparisons; it is neither a count nor proof that the final value differs from frame entry. Do not perform the same side effect through both callback and result inspection.

Handler, user and payload are borrowed only during synchronous submission; copying the pointer-free payload preserves it beyond frame/context lifetime. Keep caller arrays, active state, users and render resources alive for their existing submission/render boundaries. Callback changes to copied options/handler descriptors affect later invocations. Consuming later input changes later eligibility. Schedule removal/teardown in application state and perform it after submission/frame completion: context destruction, freeing active state/resources, and recursive same-frame submission during callbacks remain unsupported. No runtime destruction guard, queue, scheduler or routing change is added.

`tests/unit/test_selection_events.c` exercises real Base and Styled reducers, owner rejection/acceptance/redirection, per-event navigation transitions, missing observers, accessibility source/index facts frozen before legacy consumption, omitted activation evidence, compound identity, consuming later input, disabled/passive suppression and copy lifetime. Legacy RadioGroup suites remain compatibility checks. Native tests use an invisible display:

```sh
python scripts/test_local.py --build-dir build -- -R 'selection_events|radio_group'
```

Module registration, generated API indexes/distributions and bindings are integrator-owned. Typed RadioGroup requests, Select/Combobox notifications, query-edit events, same-item confirmation events, popup lifecycle and multi-selection remain outside this slice.
