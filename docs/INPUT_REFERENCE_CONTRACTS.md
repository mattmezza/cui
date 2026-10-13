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

## Explicit validation gap

Source `InputBaseState::validate` accepts a predicate for the proposed complete
text. `is_valid_input` treats empty input as valid; replacement rejects only when
the candidate is invalid and the previous text is valid, allowing edits to repair
pre-existing invalid defaults. History replay bypasses that validation.
CUI currently has **no equivalent whole-value edit acceptance callback**.
`cui_input_options.invalid` is an explicit semantic/paint flag, not a predicate;
`required` likewise does not reject an empty edit. Edit errors such as capacity,
invalid UTF-8 or forbidden line breaks produce same-frame invalid semantics and
paint without mutating text/history. A flag cannot substitute for source validation.

A future safe validator needs an engine-owned replacement preview seam that knows
all model storage extents and can enforce scratch alias rules before writes.
Proposed policy is a synchronous `(void *user, cui_str candidate) -> bool` callback
with bounded caller scratch, candidate+NUL sizing, atomic capacity rejection and
observable rejection distinct from engine errors. Empty candidate bypasses the
predicate; invalid candidate invokes the old-value predicate, rejecting only a
previously valid value. Preedit and Undo/Redo bypass it; IME commit, typing, paste,
cut and erasure require consistent preflight. The opaque current model cannot
expose enough storage information to safely bolt this onto a partial widget-only
facade. No callback API is introduced in this batch.

Other source-specific gaps are deliberately preserved: source single-line
normalization strips CR/LF while CUI's edit model rejects them with
`CUI_EDIT_LINE_BREAK`; mask/regex validation, source content-type/password roles,
context-menu capabilities and source lifecycle event callbacks are not supplied
by these two fixes. This guide does not claim those behaviors or full parity.

Integrator: register `tests/unit/test_input_reference_contracts.c` linked only to
`cui`, including portable/WASI configurations. Strict warnings and CTest are
required; root owns registration and generated distribution regeneration.
