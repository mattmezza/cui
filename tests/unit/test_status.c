/* SPDX-License-Identifier: MIT */
#include "cui/cui_status.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#ifndef CUI_STATUS_BASE_ONLY
static bool equal(cui_color a, cui_color b) {
    return memcmp(&a, &b, sizeof(a)) == 0;
}
#endif
static void marker_models(void) {
    cui_marker_options o = cui_marker_options_default();
    cui_marker_plan p;
    cui_marker_child text = {.kind = CUI_MARKER_TEXT, .text = {"Thinking", 8}};
    o.loading = true;
    o.loading_style = CUI_MARKER_SHIMMER;
    o.now_seconds = .5;
    assert(cui_base_marker(o, &text, 1, &p) && p.animated && !p.spinner && p.phase == .25f &&
           p.next_wake_seconds > .5);
    o.reverse = true;
    assert(cui_base_marker(o, &text, 1, &p) && p.phase == .75f);
    o.reduced_motion = true;
    assert(cui_base_marker(o, &text, 1, &p) && !p.animated && p.rich_opacity == 1 &&
           p.next_wake_seconds == 0);
    o.reduced_motion = false;
    o.once = true;
    o.now_seconds = 3;
    assert(cui_base_marker(o, &text, 1, &p) && !p.animated && p.phase == 0);
    o.variant = CUI_MARKER_SEPARATOR;
    o.alignment = CUI_MARKER_ALIGN_END;
    assert(cui_base_marker(o, &text, 1, &p) && p.alignment == CUI_MARKER_ALIGN_END);
    assert(!cui_base_marker(o, &text, CUI_MARKER_MAX_CHILDREN + 1, &p));
}
#ifndef CUI_STATUS_BASE_ONLY
static void marker_render(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_marker_child children[] = {{.kind = CUI_MARKER_TEXT, .text = {"Thinking", 8}},
                                   {.kind = CUI_MARKER_DIRECT, .text = {"!", 1}, .width = 10}};
    for (size_t theme = 0; theme < 4; theme++) {
        cui_marker_state state = {0};
        cui_marker_options o = cui_marker_options_default();
        o.loading = true;
        o.loading_style = CUI_MARKER_SHIMMER;
        o.now_seconds = 1;
        o.role = CUI_ROLE_STATUS;
        o.name = cui_string("Progress");
        for (int frame = 0; frame < 2; frame++) {
            assert(cui_begin_frame(ui, 500, 300, .016f));
            assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            if (frame == 1) {
                size_t clips = 0, text = 0;
                for (size_t i = 0; i < list.count; i++) {
                    clips += list.commands[i].type == CUI_DRAW_CLIP_PUSH;
                    text += list.commands[i].type == CUI_DRAW_TEXT;
                    if (list.commands[i].type == CUI_DRAW_CLIP_PUSH) {
                        assert(list.commands[i].bounds.w > 0 &&
                               list.commands[i].bounds.w <= state.text_bounds[0].w);
                    }
                }
                assert(clips == 12 && text == 14);
            }
        }
        o.reduced_motion = true;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
        cui_draw_list list = cui_end_frame(ui);
        for (size_t i = 0; i < list.count; i++)
            assert(list.commands[i].type != CUI_DRAW_CLIP_PUSH);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == 1 && sem[0].role == CUI_ROLE_STATUS);
        o.variant = CUI_MARKER_BORDER;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 91, &themes[theme], &state, children, 2, o));
        list = cui_end_frame(ui);
        bool border = false;
        for (size_t i = 0; i < list.count; i++)
            if (list.commands[i].type == CUI_DRAW_BORDER) {
                assert(list.commands[i].custom_border_edges &&
                       list.commands[i].border_edges.bottom == 1 &&
                       list.commands[i].border_edges.top == 0);
                border = true;
            }
        assert(border);
    }
    cui_marker_state state = {0};
    cui_marker_options o = cui_marker_options_default();
    cui_marker_child textchild = {.kind = CUI_MARKER_TEXT, .text = {"Center", 6}};
    for (int alignment = CUI_MARKER_ALIGN_START; alignment <= CUI_MARKER_ALIGN_END; alignment++) {
        o.variant = CUI_MARKER_SEPARATOR;
        o.alignment = (cui_marker_alignment)alignment;
        assert(cui_begin_frame(ui, 500, 300, .016f));
        assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
        cui_draw_list list = cui_end_frame(ui);
        float x = -1, w = 0;
        size_t lines = 0;
        for (size_t i = 0; i < list.count; i++) {
            if (list.commands[i].type == CUI_DRAW_TEXT) {
                x = list.commands[i].bounds.x;
                w = list.commands[i].bounds.w;
            }
            if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].bounds.h == 1)
                lines++;
        }
        assert(lines == (alignment == CUI_MARKER_ALIGN_CENTER ? 2u : 1u));
        float expected = alignment == CUI_MARKER_ALIGN_START    ? 0
                         : alignment == CUI_MARKER_ALIGN_CENTER ? (o.width - w) / 2
                                                                : o.width - w;
        assert(fabsf(x - expected) < 1);
    }
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    o.loading = true;
    o.loading_style = CUI_MARKER_SHIMMER;
    o.now_seconds = .4;
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 500, 300, .016f));
    assert(cui_marker(ui, 92, &themes[0], &state, &textchild, 1, o));
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    cui_destroy(ui);
}
#endif
int main(void) {
    marker_models();
#ifndef CUI_STATUS_BASE_ONLY
    marker_render();
#endif
#ifdef CUI_STATUS_BASE_ONLY
    cui_marker_options o = cui_marker_options_default();
    cui_marker_plan p;
    assert(cui_base_marker(o, NULL, 0, &p));
    assert(p.alignment == CUI_MARKER_ALIGN_START && !p.animated);
    o.variant = CUI_MARKER_SEPARATOR;
    assert(cui_base_marker(o, NULL, 0, &p) && p.alignment == CUI_MARKER_ALIGN_CENTER);
    o.loading = true;
    assert(cui_base_marker(o, NULL, 0, &p) && p.spinner && p.animated);
    cui_marker_child icon = {.kind = CUI_MARKER_ICON};
    assert(cui_base_marker(o, &icon, 1, &p) && !p.spinner && !p.animated);
    o.width = NAN;
    assert(!cui_base_marker(o, NULL, 0, &p));
    return 0;
#else
#ifdef CUI_STATUS_BASE_ONLY
    cui_theme themes[4] = {
        {.font_size = 14, .radius = 8, .surface = {1, 2, 3, 255}, .text = {4, 5, 6, 255}}};
    for (size_t i = 1; i < 4; i++)
        themes[i] = themes[0];
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    cui_tag_options o = cui_tag_options_default();
    assert(o.variant == CUI_TAG_SECONDARY && o.size == CUI_STATUS_MEDIUM);
    for (size_t i = 0; i < 4; i++) {
        cui_theme *t = &themes[i];
        cui_tag_style s;
        assert(cui_tag_resolve(t, o, false, &s));
        assert(equal(s.background, t->surface));
        assert(s.padding_x == 10 && s.padding_y == 4 && s.radius == t->radius);
        for (int v = CUI_TAG_PRIMARY; v <= CUI_TAG_INFO; v++) {
            o.variant = (cui_tag_variant)v;
            o.outline = false;
            assert(cui_tag_resolve(t, o, false, &s));
            cui_color bg = s.background, border = s.border;
            o.outline = true;
            assert(cui_tag_resolve(t, o, false, &s));
            assert(s.background.a == 0 && equal(s.foreground, bg) && equal(s.border, border));
        }
        o = cui_tag_options_default();
        o.size = CUI_STATUS_SMALL;
        assert(cui_tag_resolve(t, o, false, &s));
        assert(s.padding_x == 6 && s.padding_y == 2 && s.radius == t->radius / 2);
        o.custom_radius = true;
        o.radius = 0;
        assert(cui_tag_resolve(t, o, false, &s) && s.radius == 0);
        o.pill = true;
        assert(cui_tag_resolve(t, o, false, &s) && s.radius > 1000);
        o = cui_tag_options_default();
#ifndef CUI_STATUS_BASE_ONLY
        cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
        assert(ui);
        assert(cui_begin_frame(ui, 300, 100, .016f));
        assert(cui_tag(ui, 21, t, cui_string("Ready"), o));
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        bool text = false, border = false;
        for (size_t k = 0; k < list.count; k++) {
            text |= list.commands[k].type == CUI_DRAW_TEXT;
            border |= list.commands[k].type == CUI_DRAW_BORDER;
        }
        assert(text && border);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].id == 21 && sem[0].role == CUI_ROLE_LABEL);
        cui_destroy(ui);
