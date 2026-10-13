/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    unsigned measured, painted, titled, content;
} calls;
static cui_size measure(void *u, cui_str s, float f, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){s.length * f * .5f, f};
}
static cui_size label_measure(cui_context *ui, const cui_chart_datum *d, size_t i, void *u) {
    (void)ui;
    (void)d;
    (void)i;
    ((calls *)u)->measured++;
    return (cui_size){20, 10};
}
static void label_paint(cui_context *ui, cui_id id, const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    (void)id;
    ((calls *)u)->painted++;
    cui_text(ui, cui_string("Label"), 10, (cui_color){0, 0, 0, 255});
}
static cui_str title(const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    ((calls *)u)->titled++;
    return cui_string("Override");
}
static cui_str empty_title(const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    ((calls *)u)->titled++;
    return (cui_str){0};
}
static void content(cui_context *ui, cui_id id, size_t i, const cui_chart_datum *d, void *u) {
    (void)i;
    (void)d;
    (void)id;
    ((calls *)u)->content++;
    cui_text(ui, cui_string("Content"), 10, (cui_color){0, 0, 0, 255});
}
static bool text(cui_draw_list draw, const char *s, bool title_only) {
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *c = &draw.commands[i];
        if (c->type == CUI_DRAW_TEXT &&
            (!title_only || c->text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD) &&
            c->text.length == strlen(s) && !memcmp(c->text.data, s, c->text.length))
            return true;
    }
    return false;
}
static cui_draw_list frame(cui_context *ui, cui_chart_state *state, cui_chart_options *o) {
    assert(cui_chart_render(state, o) == CUI_PLOT_OK);
    state->hovered = 0;
    if (o->interaction_mode == CUI_PLOT_INTERACTION_CURRENT_FRAME)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 64, .y = 20}));
    assert(cui_begin_frame(ui, 240, 220, .016f));
    assert(cui_chart(ui, 1, state, o, cui_string("Radar")));
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == 0);
    return draw;
}
static void behavior(cui_plot_interaction_mode interaction) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 1024,
                                              .frame_bytes = 65536,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 128 * 4];
    cui_chart_datum data[] = {{.label = {"Top", 3}, .value = 20},
                              {.label = {"Side", 4}, .value = 40},
                              {.label = {"End", 3}, .value = 30}};
    calls c = {0};
    cui_chart_options o = {.kind = CUI_CHART_RADAR,
                           .data = data,
                           .count = 3,
                           .width = 128,
                           .height = 128,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .interaction_mode = interaction,
                           .radar_label_measure = label_measure,
                           .radar_label_paint = label_paint,
                           .radar_label_user = &c,
                           .tooltip_user = &c};
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = 0};
    cui_draw_list draw = frame(ui, &state, &o);
    assert(text(draw, "Top", true) && text(draw, "20", false));
    assert(c.measured == 3 && c.painted == 3);
    o.radar_tooltip_title_mode = CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL;
    draw = frame(ui, &state, &o);
    assert(!text(draw, "Top", true) && text(draw, "20", false));
    assert(c.measured == 6 && c.painted == 6);
    o.tooltip_title = title;
    draw = frame(ui, &state, &o);
    assert(text(draw, "Override", true) && c.titled == 1);
    o.tooltip_title = empty_title;
    draw = frame(ui, &state, &o);
    assert(!text(draw, "Top", true) && !text(draw, "Override", true) && c.titled == 2);
    o.tooltip_title = NULL;
    o.radar_label_paint = NULL;
    draw = frame(ui, &state, &o);
    assert(text(draw, "Top", true));
    o.radar_label_measure = NULL;
    draw = frame(ui, &state, &o);
    assert(text(draw, "Top", true));
    if (interaction != CUI_PLOT_INTERACTION_CURRENT_FRAME) {
        o.tooltip_title = title;
        o.tooltip_content = content;
        draw = frame(ui, &state, &o);
        assert(text(draw, "Content", false) && !text(draw, "Override", true));
        assert(c.content == 1 && c.titled == 2);
    }
    o.tooltip_content = NULL;
    o.tooltip_title = NULL;
    o.radar_label_measure = label_measure;
    o.radar_label_paint = label_paint;
    draw = frame(ui, &state, &o);
    assert(!text(draw, "Top", true) && text(draw, "20", false));
    o.reduced_motion = false;
    if (interaction == CUI_PLOT_INTERACTION_CURRENT_FRAME)
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 200, .y = 200}));
    else
        assert(!cui_chart_update_hover(&state, &o, NAN, NAN).valid);
    for (unsigned step = 0; step < 30; step++) {
        assert(cui_begin_frame(ui, 240, 220, .016f));
        assert(cui_chart(ui, 1, &state, &o, cui_string("Radar")));
        draw = cui_end_frame(ui);
        assert(!text(draw, "Top", true));
        if (!step)
            assert(text(draw, "20", false));
    }
    assert(state.tooltip_focus == 0 && !text(draw, "20", false));
    o.reduced_motion = true;
    o.radar_label_measure = NULL;
    o.radar_label_paint = NULL;
    cui_chart_options copy = o;
    cui_chart_state copied = state;
    draw = frame(ui, &copied, &copy);
    assert(text(draw, "Top", true));
    assert(cui_begin_frame(ui, 240, 220, .016f));
    uint64_t revision = copied.revision, cache = copied.cache_key;
    bool appeared = copied.appear_started;
    unsigned char before[sizeof pixels];
    memcpy(before, pixels, sizeof pixels);
    copy.radar_tooltip_title_mode = (cui_chart_radar_tooltip_title_mode)99;
    assert(!cui_chart(ui, 1, &copied, &copy, cui_string("Radar")) && copied.revision == revision);
    assert(cui_chart_render(&copied, &copy) == CUI_PLOT_INVALID);
    copy.radar_tooltip_title_mode = CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL;
    copy.kind = CUI_CHART_LINE;
    assert(!cui_chart(ui, 1, &copied, &copy, cui_string("Radar")) && copied.revision == revision);
    assert(cui_chart_render_animated(ui, &copied, &copy) == CUI_PLOT_INVALID);
    assert(cui_end_frame(ui).count == 0);
    assert(copied.cache_key == cache && copied.appear_started == appeared);
    assert(!memcmp(before, pixels, sizeof pixels));
    copy.kind = CUI_CHART_RADAR;
    copy.radar_tooltip_title_mode = CUI_CHART_RADAR_TOOLTIP_TITLE_LEGACY;
    copy.noninteractive = true;
    copy.data_revision = 1;
    assert(cui_chart_render(&copied, &copy) == CUI_PLOT_OK);
    cache = copied.cache_key;
    revision = copied.revision;
    copy.radar_tooltip_title_mode = CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL;
    assert(cui_chart_render(&copied, &copy) == CUI_PLOT_OK);
    assert(copied.cache_key == cache && copied.revision == revision);
    cui_destroy(ui);
}
int main(void) {
    behavior(CUI_PLOT_INTERACTION_LEGACY);
    behavior(CUI_PLOT_INTERACTION_CURRENT_FRAME);
    return 0;
}
