/* SPDX-License-Identifier: MIT */
#include "app.h"
cui_typography_roles typography_example_roles(void) {
    return (cui_typography_roles){
        .body = {.style = {.family = TYPOGRAPHY_BODY_FAMILY}, .size = 16.5f,
                 .line_height = 26, .wrap = true},
        .heading = {.style = {.family = TYPOGRAPHY_HEADING_FAMILY,
                              .weight = CUI_TEXT_WEIGHT_BOLD},
                    .size = 29.5f, .line_height = 38, .align = CUI_TEXT_ALIGN_CENTER, .wrap = true},
        .code = {.style = {.family = TYPOGRAPHY_CODE_FAMILY, .monospace = true},
                 .size = 14.5f, .line_height = 23, .wrap = false}};
}
cui_draw_list typography_example_frame(cui_context *ui, const cui_typography_roles *roles,
                                        float width, float height) {
    cui_typography_token body, heading, code;
    if (!cui_typography_resolve(roles, CUI_TYPOGRAPHY_BODY, &body) ||
        !cui_typography_resolve(roles, CUI_TYPOGRAPHY_HEADING, &heading) ||
        !cui_typography_resolve(roles, CUI_TYPOGRAPHY_CODE, &code) ||
        !cui_begin_frame(ui, width, height, .016f))
        return (cui_draw_list){0};
    const cui_color ink = {28, 35, 48, 255};
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 24, .gap = 16});
    (void)cui_typography_text(ui, cui_string("Type that fits the story"), heading, ink);
    (void)cui_typography_text(ui, cui_string("Body text uses a readable family, generous line height, "
        "and wrapping that follows the available width when the window changes."), body, ink);
    cui_typography_token emphasis = body;
    emphasis.style.italic = true;
    (void)cui_typography_text(ui, cui_string("A local token override selects an actual italic face."),
                              emphasis, ink);
    /* Code retains whitespace and does not wrap; clip overflow explicitly. */
    cui_begin_box(ui, 2, (cui_box){.width = -1, .clip = true});
    (void)cui_typography_text(ui, cui_string("const float leading = 23.0f;"), code, ink);
    cui_end_box(ui);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
#ifdef CUI_TYPOGRAPHY_NATIVE
cui_font *typography_example_font(const char *body, const char *heading,
                                   const char *heading_bold, const char *body_italic,
                                   const char *code) {
    cui_font *font = cui_font_open(body);
    if (!font)
        return NULL;
    if (!cui_font_register_style(font, (cui_text_style){.family = TYPOGRAPHY_BODY_FAMILY}, body) ||
        !cui_font_register_style(font, (cui_text_style){.family = TYPOGRAPHY_HEADING_FAMILY}, heading) ||
        !cui_font_register_style(font, (cui_text_style){.family = TYPOGRAPHY_HEADING_FAMILY,
                                 .weight = CUI_TEXT_WEIGHT_BOLD}, heading_bold) ||
        !cui_font_register_style(font, (cui_text_style){.family = TYPOGRAPHY_BODY_FAMILY,
                                 .italic = true}, body_italic) ||
        !cui_font_register_style(font, (cui_text_style){.family = TYPOGRAPHY_CODE_FAMILY,
                                 .monospace = true}, code)) {
        cui_font_close(font);
        return NULL;
    }
    return font;
}
#endif