#endif
    }
    cui_tag_style s = {.radius = 123};
    o = cui_tag_options_default();
    o.radius = NAN;
    assert(!cui_tag_resolve(&themes[0], o, false, &s) && s.radius == 123);
    o = cui_tag_options_default();
    o.variant = CUI_TAG_PALETTE;
    o.palette = (cui_tag_palette){.scale50 = {1, 2, 3, 255},
                                  .scale200 = {4, 5, 6, 255},
                                  .scale600 = {7, 8, 9, 255},
                                  .scale950 = {10, 11, 12, 255},
                                  .scale800 = {13, 14, 15, 255},
                                  .scale300 = {16, 17, 18, 255}};
    assert(cui_tag_resolve(&themes[0], o, false, &s) && s.background.r == 1 &&
           s.foreground.r == 7 && s.border.r == 4);
    o.palette.dark = true;
    assert(cui_tag_resolve(&themes[0], o, false, &s) && s.background.r == 10 &&
           s.background.a == 127 && s.foreground.r == 16 && s.border.r == 13);
    assert(cui_tag_resolve(&themes[0], o, true, &s) && s.background.a == 114);
    cui_tag_palette named;
    assert(cui_tag_named_palette(CUI_STATUS_COLOR_BLUE, false, &named));
    assert(named.scale50.r == 239 && named.scale50.g == 246 && named.scale50.b == 255);
    for (int name = CUI_STATUS_COLOR_WHITE; name <= CUI_STATUS_COLOR_ROSE; name++)
        assert(cui_tag_named_palette((cui_status_color)name, true, &named) && named.dark);
    named.dark = false;
    assert(!cui_tag_named_palette((cui_status_color)99, true, &named) && !named.dark);
#ifndef CUI_STATUS_BASE_ONLY
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 100});
    assert(ui);
    o = cui_tag_options_default();
    assert(cui_begin_frame(ui, 300, 100, .016f));
    assert(cui_begin_tag(ui, 44, &themes[0], o, &s));
    cui_begin_box(ui, 45, (cui_box){.width = 40, .height = 20});
    cui_end_box(ui);
    cui_end_tag(ui, 44, &s);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    bool geometry = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_RECT &&
            equal(list.commands[i].color, themes[0].surface)) {
            assert(list.commands[i].bounds.w == 60 && list.commands[i].bounds.h == 28);
            geometry = true;
        }
    assert(geometry);
    cui_destroy(ui);
#endif
    return 0;
#endif
}
