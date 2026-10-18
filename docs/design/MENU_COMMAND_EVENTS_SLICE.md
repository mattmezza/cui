# MenuTree command observer checkpoint

Status: design only, audited at `4ecd73dd`. No signatures below ship. This narrows the [event contract](COMPONENT_EVENTS_CONTRACT.md) to Base/Styled MenuTree command observation. Simple Menu, ContextMenu, Dropdown and other forwarding remain later slices. No Core/surface change, ledger promotion or complete Menu family claim is proposed.

## Existing contract and evidence

- [Public descriptors/state/options](../../include/cui/cui_choice.h): caller owns action output, tree state, descriptor tree, check bools and radio integers. IDs must be globally unique throughout the tree. Render slots borrow users/resources; no command callback exists on MenuTree. `cui_base_menu_tree` returns an aggregate bool, and `cui_menu_tree` forwards that bool after drawing.
- [Base reducer](../../src/base/cui_choice.c), `cui_base_menu_tree`: ordered input loop resolves the active level/item, navigation/typeahead, submenu opening, depth limits, scroll/drag invalidation, and command activation. At its command branch it updates active/depth, toggles CHECK or assigns RADIO, writes `*action`, applies close policy, sets `activated_any`, then returns immediately if closed or continues if still open. ACTION normally closes; CHECK/RADIO stay open only with `keep_open_checks`. Repeating a check can toggle twice; assigning the already-selected radio option is still a command activation. No equality check suppresses commands.
- `cui_menu_entry_is_disabled` excludes explicit disabled entries, labels/separators, CHECK without storage, RADIO without storage, and SUBMENU without children. Opening a submenu is navigation, not committing that submenu as a command. Raw local activation and keyboard redirection establish the reducer decision; they are not proof of an already accepted current Core activation snapshot.
- [Styled renderer](../../src/components/cui_choice.c), `cui_menu_tree`: validates existing geometry/theme arguments, sets drag policy, calls Base once, draws trigger, retains metrics, and paints open levels. Entry semantics are registered by later `cui_base_menu_entry`/paint work. Do not move registration to obtain earlier evidence.
- [Choice tests](../../tests/unit/test_choice.c), `test_menu_tree`: two Enter events toggle the same CHECK twice in one call while only bool=true and the final action ID survive. Tests also establish radio mutation, nested navigation/escape, depth/focus restoration, hover opening, parent check activation, role/checked snapshots, typeahead and disabled close. `test_menu_depth_bound`, `test_menu_configuration`, `test_hierarchical_menu_scroll`, `test_menu_pointer_drag_scroll`, `test_menu_check_glyphs` and `test_menu_popup_paint_composition` establish other retained behavior. These are legacy proofs, not typed event/lifetime coverage.

Core's existing accessibility ACTIVATE allowlist includes MENUITEM, MENUITEMCHECKBOX and MENUITEMRADIO (unlike OPTION). A new fixture can therefore prove genuine menu accessibility input without a Core extension, but still must not fabricate an accepted snapshot before current semantic registration.

## Compare two possible seams

| Approach | Preserves existing reducer? | What it can truthfully deliver |
| --- | --- | --- |
| Aggregate-last wrapper around existing bool/output | Yes, if purely observed after return. | At most “this call activated something; final action output is X.” It cannot recover each command, repeated same-ID commands, check/radio intermediate mutations, or each input index. Calling that one event a per-command ACTIVATE would misrepresent multiplicity. |
| Private per-command hook at the existing commit branch (recommended) | Yes, with no replay, early registration, new veto path or close-policy rewrite. | One synchronous ACTIVATE notification for every command branch actually reached, including repeated same ID, plus genuine input facts captured before mutation/close. Closing commands still terminate the existing loop; keep-open commands preserve the remaining event sequence. |

The aggregate-last exception is unnecessary here: the single commit branch is narrow and already establishes the command ID. Do not reconstruct events from final `*action`, state getters, a post-frame queue, or check/radio equality. Aggregate bool/output remain unchanged compatibility results beside new observer bookkeeping.

## Proposed additive API

New Base-dependent `cui_menu_events.h` contains common metadata and pointer-free command payload/handler/result. It does not depend on Components or expose descriptor pointers. Existing `cui_choice.h` declares Base/Styled forwarding functions. Proposed exact names/layout:

```c
typedef struct {
    cui_component_event_metadata event; /* ACTIVATE, target=public trigger */
    cui_id command;                     /* actual committed leaf entry ID */
} cui_menu_command;
typedef void (*cui_menu_command_fn)(void *user, const cui_menu_command *command);
typedef struct {
    cui_menu_command_fn on_activate;
    void *user;
} cui_menu_command_handler;
typedef struct {
    bool valid, submitted, count_overflow;
    size_t command_count;               /* actual observer invocations */
    bool legacy_activated;
} cui_menu_command_result;

cui_menu_command_result cui_base_menu_tree_events(
    cui_context *, cui_id trigger, cui_str name, const cui_menu_entry *, size_t count,
    cui_id *action, cui_menu_tree_state *, bool disabled, float dt,
    bool keep_open_checks, cui_menu_command_handler);
cui_menu_command_result cui_menu_tree_events(
    cui_context *, cui_id trigger, const cui_theme *, cui_str name,
    const cui_menu_entry *, size_t count, cui_id *action, cui_menu_tree_state *,
    cui_menu_tree_options, cui_menu_command_handler);
```

`valid` means applicable argument/geometry preflight passed, not success of all later rendering. `submitted` follows the shipped ordinary active-submission convention; disabled may submit but cannot command, inert/layout-only cannot notify. `command_count` counts actual observer invocations, saturating at `SIZE_MAX` with `count_overflow`; missing observer preserves all ordinary commands/mutations/close behavior and reports zero. `legacy_activated` is exactly the original aggregate bool and can be true with zero observer calls. No generic request count or selection-change result is reused with a different meaning.

