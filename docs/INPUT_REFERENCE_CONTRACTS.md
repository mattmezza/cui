# Input reference contracts

Audit pin: GPUI Kit commit `87d10ae5e1299d1be18670a37570c5402f952e34`.
Primary source: [movement](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/movement.rs),
[state/validation](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/base/src/input/base/state.rs),
[styled Input](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/input/input.rs), and
[history contracts](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/kit/tests/input/history.rs).
This change addresses two widget behavior gaps and adds tests through the actual
styled `cui_input_ex` control. It does not establish full Input-family parity.

The source `crates/base/src/input/base/movement.rs` `move_vertical` returns
immediately for a single-line Input; `state.rs` `select_up`/`select_down` do too.
CUI's single-line Input now leaves selection unchanged for Up/Down, including
Shift/Ctrl modifiers and read-only inputs. Its Base reducer consumes those keys
unless `delegate_up_down` explicitly yields to a popup/list owner. Multiline
edit models retain existing vertical movement. The generic edit engine's line
movement APIs remain unchanged.

Source `state.rs` `on_blur` breaks undo transaction coalescing while preserving
selection. CUI's retained `cui_input_behavior_state` now records focus and numeric
model identity, splitting coalescing on focus transitions, first binding and model
rebinding. It never dereferences an old model. These operations do not change
document revision, clear history or discard selection. Zero-initialize state and
retain it with its Input; reset state when replacing a model if the allocator can
reuse the exact old address, or when transferring state to a different UI/ID.
The state observes transitions when the widget is submitted: applications that
omit it or change focus away and back entirely between submissions must explicitly
call `cui_edit_break_coalescing` at that application lifecycle boundary. This is
not an OS focus subscription or engine-wide focus observer. The legacy stateless
`cui_base_input` wrapper preserves its prior engine grouping; use retained
`cui_base_input_ex` state for observed focus-transition grouping. The styled
`cui_input` convenience wrapper already retains state through Core resources.

Existing composed paths covered by the new tests include grouped consecutive
typing Undo/Redo, Ctrl-Z/Ctrl-Y/Meta-Shift-Z, grapheme arrow/backspace over combining
marks and regional-indicator flags, history separation after blur, model rebinding,
multiline selection, delegated popup keys, readonly/disabled mutation guards,
required/invalid semantics, same-frame error presentation and IME commit/Undo plus
focus-loss cancellation. IME preedit remains outside committed document/history;
commit is one Undo step. Existing size/geometry/IME APIs remain intact.

## Whole-value validation

The previously missing source predicate is now implemented by the checked engine
seam and composed Input options; see [Input acceptance](INPUT_ACCEPTANCE.md) for
exact callback, healing, IME, borrowing and rollback contracts. Explicit `invalid`
and `required` flags remain distinct from edit acceptance. Source preedit updates
also run the predicate; CUI preserves provisional rendering without committed
mutation while providing that validation. This does not implement regex/mask
constraints or source content-type roles.

Other source-specific gaps are deliberately preserved: source single-line
normalization strips CR/LF while CUI's edit model rejects them with
`CUI_EDIT_LINE_BREAK`; mask/regex validation, source content-type/password roles,
context-menu capabilities and source lifecycle event callbacks are not supplied
by these two fixes. This guide does not claim those behaviors or full parity.

Integrator: register `tests/unit/test_input_reference_contracts.c` linked only to
`cui`, including portable/WASI configurations. Strict warnings and CTest are
required; root owns registration and generated distribution regeneration.
