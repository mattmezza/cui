# ContextMenu command notifications

`cui_base_context_menu_events` and `cui_context_menu_events` add synchronous per-command observation to the existing Base and Styled ContextMenu. They use the unchanged `cui_menu_command_handler`, payload and result from [MenuTree events](MENU_EVENTS.md). Legacy ContextMenu entry points retain their behavior. Simple Menu and DropdownButton forwarding are separate, pending slices; this does not establish uniform component or binding coverage.

```c
static void selected(void *user, const cui_menu_command *event) {
    cui_id *last_command = user;
    *last_command = event->command;
}
/* During the existing frame, after submitting the receiving target: */
cui_menu_command_result result = cui_context_menu_events(
    ui, receiving_target, &theme, cui_string("Actions"), entries, entry_count,
    &action, &menu, options, (cui_menu_command_handler){selected, &last_command});
```

The Base form takes the legacy Base arguments followed by the handler: `ui, target, entries, count, action, state, disabled, dt, keep_open_checks, handler`. It reduces without drawing; Styled retains the existing popup layout and painting. Call either facade once for a given submission, rather than calling the legacy reducer as well.

Right-click opening, reanchoring and programmatic `cui_context_menu_open` emit no command notification. The original receiving bounds, popup-scope checks, anchor, focus restoration and opening order remain intact. `event.target` is the public receiving target, even if `state.overlay.trigger` restores focus to another control. `command` is the activated leaf ID, including nested leaves.

Each actual MenuTree command forwards exactly once at its existing commit seam, after action output, check/radio writes and close policy. Repeated keep-open commands remain separate notifications, including a return to the initial checked value. Closing-command exit policy is frozen before observation. `legacy_activated` preserves the aggregate bool; `command_count` counts actual handler invocations, so a missing handler yields zero while normal mutations still occur. `valid`, `submitted` and overflow semantics are those of MenuTree, including inert/layout-only suppression.

Metadata kind is ACTIVATE. Genuine pointer/touch, keyboard and supported menu accessibility events retain their source/frame/index evidence. MenuTree's reducer precedes current semantic registration, so the normal path has no copied activation snapshot; no opening event, focused target or programmatic action is converted into invented activation evidence. Copied pointer-free payloads survive the callback, frame and context lifetime.

Handlers may update caller-owned values or consume later input under the existing reducer rules. Payload/handler arguments are borrowed synchronously; active descriptors, child arrays, state, values and user storage must remain alive through submission. Direct mutation of in-flight menu navigation, freeing active storage, recursive submission and context destruction inside dispatch remain unsupported. Schedule removal or shutdown after submission/frame completion. Notifications cannot veto applied mutations and do not introduce open/close callbacks, queues or callback retiming.

[test_context_menu_events.c](../tests/unit/test_context_menu_events.c) exercises Base and Styled with distinct receiver/restore-focus identities, real right-click opening/reanchoring, repeated accessibility check commands, nested keyboard activation, mouse/touch, missing observers, owner updates, later-input consumption, disabled/inert/omitted calls, modal rejection and copied lifetimes. Existing `test_choice` remains the legacy behavior regression suite. Native tests run only through the invisible-display wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R '^(context_menu_events|choice)$'
```

The integrator registers the new fixture in CMake and distribution/documentation exports. No binding facade or parity promotion is included.
