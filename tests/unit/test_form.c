/* SPDX-License-Identifier: MIT */
#include "cui/cui_form.h"
#include <assert.h>
#include <math.h>
#ifndef CUI_FORM_BASE_ONLY
#include "cui/cui_components.h"
#endif
static void planning(void) {
    cui_form_field f[] = {{.id = 1},
                          {.id = 2},
                          {.id = 3, .column_span = 2},
                          {.id = 4, .column_start = 2},
                          {.id = 5, .hidden = true}};
    cui_form_cell c[5];
    size_t rows;
    cui_form_options o = {.width = 400, .columns = 2};
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_OK && rows == 3);
    assert(c[0].column == 0 && c[1].column == 1 && c[0].row == c[1].row);
    assert(c[2].row == 1 && c[2].width == 400 && c[3].row == 2 && !c[4].visible);
    assert(fabsf(c[0].width - 188) < .01f);
    f[2].column_start = 1;
    f[2].column_end = -1;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_OK && c[2].span == 2);
    f[2].column_end = 1;
    c[0].row = 777;
    rows = 888;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_INVALID && c[0].row == 777 &&
           rows == 888);
    f[2].column_end = 0;
    assert(cui_form_plan(f, 5, o, c, 4, &rows) == CUI_FORM_CAPACITY);
    f[1].id = 1;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_DUPLICATE_ID);
    f[1].id = 2;
    o.width = NAN;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_INVALID);
}
static void base_semantics(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 500});
    assert(ui);
    cui_form_field f[] = {{.id = 10,
                           .control = 11,
                           .label = {"Email", 5},
                           .required = true,
                           .invalid = true,
                           .error = {"Invalid email", 13}},
                          {.id = 20, .hidden = true}};
    assert(cui_begin_frame(ui, 500, 500, .016f));
    assert(cui_base_form(ui, 1, f, 2, (cui_form_options){0}) == CUI_FORM_OK);
    (void)cui_interact(ui, 11, CUI_ROLE_TEXTBOX, cui_string("Email"), false, false, 0);
    cui_form_associate_control(ui, f);
    cui_end_frame(ui);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 4);
    assert(sem[1].details.required && sem[1].details.invalid && sem[1].details.parent == 1);
    assert(sem[3].details.parent == 10 && sem[3].details.labelled_by == cui_form_label_id(10));
    assert(sem[3].details.description.length == 13);
    cui_destroy(ui);
}
#ifndef CUI_FORM_BASE_ONLY
typedef struct {
    cui_id control;
    unsigned descriptions;
} content;
static void control(cui_context *ui, const cui_theme *t, void *user, float width) {
    (void)t;
    content *c = user;
    cui_begin_box(ui, c->control, (cui_box){.width = width, .height = 24});
    cui_end_box(ui);
}
static void description(cui_context *ui, const cui_theme *t, void *user, float width) {
    (void)width;
    ((content *)user)->descriptions++;
    cui_text(ui, cui_string("Dynamic description"), t->font_size * .75f, t->muted);
}
static void footer(cui_context *ui, const cui_theme *t, void *user, float width) {
    (void)t;
    (void)user;
    (void)width;
    cui_begin_box(ui, 999, (cui_box){.width = 80, .height = 30});
    cui_end_box(ui);
}
static void composition(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (unsigned horizontal = 0; horizontal < 2; horizontal++) {
            cui_context *ui = cui_create((cui_config){.width = 800, .height = 600});
            assert(ui);
            content data[] = {{11, 0}, {21, 0}, {31, 0}};
            cui_form_field f[] = {
                {.id = 10,
                 .label = {"First", 5},
                 .required = true,
                 .render_control = control,
                 .render_description = description,
                 .user = &data[0]},
                {.id = 20, .label = {"Second", 6}, .render_control = control, .user = &data[1]},
                {.id = 30,
                 .label = {"Wide", 4},
                 .column_span = 2,
                 .render_control = control,
                 .user = &data[2]},
                {.id = 40, .hidden = true, .render_control = control, .user = &data[2]}};
            cui_form_options o = {.width = 600,
                                  .columns = 2,
                                  .horizontal_labels = horizontal != 0,
                                  .label_width = 100,
                                  .render_footer = footer};
            assert(cui_begin_frame(ui, 800, 600, .016f));
            assert(cui_form(ui, 1, &themes[theme], f, 4, o) == CUI_FORM_OK);
            assert(cui_end_frame(ui).count > 0 && cui_last_error(ui) == CUI_OK);
            cui_rect a = cui_bounds(ui, 11), b = cui_bounds(ui, 21), wide = cui_bounds(ui, 31),
                     label = cui_bounds(ui, cui_form_label_id(10)), foot = cui_bounds(ui, 999);
            assert(a.y == b.y && b.x > a.x + a.w && wide.y > a.y + a.h);
            assert(wide.w > a.w && foot.y > wide.y + wide.h && foot.x + foot.w == 600);
            if (horizontal)
                assert(label.x + label.w < a.x && label.y == a.y);
            else
                assert(label.y + label.h < a.y && label.x == a.x);
            assert(data[0].descriptions == 1 && data[2].descriptions == 0);
            cui_destroy(ui);
        }
}
typedef struct {
    cui_edit *edit;
    cui_input_state state;
    cui_input_options options;
} editor;
static void input(cui_context *ui, const cui_theme *t, void *user, float width) {
    editor *e = user;
    e->options.width = width;
    (void)cui_input_ex(ui, 11, t, cui_string("Email"), e->edit, &e->state, e->options);
}
static void input_scene(cui_context *ui, editor *e, bool invalid) {
    cui_theme t = cui_theme_light();
    cui_form_field f = {.id = 10,
                        .control = 11,
                        .label = {"Email", 5},
                        .required = true,
                        .invalid = invalid,
                        .error = {"Use an email address", 20},
                        .render_control = input,
                        .user = e};
    assert(cui_begin_frame(ui, 500, 500, .016f));
    assert(cui_form(ui, 1, &t, &f, 1, (cui_form_options){.width = 400}) == CUI_FORM_OK);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void editing(void) {
    char buffer[64] = "";
    editor e = {
        .edit = cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)})};
    assert(e.edit);
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 500});
    assert(ui);
    input_scene(ui, &e, false);
    cui_focus(ui, 11);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "bad"}));
    input_scene(ui, &e, true);
    assert(cui_edit_get_text(e.edit).length == 3);
    e.options.readonly = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
    input_scene(ui, &e, true);
    assert(cui_edit_get_text(e.edit).length == 3);
    e.options.readonly = false;
    e.options.disabled = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
    input_scene(ui, &e, true);
    assert(cui_edit_get_text(e.edit).length == 3);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 11) {
            found = true;
            assert(sem[i].disabled && sem[i].details.required && sem[i].details.invalid &&
                   sem[i].details.labelled_by == cui_form_label_id(10));
        }
    assert(found);
    cui_edit_destroy(e.edit);
    cui_destroy(ui);
}
#endif
int main(void) {
    planning();
    base_semantics();
#ifndef CUI_FORM_BASE_ONLY
    composition();
    editing();
#endif
    return 0;
}
