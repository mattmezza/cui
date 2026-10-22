# Questionnaire named text presentation

Include `cui_questionnaire_presentation.h` with its matching library revision.
Title, Description and ChoiceDescription use independent explicit text options;
there is no style cascade or state/options ABI extension. Existing heading stays
unchanged. Zero named options extract its title/description text metrics; two
separate wrappers do not recreate heading's internal gap automatically.

`TEXT_TOKENS` opts into default token tiers for four legacy sizes: title
14/20,16/24,18/28,20/28 and secondary 12/16,12/16,14/20,16/24
(XSmall,Small,Medium,Large). Title is medium weight; secondary normal. It adds
an item-gap spacer after title when schema description is absent. Shared XS–9XL
uses proportional title 1.125/secondary.875 font and line scale, a CUI extension.
Explicit masked refinements win, including natural line height zero, normal
style and transparent color. Font family IDs require matching host font support.

Title/Description check schema existence, independently of current/disabled
status. Use a successful Item container to gate them, as below. Empty schema
Description omits its wrapper unless explicit visible content or child exists.
ChoiceDescription takes explicit content without state. Error retains legacy
current-item/invalid gating and one Alert named by localized model error.
Custom decorative content replaces paint, not the Alert name. Token Error adds
an 8px top spacer and secondary typography; zero Error preserves legacy paint.

```c
#include <cui/cui_questionnaire_presentation.h>
static void custom_description(cui_context *ui, cui_id id,
                                const cui_questionnaire_text_view *view, void *user) {
    (void)user;
    cui_begin_box(ui, id, (cui_box){.width = -1});
    cui_text_ex(ui, view->text, view->font_size, view->color,
                view->line_height, view->style);
    cui_end_box(ui);
}
void draw_question(cui_context *ui, const cui_theme *theme,
                   cui_questionnaire_state *state, size_t item) {
    cui_questionnaire_options base = {0};
    cui_questionnaire_text_options text = {.policy = CUI_QUESTIONNAIRE_TEXT_TOKENS};
    if (cui_questionnaire_item_begin(ui, 100, state, item, base)) {
        (void)cui_questionnaire_title_ex(ui, 101, theme, state, item, base, text);
        text.render_child = custom_description;
        (void)cui_questionnaire_description_ex(ui, 102, theme, state, item, base, text);
        /* Compose existing choices/input here with the same state. */
        text.render_child = NULL;
        (void)cui_questionnaire_error_ex(ui, 103, theme, state, item, base, text);
        cui_questionnaire_part_end(ui);
    }
}
```

Masked visible strings accept explicit empty, require valid UTF-8 without NUL,
and remain borrowed through completed-frame draw use. Descriptors are copied at
entry. Callback/view/user are borrowed synchronously and not retained; callback
must balance child boxes and must not mutate state, route input or close owner.
Use the supplied child namespace and avoid collisions with application IDs.
A child wins over visible override; resolved view retains that override for use
by the child. Invalid active options reject before output or callbacks. True
means accepted, including omission, not a transaction over child/layout errors.

No model events, validation or editor flushing occurs from these parts. This is
bounded text composition, not arbitrary Styled: card/container/input/Button,
action trailing layout, Form role and full binding presentation remain separate.

The focused unit suite compares independent extracted pre-change text bodies to
zero presentation using draw values, memberwise Alert checks and actual software
pixels. The deterministic test glyph provider proves metrics/style/color transport,
not native font shaping. Token tables, first-line line boxes, long copy wrapping,
explicit empty/custom precedence, descriptor snapshots and passive routed
pointer/key/AT behavior are covered. Native host screen-reader acceptance and
browser presentation controls are not claimed by this slice.
