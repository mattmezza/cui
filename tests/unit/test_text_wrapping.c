/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <string.h>

static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    (void)size;
    return (cui_size){(float)text.length * 10, 12};
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    const char *paragraph = "first second third fourth";
    assert(cui_measure_text(ui, cui_string(paragraph), 12).width > 80);
    for (unsigned api = 0; api < 4; ++api) {
        for (unsigned sizing = 0; sizing < 3; ++sizing) {
            assert(cui_begin_frame(ui, 300, 200, .016f));
            cui_begin_box(ui, 10, (cui_box){.width = 80, .fixed_width = true});
            cui_box box = sizing == 0   ? (cui_box){.width = 80}
                          : sizing == 1 ? (cui_box){.width = 80, .fixed_width = true}
                                        : (cui_box){.width = -1, .max_width = 80};
            cui_begin_box(ui, 11, box);
            if (api == 0)
                cui_text(ui, cui_string(paragraph), 12, (cui_color){0});
            else if (api == 1)
                cui_text_with_line_height(ui, cui_string(paragraph), 12, (cui_color){0}, 20);
            else if (api == 2)
                cui_text_ex(ui, cui_string(paragraph), 12, (cui_color){0}, 20,
                            (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD});
            else
                cui_text_line(ui, cui_string(paragraph), 12, (cui_color){0}, 20,
                              (cui_text_style){0});
            cui_end_box(ui);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            assert(cui_bounds(ui, 11).w == 80);
            assert(list.count == (api == 3 ? 1u : 4u));
            float height = api == 0 ? 12 : 20;
            assert(cui_bounds(ui, 11).h == height * (api == 3 ? 1 : 4));
            for (size_t i = 0; i < list.count; ++i) {
                assert(list.commands[i].type == CUI_DRAW_TEXT);
                if (api == 3) {
                    assert(list.commands[i].text.length == strlen(paragraph));
                    assert(list.commands[i].bounds.w > 80);
                } else {
                    assert(list.commands[i].bounds.w <= 80);
                    if (i)
                        assert(list.commands[i].bounds.y - list.commands[i - 1].bounds.y == height);
                }
            }
        }
    }
    cui_destroy(ui);
    return 0;
}
