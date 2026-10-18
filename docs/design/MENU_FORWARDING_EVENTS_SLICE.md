# Menu forwarding event checkpoints

Status: ContextMenu slice implemented in this branch; Simple Menu and DropdownButton remain proposals. Original source audit: `b4146c89`. See [ContextMenu events](../CONTEXT_MENU_EVENTS.md) for the implemented API and proof. Preserve [MenuTree's implemented per-command contract](../MENU_EVENTS.md), [event ownership/timing rules](COMPONENT_EVENTS_CONTRACT.md), all legacy entry points/layouts, and the 78-family manifest. No Core/surface change, new queue, capture/bubble routing or binding request hook is proposed. No builds were run for this read-only checkpoint.

## Actual entry points and seams

| Public path | Current reduction and IDs | Existing result/callback limitation |
| --- | --- | --- |
| Simple `cui_menu` | Styled `cui_choice_popup_render(..., menu=true)` uses flat `cui_choice_item` IDs, caller-owned action output and `cui_choice_popup`. Full `cui_base_list` pass precedes popup close and focus-scope dismissal. Trigger is its public `trigger` argument. | Bool means any activation, including same action again; only final action output survives. Public options expose render slots, not command handlers. The private Select list observer emits CHANGE only when previous differs, so it is unsuitable for repeated commands. |
| Base/Styled ContextMenu | Base scans unconsumed secondary pointer-down events inside existing target bounds and permitted popup scope, calling `cui_context_menu_open`. It then delegates to MenuTree with `trigger=target`; Styled validates geometry, sets drag policy, reduces once and paints at `state->anchor`. | Bool is MenuTree aggregate; per-command forwarding is currently missing. Context opening/reanchoring is not a command. `overlay.trigger` is independently used for restoring focus and may differ from the target; do not substitute it for metadata identity. |
| Base/Styled `cui_dropdown_button` | Separate public `action_id` and `trigger_id`; Styled also has a paint/layout `container`. Base calls `cui_base_button` for action first, updates loading semantics, then reduces MenuTree for menu commands. Hidden/absent trigger closes existing menu and clears depth. Styled draws halves, retains menu metrics, composes action tooltip, maps anchor corners, then paints menu. | `{action_activated,menu_activated}` are aggregate booleans, not per-event counts. Primary action has no typed observer; menu command observer is not forwarded. Content/render/tooltip callbacks are not command listeners. |

Source: [choice header](../../include/cui/cui_choice.h), [Base choice](../../src/base/cui_choice.c), [Styled choice](../../src/components/cui_choice.c), [`cui_base_button`](../../src/base/cui_base.c), [typed activation handler](../../include/cui/cui_control_events.h). There is no separate public `cui_split_button` symbol: split-button presentation is the paired DropdownButton; `hide_action` gives a menu-only presentation and `hide_menu` gives action-only. The canonical manifest family is DropdownButton; these presentations do not add manifest families.

Actual legacy proof in [test_choice.c](../../tests/unit/test_choice.c): `test_lists_and_popups` verifies Simple Menu same-ID reactivation, popup close/focus and paging; `test_context_menu` verifies right-click anchor/reanchor, nested command dispatch, focus restore and outside target; `test_context_modal_guard` verifies popup scope restrictions. `test_dropdown_button`, `test_dropdown_optional_halves`, `test_dropdown_anchor_corners`, `test_dropdown_action_tooltip`, `test_dropdown_loading_keeps_content`, `test_dropdown_loading_subtree_opacity`, `test_dropdown_real_artwork` and presentation/inheritance tests cover paired-half behavior and rendering. These legacy tests do not establish new typed multiplicity, callback mutation or evidence lifetimes.

## Slice 1: ContextMenu command forwarding

Implemented as the first slice: reuse `cui_menu_command_handler/result` unchanged. Exact additive signatures append the handler to existing arguments:

```c
cui_menu_command_result cui_base_context_menu_events(
    cui_context *, cui_id target, const cui_menu_entry *, size_t count,
    cui_id *action, cui_menu_tree_state *, bool disabled, float dt,
    bool keep_open_checks, cui_menu_command_handler);
cui_menu_command_result cui_context_menu_events(
    cui_context *, cui_id target, const cui_theme *, cui_str name,
    const cui_menu_entry *, size_t count, cui_id *action, cui_menu_tree_state *,
    cui_menu_tree_options, cui_menu_command_handler);
```

Refactor only ContextMenu's opening scan into a shared private implementation with optional forwarding. Legacy calls the old MenuTree path; new calls MenuTree events exactly once. Do not call `cui_base_context_menu` and then try to re-observe its aggregate result, or scan/reduce twice. Preserve original opening/reanchoring order, scope checks, anchor bounds, retained state, check/radio writes, keep-open behavior and close timing.

`event.target` is the public ContextMenu receiving `target`, even when restore focus is another control or programmatic `cui_context_menu_open` was used. Command is the actual leaf ID; copied activation retains its real routed child target. Right-click opening alone emits no ACTIVATE command, and opening input is not relabeled as the later command's source. Programmatic open adds no fabricated input evidence. Reuse MenuTree source/snapshot limitations and callback-count semantics: missing handler mutates normally with count zero; repeated keep-open command IDs notify individually.

Proof: real right-click open/reanchor without command; nested action/check/radio per-command multiplicity; programmatic open with distinct restore-focus ID; pointer/touch/key/supported accessibility menu actions; scoped modal rejection; missing handler; disabled/inert/omitted invocation; callback owner writes/later consumption; close/focus/anchor invariants and copied payload lifetime. Preserve compact-ID renderer and legacy ContextMenu/MenuTree regression tests. No open/close observer is added.

## Slice 2: Simple Menu flat command observer

Proposed exact signature reuses existing command payload/result:

```c
cui_menu_command_result cui_menu_events(
    cui_context *, cui_id trigger, const cui_theme *, cui_str name,
    const cui_choice_item *, size_t count, cui_id *action, cui_choice_popup *,
    cui_choice_popup_options, cui_menu_command_handler);
```

Add private per-activation command forwarding to the flat list reducer, without changing public list options or the current Select observer seam. A small private list implementation can take optional selection and command forwarding; legacy/Select calls retain their existing mode, and only new Simple Menu enables command notifications. Do not interpret `previous!=value` as command eligibility, and do not accidentally emit both CHANGE and ACTIVATE for the same Menu call.

At every existing flat-list activation, freeze command ID/input facts before callbacks, preserve assignment and existing activation→confirmation callback order, then emit ACTIVATE for that command even if action output already matched. Preserve the complete list pass and close/dismiss after it: A→A→B yields three notifications and final B, while legacy bool remains true. Simple Menu cannot gain MenuTree's early-return-after-closing-command policy: its existing close occurs after the list pass. Render slots/painting remain in their current phase.

`event.target` is the public trigger; command is the activated flat item ID. The existing flat `menu=true` path registers MENUITEM roles (unlike Select's OPTION roles), so genuine accessibility ACTIVATE should be tested against the actual previous-frame menu semantics. Accepted snapshots are ordinarily unavailable before current registration; retain genuine facts only and never register early. Existing Menu flat descriptors do not expose check/radio/submenu behavior; do not imply that this API adds those features.

Proof: repeated same-ID commands and multi-command list pass, final output/legacy bool, observer owner writes and consumed later input, trigger/leaf identity, pointer/touch/key/accessibility menu roles, same-item closing, dismissal/scroll-invalidated pointer activation, disabled/inert/omitted/missing handler, renderer slots and copied lifetime. Keep Select/Combobox/List callbacks and all legacy aggregate semantics unchanged.

## Slice 3: DropdownButton paired action and menu events

Implement this as one separately reviewed public paired facade after the two forwarding slices, rather than silently declaring a partial uniform event API. Reuse the shipped typed activation handler for primary action and MenuTree command handler for menu leaves:

```c
typedef struct {
    cui_component_activation_handler action;
    cui_menu_command_handler menu;
} cui_dropdown_event_handlers;
typedef struct {
    bool valid, submitted, count_overflow;
    size_t action_count, command_count; /* actual callback invocations */
    cui_dropdown_button_result legacy;
} cui_dropdown_event_result;

cui_dropdown_event_result cui_base_dropdown_button_events(
    cui_context *, cui_id action_id, cui_id trigger_id, cui_str action_name,
    cui_str menu_name, const cui_menu_entry *, size_t count, cui_id *menu_action,
    cui_menu_tree_state *, bool disabled, bool action_disabled, bool action_loading,
    float dt, bool keep_open_checks, cui_dropdown_event_handlers);
cui_dropdown_event_result cui_dropdown_button_events(
    cui_context *, cui_id container, cui_id action_id, cui_id trigger_id,
    const cui_theme *, cui_str action_name, cui_str menu_name,
    const cui_menu_entry *, size_t count, cui_id *menu_action, cui_menu_tree_state *,
    cui_dropdown_button_options, cui_dropdown_event_handlers);
```

The result deliberately counts callback invocations, with zero for missing handler. Do not reuse Button's `cui_component_action_result.activation_count`, whose accepted-attempt semantics differ for missing handlers; no unused request count is introduced. Legacy aggregate booleans remain unchanged. Both counts saturate; overflow never suppresses dispatch. `cui_control_events.h` provides the Base-dependent activation handler; adding its include/type use needs public-header C/C++ proof, not a Components dependency in Base.

Preserve existing **action phase → menu phase → Styled painting/tooltip**. Call existing `cui_base_button` once, retain its aggregate result and loading/busy semantics, then enumerate genuinely accepted action events/snapshots before entering the menu reducer. This is possible because Base Button submits its current semantic and does not itself run external action callbacks. Reuse current acceptance/snapshot APIs; do not call Styled `cui_button_events`, draw another button or synthesize one callback from the aggregate bool. Action metadata kind ACTIVATE targets `action_id`; menu command metadata targets `trigger_id`. `container` is layout identity, never an invented input target; opening the menu trigger is not itself a menu leaf command.

Action and menu phases may report nonmonotonic event indices when mixed input was queued; preserve those phases and truthful indices rather than sort/replay/defer. Action callbacks can consume later input before menu reduction; menu observer uses the existing per-command seam exactly once. Freeze callback payloads before mutable callbacks. Action disabled/loading suppress only action; outer/menu disabled policy suppresses both as currently composed. Hidden action gets no semantic/callback; hidden menu closes existing popup and dispatches no menu commands. Preserve Link role updates, menu-only/action-only geometry, anchor mappings, content slots, tooltip trigger/disabled policy and all retained resource lifetimes. Any role-sensitive evidence mismatch from Styled Link's existing post-reduction role update must be reported rather than normalized into fabricated evidence.

Proof: multiple primary activations with exact evidence, independently disabled/loading and missing handlers, action-first ordering versus earlier-index menu input, later-input consumption/owner mutation, menu check/radio repeated commands, distinct action/trigger/container IDs, hidden halves and existing popup-close behavior, Link accessibility roles, tooltip/content order and rendering. Use genuine supported accessibility actions, pointer/touch/keyboard, inert/layout-only/omitted calls and copied payload lifetime. No extra SplitButton family/symbol alias is necessary unless a separately reviewed discoverability decision requests it.

## Shared boundaries and implementation ownership

All new payloads are synchronous borrowed views; pointer-free copies survive frame/context destruction. Copy handler/options at entry; active descriptors/children, values, users/state and render resources must remain alive/stable through existing boundaries. Callback owner writes are not recursive events, and notification is not rejection/rollback. Direct in-flight popup/navigation mutation, context destruction, freeing active storage and recursive same-frame submission remain unsupported. Schedule removal/shutdown for after submission/frame completion. Preserve MenuTree's frozen closing-command return decision; do not transfer it to flat Simple Menu's different close policy.

Each slice should be its own production commit plus meaningful unit/guide proof, with source ownership limited to `cui_choice.h`, Base/Styled choice implementation, a new slice fixture and human/LLM guide. Slice3 may need a narrow include/use of existing control-event types; no shared type layout changes. Root owns CMake/modules/generated distributions/SDK/bindings/indexing. No build or production edit belongs to this design checkpoint. Future implementation must use strict portable C11/C++ header checks and native CTest through `python scripts/test_local.py --build-dir build`; browser tests stay headless.

Deferred: menu open/close observers, vetoable/synchronous foreign requests, check/radio CHANGE payloads, global event-index ordering, auto callback fanout, tab-overflow/command/surface forwarding, async listeners and parity promotion. Aggregate-only fallback must be explicitly labeled if a later path lacks a true per-event seam; it must never masquerade as complete per-command observation.
