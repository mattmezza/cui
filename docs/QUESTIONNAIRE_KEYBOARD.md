# Questionnaire keyboard policies

Include `cui_questionnaire_keyboard.h` with a matching library revision. These
additive entry points preserve the existing options/model layout. They provide
keyboard policy, not a snapshot/event facade. Existing APIs remain available.

Zero/NULL preserves the corresponding legacy entry point. The three enum fields
normalize unknown values independently to legacy. A copied options record carries
no callbacks, strings, allocation, platform types or retained caller pointers.
Root policy is copied before capture and applies to ordered input routing, including
confirmation routed through an editor. Existing choice parts inherit the matching
active root's shortcut mode; explicit standalone overrides are documented in the
header. The host must keep per-part and root activation policies consistent.
Legacy matching-root inheritance predates this API: the active root's
`numeric_shortcuts` supplies ordinary choice badges even when the choice's own
boolean differs. A zero-valued extension root preserves that behavior in both
directions. Standalone choices retain their own boolean; a non-NULL explicit
choice keyboard record follows the override rules in the header.

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
begin. A stack is not evidence of nearest-root dispatch isolation. The
characterization test with distinct states confirms that outer capture can consume
the key before the inner root sees it. Existing consumption order remains. No
nesting architecture change is part of this slice; source capture ordering must
be measured before claiming parity.

The focused fixture covers queued keys for OFF/letters/numbers, item versus action
focus, host modifiers, independent enum fallback, IME/repeat/modifier guards,
standalone helper policies, real badge pixels, semantic names, custom renderer
calls, zero-policy pixel/semantics/callback equivalence, and pointer/accessibility
activation with shortcuts OFF. Existing Styled regressions cover editor ownership,
disabled transitions and delayed confirmation. Validation uses strict Release via
the private headless wrapper, focused WASI, compiled C guide and C++ header, and
actual pinned repository formatting after the final source edit. Canonical registration,
generated distributions, metadata and skills remain integrator-owned.

## Consumer composition

The host chooses its submit modifier explicitly. Submit the policy once on the
root; ordinary choice parts inherit its shortcut mode. OFF suppresses shortcut
adornments (including custom shortcut callbacks) and letter/number activation,
while pointer, focused-control keys and accessibility activation remain enabled.
A standalone choice override only changes that part's display; it does not change
root-wide shortcut routing. Keep root and part policies consistent.

```c
#include "cui/cui_questionnaire_keyboard.h"
void questionnaire_page(cui_context *ui, const cui_theme *theme,
                        cui_questionnaire_state *state) {
    const cui_questionnaire_keyboard_options keyboard = {
        .shortcuts = CUI_QUESTIONNAIRE_SHORTCUT_OFF,
        .answer_entry = CUI_QUESTIONNAIRE_ENTRY_ITEM_FOCUS_ONLY,
        .submit = CUI_QUESTIONNAIRE_SUBMIT_CONTROL
    };
    const cui_questionnaire_options options = {.width = 300};
    if (!cui_questionnaire_begin_with_keyboard(ui, 100, theme, state, options, &keyboard))
        return;
    size_t item = state->current;
    if (cui_questionnaire_item_begin(ui, 101, state, item, options)) {
        cui_questionnaire_heading(ui, 102, theme, state, item, options);
        for (size_t choice = 0; choice < state->items[item].choice_count; ++choice)
            (void)cui_questionnaire_choice_control(ui, 1000 + choice, theme, state,
                                                   item, choice, options);
        cui_questionnaire_part_end(ui);
    }
    cui_questionnaire_end(ui);
}
```

Definitions and caller workspace still outlive the retained state. The record is
copied synchronously and may be stack allocated. IDs must remain stable and unique
in the host tree; the example uses a single active item. Root capture uses previous
frame bindings, so draw an initial frame before sending keyboard input. Standalone
helpers do not inherit root options; pass their record explicitly. Their LEGACY
shortcut mode preserves both `numbers=false` (letters) and `numbers=true`.

The worker fixture registers the focused test and compiles this example without
changing canonical build files. The private WASI driver extends the existing
suite list in memory; canonical registration is an integrator follow-up. This
slice does not complete the family's payload, compound presentation, localization,
Form semantic or general styling gaps.

The interactive gallery exposes an Enable choice shortcuts control alongside the existing numeric-hints switch. Its actual C composition uses `cui_questionnaire_begin_with_keyboard` and the matching standalone shortcut helper, so OFF applies to both rendering and activation. The copyable usage example shows explicit OFF. Headless `tests/web/test_questionnaire_shortcuts_gallery.py` checks actual Tab focus, ignored shortcut keys, default/custom hint suppression, letter/number selection and genuine touch selection while shortcuts are off. It does not establish the remaining snapshot, localization, style or binding surfaces.
