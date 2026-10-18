# MenuTree command notifications

`cui_base_menu_tree_events` and `cui_menu_tree_events` add synchronous per-command ACTIVATE observers. They preserve Base/Styled MenuTree mutation, navigation and close behavior. Simple Menu, ContextMenu, Dropdown, SplitButton and other forwarding are outside this slice; no uniform menu-family or binding coverage is implied.

```c
#include <cui/cui_choice.h>

static void command_activated(void *user, const cui_menu_command *command) {
    cui_id *last_seen = user;
    *last_seen = command->command;
}

/* During composition; action/tree/descriptors and pointed-to values belong to app. */
static cui_menu_command_result file_menu(cui_context *ui, const cui_theme *theme,
    const cui_menu_entry *entries, size_t count, cui_id *action,
    cui_menu_tree_state *tree, cui_id *last_seen) {
    return cui_menu_tree_events(ui, 2, theme, cui_string("File"), entries, count,
        action, tree,
        (cui_menu_tree_options){.popup = {.viewport = {0, 0, 640, 480}},
                                .keep_open_checks = true},
        (cui_menu_command_handler){command_activated, last_seen});
}
```

Initialize tree once with `cui_menu_tree_init` and a stable nonzero popup ID. The new entry points match the legacy argument order and append `cui_menu_command_handler`. Base draws nothing; caller submits its normal menu-entry semantics/geometry after reduction. Styled retains its existing trigger, metrics and popup painting order. Descriptor/child IDs remain globally unique throughout the tree; pointed-to check/radio storage stays caller-owned.

`cui_menu_command_result` contains `valid` (applicable preflight passed), `submitted` (ordinary active submission), `command_count` (actual observer invocations), `count_overflow`, and `legacy_activated` (the original aggregate bool). Counts saturate at `SIZE_MAX` while dispatch continues. Missing observer preserves all commands, mutations and close behavior with count zero; `legacy_activated` may still be true. Disabled controls can submit but cannot command; inert/layout-only ancestry cannot notify. Later Core/rendering errors remain possible after preflight.

Every command branch reached emits once, including repeated same ID, CHECK toggled twice in one call, and RADIO confirmed while its value already matches. This is ACTIVATE, not a value CHANGE or owner-rejectable request. CHECK/RADIO writes and action output are applied before the observer; no callback return can veto them. Navigation/typeahead, submenu opening, scrolling, dismissal, labels/separators, disabled or malformed entries and external state writes do not emit command activation. Missing check/radio storage or empty submenu follows existing disabled policy.

The callback receives pointer-free `cui_menu_command { event, command }`. `command` is the actual committed leaf ID; `event.kind` is ACTIVATE and `event.target` is the public trigger. Payloads omit descriptor pointers, labels, shortcut/icon text and generic check/radio snapshots. Applications own their model and can inspect applied state synchronously. Copying the payload preserves it beyond frame/context lifetime; referenced users/state/resources remain subject to their existing lifetime boundaries.

The order at each command is existing active/depth updates, check/radio write, action output write, existing close policy, then observer. Thus callbacks see applied values and already-applied close/focus state. The reducer freezes whether that command must return before invoking the observer, preserving the closing command boundary. Keep-open commands continue through remaining eligible events in original reducer order. Observers may update command-owner values or unrelated application state and consume later input; those writes are not recursively synthesized commands. Do not perform duplicate side effects through both callback and aggregate result.

Metadata is frozen before writes/close. A genuine accepted Core activation snapshot is copied only if already available, retaining its original child target. Normal MenuTree entry semantics register after reduction, so HAS_ACTIVATION is ordinarily absent. Fallback metadata contains only actual frame/index and source proven by pointer-up, keyboard or accessibility event type. No coordinates, touch details, accepted accessibility snapshot or programmatic source is invented. Keyboard redirection can commit a different active leaf from the original routed target; command ID and any genuine snapshot each retain their actual meaning. See [shared metadata](COMPONENT_EVENTS.md).

Core already supports accessibility ACTIVATE for MENUITEM, MENUITEMCHECKBOX and MENUITEMRADIO. The tests use genuine supported menu accessibility actions; this does not remove Select/Combobox OPTION activation limitations. Touch input establishes pointer source at this seam without implying a copied touch activation snapshot.

Handler/user/payload are borrowed synchronously. Copy handler/options at call entry; descriptor arrays/children and active storage must remain stable/alive throughout submission, and rendering resources through their existing completion boundary. Directly mutating in-flight popup/navigation state, freeing active state/users/resources, destroying the active context or recursively submitting the same frame is unsupported. Request removal/shutdown in application state and perform it after submission/frame completion. The frozen return policy is not a navigation-mutation API or a runtime destruction guard. No registration, queue, routing, scheduler or foreign request hook is added.

`tests/unit/test_menu_events.c` exercises real Base/Styled commands, repeated check/radio/action IDs, post-write/post-close state, missing handlers, pointer/touch/key and supported accessibility input, nested keyboard redirection, owner updates/later consumption, closing-command termination, disabled/inert/omitted calls, legacy coexistence and copied lifetime. Existing `test_choice.c` retains navigation/depth/scroll/drag/render compatibility proof. Run registered native tests only through the invisible wrapper:

```sh
python scripts/test_local.py --build-dir build -- -R '^(menu_events|choice)$'
```

[Design checkpoint](design/MENU_COMMAND_EVENTS_SLICE.md) compares the per-command seam with the lossy aggregate-last alternative. Simple Menu/ContextMenu/Dropdown forwarding, controlled/vetoable commands, check/radio CHANGE payloads, lifecycle observers, binding exports and parity promotion remain deferred. Root owns module/CMake/generator/SDK and human/LLM API indexing integration.

Known pending renderer issue (separate from command dispatch): compact valid entry IDs can collide in existing derived private paint IDs. RADIO entry12's left-icon wrapper is `12 ^ 0x9e3f5d2a91bca001`; SUBMENU entry15's chevron is `15 ^ 0x9e3f5d2a91bca002`; both equal `0x9e3f5d2a91bca00d`. Once the radio is checked, Styled rendering reports duplicate identity. The unit source preserves this exact opt-in reproducer under `CUI_MENU_COMPACT_ID_REPRO`; compile that fixture with the definition to reproduce, while the ordinary event suite uses spaced IDs. This is a pending renderer correctness gap, not a reason to claim full Menu renderer parity from event tests.
