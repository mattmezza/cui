# Tabs aggregate selection observers

Include `cui/cui_tabs_events.h` for `cui_base_tabs_events` and `cui_tabs_events`.
They add an optional applied selection CHANGE observer to existing Base/Styled
Tabs, preserving the old APIs, options, bool results, input handling and painting.

```c
static void tab_changed(void *user, const cui_tabs_selection_change *event) {
    app_state *app = user;
    app->last_tab = event->value; /* unrelated application state */
}
cui_tabs_event_result result = cui_tabs_events(
    ui, container_id, &theme, items, count, &selected, options,
    render_panel, panel_user, (cui_tabs_handler){tab_changed, app});
```

The Base variant draws nothing and takes an explicit nonzero compound `target`
before the item array; Styled uses its nonzero container ID. These IDs identify
the aggregate event, not an invented activating tab. Null context, zero target or
null selected storage reject before calling the legacy reducer. Other legacy
validation and maintenance remain unchanged. `result.changed` is the exact old
bool, which is not a successful-admission/submission signal.

At most one callback runs after Base finishes its selection and semantic/roving
focus finalization, before Styled reveal/animation updates, paint, render slots
and panel callback. The selected pointer has already been written. A missing
observer still permits legacy writes and has zero callback count; its unused user
pointer is ignored. Reading either result field does not dispatch again.

Payload `previous` and `value` are stable selected IDs. All payload fields are
pointer-free and may be copied. Metadata has CHANGE, the compound target, UNKNOWN
source and HAS_FRAME with the actual active frame sequence. There is no event
index, activation or reason: overflow MenuTree reduction and direct tab navigation
are separate passes, so a final selection cannot honestly identify one input.

This is an explicit aggregation exception. Multiple transitions coalesce; if the
final ID equals the initial ID, there is no callback. Selecting the same tab,
manual focus-only navigation, opening/closing overflow, scrolling and reorder
requests do not by themselves report selection CHANGE. Existing drag request
fields remain separate and the caller still applies reordering after the frame.
The observer cannot reject selection that has already been applied.

No callbacks are delivered outside an active painted submission, from layout-only
work or when globally disabled. The wrapper still calls the legacy Base reducer
for its normal maintenance, including disabled drag cleanup and empty overflow
cleanup. Inert/modal/consumed-input behavior remains Core's routing behavior;
there is no new readonly/inert policy and no synthesized input for maintenance.
Invalid or disabled selected IDs remain caller-owned as before; no repaired value
or synthetic change is introduced.

Observer/user are borrowed during submission. Keep selection, item arrays, labels,
state, theme, render/panel users and overflow scratch alive and unchanged. Do not
mutate/free active resources, destroy the context or recursively submit UI from the
observer. Set unrelated flags to request omission/removal after the outer call;
wait through frame/render consumption before teardown. The indicator's measured
bounds and other retained state keep their existing longer lifetimes. Omission
does not dispatch cancellation of retained work. Render/panel callbacks retain
legacy order; their own arbitrary model mutations are not Tabs reducer events.

`tests/unit/test_tabs_events.c` compares old/new selection, focus, panel choice,
semantics and draw commands. It covers queued pointer/key selection, same-tab
silence, multiple/net-zero transitions, manual navigation, absent observer,
callback-before-panel order, consumed input, disabled/inert/layout-only work,
invalid selected IDs, copied payload lifetime, deferred omission, overflow
selection, real reorder-request silence, and disabled/empty Base maintenance.
Legacy `test_choice.c` remains required. Native tests run through the private
display wrapper. No new browser, host accessibility or foreign-binding proof is
claimed; this slice adds no REQUEST/COMMIT/ACTIVATE or popup lifecycle facade.
