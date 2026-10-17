/* SPDX-License-Identifier: MIT */
#include "cui/cui_typography.h"
#include <assert.h>
#include <string.h>
static cui_size measure(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    (void)style;
    return (cui_size){(float)text.length * size * .5f, size};
}
int main(void) {
    cui_typography_roles roles = {
        .body = {.size = 16.5f, .line_height = 25, .wrap = true, .style = {.family = UINT32_MAX}},
        .heading = {.size = 30,
                    .style = {.weight = CUI_TEXT_WEIGHT_BOLD},
                    .align = CUI_TEXT_ALIGN_CENTER},
        .code = {.size = 14, .style = {.monospace = true}}};
    cui_typography_token out = {.size = 77};
    assert(!cui_typography_resolve(NULL, CUI_TYPOGRAPHY_BODY, &out) && out.size == 77);
    assert(!cui_typography_resolve(&roles, (cui_typography_role)99, &out) && out.size == 77);
    assert(!cui_typography_resolve(&roles, CUI_TYPOGRAPHY_CODE, NULL));
    assert(cui_typography_resolve(&roles, CUI_TYPOGRAPHY_BODY, &out));
    assert(out.size == 16.5f && out.style.family == UINT32_MAX && out.wrap);
    out.size = 18;
    assert(roles.body.size == 16.5f); /* Resolved overrides never mutate defaults. */
    cui_typography_token invalid[] = {{.size = 0},
                                      {.size = NAN},
                                      {.size = INFINITY},
                                      {.size = -1},
                                      {.size = 65536},
                                      {.size = 16, .line_height = -1},
                                      {.size = 16, .line_height = INFINITY},
                                      {.size = 16, .line_height = 65536},
                                      {.size = 16, .align = (cui_text_align)99},
                                      {.size = 16, .style = {.weight = (cui_text_weight)99}}};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        assert(!cui_typography_token_valid(invalid[i]));
        roles.body = invalid[i];
        out.size = 77;
        assert(!cui_typography_resolve(&roles, CUI_TYPOGRAPHY_BODY, &out) && out.size == 77);
    }
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = measure});
    assert(ui && cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 180, .fixed_width = true});
    assert(!cui_typography_text(ui, cui_string("Rejected"), invalid[0], (cui_color){0}));
    cui_text_color_span span = {0, 2, {255, 0, 0, 255}};
    assert(cui_typography_text_colored(ui, cui_string("Hi"), roles.heading,
                                       (cui_color){0, 0, 0, 255}, &span, 1));
    assert(!cui_typography_text(ui, (cui_str){"\xff", 1}, roles.heading, (cui_color){0}));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t text = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type != CUI_DRAW_TEXT)
            continue;
        ++text;
        assert(c->font_size == 30 && c->text_style.weight == CUI_TEXT_WEIGHT_BOLD);
        assert(c->text_color_count == 1 && c->text_colors[0].color.r == 255);
        assert(c->bounds.x > 0); /* Centered within a constrained parent. */
    }
    assert(text == 1 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
