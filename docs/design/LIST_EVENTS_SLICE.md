# List and ToggleGroup typed transition slice

Design only, audited at `dccb02f2`. Choice sources are currently owned by the
Dropdown worker; implementation requires their completed changes and root review.
Keep Tabs, popup policy, input dispatch and bridge schemas outside this slice.

## Evidence and current behavior

- [Public choice types](../../include/cui/cui_choice.h#L120): ToggleGroup borrows a
  bool array, List single borrows a stable `cui_id` owner, List multi borrows bools.
  `cui_list_state.active` is the independent navigation cursor; it is not selection.
- [ToggleGroup reducer](../../src/base/cui_choice.c#L490) registers BUTTON items,
  processes unconsumed events in order, toggles the cursor item on activation,
  applies `allow_empty`/`multiple`, navigates/focuses, then updates semantics.
  In single mode selecting a new item clears all bools before setting that item.
  Caller arrays can initially contain several true values; do not silently add
  normalization. Its bool means some accepted mutation occurred, including
  toggling back to entry, not final-array inequality. It has no legacy callbacks.
- [Private List loop](../../src/base/cui_choice.c#L614) handles scrolling, scope,
  disabled/consumed events, secondary highlighting and Escape. Activation writes
  selected ID and active cursor, then calls `on_activate`, then `on_confirm`.
  `on_confirm` receives the configured secondary-modifier interpretation. Same-ID
  confirmation still invokes both callbacks and returns activated=true, although
  it need not change selection. `on_cancel` handles nonrepeat Escape independently
  of activation/selection, including an empty scoped list. Navigation/typeahead
  changes active/focus/window without writing selected ID. Scroll-window changes
  discard stale pointer-up activation. Final semantics follow the whole loop.
- [Existing selection forwarding](../../src/base/cui_choice.c#L690) snapshots the
  prior owner, captures available input metadata, performs activation and the old
  callbacks, then reads the final owner and emits typed CHANGE only if different.
  [Select](../../src/components/cui_choice.c#L1279) and
  [Combobox](../../src/components/cui_choice.c#L1444) use this seam; typed delivery
  precedes their existing close/paint policies. Combo's ordered editor/reducer
  path has its own forwarding, and must not be rerouted through a second List
  pass. [Simple Menu tests](../../tests/unit/test_simple_menu_events.c) exercise
  the sibling per-command ACTIVATE seam, including repeated same-ID activation;
  command activation is not selection CHANGE.
- [List multi](../../src/base/cui_choice.c#L1095) installs a synchronous activation
  adapter that flips the matched bool **before** the user's old `on_activate`;
  it forwards confirm/cancel with original userdata. Final multi semantics are
  patched after the List pass. Its return is activation, even two flips net zero.
  Do not observe the temporary internal single action ID as multi selection.
- [Styled List](../../src/components/cui_choice.c#L1137),
  [multi](../../src/components/cui_choice.c#L1459) and
  [ToggleGroup](../../src/components/cui_choice.c#L898) reduce once before paint.
  List resolves scroll receiving geometry and design metrics first. Rendering
  callbacks borrow item/index; indices are relative to the displayed array.

Existing tests establish these contracts: [multi selection and two same-item
flips](../../tests/unit/test_choice.c#L340), [confirmation/secondary/cancel
ordering](../../tests/unit/test_choice.c#L3086), [Select per-transition net-zero
and missing activation snapshot](../../tests/unit/test_select_events.c), and
[ordered Combo events](../../tests/unit/test_combobox_events.c). Preserve them.

## Smallest independent API

A separate `cui_list_events.h` includes `cui_choice.h` and existing shared
selection types; no edits to old options/state layouts. Single List reuses
`cui_selection_change`/`cui_selection_handler` and its existing per-transition
private seam, rather than wrapping a whole-frame bool. Add explicit compound ID
to the new Base signature because the old Base List has no container parameter.

```c
typedef struct {
    cui_selection_event_result events;
    bool legacy_activated; /* Original bool, not final owner inequality. */
} cui_list_event_result;
cui_list_event_result cui_base_list_events(cui_context *, cui_id control,
    const cui_choice_item *, size_t count, cui_id *selected,
    cui_list_state *, cui_list_options, cui_selection_handler);
cui_list_event_result cui_list_events(cui_context *, cui_id container,
    const cui_theme *, const cui_choice_item *, size_t count, cui_id *selected,
    cui_list_state *, cui_list_options, cui_selection_handler);

typedef struct {
    cui_component_event_metadata event;
    cui_id item;         /* Item whose bool changed. */
    cui_id trigger_item; /* Activated item causing this operation. */
    bool previous, value;
} cui_choice_item_change;
typedef void (*cui_choice_item_change_fn)(void *user, const cui_choice_item_change *event);
typedef struct { cui_choice_item_change_fn on_change; void *user; } cui_choice_item_handler;
typedef struct {
    bool valid, submitted, legacy_activated, count_overflow;
    size_t change_count; /* Actual typed calls; can exceed activation count. */
} cui_choice_items_event_result;
cui_choice_items_event_result cui_base_list_multi_events(cui_context *, cui_id control,
    const cui_choice_item *, size_t count, bool *selected,
    cui_list_state *, cui_list_options, cui_choice_item_handler);
cui_choice_items_event_result cui_list_multi_events(cui_context *, cui_id container,
    const cui_theme *, const cui_choice_item *, size_t count, bool *selected,
    cui_list_state *, cui_list_options, cui_choice_item_handler);
cui_choice_items_event_result cui_base_toggle_group_events(cui_context *, cui_id control,
    const cui_choice_item *, size_t count, bool *selected,
    cui_toggle_group_options, cui_choice_item_handler);
cui_choice_items_event_result cui_toggle_group_events(cui_context *, cui_id container,
    const cui_theme *, const cui_choice_item *, size_t count, bool *selected,
    cui_toggle_group_options, cui_choice_item_handler);
```

`legacy_activated` preserves the original bool for each family; for ToggleGroup
it specifically means its old accepted-change bool, not proof of a rejected
activation. Consider naming this common field `legacy_result` before production
if the mixed naming would mislead consumers. No invented acceptance counter.
Missing callback accepts/ignores user and still writes owners. Actual callback
counts saturate with `count_overflow`, following shared conventions.

Single List must keep its existing order: owner write -> legacy activate -> legacy
confirm -> typed selection CHANGE; cancel remains on its old independent branch.
Same-ID confirmation emits no CHANGE unless old callbacks actually redirect the
owner. Do not replace confirm/cancel with SUBMIT/LIFECYCLE, add a second dispatch,
or change when popup adapters act. Options' old callbacks and typed handler may
coexist; no arbitrary conflict-preflight is needed for this independent observer.
Retain old callback arguments/userdata and permit their existing owner-update
behavior. As with current Select, typed payload reads the final owner after those
callbacks; writes elsewhere by user callbacks are not automatically observed.

List multi needs a real typed hook in the private activation/adapter path: capture
that item's previous bool immediately before its reducer flip, preserve legacy
activate/confirm calls, then read that item's final bool and emit if different.
Do not dispatch from the adapter before legacy confirm, nor infer bool transitions
from action IDs after the full pass. Repeated toggles deliver true/false in order,
even when final array equals entry. Legacy callback redirects may cancel the
item's net transition; unrelated legacy array writes get no fabricated events.

ToggleGroup needs a private common reducer, with the old entrypoint using its
unobserved path. On single-mode selection emit each actual true->false clear in
array order at its real write, then the chosen false->true write; skip false->false
stores. `trigger_item` identifies the activated cause for cleared siblings. In
multiple mode emit only that item's actual flip. `allow_empty` rejection emits
nothing. Callbacks can observe an intermediate array during the clearing loop;
this is a new per-write observer contract, not an atomic whole-array snapshot.
Preserve old navigation/focus/consumption and paint after all reduction. No heap
copy or aggregate compare is needed. Single List, multi List and ToggleGroup can
land as separately reviewed/tested slices.

## Metadata, ownership and limitations

Only ordinary active, enabled submissions notify; disabled/inert/layout-only
paths retain their exact old behavior without typed calls. Preserve event order.
At a real transition capture shared CHANGE metadata with compound control target,
current frame/index and source only when established from the accepted event.
Use an activation snapshot only if the existing API actually supplies it; List
registers semantics after reduction and existing Select tests explicitly lack
HAS_ACTIVATION. Fall back to truthful frame/index/source, never synthesize a
snapshot or claim a physical origin from selection values. Sibling clear events
share the causal input metadata; their `item` differs from `trigger_item`.
Navigation-only focus/typeahead/scroll changes are not selection transitions.

List OPTION is focusable in existing routing, but
[Core's accessibility ACTIVATE allowlist](../../src/core/cui.c#L1245) does not
include ordinary OPTION (unless explicit pointer-activatable details already
apply). Test actual queue rejection; do not relabel options as buttons or claim
new accessibility activation parity. Menu-mode MENUITEM and ToggleGroup BUTTON
have different supported activation contracts. Secondary highlight is not CHANGE;
confirm modifier semantics remain legacy. No general bubbling or input routing.

Payloads contain stable IDs/bools and are pointer-free copies, not array indices
or item pointers. Handler/user borrow only this synchronous submission. Descriptors,
labels, state and selected arrays must remain alive and retain ordering/identity
through reduction, legacy callbacks, custom rendering and renderer consumption.
During typed callbacks forbid owner/array/state/model mutation, item insertion,
removal/reorder, recursive submission, storage/context destruction and foreign
unwinding. Unrelated application state may change; defer edits/teardown until
submission and renderer consumption finish. Legacy callback timing is preserved,
not broadened into permission for arbitrary destructive legacy operations.
No retained registrations or unsubscribe/teardown architecture is introduced.

## Regression plan and omissions

Use real queued pointer, key and supported accessibility input. Check each
transition and copied payload, including single A->B->A, repeated same-item
confirmation, multi double flip and ToggleGroup multiple initially-true values
cleared before the new selection. Assert legacy activate/confirm/cancel trace and
userdata, including owner redirects, nil typed observer, consumed events,
allow_empty rejection, disabled items/parents, inert/layout-only and empty lists.
Exercise roving focus, typeahead, window scrolling plus stale pointer-up,
secondary modifiers/highlight and empty scoped Escape. Compare legacy/new bools,
owners, states, semantics/focus, visible windows and custom/default paint. Keep
Select/Combo/Simple Menu regressions unchanged to prove private seam compatibility.
Run strict C11/C++ checks and private-display wrapper tests after implementation.

No new Select-multi/Combo-multi/chip-removal facade or binding schema in this first
proposal: their filtered/scratch arrays and removal paths require separate stable-ID
mapping audits before forwarding these bool events. Existing bridge legacy change
queues and Python/Node/Go/Rust wrappers remain unchanged. Native typed APIs do not
imply typed binding selection payloads or new accessibility support.
