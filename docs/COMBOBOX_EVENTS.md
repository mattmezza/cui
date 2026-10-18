# Combobox selection notifications

`cui_base_combobox_events` and `cui_combobox_events` add synchronous selection CHANGE observers to single-selection Combobox. Legacy APIs, query editing, Select, List/Menu and multi-selection retain their behavior. Shared pointer-free payloads come from [selection events](SELECTION_EVENTS.md); this does not enable bridge typed-selection tags or uniform family coverage.

```c
#include <cui/cui_choice.h>

static void selected_changed(void *user, const cui_selection_change *change) {
    cui_id *last_seen = user;
    *last_seen = change->value;
}

/* During active composition; query, scratch, selection and popup belong to app. */
static cui_combobox_event_result search(cui_context *ui, const cui_theme *theme,
    cui_edit *query, const cui_choice_item *items, size_t count,
    cui_choice_item *scratch, size_t capacity, cui_id *selected,
    cui_choice_popup *popup, cui_id *last_seen) {
    return cui_combobox_events(ui, 2, theme, cui_string("Find account"), query,
        items, count, scratch, capacity, selected, popup,
        (cui_choice_popup_options){.viewport = {0, 0, 640, 480}},
        (cui_selection_handler){selected_changed, last_seen});
}
```

Initialize popup once with `cui_choice_popup_init` and a stable nonzero popup ID. The Base entry point matches `cui_base_combobox` arguments followed by a handler; the Styled entry point likewise appends a handler to `cui_combobox`. Base still draws nothing and handles query/Enter reduction; Styled adds query/option rendering and popup-list selection. The caller supplies a single-line editor and filtered scratch capacity at least equal to item count. Existing matching is ASCII case-insensitive substring search, byte-exact otherwise. Committed IDs remain stable under filtering and are not replaced by array indices.

`cui_combobox_event_result.legacy` preserves the complete old `changed/query_changed/scrolled/matching_count/error` result. `.events` contains preflight `valid`, active `submitted`, actual observer invocation `change_count`, and `count_overflow`. Counts saturate at `SIZE_MAX` while dispatch continues. Missing observer preserves ordinary editing/selection and reports zero callbacks. Disabled controls can submit but emit no selection notifications; inert/layout-only ancestry emits none. Scratch/argument errors remain explicit and do not report observer calls; a valid preflight is not a promise against later editing/render errors.

The callback reports an applied selection transition, not an owner-rejectable request. `cui_selection_change` copies actual `previous/value`, attempted `item`, common metadata and ACTIVATION reason. Here ACTIVATION describes semantic option confirmation, including Enter. Metadata kind is **CHANGE**, not ACTIVATE, and the reason does not imply a Core activation snapshot. Query-only edits, cursor navigation, external owner assignments, unmatched Enter, and confirming the existing value emit no selection CHANGE. Query edits have their own `.legacy.query_changed`; no query/SUBMIT event is inferred from selection.

Base first reduces query editing/filtering and Enter confirmation; Styled then reduces popup-list option input if still open. Observers run synchronously at the existing selection-write points, without sorting, replay or deferred delivery. Event indices can decrease across those phases: a later-index Enter transition can precede an earlier-index captured pointer release. Use indices as truthful input identity, not a promise of globally sorted callback delivery. Preserve existing text-before-Enter versus Enter-before-text behavior and the complete list pass before popup close. Legacy `.changed` may collapse/overwrite transitions in its existing paths; it is not a callback count or proof of final owner acceptance.

An observer may update the external selection; later events and final painting/results see that owner state according to the existing pointer-owned contract. Such writes do not recursively synthesize another notification. Each payload is frozen before the observer runs. Consuming later input changes later eligibility; no prequeued callback batch overrides consumption. Do not repeat the same side effect from both the callback and aggregate result.

`event.target` is the public input ID. Enter's direct key path copies real KEYBOARD/frame/index metadata with **no HAS_ACTIVATION**. The option-list seam copies a genuine accepted snapshot only if Core already makes it available, retaining the actual child `activation.target`; it normally registers option semantics after reduction, so snapshots are absent there too. The fallback records only source proven by the actual event type plus its frame/index. No pointer coordinates, touch snapshot or accessibility activation evidence is fabricated from source alone. See [metadata presence/lifetime conventions](COMPONENT_EVENTS.md).

Current Core excludes OPTION from accessibility ACTIVATE support. FOCUS is supported, but focus alone does not synchronize the retained list cursor. This slice preserves those limits and tests explicit focus/navigation rather than claiming successful accessibility option activation. Enabling that capability needs a separate Core review.

Handler/user/payload are borrowed through synchronous submission. Pointer-free payload copies survive later frames and context destruction. Keep active item arrays, editor/scratch/popup/selection, callback users and render resources alive through their existing boundaries. Copied handler/options changes affect later calls. Schedule removal/teardown for after submission/frame completion: active context destruction, freeing active state and recursive same-frame submission remain unsupported. No runtime destruction guard, queue, scheduler or request binding is introduced.

`tests/unit/test_combobox_events.c` exercises real Base/Styled query and Enter paths, both text/Enter orders, filtered-out committed IDs, same-value confirmation, missing handler, scratch errors, disabled/inert/omitted calls, real pointer/touch input, owner writes/later consumption, existing accessibility boundaries, phase-order evidence and copied lifetime. The mixed-phase case uses a genuine cross-frame option pointer capture, restores input focus and the caller-owned active cursor, then queues release/Enter/text; it checks callback indices1 then0 without fabricated snapshots. Legacy choice and Select suites remain compatibility checks. Run native tests only through the invisible wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R '^(combobox_events|select_events|choice)$'
```

[Private plumbing checkpoint](design/COMBOBOX_EVENTS_SLICE.md) records the implementation boundary. Controlled selection rejection, query/edit events, confirmation/lifecycle events, multi-selection, foreign request hooks and parity promotion remain deferred. Module/generator/SDK/binding and human/LLM index integration are integrator-owned.