The minimal payload deliberately omits label/shortcut/icon text, entry pointers, indices, menu kind and check/radio value fields. Applications already own their model and can dispatch by stable command ID; observers can read applied owner state synchronously. Any future copied typed check/radio snapshot needs a separate additive payload/version decision, not an ACTIVATE-to-CHANGE reinterpretation. Check/radio writes are applied before observation and are not vetoable. Command callbacks have no bool return, cancellation/default prevention or owner acceptance result.

## Exact timing, mutation and evidence

Refactor Base into an internal implementation with optional handler/result bookkeeping. Legacy Base passes no observer. New Base and Styled wrappers borrow copied handler/options for one invocation; Styled calls the implementation once and keeps its existing draw/retain/paint order. No public option/state/result struct layout changes are needed.

At each existing command branch:

1. Freeze command ID and genuine event metadata before check/radio writes or close/focus changes. Do not retain `entry` in the payload.
2. Run existing active/depth updates, check/radio mutation, `*action` write, close policy and `activated_any=true` in their current order.
3. Freeze the existing `exit_after_command = !state->overlay.open` decision before calling the observer. Increment callback count and synchronously dispatch ACTIVATE if an observer exists and submission is active.
4. Use that frozen decision for the original immediate return versus continue. A callback cannot extend a closing command's reducer pass by reopening state. Keep-open input still proceeds in original event order, subject to callback consumption of later input.

This position means callbacks see already-applied check/radio/action state and the already-applied close policy/focus effects. Repeated same command always notifies when the reducer reaches it, even if the radio value was unchanged or action output already matched. Navigation, submenu opening, Escape/outside close, scrolling, disabled/malformed entries and external state assignments are not command ACTIVATE notifications. No CHANGE, COMMIT, SUBMIT or lifecycle event is inferred from these branches.

`event.target` is the public trigger ID and `command` is the leaf ID. If a genuine accepted `cui_activation_snapshot` is already available before mutation/close, copy it unchanged and retain its original child target; never rewrite that snapshot to impersonate a redirected command. Normal Styled MenuTree registers entry semantics later, so HAS_ACTIVATION is ordinarily absent. Fallback metadata contains only actual frame/index and source established by explicit pointer-up, key-down/up or accessibility-action event type. It cannot invent coordinates, touch detail, accepted accessibility evidence or programmatic origin from an absent snapshot. Keyboard redirection can commit a different active leaf than the original routed target: preserve both facts without manufacturing a Core activation for the leaf.

Callbacks can update unrelated application state and owned command values, or consume later input. Such writes are not additional synthesized commands and do not undo an already-applied operation. Later eligible commands use existing live owner values. Directly changing popup/tree navigation state in a callback is outside this first slice's supported mutation contract: the frozen return policy only preserves the reducer boundary; it is not permission to mutate in-flight navigation. Descriptor arrays/children must remain stable throughout submission, and active storage/users/resources cannot be freed. Request removal/shutdown in application state and perform it after submission/frame completion. Context destruction and recursive same-frame submission remain unsupported; no fake runtime guard is introduced.

Payload/user are borrowed synchronously; copying the pointer-free payload preserves it beyond frame/context lifetime. No callback retention, new input routing, queue, scheduler, binding hook or deferred fanout is added. Existing queued foreign notifications keep their timing and current typed allowlist.

## Ownership and meaningful proof before shipping

Suggested production scope, pending approval: new `include/cui/cui_menu_events.h`; narrow declarations in `include/cui/cui_choice.h`; Base/Styled implementation changes in `src/base/cui_choice.c` and `src/components/cui_choice.c`; new `tests/unit/test_menu_events.c`; human/LLM guide `docs/MENU_EVENTS.md`; this checkpoint. Root owns CMake/module/generator/SDK/bindings and API-guide indexing. No Core, surface, descriptor-layout or manifest edit.

Required tests with real reducers/input:

- Base and Styled ACTION, CHECK and RADIO; repeated CHECK twice in one invocation (two callbacks and original final bool/action), same-ID/same-radio reactivation, nested leaf command ID and trigger identity. Observer sees applied model and close state.
- Keep-open commands continue; ordinary closing command returns before later eligible commands. Callback count zero with missing observer while mutation/close/bool remain normal. Frozen original return decision has focused internal proof without endorsing arbitrary navigation mutation.
- Pointer, touch, Enter/Space and genuine supported accessibility menu actions; snapshot absence at the legacy registration seam, truthful source/index, any genuinely available snapshot retained unchanged. Keyboard redirection must not fabricate leaf-target evidence.
- Observer updates check/radio owner state before later commands, consumes later input, and copies payload past frame/context lifetime; handler/user separation and no recursively generated events. Descriptor/user teardown remains explicitly unsupported rather than tested as safe.
- Disabled group/entry, labels/separators, missing check/radio storage, empty submenu, inert/layout-only/omitted submission, invalid arguments; no observer callbacks for navigation/typeahead/hover/scroll/dismissal. Scroll/drag invalidation and depth/focus restoration retain existing legacy suites.
- Strict portable C11 and C++17 header checks, targeted legacy `choice` regression and new CTest through `python scripts/test_local.py --build-dir build` only. Native/headless evidence must not claim desktop/GPU or full-family parity.

Defer simple Menu, ContextMenu, Dropdown, SplitButton, tab-overflow and menu-popup-paint forwarding until each trigger/context identity and callback boundary is audited. Also defer controlled/vetoable commands, check/radio CHANGE payloads, open/close observers, async listeners, foreign synchronous hooks and full menu-family verification.
