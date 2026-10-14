/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_unicode_grapheme.h"
#include "cui/cui.h"
#include <string.h>
extern float canvas_width(const char *, size_t, float)
    __attribute__((import_module("env"), import_name("canvas_width")));
static cui_context *ui;
static cui_draw_list draw;
static const char *const cases[] = {
    "SupercalifragilisticexpialidociousWithoutAnySpacesAtAll",
    "你好世界中文换行测试日本語かなカナ",
    "👩🏽‍🚀👨‍👩‍👧‍👦🇨🇭äé👩🏽‍🚀🇨🇭",
    "Alpha/beta!gamma—delta（日本語）終わり",
    "  alpha beta gamma    ",
    "äää🇨🇭🇨🇭👨‍👩‍👧‍👦"};
static cui_str text;
static size_t split;
static cui_size measured(void *user, cui_str value, float size, cui_text_style style) {
    (void)user;
    (void)style;
    return (cui_size){canvas_width(value.data, value.length, size), size * 1.25f};
}
int probe_init(void) {
    ui = cui_create((cui_config){.measure_styled = measured,
                                 .max_elements = 1024,
                                 .max_commands = 1024,
                                 .frame_bytes = 262144});
    return ui ? 0 : 1;
}
int probe_frame(unsigned sample, float width) {
    if (sample >= sizeof(cases) / sizeof(cases[0]) || !ui)
        return 1;
    text = cui_string(cases[sample]);
    split = 0;
    while (split < text.length / 2)
        split = cui_core_grapheme_next_valid(text, split);
    cui_text_color_span spans[] = {{0, split, {200, 10, 20, 255}},
                                   {split, text.length, {10, 20, 200, 255}}};
    if (!cui_begin_frame(ui, width, 2000, 0))
        return 2;
    cui_begin_box(ui, 1, (cui_box){.width = -1, .padding = 4});
    cui_begin_box(ui, 2, (cui_box){.width = -1});
    bool ok = cui_text_paragraph_colored(ui, text, 16, (cui_color){0}, 20, (cui_text_style){0},
                                         true, spans, 2);
    cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return !ok || cui_last_error(ui) != CUI_OK ? 3 : 0;
}
unsigned probe_count(void) {
    return (unsigned)draw.count;
}
const char *probe_text(void) {
    return text.data;
}
unsigned probe_length(void) {
    return (unsigned)text.length;
}
float probe_width(void) {
    return cui_bounds(ui, 2).w;
}
unsigned probe_split(void) {
    return (unsigned)split;
}
static const cui_draw_command *row(unsigned index) {
    return index < draw.count ? &draw.commands[index] : NULL;
}
unsigned probe_boundary(unsigned offset) {
    size_t at = 0;
    while (at < offset && at < text.length)
        at = cui_core_grapheme_next_valid(text, at);
    return at == offset;
}
float probe_row(unsigned index, unsigned field) {
    const cui_draw_command *r = row(index);
    if (!r)
        return -1;
    switch (field) {
    case 0:
        return (float)r->text_source.byte_start;
    case 1:
        return (float)r->text_source.byte_end;
    case 2:
        return r->bounds.x;
    case 3:
        return r->bounds.y;
    case 4:
        return r->bounds.w;
    case 5:
        return r->bounds.h;
    case 6:
        return (float)r->text_color_count;
    case 7:
        return (float)r->text_source.paragraph.length;
    case 8:
        return (float)r->text.length;
    case 9:
        return (float)r->type;
    default:
        return -1;
    }
}
const char *probe_row_text(unsigned index) {
    const cui_draw_command *r = row(index);
    return r ? r->text.data : NULL;
}
const char *probe_row_paragraph(unsigned index) {
    const cui_draw_command *r = row(index);
    return r ? r->text_source.paragraph.data : NULL;
}
float probe_span(unsigned index, unsigned span, unsigned field) {
    const cui_draw_command *r = row(index);
    if (!r || span >= r->text_color_count)
        return -1;
    const cui_text_color_span *s = &r->text_colors[span];
    return field == 0 ? (float)s->byte_start : field == 1 ? (float)s->byte_end : (float)s->color.r;
}
