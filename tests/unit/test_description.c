/* SPDX-License-Identifier: MIT */
#include "cui/cui_description.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void near(float a, float b) {
    assert(fabsf(a - b) < .02f);
}
static void grouping(void) {
    cui_description_item items[] = {{.span = 1},         {.span = 2}, {.span = 1},
                                    {.separator = true}, {.span = 8}, {.span = 0}};
    size_t cursor = 0;
    cui_description_row row;
    assert(cui_description_next_row(items, 6, 3, &cursor, &row));
    assert(row.first == 0 && row.count == 2 && row.spans == 3 && cursor == 2);
    near(cui_description_fraction(&row, 1, 3), 1.f / 3);
    assert(cui_description_next_row(items, 6, 3, &cursor, &row));
    assert(row.count == 1 && cursor == 3);
    near(cui_description_fraction(&row, 1, 3), 1);
    assert(cui_description_next_row(items, 6, 3, &cursor, &row) && row.separator);
    assert(cui_description_next_row(items, 6, 3, &cursor, &row) && row.spans == 8);
    near(cui_description_fraction(&row, 8, 3), 1);
    assert(cui_description_next_row(items, 6, 3, &cursor, &row) && row.spans == 1);
    assert(!cui_description_next_row(items, 6, 3, &cursor, &row));
    row = (cui_description_row){.count = 2, .spans = 3};
    near(cui_description_fraction(&row, 1, 4), .375f);
    near(cui_description_fraction(&row, 2, 4), .625f);
    assert(!cui_description_next_row(NULL, 6, 3, &cursor, &row));
}
static unsigned callbacks;
static void custom(cui_context *ui, const cui_theme *theme, cui_id id, void *user) {
    assert(theme->font_size > 0 && user == &callbacks);
    callbacks++;
    cui_begin_box(ui, id ^ 1337, (cui_box){.width = 15, .height = 47});
    cui_end_box(ui);
}
static void layout(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
    assert(ui);
    cui_description_item items[] = {
        {.id = 100, .label = {"Name", 4}, .value = {"Ada", 3}},
        {.id = 200, .label = {"Role", 4}, .value = {"Engineer", 8}, .span = 2},
        {.id = 300, .separator = true},
        {.id = 400, .label = {"Custom", 6}, .render_value = custom, .value_user = &callbacks}};
    for (unsigned t = 0; t < 4; ++t)
        for (unsigned size = 0; size < 4; ++size)
            for (unsigned vertical = 0; vertical < 2; ++vertical)
                for (unsigned plain = 0; plain < 2; ++plain) {
                    cui_description_options o = cui_description_options_default();
                    o.columns = 4;
                    o.size = (cui_description_size)size;
                    o.vertical = vertical != 0;
                    o.unbordered = plain != 0;
                    o.custom_label_width = true;
                    o.label_width = 70;
                    for (unsigned resize = 0; resize < 2; ++resize) {
                        float width = resize ? 720 : 600;
                        assert(cui_begin_frame(ui, 800, 600, .016f));
                        cui_begin_box(ui, 1, (cui_box){.width = width});
                        assert(cui_description_list(ui, 2, &themes[t], items, 4, o));
                        cui_end_box(ui);
                        cui_end_frame(ui);
                        cui_rect a = cui_bounds(ui, 100), b = cui_bounds(ui, 200);
                        float inner = width - (plain ? 0 : 2);
                        near(a.w, inner * .375f);
                        near(b.w, inner * .625f);
                        near(b.x, a.x + a.w);
                        assert(a.h > 0 && b.h > 0);
                        near(cui_bounds(ui, 300).h, 8);
                        assert(cui_bounds(ui, 400).h >= 47);
                        size_t count;
                        const cui_semantic *sem = cui_semantics(ui, &count);
                        assert(count == 6);
                        for (size_t n = 0; n < count; ++n)
                            assert(sem[n].tab_index == -1 && (sem[n].role == CUI_ROLE_LABEL ||
                                                              sem[n].role == CUI_ROLE_GROUP));
                    }
                }
    assert(callbacks == 128);
    cui_description_item wrap[] = {{.id = 100,
                                    .label = {"Label", 5},
                                    .value = {"one two three four five six seven eight", 38}},
                                   {.id = 200, .label = {"Other", 5}, .value = {"short", 5}}};
    cui_description_options o = cui_description_options_default();
    o.columns = 2;
    o.width = 300;
    o.custom_label_width = o.label_width_percent = true;
    o.label_width = .4f;
    assert(cui_begin_frame(ui, 800, 600, .016f));
    assert(cui_description_list(ui, 2, themes, wrap, 2, o));
    cui_end_frame(ui);
    cui_rect a = cui_bounds(ui, 100), b = cui_bounds(ui, 200);
    near(a.w, 149);
    near(a.h, b.h);
    assert(a.h > 2 * themes[0].font_size * 1.618034f);
    size_t semantics_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantics_count);
    assert(semantics_count == 5);
    /* Label body excludes horizontal padding and the right border. */
    near(semantics[1].bounds.w, 149 * .4f - 17);
    near(semantics[1].bounds.x, a.x + 8);
    assert(cui_begin_frame(ui, 800, 600, .016f));
    items[1].id = items[0].id;
    assert(!cui_description_list(ui, 2, themes, items, 4, cui_description_options_default()));
    assert(!cui_description_list(ui, 0, themes, NULL, 0, cui_description_options_default()));
    cui_end_frame(ui);
    cui_destroy(ui);
}
int main(void) {
    grouping();
    layout();
    return 0;
}
