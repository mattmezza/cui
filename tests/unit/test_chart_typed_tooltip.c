/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    unsigned mode, calls, formatters;
} context;
static cui_size measure(void *u, cui_str text, float font, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){text.length * font * .5f, font};
}
static cui_str forbidden_title(const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    ((context *)u)->formatters++;
    return cui_string("Forbidden");
}
static cui_str forbidden_label(const cui_chart_datum *d, size_t i, size_t series, void *u) {
    (void)d;
    (void)i;
    (void)series;
    ((context *)u)->formatters++;
    return cui_string("Forbidden");
}
static size_t forbidden_value(const cui_chart_datum *d, size_t i, size_t series, double v,
                              char *buffer, size_t capacity, void *u) {
    (void)d;
    (void)i;
    (void)series;
    (void)v;
    (void)buffer;
    (void)capacity;
    ((context *)u)->formatters++;
    return 0;
}
static cui_color forbidden_color(const cui_chart_datum *d, size_t i, size_t series, double v,
                                 void *u) {
    (void)d;
    (void)i;
    (void)series;
    (void)v;
    ((context *)u)->formatters++;
    return (cui_color){255, 0, 0, 255};
}
static void unused_content(cui_context *ui, cui_id id, size_t index, const cui_chart_datum *d,
                           void *u) {
    (void)ui;
    (void)id;
    (void)index;
    (void)d;
    (void)u;
}
static bool build(const cui_chart_tooltip_info *info, cui_chart_tooltip_builder *b, void *u) {
    context *c = u;
    c->calls++;
    assert(info->kind == CUI_CHART_LINE && info->index == 0 && info->datum &&
           info->value_count == 1);
    assert(info->values[0] == 20 && info->has_fallback_title);
    if (c->mode == 4)
        return cui_chart_tooltip_set_title(b, cui_string("Title"));
    if (c->mode == 5)
        return cui_chart_tooltip_set_title(b, (cui_str){0});
    if (c->mode == 15) {
        char title[] = "Title";
        assert(cui_chart_tooltip_set_title(b, (cui_str){title, 5}));
        memset(title, 'X', 5);
        return true;
    }
    if (c->mode == 16)
        return cui_chart_tooltip_set_title(b, (cui_str){"\xf0\x9f\x8c\x8d", 4});
    if (c->mode == 13)
        return true;
    if (c->mode == 10)
        return false;
    if (c->mode == 7) {
        for (unsigned i = 0; i < 33; i++)
            (void)cui_chart_tooltip_add_row(b, &(cui_chart_tooltip_row){0});
        return true; /* ignored false append must remain latched */
    }
    if (c->mode == 14) {
        char big[1024];
        memset(big, 'a', sizeof big);
        for (unsigned i = 0; i < 4; i++)
            assert(cui_chart_tooltip_set_title(b, (cui_str){big, sizeof big}));
        return true;
    }
    if (c->mode == 8) {
        char big[1024];
        memset(big, 'a', sizeof big);
        for (unsigned i = 0; i < 5; i++)
            (void)cui_chart_tooltip_set_title(b, (cui_str){big, sizeof big});
        return true;
    }
    if (c->mode == 9) {
        char big[1025];
        memset(big, 'a', sizeof big);
        (void)cui_chart_tooltip_set_title(b, (cui_str){big, sizeof big});
        return true;
    }
    if (c->mode == 11) {
        (void)cui_chart_tooltip_add_row(b, &(cui_chart_tooltip_row){.label = {"\xc0\x80", 2}});
        return true;
    }
    if (c->mode == 12) {
        (void)cui_chart_tooltip_add_row(b, &(cui_chart_tooltip_row){.value = {NULL, 1}});
        return true;
    }
    unsigned rows = c->mode == 6 ? 32 : 2;
    for (unsigned i = 0; i < rows; i++) {
        char label[32], value[32];
        int ln = snprintf(label, sizeof label, "%s", i ? "Total" : "Series");
        int vn = snprintf(value, sizeof value, "%s", i ? "1000" : "20");
        cui_chart_tooltip_row row = {.label = {label, (size_t)ln},
                                     .value = {value, (size_t)vn},
                                     .has_swatch = (c->mode == 2 || c->mode == 3) && i == 0,
                                     .swatch = {220, 10, 20, c->mode == 3 ? 0 : 255},
                                     .has_value_color = i == 1,
                                     .value_color = {0, 180, 0, 255}};
        assert(cui_chart_tooltip_add_row(b, &row));
        memset(label, 'X', sizeof label);
        memset(value, 'Y', sizeof value);
    }
    return true;
}
static const cui_draw_command *find(cui_draw_list draw, const char *s) {
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *c = &draw.commands[i];
        if (c->type == CUI_DRAW_TEXT && c->text.length == strlen(s) &&
            !memcmp(c->text.data, s, c->text.length))
            return c;
    }
    return NULL;
}
static float tooltip_height(cui_draw_list draw) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_BORDER && draw.commands[i].bounds.w == 150)
            return draw.commands[i].bounds.h;
    return -1;
}
static cui_draw_list frame(cui_context *ui, cui_chart_state *state, cui_chart_options *o,
                           context *c, float left, bool pointer) {
    cui_clear_error(ui);
    if (pointer && o->interaction_mode != CUI_PLOT_INTERACTION_LEGACY)
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = left + 20, .y = 20}));
    assert(cui_begin_frame(ui, 400, 260, .016f));
    cui_begin_box(ui, 3, (cui_box){.width = 400, .height = 260});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){left, 0, 128, 96});
    unsigned before = c->calls;
    bool ok = cui_chart(ui, 1, state, o, cui_string("Chart"));
    bool failure = c->mode >= 7 && c->mode <= 12;
    if (o->interaction_mode == CUI_PLOT_INTERACTION_LEGACY)
        assert(ok != failure);
    else
        assert(ok);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(c->calls == before + 1);
    if (failure) {
        assert(!find(draw, "Series") && !find(draw, "20"));
        if (o->interaction_mode == CUI_PLOT_INTERACTION_LEGACY)
            assert(draw.count == 0 && cui_last_error(ui) == 0);
        else
            assert(cui_last_error(ui) == CUI_ERROR_LAYOUT);
    } else
        assert(cui_last_error(ui) == 0);
    return draw;
}
static void behavior(cui_plot_interaction_mode mode) {
    cui_context *ui = cui_create((cui_config){.max_elements = 512,
                                              .max_commands = 2048,
                                              .frame_bytes = 131072,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"Default", 7}, .value = 20},
                              {.label = {"Next", 4}, .value = 40}};
    context c = {.mode = 1};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .interaction_mode = mode,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .hide_axes = true,
                           .hide_legend = true,
                           .tooltip_build = build,
                           .tooltip_build_user = &c,
                           .tooltip_title = forbidden_title,
                           .tooltip_label = forbidden_label,
                           .tooltip_value = forbidden_value,
                           .tooltip_value_color = forbidden_color,
                           .tooltip_user = &c,
                           .foreground = {10, 10, 10, 255},
                           .background = {255, 255, 255, 255},
                           .grid = {100, 100, 100, 255}};
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    state.hovered = 0;
    cui_draw_list draw = frame(ui, &state, &o, &c, 0, true);
    const cui_draw_command *series = find(draw, "Series"), *total = find(draw, "Total"),
                           *v1 = find(draw, "20"), *v2 = find(draw, "1000");
    assert(series && total && v1 && v2 && series->bounds.x == total->bounds.x);
    float plain_x = series->bounds.x;
    assert(fabsf(v1->bounds.x + v1->bounds.w - v2->bounds.x - v2->bounds.w) < .01f);
    assert(v2->color.g == 180 && !find(draw, "Forbidden") && c.formatters == 0);
    c.mode = 2;
    draw = frame(ui, &state, &o, &c, 0, false);
    series = find(draw, "Series");
    total = find(draw, "Total");
    assert(series && total && series->bounds.x == plain_x + 14 &&
           total->bounds.x == series->bounds.x);
    c.mode = 3;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(find(draw, "Series")->bounds.x == plain_x + 14);
    c.mode = 4;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(find(draw, "Title") && !find(draw, "Series"));
    c.mode = 5;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(!find(draw, "Title") && !find(draw, "Series") && tooltip_height(draw) == 34);
    c.mode = 13;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(tooltip_height(draw) == 16);
    c.mode = 6;
    draw = frame(ui, &state, &o, &c, 0, false);
    unsigned values = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 4 &&
            !memcmp(draw.commands[i].text.data, "1000", 4))
            values++;
    assert(mode == CUI_PLOT_INTERACTION_LEGACY ? values > 0 && values <= 31 : values == 31);
    c.mode = 15;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(find(draw, "Title"));
    c.mode = 16;
    draw = frame(ui, &state, &o, &c, 0, false);
    assert(find(draw, "\xf0\x9f\x8c\x8d"));
    c.mode = 14;
    draw = frame(ui, &state, &o, &c, 0, false);
    for (unsigned failure = 7; failure <= 12; failure++) {
        c.mode = failure;
        size_t hovered = state.hovered;
        uint64_t revision = state.revision;
        float focus = state.tooltip_focus;
        bool appeared = state.appear_started;
        draw = frame(ui, &state, &o, &c, 0, false);
        assert(state.hovered == hovered && state.revision == revision &&
               state.tooltip_focus == focus && state.appear_started == appeared);
    }
    c.mode = 1;
    cui_chart_options copied = o;
    cui_chart_state copied_state = state;
    memset(&o, 0, sizeof o);
    memset(&state, 0, sizeof state);
    /* Persistent bounds tracking must not target the local legacy next-state. */
    draw = frame(ui, &copied_state, &copied, &c, 25, mode != CUI_PLOT_INTERACTION_LEGACY);
    assert(find(draw, "Series") && copied_state.bounds.x == 25 && c.formatters == 0);
    assert(cui_begin_frame(ui, 400, 260, .016f));
    copied.tooltip_content = unused_content;
    unsigned before = c.calls;
    assert(!cui_chart(ui, 1, &copied_state, &copied, cui_string("Chart")));
    assert(cui_end_frame(ui).count == 0 && c.calls == before);
    cui_destroy(ui);
}
static void presence_and_fade(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 1024,
                                              .frame_bytes = 65536,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"Default", 7}, .value = 20},
                              {.label = {"Next", 4}, .value = 40}};
    context c = {.mode = 1};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .hide_axes = true,
                           .hide_legend = true,
                           .tooltip_build = build,
                           .tooltip_build_user = &c};
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = 0};
    assert(cui_begin_frame(ui, 400, 260, .016f));
    assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    cui_draw_list draw = cui_end_frame(ui);
    assert(c.calls == 0 && !find(draw, "Series") && state.hovered == SIZE_MAX);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
    assert(cui_begin_frame(ui, 400, 260, .016f));
    assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    draw = cui_end_frame(ui);
    assert(c.calls == 1 && find(draw, "Series"));
    assert(cui_begin_frame(ui, 400, 260, .016f));
    cui_begin_box(ui, 3, (cui_box){.width = 400, .height = 260});
    cui_begin_absolute_box(ui, 2, (cui_box){.clip = true, .custom_opacity = true, .opacity = .5f},
                           (cui_rect){0, 0, 50, 50});
    assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    const cui_draw_command *outside = find(draw, "1000");
    assert(outside && outside->bounds.y > 50 && outside->color.a == 127);
    unsigned before_modal = c.calls;
    assert(cui_begin_frame(ui, 400, 260, .016f));
    cui_begin_box(ui, 3, (cui_box){.width = 400, .height = 260});
    assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    assert(cui_begin_focus_scope(ui, 4, true));
    cui_begin_portal(ui, 5, (cui_box){.background = {255, 255, 255, 255}},
                     (cui_rect){300, 200, 40, 40}, 20);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    assert(c.calls == before_modal && !find(draw, "Series"));
    assert(cui_begin_frame(ui, 400, 260, .016f));
    assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    draw = cui_end_frame(ui);
    assert(find(draw, "Series"));
    o.reduced_motion = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 200, .y = 200}));
    for (unsigned i = 0; i < 30; i++) {
        unsigned before = c.calls;
        assert(cui_begin_frame(ui, 400, 260, .016f));
        assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
        draw = cui_end_frame(ui);
        if (state.tooltip_focus > 0)
            assert(c.calls == before + 1 && find(draw, "Series"));
        else
            assert(c.calls == before && !find(draw, "Series"));
    }
    assert(state.tooltip_focus == 0 && cui_last_error(ui) == 0 && !cui_needs_redraw(ui));
    cui_destroy(ui);
}
typedef struct {
    cui_chart_kind kind;
    unsigned calls;
} metadata_context;
static bool metadata_build(const cui_chart_tooltip_info *info, cui_chart_tooltip_builder *b,
                           void *user) {
    metadata_context *m = user;
    m->calls++;
    assert(info->kind == m->kind && info->index == 0 && info->datum);
    if (m->kind == CUI_CHART_CANDLESTICK) {
        assert(info->value_count == 4 && info->values[0] == 10 && info->values[1] == 23 &&
               info->values[2] == 5 && info->values[3] == 20);
        assert(info->series_labels[0].length == 4 &&
               !memcmp(info->series_labels[0].data, "Open", 4));
        assert(info->series_labels[3].length == 5 &&
               !memcmp(info->series_labels[3].data, "Close", 5));
        assert(info->series_colors[0].g == 200);
    } else if (m->kind == CUI_CHART_LINE || m->kind == CUI_CHART_AREA ||
               m->kind == CUI_CHART_RADAR) {
        assert(info->value_count == 2 && info->values[0] == 20 && info->values[1] == 15);
        assert(info->series_labels[1].length == 5 &&
               !memcmp(info->series_labels[1].data, "Extra", 5));
        assert(info->series_colors[1].g == 180);
    } else {
        assert(info->value_count == 1 && info->values[0] == (m->kind == CUI_CHART_SANKEY ? 7 : 20));
        if (m->kind != CUI_CHART_SANKEY)
            assert(info->series_colors[0].r == 220);
    }
    assert(info->has_fallback_title == (m->kind != CUI_CHART_PIE && m->kind != CUI_CHART_SANKEY));
    char value[32];
    int n = snprintf(value, sizeof value, "%.0f", info->values[0]);
    return cui_chart_tooltip_add_row(
        b, &(cui_chart_tooltip_row){.label = {"Value", 5}, .value = {value, (size_t)n}});
}
static void metadata_values(void) {
    for (cui_chart_kind kind = CUI_CHART_LINE; kind <= CUI_CHART_SANKEY; kind++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 256,
                                                  .max_commands = 1024,
                                                  .frame_bytes = 65536,
                                                  .measure_styled = measure});
        assert(ui);
        unsigned char pixels[128 * 128 * 4];
        cui_chart_datum data[] = {{.label = {"First", 5},
                                   .value = 20,
                                   .open = 10,
                                   .high = 23,
                                   .low = 5,
                                   .close = 20,
                                   .color = {220, 0, 0, 255}},
                                  {.label = {"Second", 6}, .value = 40},
                                  {.label = {"Third", 5}, .value = 30}};
        double extra_values[] = {15, 25, 35};
        cui_chart_series extra = {
            .name = {"Extra", 5}, .values = extra_values, .count = 3, .color = {0, 180, 0, 255}};
        metadata_context metadata = {.kind = kind};
        cui_chart_options o = {.kind = kind,
                               .data = data,
                               .count = 3,
                               .width = 128,
                               .height = 128,
                               .disable_appear = true,
                               .reduced_motion = true,
                               .hide_axes = true,
                               .hide_legend = true,
                               .interaction_mode = CUI_PLOT_INTERACTION_LEGACY,
                               .tooltip_build = metadata_build,
                               .tooltip_build_user = &metadata,
                               .bullish = {0, 200, 0, 255},
                               .bearish = {200, 0, 0, 255}};
        if (kind == CUI_CHART_LINE || kind == CUI_CHART_AREA || kind == CUI_CHART_RADAR) {
            o.extra_series = &extra;
            o.extra_series_count = 1;
        }
        cui_chart_link link = {.source = 0, .target = 1, .value = 7};
        if (kind == CUI_CHART_SANKEY) {
            data[0].value = data[1].value = 0;
            o.count = 2;
            o.links = &link;
            o.link_count = 1;
            o.custom_sankey_margins = true;
        }
        cui_chart_state state = {
            .pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
        assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
        state.hovered = 0;
        state.hovered_value = kind == CUI_CHART_SANKEY ? 7 : 20;
        assert(cui_begin_frame(ui, 400, 260, .016f));
        assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
        cui_draw_list draw = cui_end_frame(ui);
        assert(metadata.calls == 1 && find(draw, "Value") && cui_last_error(ui) == 0);
        cui_destroy(ui);
    }
}
int main(void) {
    assert(!cui_chart_tooltip_set_title(NULL, cui_string("x")));
    assert(!cui_chart_tooltip_add_row(NULL, NULL));
    behavior(CUI_PLOT_INTERACTION_AUTO);
    behavior(CUI_PLOT_INTERACTION_CURRENT_FRAME);
    behavior(CUI_PLOT_INTERACTION_LEGACY);
    presence_and_fade();
    metadata_values();
    return 0;
}
