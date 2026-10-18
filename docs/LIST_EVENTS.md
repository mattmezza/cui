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

This checkpoint implements single List only. Multi List and ToggleGroup are
separate transition slices. Bridge and Python/Node/Go/Rust behavior is unchanged.
