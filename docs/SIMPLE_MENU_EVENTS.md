# Simple Menu command notifications

`cui_menu_events` adds synchronous per-command notifications to Styled Simple Menu. It takes the same arguments as `cui_menu`, followed by a `cui_menu_command_handler`, and returns `cui_menu_command_result` from [MenuTree events](MENU_EVENTS.md). Existing List options/layout, Select and Combobox observers, legacy Menu behavior and the 78-family manifest remain unchanged. This is not a DropdownButton facade or a binding API.

```c
static void activated(void *user, const cui_menu_command *event) {
    cui_id *last = user;
    *last = event->command;
}
cui_menu_command_result result = cui_menu_events(
    ui, trigger, &theme, cui_string("Commands"), items, count, &action,
    &popup, options, (cui_menu_command_handler){activated, &last_command});
```

Submit either the legacy or new Menu entry point once per frame. Opening the trigger emits no leaf command. Each accepted flat-list activation writes the caller's action and active cursor, runs existing List activation then confirmation hooks, and finally notifies the command observer. The command ID and input metadata are frozen before those callbacks. This private seam does not expose new public List callback fields.

Command notifications are ACTIVATE, including repeated same-ID activation: A→A→B reports three commands even when the initial action was A. The full flat-list pass completes before Menu closes and restores focus. This differs from MenuTree's closing-command early return; forwarding does not transfer that policy. Owner updates during a callback can affect later reduction; consuming later events suppresses their subsequent handling. A notification cannot veto or roll back applied state. Missing handlers preserve normal mutation and closing with `command_count == 0`. `legacy_activated` retains the aggregate bool, even if a handler writes the action back to its original value. Counts saturate and overflow does not stop dispatch.

`event.target` is the public trigger; `command` is the reducer's actual confirmed item. Real mouse/touch, keyboard and supported MENUITEM accessibility events retain their genuine source, frame and event index. Current registration follows reduction, so ordinary dispatch has no copied activation snapshot. No trigger-opening event, focused target or programmatic state write is invented as activation evidence. Existing flat-list non-pointer confirmation uses its active cursor; accessibility target identity does not independently replace that cursor. This slice preserves that behavior and does not claim complete accessibility parity. Flat descriptors do not gain MenuTree check/radio/submenu features.

The payload is borrowed synchronously and pointer-free copies survive the frame/context. Keep descriptor/value/state/user/render storage alive through submission. Direct mutation of in-flight navigation, freeing active storage, recursive submission and context destruction during callbacks remain unsupported; defer removal/shutdown until submission/frame completion. Existing disabled/inert/layout-only, popup dismissal and scrolling invalidation rules apply. Existing custom row rendering remains in the same painting phase; a closing frame completes reduction without repainting the closed popup.

[test_simple_menu_events.c](../tests/unit/test_simple_menu_events.c) proves repeated A/A/B, complete-pass close timing, pointer/touch/key/accessibility commands, actual legacy-hook ordering and frozen command identity, owner writes/later consumption, missing/disabled/inert/omitted handlers, custom row rendering through old/new paths, scroll rejection and copied lifetimes. Legacy Choice plus Select/Combobox event tests protect neighboring reducers. Native validation uses the invisible wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R '^(simple_menu_events|choice|select_events|combobox_events)$'
```

The integrator registers the test and documentation exports; no generated artifacts or parity promotion are included.
