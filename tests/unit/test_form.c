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
    f[3].column_start = 1;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_OK && c[3].row > c[2].row);
    o.width = 1;
    assert(cui_form_plan(f, 5, o, c, 5, &rows) == CUI_FORM_INVALID);
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
static cui_input_group_result group_scene(cui_context *ui, cui_edit *edit,
                                          cui_input_group_state *state, cui_input_group_options o,
                                          const cui_input_group_addon *a, size_t count) {
    assert(cui_begin_frame(ui, 500, 500, .016f));
    cui_input_group_result result;
#ifdef CUI_FORM_BASE_ONLY
    result = cui_base_input_group(ui, 100, 101, edit, state, a, count, o);
    cui_begin_box(ui, 100, (cui_box){.width = 300});
    for (size_t i = 0; i < count; i++) {
        cui_begin_box(ui, a[i].id, (cui_box){.width = 60, .height = 30});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 101, (cui_box){.width = 180, .height = 30});
    cui_end_box(ui);
    cui_end_box(ui);
#else
    cui_theme t = cui_theme_dark();
    result = cui_input_group(ui, 100, 101, &t, edit, state, a, count, o);
#endif
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && result.error == CUI_FORM_OK);
    return result;
}
static void click(cui_context *ui, cui_id id) {
    cui_rect r = cui_bounds(ui, id);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w / 2, .y = r.y + r.h / 2}));
}
static void input_groups(void) {
    char buffer[64] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
    assert(edit);
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 500});
    assert(ui);
    cui_input_group_state state = {0};
    cui_input_group_options o = {.width = 300, .name = {"Search", 6}};
    cui_input_group_addon a[] = {
        {.id = 110, .label = {"https://", 8}, .width = 60},
        {.id = 111,
         .alignment = CUI_INPUT_GROUP_INLINE_END,
         .kind = CUI_INPUT_GROUP_BUTTON,
         .label = {"Clear", 5},
         .width = 60},
        {.id = 112, .alignment = CUI_INPUT_GROUP_BLOCK_START, .label = {"Heading", 7}},
        {.id = 113, .alignment = CUI_INPUT_GROUP_BLOCK_END, .label = {"Counter", 7}}};
    group_scene(ui, edit, &state, o, a, 4);
#ifndef CUI_FORM_BASE_ONLY
    cui_rect left = cui_bounds(ui, 110), input = cui_bounds(ui, 101), right = cui_bounds(ui, 111),
             top = cui_bounds(ui, 112), bottom = cui_bounds(ui, 113);
    assert(left.x + left.w <= input.x && input.x + input.w <= right.x && top.y + top.h <= input.y &&
           bottom.y >= input.y + input.h);
#endif
    /* Prefix focus and subsequent text must route in one queued frame. */
    click(ui, 110);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "abc"}));
    assert(group_scene(ui, edit, &state, o, a, 4).input.changed &&
           cui_edit_get_text(edit).length == 3 && cui_focused(ui) == 101);
    click(ui, 111);
    assert(group_scene(ui, edit, &state, o, a, 4).activated_addon == 111 && cui_focused(ui) == 111);
    o.input.readonly = true;
    click(ui, 110);
    group_scene(ui, edit, &state, o, a, 4);
    assert(cui_focused(ui) == 101);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
    assert(!group_scene(ui, edit, &state, o, a, 4).input.changed &&
           cui_edit_get_text(edit).length == 3);
    click(ui, 111);
    assert(group_scene(ui, edit, &state, o, a, 4).activated_addon == 111);
    o.input.disabled = true;
    click(ui, 111);
    assert(!group_scene(ui, edit, &state, o, a, 4).activated_addon);
    o.input.disabled = false;
    a[1].loading = true;
    click(ui, 111);
    assert(!group_scene(ui, edit, &state, o, a, 4).activated_addon);
    o.input.invalid = true;
    group_scene(ui, edit, &state, o, a, 4);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(sem[0].details.invalid && sem[0].details.readonly);
#ifndef CUI_FORM_BASE_ONLY
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme = 0; theme < 4; theme++)
        for (unsigned size = 0; size < 4; size++) {
            o.size = (cui_form_size)size;
            assert(cui_begin_frame(ui, 500, 500, .016f));
            assert(cui_input_group(ui, 100, 101, &themes[theme], edit, &state, a, 4, o).error ==
                   CUI_FORM_OK);
            cui_draw_list draw = cui_end_frame(ui);
            bool danger = false;
            for (size_t i = 0; i < draw.count; i++)
                if (draw.commands[i].type == CUI_DRAW_BORDER &&
                    draw.commands[i].color.r == themes[theme].danger.r &&
                    draw.commands[i].color.g == themes[theme].danger.g &&
                    draw.commands[i].color.b == themes[theme].danger.b)
                    danger = true;
            assert(danger && cui_last_error(ui) == CUI_OK);
        }
#endif
    assert(cui_begin_frame(ui, 500, 500, .016f));
    o.width = INFINITY;
    assert(cui_base_input_group(ui, 100, 101, edit, &state, a, 4, o).error == CUI_FORM_INVALID);
    o.width = 300;
    a[0].width = NAN;
    assert(cui_base_input_group(ui, 100, 101, edit, &state, a, 4, o).error == CUI_FORM_INVALID);
    a[0].width = 60;
    a[1].id = a[0].id;
    assert(cui_base_input_group(ui, 100, 101, edit, &state, a, 4, o).error == CUI_FORM_INVALID);
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
#ifndef CUI_FORM_BASE_ONLY
static void loading_addon_artwork(void) {
    char buffer[32] = "";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof(buffer)});
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 100});
    cui_theme t = cui_theme_light();
    cui_input_group_state state = {0};
    cui_input_group_addon addon = {.id = 110,
                                   .kind = CUI_INPUT_GROUP_BUTTON,
                                   .alignment = CUI_INPUT_GROUP_INLINE_END,
                                   .label = {"Save", 4},
                                   .width = 40,
                                   .loading = true};
    cui_input_group_options options = {.width = 300, .size = CUI_FORM_SMALL, .name = {"Search", 6}};
    assert(ui && edit);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 500, 100, 0));
    cui_input_group_result r = cui_input_group(ui, 100, 101, &t, edit, &state, &addon, 1, options);
    cui_draw_list draw = cui_end_frame(ui);
    unsigned images = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            images++;
            assert(draw.commands[i].bitmap.bitmap.width == 14 &&
                   cui_bitmap_valid(draw.commands[i].bitmap.bitmap));
            assert(draw.commands[i].bitmap.tint.r == t.muted.r);
        }
    assert(images == 1 && r.error == CUI_FORM_OK && !r.activated_addon);
    click(ui, 110);
    assert(!group_scene(ui, edit, &state, options, &addon, 1).activated_addon);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
#endif
int main(void) {
#ifndef CUI_FORM_BASE_ONLY
    loading_addon_artwork();
#endif
    planning();
    base_semantics();
    input_groups();
#ifndef CUI_FORM_BASE_ONLY
    composition();
    editing();
#endif
    return 0;
}
