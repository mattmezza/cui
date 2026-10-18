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

New facades require a nonzero compound ID. ToggleGroup is the next independent
slice. Bridge and Python/Node/Go/Rust behavior is unchanged.
