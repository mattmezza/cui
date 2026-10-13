# Targeted accessibility actions (Core prerequisite)

Portable Core now queues `CUI_ACCESSIBILITY_FOCUS` and
`CUI_ACCESSIBILITY_ACTIVATE` by stable `cui_id`, without pointer coordinates or
synthetic keyboard input. This is an action-routing prerequisite; it does not
register a native accessibility tree or provide an OS/screen-reader bridge.

```c
if (cui_accessibility_action_supported(ui, button_id, CUI_ACCESSIBILITY_ACTIVATE)) {
    bool admitted = cui_queue_accessibility_action(ui, button_id, CUI_ACCESSIBILITY_ACTIVATE);
    /* Check admitted and cui_last_error(ui); admission is not execution. */
}
```

Call both APIs between frames on the context's owning thread. The query reads
only the last completed semantic snapshot and returns false during an open frame.
Queueing during a frame reports `CUI_ERROR_LIFECYCLE`. Invalid action enums, zero,
missing, disabled, modal-blocked or unsupported targets report appended
`CUI_ERROR_INVALID_ACTION`. The shared input queue holds 256 events; a valid action
when full reports `CUI_ERROR_CAPACITY`. Failure appends nothing and changes no
focus, activation, pointer or capture state. Errors follow the existing sticky
Core error contract; `cui_clear_error` permits callers to acknowledge rejected
external requests without poisoning later work. No platform/Clay types or
borrowed payload are needed. Existing enum ordinals and zero event initializers
remain intact.

The new event type is `CUI_EVENT_ACCESSIBILITY_ACTION`; its appended
`accessibility_target` and `accessibility_action` fields provide distinct source
provenance. Direct `cui_queue_event` submission of that event uses identical
validation, and unrelated pointer/key/text fields are discarded. No OS callback
may call these APIs concurrently; a future native adapter must copy requests and
drain them on the main thread.

## Supported targets and delivery

Focus uses Core's normal focusability rules, including explicit `focusable`
containers and programmatic-only (`tab_index == -1`) controls. `no_focus` and
disabled IDs are rejected. It makes focus visible, preserving pointer and Space
captures, and respects the published modal scope. Passive label focus proxies
are not silently redirected; submit their actual eligible target ID.

Default Activate roles are Button, Link, Checkbox, Switch, Radio, MenuItem,
MenuItemCheckbox and MenuItemRadio. An explicit `pointer_activatable` semantic
also opts in, including `no_focus` pointer-only controls. Activate does not move
focus, pointer position or capture. Textbox, Slider, Scrollbar, Tab, Option,
TreeItem, table headers/grid cells and passive containers have no default
Activate capability in this first increment. Focus remains available where
eligible. Consumers that opt into a custom activation capability must implement
that action rather than rely on keyboard-only handlers.

Dispatch revalidates the completed target and produces normal
`cui_event_target`/`cui_event_activation` metadata and pending activation slots.
Widget submission checks current disabled state, semantic role and active modal
scope. Current inert/layout-only or removed targets cannot retain a pending
action into another frame; completion withdraws their metadata. A removed or
newly disabled Focus target restores the prior surviving focus where possible,
without overriding a subsequent explicit focus change. Removal of multiple queued
Focus targets unwinds in reverse order. Accepted queue admission therefore does
not guarantee the widget will still be eligible or submitted in that frame.

Stable IDs represent stable logical widgets: use a new ID for a new widget
identity. Core cannot distinguish replacement with an identical ID/role and no
intervening completed removal. Capability details are declared in semantic
submission after `cui_interact`; admission follows their published snapshot.
Current disabled/role/scope checks are immediate, while final semantic withdrawal
occurs at frame completion. Applications should keep published action capability
consistent with their current reducer and must not perform speculative external
side effects during layout-only passes.

## Consumption and existing activation semantics

Simple controls consume pending activation through their normal `cui_interact`
path; actual styled button and checkbox tests verify callback/value changes.
No request is replayed on an idle frame. Multiple same-ID pending activations
coalesce into one `activated` boolean in a frame, exactly as existing pointer/key
activations do. Individual event-aware reducers can inspect each event. Queue
requests across frames if separate callbacks are required from a boolean API.

A deferred/capture-phase owner may handle the event and call `cui_consume_event`.
For the new accessibility event, consumption also cancels its pending activation
and clears activation metadata, so a subsequently submitted widget cannot replay
that callback. Ordinary input consumption behavior is unchanged.
`cui_cancel_event_activation` suppresses activation alone while retaining the
event target, matching its existing capture-phase contract. Deferred handlers
must check consumption and validated activation, not event type alone.

## Compound reducer audit and verification

Read-only source audit found Tabs and List selection reducers keep an event-local
cursor and reset it to the explicit target only for pointer-up. Multiple targeted
accessibility activations can select an earlier cursor; these roles are therefore
Focus-only by default pending assigned base fixes. Menu reducers generally use
activation metadata but have not received native accessibility acceptance here.
Tree/table reducers also inspect activation metadata; full selection, expansion,
scroll and value/text actions need independent family tests. No base/component
implementation was changed, and no full compound-family support is claimed.

Headless Core tests exercise actual styled button/checkbox scenes, separate target
and focus, same-ID coalescing, consumed/deferred/cancelled activations, invalid
enums, queue capacity, programmatic-only stops, Tab and pointer-only eligibility,
removed/reintroduced/changed-role targets, current-frame disabled/modal/inert
rejection, ordered Focus plus keyboard events, focus rollback, and preserved
pointer/Space capture. Full warning build and CTest run: 90/91 pass; only expected
stale generated distribution fails. Four focused sanitizer checks (Core, base,
components, choice) pass. Native transport/screen-reader validation
belongs to the optional bridge implementation, not this prerequisite.
