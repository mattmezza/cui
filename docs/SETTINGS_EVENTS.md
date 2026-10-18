# Settings applied events

Include `cui/cui_settings_events.h`. `cui_settings_events` preserves the existing
Styled Settings operation and adds navigation and built-in scalar observers.
`on_navigation` reports applied page/group selection, not an owner-rejectable request. Missing observers permit all legacy
work and ignore user. No Base filter/reset helper or field callback contract changes.

```c
static void navigation_changed(void *user, const cui_settings_navigation_change *change) {
    bool *save_navigation = user; /* unrelated application flag */
    *save_navigation = true;
    /* Copy *change to retain its pointer-free page/group and metadata values. */
}
/* During UI submission with existing caller-owned pages/state/options: */
/* cui_settings_events(ui, id, theme, pages, count, &state, options,
       (cui_settings_handler){.on_navigation = navigation_changed, .user = &save_navigation}); */
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

Navigation and built-in scalar field changes are typed here. Text, reset, search
and viewport observers need their own slices. C bridge and Python/JS/TS/Go/Rust still use their existing
Settings callback/polling paths; no per-navigation foreign callback or synchronous
request is implied. Focused C tests do not establish browser or binding parity.


## Built-in scalar fields

Set `on_boolean`, `on_number` and/or `on_choice` in `cui_settings_handler` for the
corresponding `cui_settings_bool_change`, `cui_settings_number_change` and
`cui_settings_choice_change`. They share the borrowed handler user pointer.
Each payload identifies `page`, `group`, `item` source indices and the derived
`field_id` (`cui_settings_part_id(root, page, group, item, 24)`). Metadata target
remains the Settings compound. Payloads are pointer-free and copyable by value.

Boolean covers built-in Switch and Checkbox; Number carries actual doubles; choice
carries owner choice IDs for both Menu-backed dropdown and legacy Select. Custom
render overrides bypass these observers even when host rendering edits an owner.
No text value, reset reason, request, COMMIT or per-input event is invented.

Capture occurs immediately before one existing built-in adapter. The adapter runs
once, records the existing first edit error, and invokes the old item `on_change`
at its original point. Only then does the typed observer compare/read the actual
final owner. A legacy callback restoring entry value produces no typed CHANGE;
redirecting it reports the redirected value. Callback count does not replay effects.
Two nested edits ending at entry produce no aggregate CHANGE. Existing Switch may
coalesce several activations into one flip; the facade preserves that behavior.
Each adapter emits at most one typed callback in existing content order. Scalars
are **aggregate notifications**, unlike per-navigation transitions.

`boolean_count`, `number_count`, `choice_count` independently count actual calls,
saturating at SIZE_MAX and setting shared `count_overflow`. Missing typed handlers
preserve all old writes/callbacks. Disabled fields and passive/layout-only/inert
submissions retain legacy maintenance, including Number bounds/last-value writes
and old callbacks, but emit no new typed scalar notification. Enabled real final
owner changes can be notified even with a nested edit error; errors and invalid
flags are preserved, never used as an artificial change gate.

Scalar metadata has a proven nonzero active frame, UNKNOWN source and no input
index/activation. A whole adapter and its legacy callback may combine inputs or
apply bounds without input, so a unique causal event cannot honestly be inferred.
An enabled bounds adjustment is an applied change, not proof of user interaction.

Number equality treats both NaNs as unchanged and signed zeros as equal. Payload
`previous_finite` and `value_finite` describe the actual numeric values, not their
acceptance by Number. Settings initially validates finite owners, but old callbacks
can redirect a valid edit to infinity/NaN or change a later field after validation.
These facts remain visible; the observer does not clamp again, repair the owner,
resynchronize the draft or rewrite `number_state.last_value` after the callback.
Settings' existing extra bounds/last-value pass stays before old `on_change`;
therefore the callback's final owner can intentionally differ from draft/last_value
until later old synchronization. Invalid nonfinite owners may fail the next filter.

Old callbacks must leave observed scalar owner pointers readable through typed
notification. The new observer cannot mutate active owners or other active model
resources; use unrelated flags and defer teardown. Entry is captured at each field,
not at Settings-call entry, so earlier host changes are not independently reported.
Reset helpers and reset-button effects have no new typed reset notification.
This extends the new C structs and requires rebuilding consumers with this header;
existing legacy Settings options/state/result layouts and function remain unchanged.
