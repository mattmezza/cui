# DropdownButton paired notifications

`cui_base_dropdown_button_events` and `cui_dropdown_button_events` add synchronous observers for the primary action and individual menu commands. They preserve the paired DropdownButton presentation and legacy aggregate result; no separate SplitButton family, new queue or binding facade is introduced. [MenuTree events](MENU_EVENTS.md), [control events](CONTROL_EVENTS.md) and [ContextMenu events](CONTEXT_MENU_EVENTS.md) describe the reused payload types.

```c
static void action(void *user, const cui_component_event_metadata *event) {
    /* ACTIVATE, target == action_id; copied accepted Core evidence. */
    (void)user; (void)event;
}
static void command(void *user, const cui_menu_command *event) {
    /* ACTIVATE, event.target == trigger_id; command is the leaf ID. */
    *(cui_id *)user = event->command;
}
cui_dropdown_event_result result = cui_dropdown_button_events(
    ui, container, action_id, trigger_id, &theme,
    cui_string("Save"), cui_string("More actions"), entries, count,
    &menu_action, &menu, options,
    (cui_dropdown_event_handlers){{action, NULL}, {command, &last_command}});
```

The Base facade takes the legacy Base arguments followed by `cui_dropdown_event_handlers`. Its zero action/trigger ID omits that half; both zero and equal IDs remain invalid. Styled uses the existing `hide_action`/`hide_menu` options. `container` is paint identity, not an invented activation target.

`cui_dropdown_event_result` contains `valid`, `submitted`, `count_overflow`, `action_count`, `command_count` and `legacy` (`cui_dropdown_button_result`). Both counts mean actual callback invocations: a missing observer counts zero while normal activation, writes and popup closing continue. Counts saturate without stopping dispatch. The two legacy booleans preserve their old aggregate meanings; they do not establish how many events occurred. This differs from Button's facade activation-attempt count.

The existing order is preserved: **Base action reduction → action observation → Base menu reduction/observation → Styled role updates, painting, retained metrics and tooltip composition**. Base Button is called once. The observer pass passively enumerates actual accepted activations after that registration and copies each Core activation snapshot before calling user code. It neither re-runs Button nor reconstructs events from its aggregate bool. MenuTree's real per-command commit seam runs once, preserving check/radio writes, keep-open multiplicity and frozen closing-command exit decisions. Trigger opening emits no menu leaf notification.

Action metadata targets `action_id`, includes genuine accepted evidence, and preserves mouse/touch/key/accessibility source and frame/index. Menu metadata targets `trigger_id`; any copied activation retains its actual child target. MenuTree's ordinary reducer-before-registration path has no copied activation snapshot. No programmatic evidence is synthesized. Action phase precedes menu phase even if the action's queued event index is larger; callbacks are not sorted, replayed or deferred. An action observer may update caller values or consume events before menu reduction.

The normal Styled root menu painter creates a modal focus scope, even if `state.overlay.modal` was changed. Consequently an open Styled popup does not independently admit outside primary-action input. The fixture proves mixed action/menu ordering with a genuine Base composition that paints menu rows in a modeless scope; it does not claim that the normal Styled popup is modeless. No scope behavior changed in this slice.

Action disabled/loading suppresses that half; loading retains existing busy/Loading semantics and visuals. Outer/menu disabled policy still suppresses both. Hidden action has no semantic or observer; hidden menu closes the popup and clears depth. Existing Link presentation updates roles only after Base reduction. A genuine accessibility ACTIVATE queued against the previous LINK role is rejected when the current Base registration is BUTTON; this existing mismatch affects legacy and new results. The new facade reports no invented callback/evidence and does not claim complete Link accessibility parity. Pointer behavior and the final Link semantic roles remain unchanged.

Handlers and payloads are borrowed only during synchronous submission. Pointer-free metadata/command copies survive frames and context destruction. Keep active descriptors, state, values, users and render/tooltip resources alive and stable through submission. Owner writes are notifications' effects, not recursive change events or vetoes. Direct mutation of in-flight navigation, freeing active storage, context destruction and recursive submission during dispatch remain unsupported; defer shutdown/removal until submission/frame completion. Existing inert/layout-only/omitted invocation behavior applies.

[test_dropdown_events.c](../tests/unit/test_dropdown_events.c) checks Base and Styled real mouse/touch/key/accessibility action acceptance, multiple actions, missing observers, busy/disabled/hidden halves, old/new geometry and custom painting, repeated keep-open commands, copied lifetimes, inert/omitted calls and Link rejection. Its Base modeless composition proves decreasing action/menu indices, applied owner writes and consumption before menu reduction; Styled proves normal modal exclusion. Existing Choice tests retain broader tooltip, anchor, variant and artwork coverage; ControlEvents and other menu fixtures protect neighboring facades.

```sh
python scripts/test_local.py --build-dir build -- -R '^(dropdown_events|choice|control_events_facades|menu_events|simple_menu_events|context_menu_events)$'
```

The integrator registers tests and documentation exports. No generated files, callback retiming, manifest changes or parity promotion are included.
