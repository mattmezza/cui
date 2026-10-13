/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static bool bad_measure;
static cui_size measure(void *u, cui_str text, float font, cui_text_style style) {
    (void)u;
    (void)style;
    /* Deliberately unrelated glyph height: inherited line boxes must prevail. */
    return (cui_size){bad_measure ? 30000 : (float)text.length * font * .5f, 3};
}
static unsigned calls;
static bool swatched = true;
static bool build(const cui_chart_tooltip_info *info, cui_chart_tooltip_builder *b, void *u) {
    (void)u;
    assert(info->index < 2);
    calls++;
    return cui_chart_tooltip_set_title(b, cui_string("")) &&
           cui_chart_tooltip_add_row(b, &(cui_chart_tooltip_row){.label = {"Series", 6},
                                                                 .value = {"20", 2},
                                                                 .has_swatch = swatched,
                                                                 .swatch = {220, 10, 20, 0}}) &&
           cui_chart_tooltip_add_row(
               b, &(cui_chart_tooltip_row){.label = {"Total", 5}, .value = {"1000", 4}});
}
static cui_str empty_title(const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    (void)u;
    return cui_string("");
}
static const cui_draw_command *text(cui_draw_list d, const char *s) {
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == strlen(s) &&
            !memcmp(d.commands[i].text.data, s, strlen(s)))
            return &d.commands[i];
    return NULL;
}
static const cui_draw_command *surface(cui_draw_list d) {
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_RECT && d.commands[i].color.r == 240 &&
            d.commands[i].color.g == 230)
            return &d.commands[i];
    return NULL;
}
static const cui_draw_command *ring(cui_draw_list d) {
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == CUI_DRAW_BORDER && d.commands[i].color.r == 250 &&
            d.commands[i].color.g == 0)
            return &d.commands[i];
    return NULL;
}
static void ring_geometry(cui_draw_list d, bool bare, unsigned alpha) {
    if (bare) {
        assert(!ring(d));
        return;
    }
    const cui_draw_command *r = ring(d), *box = surface(d);
    assert(r && box && r < box && r->bounds.x == box->bounds.x - 1 &&
           r->bounds.y == box->bounds.y - 1 && r->bounds.w == box->bounds.w + 2 &&
           r->bounds.h == box->bounds.h + 2 && r->stroke == 1 && r->color.a == alpha);
    assert((r->custom_corners ? r->corner_radii.top_left : r->radius) ==
           (box->custom_corners ? box->corner_radii.top_left : box->radius) + 1);
}
static cui_theme theme(void) {
    return (cui_theme){.radius = 10,
                       .popover = {240, 230, 210, 255},
                       .popover_foreground = {10, 30, 70, 255},
                       .muted = {100, 40, 120, 255},
                       .text = {250, 0, 0, 255}};
}
static cui_chart_tooltip_style style(void) {
    cui_chart_tooltip_style s;
    cui_theme t = theme();
    assert(cui_chart_tooltip_style_project(&t, 16, 20, &s));
    return s;
}
static void projection(void) {
    cui_theme t = theme();
    for (unsigned rem = 12; rem <= 20; rem += 4) {
        cui_chart_tooltip_style s;
        assert(cui_chart_tooltip_style_project(&t, (float)rem, 27, &s));
        assert(s.font_size == rem * .75f && s.line_height == 27 && s.padding == rem * .5f &&
               s.row_gap == rem * .25f && s.label_value_gap == rem * .75f &&
               s.swatch_gap == rem * .375f && s.swatch_size == rem * .5f && s.min_width == 150 &&
               s.cursor_gap == 8 && s.radius == 10 && s.swatch_radius == 5 &&
               s.foreground.b == 70 && s.background.r == 240 && s.muted_foreground.r == 100 &&
               s.ring_color.r == 250 && s.ring_color.g == 0 && s.ring_color.a == 26);
    }
    cui_chart_tooltip_style s = style(), before = s;
    float invalid[] = {NAN, INFINITY, -1, 0, 20000};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(!cui_chart_tooltip_style_project(&t, invalid[i], 20, &s));
        assert(s.font_size == before.font_size && s.foreground.b == before.foreground.b);
        assert(!cui_chart_tooltip_style_project(&t, 16, invalid[i], &s));
    }
    t.text.a = 0;
    assert(cui_chart_tooltip_style_project(&t, 16, 20, &s) && s.ring_color.a == 26);
    t.radius = NAN;
    assert(!cui_chart_tooltip_style_project(&t, 16, 20, &s));
}
static cui_chart_options options(cui_chart_datum *data, bool current, bool typed,
                                 cui_chart_tooltip_style *s) {
    return (cui_chart_options){.kind = CUI_CHART_LINE,
                               .data = data,
                               .count = 2,
                               .width = 128,
                               .height = 96,
                               .logical_width = 240,
                               .hide_axes = true,
                               .hide_x_axis = true,
                               .hide_legend = true,
                               .disable_appear = true,
                               .reduced_motion = true,
                               .interaction_mode = current ? CUI_PLOT_INTERACTION_CURRENT_FRAME
                                                           : CUI_PLOT_INTERACTION_LEGACY,
                               .tooltip_build = typed ? build : NULL,
                               .tooltip_title = empty_title,
                               .tooltip_style = s,
                               .background = {255, 255, 255, 255},
                               .foreground = {0, 0, 0, 255},
                               .grid = {120, 120, 120, 255},
                               .series = {{0, 120, 220, 255}}};
}
static cui_draw_list frame(cui_context *ui, cui_chart_state *state, cui_chart_options o, float left,
                           bool clip, bool destroy_style) {
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 500, 400, .016f));
    cui_begin_box(ui, 3, (cui_box){.width = 500, .height = 400});
    if (clip)
        cui_begin_box(ui, 4,
                      (cui_box){.width = 50,
                                .height = 50,
                                .fixed_width = true,
                                .fixed_height = true,
                                .clip = true,
                                .custom_opacity = true,
                                .opacity = .5f});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){left, 0, 240, 96});
    assert(cui_chart(ui, 1, state, &o, cui_string("Chart")));
    if (destroy_style)
        memset((void *)o.tooltip_style, 0x7f, sizeof *o.tooltip_style);
    memset(&o, 0, sizeof o);
    cui_end_box(ui);
    if (clip)
        cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return d;
}
static void behavior(bool current, bool typed, bool bare) {
    cui_context *ui = cui_create((cui_config){.max_elements = 512,
                                              .max_commands = 2048,
                                              .frame_bytes = 131072,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"Default", 7}, .value = 20},
                              {.label = {"Next", 4}, .value = 40}};
    cui_chart_state state = {.pixels = pixels,
                             .byte_capacity = sizeof pixels,
                             .hovered = 0,
                             .tooltip_cursor_valid = true,
                             .tooltip_cursor_x = 20,
                             .tooltip_cursor_y = 20};
    cui_chart_tooltip_style s = style();
    s.appearance = bare ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
    cui_chart_options o = options(data, current, typed, &s);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    state.hovered = 0;
    state.tooltip_cursor_valid = true;
    state.tooltip_cursor_x = state.tooltip_cursor_y = 20;
    if (current)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
    unsigned before = calls;
    cui_draw_list d = frame(ui, &state, o, 0, false, true);
    ring_geometry(d, bare, 26);
    assert(calls == before + (typed ? 1u : 0u));
    const cui_draw_command *label = text(d, typed ? "Series" : "Chart"),
                           *value = text(d, typed ? "1000" : "20");
    assert(label && value && label->font_size == 12 && label->bounds.h == 20 &&
           label->color.r == 100 && label->color.g == 40 && value->color.b == 70);
    if (bare) {
        assert(!surface(d));
        assert(label->bounds.x == 14 && label->bounds.y == 32.5f);
        assert(value->bounds.x + value->bounds.w == 240);
    } else {
        const cui_draw_command *box = surface(d);
        assert(box && box->bounds.x == 28 && box->bounds.y == 28);
        assert(box->bounds.h == (typed ? 84 : 60) &&
               (box->custom_corners ? box->corner_radii.top_left : box->radius) == 10);
        assert(label->bounds.x == 50 && label->bounds.y == 68.5f);
        assert(value->bounds.x + value->bounds.w == box->bounds.x + box->bounds.w - 8);
    }
    if (typed) {
        s = style();
        s.appearance = bare ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
        o.tooltip_style = &s;
        swatched = false;
        d = frame(ui, &state, o, 0, false, false);
        label = text(d, "Series");
        assert(label->bounds.x == (bare ? 0 : 36));
        swatched = true;
        s.swatch_size = 30;
        d = frame(ui, &state, o, 0, false, false);
        label = text(d, "Series");
        const cui_draw_command *second = text(d, "Total");
        assert(second->bounds.y - label->bounds.y == 34);
        if (!bare)
            assert(surface(d)->bounds.h == 104);
        s = style();
        s.appearance = bare ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
        s.row_gap = 0;
        s.label_value_gap = 0;
        s.swatch_gap = 0;
        d = frame(ui, &state, o, 0, false, false);
        label = text(d, "Series");
        second = text(d, "Total");
        assert(second->bounds.y - label->bounds.y == 20);
        if (!bare)
            assert(surface(d)->bounds.h == 76);
    }
    /* Relocate owner/options and destroy original values before another frame. */
    cui_chart_state copied = state;
    memset(&state, 0, sizeof state);
    s = style();
    s.appearance = bare ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
    cui_chart_options copied_options = o;
    copied_options.tooltip_style = &s;
    memset(&o, 0, sizeof o);
    d = frame(ui, &copied, copied_options, 10, true, false);
    label = text(d, typed ? "Series" : "Chart");
    assert(label && label->color.a == 127);
    assert(copied.bounds.x == 10);
    ring_geometry(d, bare, 13);
    if (bare)
        assert(label->bounds.x == (current ? 24 : 14));
    else
        assert(surface(d)->bounds.x == 28);
    /* Stationary pointer moved relative to current bounds; local anchor changes. */
    if (current) {
        s = style();
        copied_options.tooltip_style = &s;
        d = frame(ui, &copied, copied_options, 0, false, false);
        assert(surface(d)->bounds.x == 28);
    }
    if (!bare) {
        s = style();
        copied_options.tooltip_style = &s;
        if (current)
            assert(cui_queue_event(ui,
                                   (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 200, .y = 80}));
        else {
            copied.hovered = 1;
            copied.tooltip_cursor_x = 200;
            copied.tooltip_cursor_y = 80;
        }
        d = frame(ui, &copied, copied_options, 0, false, false);
        const cui_draw_command *box = surface(d);
        assert(box && box->bounds.x == (current ? 42 : 52) && box->bounds.y == (typed ? -12 : 12));
    }
    cui_destroy(ui);
}
static void invalid(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 1024,
                                              .frame_bytes = 65536,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.value = 20}, {.value = 40}};
    for (unsigned i = 0; i < 8; i++)
        for (unsigned current = 0; current < 2; current++) {
            cui_chart_tooltip_style s = style();
            if (i == 0)
                s.appearance = (cui_chart_tooltip_appearance)2;
            if (i == 1)
                s.padding = -1;
            if (i == 2)
                s.row_gap = NAN;
            if (i == 3)
                s.font_size = 0;
            if (i == 4)
                s.line_height = INFINITY;
            if (i == 5)
                s.swatch_radius = 20000;
            if (i == 6)
                s.padding = 10000;
            if (i == 7)
                s.swatch_gap = -1;
            cui_chart_state state = {.pixels = pixels,
                                     .byte_capacity = sizeof pixels,
                                     .hovered = SIZE_MAX,
                                     .revision = 17};
            cui_chart_options o = options(data, current, true, &s);
            assert(cui_begin_frame(ui, 500, 400, .016f));
            assert(!cui_chart(ui, 1, &state, &o, cui_string("Chart")));
            cui_draw_list d = cui_end_frame(ui);
            assert(!d.count && state.revision == 17 && state.hovered == SIZE_MAX &&
                   cui_last_error(ui) == CUI_OK);
        }
    cui_destroy(ui);
}
static void measured_failure(bool current, bool bare_overflow) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256,
                                              .max_commands = 2048,
                                              .frame_bytes = 131072,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.value = 20}, {.value = 40}};
    cui_chart_tooltip_style s = style();
    if (bare_overflow) {
        s.appearance = CUI_CHART_TOOLTIP_BARE;
        s.font_size = 100;
    }
    cui_chart_options o = options(data, current, true, &s);
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    state.hovered = 0;
    state.tooltip_cursor_valid = true;
    state.tooltip_cursor_x = state.tooltip_cursor_y = 20;
    cui_chart_state before = state;
    if (current)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
    bad_measure = !bare_overflow;
    assert(cui_begin_frame(ui, 500, 400, .016f));
    bool ok = cui_chart(ui, 1, &state, &o, cui_string("Chart"));
    cui_draw_list d = cui_end_frame(ui);
    bad_measure = false;
    assert(ok == current && (current || !d.count) && !text(d, "Series") && !surface(d) &&
           state.revision == before.revision && state.hovered == before.hovered &&
           state.tooltip_focus == before.tooltip_focus);
    for (size_t i = 0; i < d.count; i++)
        assert(d.commands[i].type == CUI_DRAW_CLIP_PUSH || d.commands[i].type == CUI_DRAW_CLIP_POP);
    assert(cui_last_error(ui) == (current ? CUI_ERROR_LAYOUT : CUI_OK));
    cui_destroy(ui);
}
static void fade(bool bare) {
    cui_context *ui = cui_create((cui_config){.max_elements = 512,
                                              .max_commands = 2048,
                                              .frame_bytes = 131072,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.value = 20}, {.value = 40}};
    cui_chart_tooltip_style s = style();
    s.appearance = bare ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
    cui_chart_options o = options(data, true, true, &s);
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    unsigned before = calls;
    cui_draw_list d = frame(ui, &state, o, 0, false, false);
    assert(calls == before && !text(d, "Series"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
    d = frame(ui, &state, o, 0, false, false);
    assert(text(d, "Series"));
    o.reduced_motion = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 400, .y = 300}));
    bool retained = false;
    for (unsigned i = 0; i < 30; i++) {
        d = frame(ui, &state, o, 0, false, false);
        if (state.tooltip_focus > 0) {
            retained = true;
            assert(text(d, "Series"));
        } else
            assert(!text(d, "Series"));
    }
    assert(retained && state.tooltip_focus == 0 && !cui_needs_redraw(ui));
    cui_destroy(ui);
}
static unsigned custom_calls;
static cui_size custom_measure(cui_context *ui, size_t index, const cui_chart_datum *datum,
                               void *u) {
    (void)ui;
    (void)index;
    (void)datum;
    return *(cui_size *)u;
}
static void custom_content(cui_context *ui, cui_id id, size_t index, const cui_chart_datum *datum,
                           void *u) {
    (void)id;
    (void)index;
    (void)datum;
    (void)u;
    custom_calls++;
    cui_text_with_line_height(ui, cui_string("Host"), 12, (cui_color){10, 30, 70, 255}, 20);
}
static void custom(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256,
                                              .max_commands = 1024,
                                              .frame_bytes = 65536,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.value = 20}, {.value = 40}};
    cui_chart_tooltip_style s = style();
    cui_chart_options o = options(data, false, false, &s);
    cui_size size = {42, 25};
    o.tooltip_content = custom_content;
    o.tooltip_measure = custom_measure;
    o.tooltip_user = &size;
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    state.hovered = 0;
    state.tooltip_cursor_valid = true;
    state.tooltip_cursor_x = state.tooltip_cursor_y = 20;
    cui_draw_list d = frame(ui, &state, o, 0, false, false);
    assert(custom_calls == 1 && text(d, "Host") && surface(d)->bounds.h == 41);
    s.appearance = CUI_CHART_TOOLTIP_BARE;
    d = frame(ui, &state, o, 0, false, false);
    assert(custom_calls == 2 && text(d, "Host") && !surface(d));
    size.width = NAN;
    cui_chart_state before = state;
    assert(cui_begin_frame(ui, 500, 400, .016f));
    assert(!cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    d = cui_end_frame(ui);
    assert(!d.count && custom_calls == 2 && state.revision == before.revision &&
           state.tooltip_focus == before.tooltip_focus && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void automatic_values(void) {
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
        cui_chart_tooltip_style projected = style();
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
                               .tooltip_style = &projected,
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
        const cui_draw_command *value = text(draw, kind == CUI_CHART_PIE      ? "20 (22.2%)"
                                                   : kind == CUI_CHART_SANKEY ? "7"
                                                                              : "20");
        assert(value && value->font_size == 12 && value->color.b == 70 &&
               cui_last_error(ui) == CUI_OK);
        if (kind == CUI_CHART_CANDLESTICK)
            assert(text(draw, "Open") && text(draw, "Close") && text(draw, "23") &&
                   text(draw, "5"));
        if (kind == CUI_CHART_LINE || kind == CUI_CHART_AREA || kind == CUI_CHART_RADAR)
            assert(text(draw, "Extra") && text(draw, "15"));
        assert(surface(draw));
        cui_destroy(ui);
    }
}
int main(void) {
    projection();
    automatic_values();
    custom();
    fade(false);
    fade(true);
    invalid();
    measured_failure(false, false);
    measured_failure(true, false);
    measured_failure(false, true);
    measured_failure(true, true);
    for (unsigned c = 0; c < 2; c++)
        for (unsigned t = 0; t < 2; t++)
            for (unsigned b = 0; b < 2; b++)
                behavior(c, t, b);
    return 0;
}
