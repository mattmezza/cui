# Select selection notifications

`cui_select_events` adds a synchronous post-mutation CHANGE observer to single-selection Styled Select. Existing `cui_select`, List, Menu, Combobox and multi-select APIs retain their behavior. Shared payloads come from [selection events](SELECTION_EVENTS.md); this is not uniform coverage of selection families or a bridge/binding typed-selection allowlist.

```c
#include <cui/cui_choice.h>

static void changed(void *user, const cui_selection_change *change) {
    cui_id *last_observed = user;
    *last_observed = change->value;
}

/* During active composition; owner keeps selected and popup between frames. */
static cui_select_event_result account_select(cui_context *ui, const cui_theme *theme,
    const cui_choice_item *items, size_t count, cui_id *selected,
    cui_choice_popup *popup, cui_id *last_observed) {
    return cui_select_events(ui, 2, theme, cui_string("Account"), cui_string("Choose"),
        items, count, selected, popup,
        (cui_choice_popup_options){.viewport = {0, 0, 640, 480}},
        (cui_selection_handler){changed, last_observed});
}
```

Initialize the popup once with `cui_choice_popup_init` and a stable nonzero popup ID. Existing item ID, popup state, viewport, theme and caller-array lifetime requirements apply. `cui_select_events` appends a `cui_selection_handler` to the existing Select arguments. It returns `cui_select_event_result`: `.legacy_changed` is the original final selection comparison, and `.events` contains preflight `valid`, active `submitted`, `change_count` of actual observer calls, and `count_overflow`. Missing observer preserves ordinary writes and reports zero. Counts saturate at `SIZE_MAX`; callbacks continue. Disabled controls may submit but emit no CHANGE; inert/layout-only ancestry emits none. Later rendering errors remain possible after successful preflight.

Select directly mutates `*selected`; this callback notifies an applied transition and cannot reject it. Every actual option-selection transition emits once, synchronously at its existing list reducer point. The payload copies previous and actual new selection, attempted `item`, ACTIVATION reason and common metadata. No label or borrowed query text is included. Zero/invalid selection remains caller-owned; filtering/reordering does not convert IDs to indices. Same-item activation can close the popup without CHANGE. Focus navigation/typeahead, trigger open/close, outside dismissal and external model assignments do not emit selection CHANGE.

The list processes its complete eligible input batch before Select closes its popup. A→B→A emits two notifications, while `.legacy_changed` remains false. Do not treat that bool as an event count or generate another callback from it. Observer owner writes affect later events and final painting/result in the existing pointer-owned model; they do not recursively synthesize notifications. Consuming later input changes its eligibility. Popup closing still occurs after the list pass, rather than immediately after the first callback.

`event.target` is the Select trigger ID. Genuine accepted activation evidence is copied only if Core already makes it available; `event.activation.target` then stays the actual child ID. The existing list normally registers option semantics after reduction, so HAS_ACTIVATION is absent at this seam. Preserve that registration timing. The fallback copies only actual frame/index and source proven by the triggering pointer-up, keyboard or accessibility event type. It does not invent a pointer position, touch snapshot, accessibility activation snapshot or an accepted activation merely from source. Presence flags, not zero-filled storage, establish evidence. See [shared metadata](COMPONENT_EVENTS.md).

Handler/user/payload are borrowed synchronously. Pointer-free payload copies survive later frames and context destruction. Keep active item arrays, popup/selected storage, users and rendering resources alive for their existing boundaries; changing copied options/handler affects a later call. Schedule removal/teardown for after submission/frame completion. Context destruction, freeing active state/resources and recursive same-frame submission during callbacks remain unsupported; no runtime destruction guard or new queue is added.

`tests/unit/test_select_events.c` uses real Select input to verify pointer/touch/key transitions, accessibility focus without selection and unsupported option activation, multiple changes returning to entry, same-item close, missing handler, owner updates, consumed later input, disabled/inert/omitted submission, legacy coexistence, copied payload lifetime and truthful evidence absence. Existing `test_choice.c` remains the regression suite for list callbacks, popup focus/close, scrolling and custom render slots. Native tests must use the private display wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R '^(select_events|choice)$'
```

[Private plumbing checkpoint](design/SELECT_EVENTS_SLICE.md) explains the reducer seam. No new public List observer, Base Select, controlled selection request, confirmation/lifecycle event or Combobox observer is implemented. Module/generator/SDK/binding and human/LLM index integration are integrator-owned.

Accessibility boundary: current Core permits FOCUS for OPTION but its ACTIVATE role allowlist excludes OPTION. Select therefore supports accessibility focus followed by explicit keyboard navigation/selection, not a newly enabled accessibility option-activation path. Accessibility focus alone also leaves the retained list active cursor unchanged; the tested fallback explicitly navigates before Enter rather than claiming focused-option confirmation. Tests assert these boundaries without changing Core or claiming positive activation coverage.
