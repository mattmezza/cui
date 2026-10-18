# Typed List transitions

Include `cui/cui_list_events.h`. `cui_base_list_events` adds an explicit compound
control ID to the old Base arguments; `cui_list_events` uses the Styled container
ID. Both accept the existing `cui_selection_handler` and return `events` plus
`legacy_activated`, the original activation bool. A missing callback ignores user
and preserves reduction and old callbacks.

Each actual selected-ID transition delivers `cui_selection_change` after the
owner write and existing `on_activate` then `on_confirm`. Multiple transitions
can return to the initial owner and still notify separately. Reconfirming the
same item invokes the old callbacks but emits no CHANGE unless they redirect the
owner. Navigation, typeahead, secondary highlight, Escape and scroll-window state
are not selection CHANGE. Old cancel/confirmation and popup behavior are retained.

Payloads are pointer-free ID snapshots, copyable beyond the call. Metadata keeps
the existing private forwarding seam: root target, actual frame/index/source,
and activation snapshot only if available. List registration follows reduction,
so HAS_ACTIVATION is often absent. Ordinary OPTION accessibility ACTIVATE is not
supported by Core's current allowlist; MENUITEM differs. No accessibility parity
or new input-routing behavior is implied. Non-pointer activation still uses the
legacy active cursor; this facade does not redefine its item resolution.

Callbacks/user borrow synchronously. During typed callbacks do not mutate/rebind
owners, state, descriptors or borrowed resources; recurse; destroy storage/context;
or unwind foreign exceptions. Copy payloads/change unrelated application state.
As with existing selection forwarding, consuming later queued events is allowed;
it does not authorize arbitrary model/resource mutation. Defer edits/removal and
teardown until submission and renderer consumption finish. Legacy callbacks keep
their existing owner-update timing; typed changes read the resulting owner after
them. Disabled/inert/layout-only calls retain old behavior without typed events.

List multi adds `cui_base_list_multi_events`/`cui_list_multi_events`, accepting
`cui_choice_item_handler`. Each actual bool transition delivers a pointer-free
`cui_choice_item_change` with affected `item`, causal `trigger_item`, `previous`
and `value`. The adapter captures the bool before its flip, then preserves legacy
activate/confirm order and observes the resulting bool. Legacy callbacks may
redirect that bool and cancel its net transition; unrelated writes by legacy
callbacks get no fabricated notifications. Repeated toggles remain distinct even
when the final array matches entry. Item/selected-array ordering and storage stay
borrowed throughout submission/paint. The result's `legacy_result` preserves the
old activation bool; `change_count` counts actual typed callbacks with saturation.

ToggleGroup adds matching Base/Styled `_events` functions with the same bool
payload and result. Unlike List multi, it notifies immediately after each actual
write. Single-mode selection clears previously true entries in array order, then
sets the chosen entry; callbacks deliberately see intermediate arrays. Sibling
clears use the affected `item` and causal activated `trigger_item`, sharing input
metadata. False-to-false stores produce no event. Disabled siblings can be cleared
by an enabled item's single-mode selection, preserving the legacy array policy.
`allow_empty` rejection emits nothing. Multiple mode emits only the chosen flip.
No workspace or array snapshot is allocated. `legacy_result` here is the old
accepted-change bool, which can remain true after returning to the initial array.
BUTTON activation support differs from ordinary List OPTION; no role remap occurs.

New facades require a nonzero compound ID. Descriptors, strings and bool arrays
must keep identity/order/storage throughout the call and rendering. Initial
multi-true arrays retain their old behavior; no normalization policy was added.
Bridge and Python/Node/Go/Rust behavior is unchanged.
