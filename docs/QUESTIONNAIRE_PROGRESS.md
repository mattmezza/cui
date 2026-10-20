# Questionnaire progress presentation

Include `cui/cui_questionnaire_progress.h`. `cui_questionnaire_progress_ex` adds
explicit progress text/children without changing legacy options or state layout.
Zero progress options preserve legacy drawing and semantics. Rebuild matching
headers/library to use the added symbol. This is bounded progress presentation,
not a full family typography cascade or language-builder facade.

```c
#include "cui/cui_questionnaire_progress.h"
#include <stdio.h>
/* Application-owned storage must survive completed-frame consumption. */
typedef struct { char label[96]; } progress_text;
bool paint_progress(cui_context *ui, const cui_theme *theme,
                    const cui_questionnaire_state *state, progress_text *storage) {
    size_t position = cui_questionnaire_position(state);
    size_t current = position == SIZE_MAX ? 0 : position + 1;
    int n = snprintf(storage->label, sizeof storage->label, "Frage %zu von %zu",
                     current, cui_questionnaire_total(state));
    if (n < 0 || (size_t)n >= sizeof storage->label)
        return false;
    return cui_questionnaire_progress_ex(
        ui, 71, theme, state, (cui_questionnaire_options){0},
        (cui_questionnaire_progress_options){
            .fields = CUI_QUESTIONNAIRE_PROGRESS_LABEL,
            .label = {storage->label, (size_t)n},
            .text = {.fields = CUI_QUESTIONNAIRE_TEXT_SIZE |
                               CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT,
                     .font_size = 14, .line_height = 20}});
}
```

LABEL replaces the accessible name and default visible sentence. VISIBLE replaces
only visible text. A custom `render_child` replaces visible content altogether;
its callback receives current/total and resolved text metrics. Numeric progress
semantics always come from enabled item position/total, never supplied text.
No current means zero. This is not completion percentage. The role remains a
passive progress bar, with minimum zero and maximum enabled total.

The callback runs once inside the full-width progress wrapper, with a stable,
nonzero child namespace distinct from the root semantic ID. Like existing child
composition, callers avoid collisions with their other IDs; derived IDs are not
a global collision-free allocator. Balance child boxes;
do not close the wrapper, mutate the model or dispatch input. View/user are
borrowed for the call, not retained. Use the view's metrics explicitly when
rendering text; CUI does not inherit an implicit text cascade. Ordinary decorative
children avoid duplicate progress nodes; custom interactive descendants own their
separate semantics. No model events, validation/editor flush or input consumption
are added by this helper.

Masked empty text is explicit empty, including `{NULL,0}`. Empty VISIBLE can hide
the sentence while keeping an accessible label/numbers. Normally keep LABEL
meaningful. Strings require valid UTF-8 with no embedded NUL and remain borrowed
through completed-frame draw/semantic consumption. Descriptors are copied on
entry; root options do not publish these strings or refinements to other parts.
Unmasked values are ignored. The host formats/pluralizes labels with public
position/total queries before submission; no locale registry is retained.

Text masks independently set font size, line height, color and the complete
`cui_text_style`. Explicit normal weight/false flags/family zero/letter spacing
zero work. Font size is finite (0,512]; line height finite [0,4096], zero natural.
Known weight and finite tracking are required. Shaping/font-family fallback and
negative tracking constraints still belong to the host/Core. Source typography
token relationships can be supplied explicitly; legacy sizes and XS..9XL/root
inheritance retain their existing metrics. No arbitrary Styled inheritance,
other-part typography, Core Form role or language presentation API is supplied.

Unknown masks/invalid active values reject before any draw, semantics or callback.
False reports invalid arguments/options. True means accepted submission, not
rollback of custom child layout errors; use normal frame error reporting. Even a
custom child must supply valid masked fields. All existing callbacks and observers
retain their previous behavior.

The focused tests use deterministic metrics and a test-only rectangular-glyph
coverage provider with the actual software renderer. This proves layout/style/
color transport and pixels, not FreeType shaping, system fonts or physical AT.
Native/WASI evidence is separate from a future headless actual-WASM gallery proof.
