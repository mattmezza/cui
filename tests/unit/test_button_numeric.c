/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static cui_measure_fn fixture_measure;
static void *fixture_measure_user;
static cui_draw_list scene(cui_context *ui, cui_theme *t, cui_button_options o) {
    assert(cui_begin_frame(ui, 2000, 1000, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 1800, .height = 900});
    cui_button_ex(ui, 10, t, cui_string("Open"), o);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 65536,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    for (unsigned density = 0; density < 2; density++) {
        for (unsigned density = 0; density < 2; density++)
            for (unsigned kind = 0; kind < 2; kind++) {
                cui_theme theme = cui_theme_light();
                if (density)
                    theme.font_size = 14;
                cui_button_options o = {.pixel_size = 80,
                                        .variant = kind ? CUI_BUTTON_TEXT : CUI_BUTTON_LINK};
                scene(ui, &theme, o);
                assert(cui_bounds(ui, 10).h < 40);
                o.icon_only = true;
                o.icon = cui_string("check");
                scene(ui, &theme, o);
                assert(fabsf(cui_bounds(ui, 10).h - 60) < .01f);
                assert(fabsf(cui_bounds(ui, 10).w - 80) > .01f);
                o.height = 90;
                o.width = 100;
                scene(ui, &theme, o);
                assert(fabsf(cui_bounds(ui, 10).h - 90) < .01f &&
                       fabsf(cui_bounds(ui, 10).w - 100) < .01f);
            }
        cui_theme t = cui_theme_light();
        if (density)
            t.font_size = 14;
        for (unsigned size = 0; size < 18; size++) {
            cui_button_options o = {.pixel_size = 80, .dropdown_caret = true};
            if (size < 4)
                o.size = (cui_control_size)size;
            else
                o.design_size = (cui_design_size)(size - 3);
            cui_draw_list normal_draw = scene(ui, &t, o);
            for (size_t i = 0; i < normal_draw.count; i++)
                if (normal_draw.commands[i].type == CUI_DRAW_TEXT)
                    assert(fabsf(normal_draw.commands[i].font_size - t.font_size) < .01f);
            cui_rect normal = cui_bounds(ui, 10);
            assert(normal.h > 0 && normal.h < 40);
            o.compact = true;
            scene(ui, &t, o);
            cui_rect compact = cui_bounds(ui, 10);
            assert(fabsf(compact.w - normal.w) < .01f);
            o.icon_only = true;
            scene(ui, &t, o);
            assert(fabsf(cui_bounds(ui, 10).w - 80) < .01f);
            assert(fabsf(cui_bounds(ui, 10).h - 80) < .01f);
            o.font_size = 30;
            o.icon_size = 22;
            o.height = 90;
            o.width = 100;
            scene(ui, &t, o);
            assert(fabsf(cui_bounds(ui, 10).w - 100) < .01f);
            assert(fabsf(cui_bounds(ui, 10).h - 90) < .01f);
        }
        cui_button_options o = {.pixel_size = 80, .open = true, .toggle_semantics = true};
        scene(ui, &t, o);
        size_t semantic_count = 0;
        const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
        bool found = false;
        for (size_t i = 0; i < semantic_count; i++)
            if (semantics[i].id == 10) {
                assert(!semantics[i].checked && semantics[i].value == 0);
                found = true;
            }
        assert(found);
        o.open = false;
        o.selected = true;
        scene(ui, &t, o);
    }
    cui_theme t = cui_theme_light();
    for (unsigned variant = 0; variant < 12; variant++) {
        cui_button_options o = {.pixel_size = 80,
                                .icon = cui_string("check"),
                                .dropdown_caret = true,
                                .variant = (cui_button_variant)variant,
                                .open = true};
        for (unsigned mode = 0; mode < 4; mode++) {
            o.disabled = mode == 1;
            o.loading = mode == 2;
            o.custom_colors = mode == 3;
            o.colors = t.buttons[CUI_THEME_BUTTON_PRIMARY];
            o.colors.foreground = (cui_color){11, 22, 33, 200};
            o.colors.outline_foreground = (cui_color){44, 55, 66, 180};
            cui_draw_list d = scene(ui, &t, o);
            bool caret = false;
            for (size_t i = 0; i < d.count; i++) {
                const cui_draw_command *c = &d.commands[i];
                if (c->type == CUI_DRAW_IMAGE && c->bounds.w < 20) {
                    assert(fabsf(c->bounds.w - 16) < .01f);
                    if (mode == 3) {
                        bool outline = o.variant == CUI_BUTTON_OUTLINE;
                        assert(c->bitmap.tint.r == (outline ? 44 : 11));
                        assert(c->bitmap.tint.g == (outline ? 55 : 22));
                        assert(c->bitmap.tint.a == (outline ? 135 : 150));
                    }
                    caret = true;
                }
            }
            assert(caret);
        }
    }
    cui_button_group_style_workspace *workspace = calloc(1, sizeof *workspace);
    assert(workspace);
    bool selected[2] = {false, true};
    cui_button_group_styled_item items[2] = {
        {.id = 20, .label = {"One", 3}, .options = {.pixel_size = 80, .open = true}},
        {.id = 21, .label = {"Two", 3}, .options = {.pixel_size = 80, .dropdown_caret = true}}};
    for (unsigned size = 0; size < 18; size++)
        for (unsigned density = 0; density < 2; density++)
            for (unsigned vertical = 0; vertical < 2; vertical++) {
                t = cui_theme_light();
                if (density)
                    t.font_size = 14;
                assert(cui_begin_frame(ui, 2000, 1000, 0));
                cui_button_group_result result;
                assert(cui_button_group_ex(
                    ui, 100, &t, items, selected, 2, workspace,
                    (cui_button_group_style_options){
                        .behavior = {.vertical = vertical},
                        .override_size = true,
                        .size = (cui_control_size)(size < 4 ? size : 0),
                        .design_size = size < 4 ? CUI_DESIGN_INHERIT : (cui_design_size)(size - 3)},
                    &result));
                cui_end_frame(ui);
                cui_rect a = cui_bounds(ui, 20), b = cui_bounds(ui, 21);
                assert(a.h > 0 && a.h < 40 && b.h > 0 && b.h < 40);
                if (vertical)
                    assert(fabsf(a.y + a.h - b.y) < .01f);
                else
                    assert(fabsf(a.x + a.w - b.x) < .01f);
                assert(!selected[0] && selected[1]);
            }
    free(workspace);
    for (unsigned scope = 0; scope < 2; scope++) {
        cui_button_state state = {.initialized = true, .hovered = true, .next_wake_seconds = 42};
        cui_button_state frozen;
        memcpy(&frozen, &state, sizeof state);
        assert(cui_begin_frame(ui, 2000, 1000, 0));
        cui_begin_box(
            ui, 1,
            (cui_box){
                .width = 1800, .height = 900, .inert = scope == 0, .layout_only = scope == 1});
        assert(!cui_button_ex(
            ui, 10, &t, cui_string("Passive numeric"),
            (cui_button_options){
                .pixel_size = 80, .open = true, .dropdown_caret = true, .state = &state}));
        cui_end_box(ui);
        cui_draw_list d = cui_end_frame(ui);
        assert(!memcmp(&state, &frozen, sizeof state));
        assert(scope == 0 ? d.count > 0 : d.count == 0);
        size_t n;
        cui_semantics(ui, &n);
        assert(n == 0);
    }
    float invalid[] = {-1, NAN, INFINITY, 4097};
    for (unsigned i = 0; i < 4; i++) {
        cui_draw_list d =
            scene(ui, &(cui_theme){0}, (cui_button_options){.pixel_size = invalid[i]});
        assert(d.count == 0);
    }
    cui_destroy(ui);
    return 0;
}
