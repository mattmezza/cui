# Questionnaire keyboard policy contract checkpoint

This header-only checkpoint proposes additive APIs in
`cui_questionnaire_keyboard.h`. They have declarations but no implementation yet;
do not link consumer code against them until the implementation slice is reviewed.
It implements no snapshot/event facade and changes no existing API.

Zero/NULL preserves the corresponding legacy entry point. The three enum fields
normalize unknown values independently to legacy. A copied options record carries
no callbacks, strings, allocation, platform types or retained caller pointers.
Root policy is copied before capture and applies to ordered input routing, including
confirmation routed through an editor. Existing choice parts inherit the matching
active root's shortcut mode; explicit standalone overrides are documented in the
header. The host must keep per-part and root activation policies consistent.

Frozen source `87d10ae5e1299d1be18670a37570c5402f952e34` defaults shortcuts to
absent (`base/questionnaire/state.rs:121,343–348`). OFF must suppress both key
activation and the default/custom shortcut adornment. It does not disable actual
choice controls. Letters/numbers retain enabled-position ordering.

Source adjacent-answer restoration applies only from item focus
(`state.rs:894–975`). The opt-in item-focus policy preserves selected-answer
restoration there, but enters the first/last enabled answer from action/root focus.
Legacy action-focus restoration remains unchanged. The standalone helper takes an
explicit item ID because the existing helper does not know that container's ID.

Source keyboard submission uses the host platform secondary modifier. The new
record explicitly chooses Control, Meta or unmodified-only; it never guesses the
platform. Exact modifier matching preserves rejection of Shift/Alt combinations.
Bare Enter still requires a filled focused answer. Automatic-root legacy accepts
Control or Meta; standalone legacy preserves its existing explicit modifier input.

Nested roots currently share a context route stack. Each begin captures before
pushing itself; therefore an outer root may inspect events before a later inner
begin. A stack is not evidence of nearest-root dispatch isolation. Implementation
will add a characterization test (including distinct states and omitted parts)
and preserve existing consumption order. No nesting architecture change is part
of this slice; source capture ordering must be measured before claiming parity.

Production acceptance requires differential queued-key tests for OFF/letters/
numbers; item versus action focus; host modifiers; unknown enums independently;
IME/repeat/consumed guards; editor ownership; disabled transitions; and delayed
confirmation. Test real badge pixels/semantic names/custom renderer calls and
zero-policy draw/semantics/callback equivalence. Run strict Release through the
private headless wrapper, focused WASI proof, C/C++ header checks, and actual pinned
repository formatting after the final source edit. Canonical registration,
generated distributions, metadata and skills remain integrator-owned.
