# Settings navigation events

Include `cui/cui_settings_events.h`. `cui_settings_events` preserves the existing
Styled Settings operation and adds `on_navigation`. It reports applied page/group
selection, not an owner-rejectable request. Missing observers permit all legacy
work and ignore user. No Base filter/reset helper or field callback contract changes.

```c
static void navigation_changed(void *user, const cui_settings_navigation_change *change) {
    bool *save_navigation = user; /* unrelated application flag */
    *save_navigation = true;
    /* Copy *change to retain its pointer-free page/group and metadata values. */
}
/* During UI submission with existing caller-owned pages/state/options: */
/* cui_settings_events(ui, id, theme, pages, count, &state, options,
       (cui_settings_handler){navigation_changed, &save_navigation}); */
```

The payload contains `previous_page`, `previous_group`, `page`, `group` using
source indices; `CUI_SETTINGS_NO_GROUP` selects the page as a whole. Filtering
does not renumber indices. Reordering caller arrays changes their meaning.
Notifications run immediately after the existing navigation writes, pending-group
scroll policy and invalidation, before content/filter-scroll resolution and paint.
A group callback can see pending scroll true even when content clears it later.
A same-pair activation can still restore pending scrolling without emitting CHANGE.
Defaults, query reconciliation, search/filter repair, scrolling, resizing, reset
and field callbacks never become navigation events.

`result` is the exact legacy aggregate `cui_settings_result`, including filter
selection repair. `navigation_count` counts actual observer calls, saturating at
SIZE_MAX with `count_overflow`. Real A→B→A navigation emits both transitions even
when the final pair equals entry; the legacy selection flag is preserved. Sidebar
processes items in traversal order, each scanning its own inputs in order. Across
items callback input indices therefore need not increase. This facade does not
reorder dispatch into global input order or replay reducers.

Metadata targets the Settings compound. The private navigation descriptor matches
each existing Sidebar callback to its next actual same-trigger activation; accepted
snapshot evidence is frozen before writes when available. Copied `activation.target`
retains the original derived page/group navigation ID. If no accepted snapshot is
available, only actual frame/index/type facts are attached. An unexpected callback/
activation mismatch has proven frame only, UNKNOWN source and no index/activation.
Presence flags are authoritative. No early semantic registration, consume, extra
reduction or invented activation is used. Passive/layout-only/inert submissions
produce no observation while retaining legacy maintenance and rendering behavior.
Sidebar TREEITEM accessibility ACTIVATE is not currently supported by Core; this
facade adds no accessibility routing or promise beyond existing Sidebar behavior.

New admission requires nonnull context/theme/state, a nonzero compound ID, and a
page array when count is nonzero. Old validation/filter behavior then applies.
Handler/user/payload pointer are borrowed synchronously. Do not mutate/rebind/free
active page/group/item arrays, values/editors/states, dropdown workspace, labels,
callback resources, input or context, or recursively submit UI in the observer.
Unrelated application flags may change; defer removal/omission/teardown until outer
return plus renderer consumption. Legacy custom search/render/change/reset callbacks
keep their timing and must preserve resources needed by the remaining old operation.

Only navigation is typed here. Scalar, text, reset, search and viewport observers
need their own slices. C bridge and Python/JS/TS/Go/Rust still use their existing
Settings callback/polling paths; no per-navigation foreign callback or synchronous
request is implied. Focused C tests do not establish browser or binding parity.
