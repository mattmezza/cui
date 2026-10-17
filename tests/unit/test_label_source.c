/* SPDX-License-Identifier: MIT */
#include "cui/cui_label.h"
#include <assert.h>
#include <string.h>
static cui_theme theme;
static char text[256];
static cui_text_color_span spans[64];
static cui_label_prepared prepared;
static cui_label_options options(const char *primary, const char *secondary, const char *match) {
    return (cui_label_options){.primary = cui_string(primary),
                               .secondary = cui_string(secondary),
                               .has_secondary = true,
                               .highlight = cui_string(match)};
}
static void prepare(cui_label_options o) {
    assert(cui_label_prepare(&theme, o, text, sizeof(text), spans, 64, &prepared));
}
static cui_color color_at(size_t byte) {
    for (size_t i = 0; i < prepared.span_count; ++i)
        if (byte >= spans[i].byte_start && byte < spans[i].byte_end)
            return spans[i].color;
    assert(0);
    return (cui_color){0};
}
static cui_size scalar_measure(void *user, cui_str value, float size, cui_text_style style) {
    (void)user;
    (void)size;
    (void)style;
    size_t n = 0;
    for (size_t i = 0; i < value.length; ++i)
        n += ((unsigned char)value.data[i] & 0xc0u) != 0x80u;
    return (cui_size){(float)n * 10, 12};
}
static void equal_color(cui_color actual, cui_color expected) {
    assert(actual.r == expected.r && actual.g == expected.g && actual.b == expected.b &&
           actual.a == expected.a);
}
static void check_prepared_draw(const cui_draw_command *draw) {
    assert(draw->type == CUI_DRAW_TEXT && draw->text.length == prepared.text_length);
    assert(draw->text_source.paragraph.length == prepared.text_length);
    assert(draw->text_source.byte_start == 0 && draw->text_source.byte_end == prepared.text_length);
    assert(!memcmp(draw->text.data, text, prepared.text_length));
    assert(!memcmp(draw->text_source.paragraph.data, text, prepared.text_length));
    assert(draw->text_color_count == prepared.span_count);
    for (size_t i = 0; i < prepared.span_count; ++i) {
        assert(draw->text_colors[i].byte_start == spans[i].byte_start);
        assert(draw->text_colors[i].byte_end == spans[i].byte_end);
        equal_color(draw->text_colors[i].color, spans[i].color);
    }
    assert(draw->font_size == theme.font_size && draw->bounds.h == 20);
}
static void caller_foreground(void) {
    cui_context *ui = cui_create(
        (cui_config){.measure_styled = scalar_measure, .max_elements = 64, .frame_bytes = 8192});
    assert(ui);
    cui_label_options o = {.primary = cui_string("Failure"), .color = theme.danger};
    prepare(o);
    assert(prepared.span_count == 1 && spans[0].byte_start == 0 && spans[0].byte_end == 7);
    equal_color(color_at(0), theme.text); /* A color value alone does not override defaults. */
    assert(cui_begin_frame(ui, 400, 200, 0) && cui_label_source(ui, &theme, o));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 1);
    check_prepared_draw(&list.commands[0]);
    equal_color(list.commands[0].color, theme.text);
    o = options("Échec", "écho", "");
    prepare(o);
    assert(prepared.span_count == 2 && spans[0].byte_end == 6 && spans[1].byte_start == 6);
    equal_color(color_at(0), theme.text);
    equal_color(color_at(6), theme.muted); /* Separator is secondary styling. */
    assert(cui_begin_frame(ui, 400, 200, 0) && cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 1);
    check_prepared_draw(&list.commands[0]);
    o.custom_color = true;
    o.color = theme.danger; /* Caller-selected error presentation, no validation state. */
    o.highlight = cui_string("ec é");
    prepare(o);
    assert(prepared.text_length == 12 && prepared.span_count == 3);
    assert(spans[0].byte_start == 0 && spans[0].byte_end == 4);
    assert(spans[1].byte_start == 4 && spans[1].byte_end == 9);
    assert(spans[2].byte_start == 9 && spans[2].byte_end == 12);
    equal_color(color_at(0), theme.danger);
    equal_color(color_at(4), theme.info);
    equal_color(color_at(6), theme.info); /* Highlight crosses primary, space, secondary. */
    equal_color(color_at(7), theme.info);
    equal_color(color_at(9), theme.muted);
    assert(cui_begin_frame(ui, 400, 200, 0) && cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 1);
    check_prepared_draw(&list.commands[0]);
    equal_color(list.commands[0].color, theme.danger);
    o = options("É\n秘密", "token", "秘密 token");
    o.custom_color = true;
    o.color = theme.danger;
    o.masked = true;
    o.semantic_id = 777;
    prepare(o);
    assert(prepared.text_length == 30 && prepared.span_count == 0);
    assert(!memcmp(text, "••••••••••", 30));
    assert(cui_begin_frame(ui, 400, 200, 0) && cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 1);
    check_prepared_draw(&list.commands[0]);
    equal_color(list.commands[0].color, theme.danger);
    assert(list.commands[0].bounds.w == 100 && cui_bounds(ui, 777).h == 20);
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].name.length == 30 && semantics[0].details.secret);
    assert(!memcmp(semantics[0].name.data, "••••••••••", 30));
    cui_destroy(ui);
}
static void wrapping(void) {
    cui_context *ui = cui_create(
        (cui_config){.measure_styled = scalar_measure, .max_elements = 128, .frame_bytes = 16384});
    assert(ui && cui_begin_frame(ui, 200, 300, 0));
    cui_begin_box(ui, 100, (cui_box){.width = 40});
    cui_label_options o = options("Éé", "aa bb", "é aa b");
    o.semantic_id = 101;
    assert(cui_label_source(ui, &theme, o));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 3 && cui_bounds(ui, 100).h == 60);
    assert(list.commands[0].text.length == 5 && !memcmp(list.commands[0].text.data, "Éé ", 5));
    assert(list.commands[0].text_color_count == 2);
    assert(list.commands[0].text_colors[0].color.r == 1 &&
           list.commands[0].text_colors[1].color.r == 3);
    assert(list.commands[1].text_source.byte_start == 5 &&
           list.commands[1].text_colors[0].color.r == 3);
    assert(list.commands[2].text_source.byte_start == 8 && list.commands[2].text_color_count == 2);
    assert(list.commands[2].text_colors[0].byte_end == 9 &&
           list.commands[2].text_colors[1].color.r == 2);
    size_t count = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].bounds.h == 60 && sem[0].name.length == 10);
    assert(cui_begin_frame(ui, 200, 300, 0));
    o = options("É\n🙂", "世界", "é\n🙂 世");
    o.semantic_id = 101;
    o.line_height = 24;
    assert(cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 2 && cui_bounds(ui, 101).h == 48);
    assert(list.commands[0].text_source.paragraph.length == 2);
    assert(list.commands[0].text_colors[0].color.r == 3);
    assert(list.commands[1].text_source.paragraph.length == 11);
    assert(list.commands[1].text_colors[0].byte_start == 0 &&
           list.commands[1].text_colors[0].byte_end == 8);
    assert(list.commands[1].text_colors[0].color.r == 3 &&
           list.commands[1].text_colors[1].color.r == 2);
    /* Newlines are masked as scalars, so secret paragraph structure never escapes. */
    assert(cui_begin_frame(ui, 200, 300, 0));
    o.masked = true;
    o.no_wrap = true;
    assert(cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 18);
    assert(!memcmp(list.commands[0].text.data, "••••••", 18));
    assert(list.commands[0].text_color_count == 0 &&
           list.commands[0].text_source.paragraph.length == 18);
    sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].name.length == 18 && !memcmp(sem[0].name.data, "••••••", 18));
    assert(cui_begin_frame(ui, 200, 300, 0));
    cui_begin_box(ui, 100, (cui_box){.width = 40});
    o = options("AA BB", "CC", "BB CC");
    o.no_wrap = true;
    assert(cui_label_source(ui, &theme, o));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 8);
    cui_destroy(ui);
}
int main(void) {
    theme = (cui_theme){.text = {1, 0, 0, 255},
                        .muted = {2, 0, 0, 255},
                        .info = {3, 0, 0, 255},
                        .danger = {201, 31, 47, 173},
                        .font_size = 16};
    cui_label_options o = options("É🙂", "世界 É", "é");
    prepare(o);
    assert(prepared.text_length == strlen("É🙂 世界 É"));
    assert(color_at(0).r == 3 && color_at(2).r == 1 && color_at(6).r == 2 && color_at(14).r == 3);
    o = options("İX É", "É", "é");
    prepare(o);
    assert(color_at(4).r == 3 && color_at(7).r == 3); /* expansion before match */
    o = options("İX", "", "i\xcc\x87");
    o.match = CUI_LABEL_MATCH_PREFIX;
    prepare(o);
    assert(color_at(0).r == 3 && color_at(2).r == 1);
    o = options("ΟΣ", "", "ος");
    prepare(o);
    assert(color_at(0).r == 3); /* final sigma */
    o = options("ΟΣΑ", "", "οσα");
    prepare(o);
    assert(color_at(0).r == 3);
    o = options("𐐀", "𐐀", "𐐨");
    prepare(o);
    assert(color_at(0).r == 3 && color_at(5).r == 3);
    o = options("ÉÉ", "", "é");
    o.match = CUI_LABEL_MATCH_PREFIX;
    prepare(o);
    assert(color_at(0).r == 3 && color_at(2).r == 1);
    o = options("É", "", "e");
    prepare(o);
    assert(color_at(0).r == 1); /* no normalization */
    o = options("", "", "");
    prepare(o);
    assert(prepared.text_length == 1 && color_at(0).r == 2);
    o = options("aaa", "", "aa");
    prepare(o);
    assert(color_at(2).r == 3); /* overlapping */
    o = options("é🙂", "世界", "🙂 世");
    o.masked = true;
    prepare(o);
    assert(prepared.text_length == 15 && prepared.span_count == 0);
    assert(!memcmp(text, "•••••", 15));
    cui_label_prepared before = prepared;
    memset(text, 42, sizeof(text));
    memset(spans, 42, sizeof(spans));
    assert(!cui_label_prepare(&theme, o, text, 14, spans, 64, &prepared));
    assert(!memcmp(&before, &prepared, sizeof(before)) && text[0] == 42);
    o.masked = false;
    o.primary = cui_string("\xc0\xaf");
    assert(!cui_label_prepare(&theme, o, text, sizeof(text), spans, 64, &prepared));
    assert(text[0] == 42 && ((unsigned char *)spans)[0] == 42);
    o = options("A", "B", "a");
    assert(!cui_label_prepare(&theme, o, text, sizeof(text), spans, 0, &prepared));
    assert(text[0] == 42);
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .frame_bytes = 8192});
    assert(ui);
    assert(cui_begin_frame(ui, 400, 200, 0));
    o = options("é🙂", "世界", "🙂 世");
    o.masked = true;
    o.semantic_id = 42;
    o.control = 43;
    o.style = (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD, .italic = true, .family = 7};
    o.line_height = 28;
    assert(cui_label_source(ui, &theme, o));
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 1);
    assert(list.commands[0].text.length == 15 && !memcmp(list.commands[0].text.data, "•••••", 15));
    assert(!memcmp(list.commands[0].text_source.paragraph.data, "•••••", 15));
    assert(list.commands[0].text_style.weight == CUI_TEXT_WEIGHT_BOLD &&
           list.commands[0].text_style.italic);
    assert(list.commands[0].text_style.family == 7 && list.commands[0].bounds.h == 28);
    size_t n = 0;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1);
    assert(sem[0].name.length == 15 && !memcmp(sem[0].name.data, "•••••", 15));
    assert(sem[0].details.controls == 43 && sem[0].details.focus_target == 43 &&
           sem[0].details.secret);
    assert(cui_begin_frame(ui, 400, 200, 0));
    o = options("Primary", "Secondary", "ary");
    assert(cui_label_source(ui, &theme, o));
    list = cui_end_frame(ui);
    assert(list.count == 1);
    assert(list.commands[0].bounds.h == 20);
    cui_destroy(ui);
    wrapping();
    caller_foreground();
    return 0;
}
