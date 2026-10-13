/* SPDX-License-Identifier: MIT */
#include "../../src/base/cui_textarea_geometry.h"
#include "cui/cui.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned created, released;
static cui_text_layout_status carets(const void *p, size_t byte, cui_text_caret *out, size_t cap,
                                     size_t *n) {
    const cui_text_line_source *source = p;
    if (byte < source->byte_start || byte > source->byte_end || !n)
        return CUI_TEXT_LAYOUT_INVALID;
    if (cap < 1)
        return CUI_TEXT_LAYOUT_CAPACITY;
    *out =
        (cui_text_caret){.byte_offset = byte, .x = (float)(source->byte_end - byte), .height = 10};
    *n = 1;
    return CUI_TEXT_LAYOUT_OK;
}
static cui_text_layout_status hit(const void *p, float x, float y, cui_text_affinity a,
                                  cui_text_caret *out) {
    (void)y;
    (void)a;
    const cui_text_line_source *s = p;
    size_t byte = x <= 0 ? s->byte_end : s->byte_start;
    *out = (cui_text_caret){.byte_offset = byte};
    return CUI_TEXT_LAYOUT_OK;
}
static cui_text_layout_status selection(const void *p, size_t a, size_t b,
                                        cui_text_selection_rect *o, size_t cap, size_t *n) {
    (void)p;
    (void)a;
    (void)b;
    (void)o;
    (void)cap;
    (void)n;
    return CUI_TEXT_LAYOUT_UNAVAILABLE;
}
static void release(void *p) {
    free(p);
    released++;
}
static cui_text_layout_status factory(void *u, cui_text_line_source s, float size,
                                      cui_text_style style, cui_text_geometry *out) {
    (void)u;
    (void)size;
    (void)style;
    static const cui_text_geometry_ops ops = {carets, hit, selection, release};
    cui_text_line_source *copy = malloc(sizeof *copy);
    assert(copy);
    *copy = s;
    *out =
        (cui_text_geometry){copy, &ops, (float)(s.byte_end - s.byte_start), 10, s.paragraph.length};
    created++;
    return CUI_TEXT_LAYOUT_OK;
}
static void descriptor(void) {
    cui_context *ui = cui_create((cui_config){.frame_bytes = 64, .max_elements = 8});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 200, 0));
    char paragraph[] = "אבג 123 !";
    cui_text_line_source source = {cui_string(paragraph), 7, 12};
    assert(cui_text_line_source_valid(source));
    assert(!cui_text_context_line(ui, (cui_text_line_source){source.paragraph, 1, 12}, 14,
                                  (cui_color){0}, 20, (cui_text_style){0}));
    assert(!cui_text_line_source_valid((cui_text_line_source){cui_string("\xed\xa0\x80"), 0, 3}));
    assert(!cui_text_line_source_valid((cui_text_line_source){cui_string("a\r\nb"), 0, 4}));
    assert(cui_text_line_source_valid((cui_text_line_source){cui_string("a\xe2\x80\xa8"
                                                                        "b"),
                                                             4, 5}));
    assert(cui_text_context_line(ui, source, 14, (cui_color){0}, 20, (cui_text_style){0}));
    assert(cui_text_context_line(ui, source, 14, (cui_color){0}, 20, (cui_text_style){0}));
    memset(paragraph, 'x', sizeof paragraph - 1);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count == 2);
    for (size_t i = 0; i < 2; i++) {
        assert(list.commands[i].text_source.byte_start == 7 &&
               list.commands[i].text_source.byte_end == 12);
        assert(!memcmp(list.commands[i].text_source.paragraph.data, "אבג 123 !", 12));
        assert(!memcmp(list.commands[i].text.data, "123 !", 5));
    }
    assert(list.commands[0].text_source.paragraph.data ==
           list.commands[1].text_source.paragraph.data);
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_text_line_source first = {cui_string("abcdef"), 0, 2}, second = {first.paragraph, 0, 4};
    assert(cui_text_context_line(ui, first, 14, (cui_color){0}, 20, (cui_text_style){0}));
    assert(cui_text_context_line(ui, second, 14, (cui_color){0}, 20, (cui_text_style){0}));
    list = cui_end_frame(ui);
    assert(list.count == 2);
    assert(list.commands[0].text_source.byte_end == 2 &&
           list.commands[1].text_source.byte_end == 4);
    assert(list.commands[0].text.data != list.commands[1].text.data);
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_text(ui, cui_string("ab\r\nab"), 14, (cui_color){0});
    list = cui_end_frame(ui);
    assert(list.count == 2);
    for (size_t i = 0; i < 2; i++)
        assert(list.commands[i].text_source.paragraph.length == 2 &&
               list.commands[i].text_source.byte_start == 0 &&
               list.commands[i].text_source.byte_end == 2);
    assert(cui_begin_frame(ui, 200, 200, 0));
    cui_text_color_span colors[] = {{0, 1, {1, 2, 3, 255}}, {1, 2, {4, 5, 6, 255}}};
    cui_text_line_source colored = {cui_string("ok"), 0, 2};
    assert(cui_text_context_line_colored(ui, colored, 14, (cui_color){0}, 20, (cui_text_style){0},
                                         colors, 2));
    colors[0].color.r = 99;
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text_color_count == 2 &&
           list.commands[0].text_colors[0].color.r == 1);
    assert(cui_begin_frame(ui, 200, 200, 0));
    assert(!cui_text_context_line_colored(ui, colored, 14, (cui_color){0}, 20, (cui_text_style){0},
                                          colors, CUI_TEXT_COLOR_SPAN_MAX + 1));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    char big[80];
    memset(big, 'a', sizeof big);
    assert(!cui_text_context_line(ui, (cui_text_line_source){{big, sizeof big}, 0, 1}, 14,
                                  (cui_color){0}, 20, (cui_text_style){0}));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    assert(cui_text_context_line(ui, (cui_text_line_source){cui_string("ok"), 0, 2}, 14,
                                 (cui_color){0}, 20, (cui_text_style){0}));
    list = cui_end_frame(ui);
    assert(list.count == 1 && list.commands[0].text.length == 2);
    cui_destroy(ui);
}
static void lifetime(void) {
    cui_context *ui = cui_create((cui_config){.text_geometry_line = factory});
    assert(ui);
    char text[] = "abc xyz";
    cui_textarea_line line = {.start = 4, .end = 7, .next = 7, .paragraph_end = 7};
    cui_text_geometry owned = {0};
    const cui_text_geometry *a = NULL, *b = NULL;
    assert(cui_begin_frame(ui, 200, 200, 0));
    assert(cui_textarea_visual_geometry(ui, cui_string(text), line, 14, (cui_text_style){0}, &owned,
                                        &a) == CUI_TEXT_LAYOUT_OK);
    assert(cui_textarea_visual_geometry(ui, cui_string(text), line, 14, (cui_text_style){0}, &owned,
                                        &b) == CUI_TEXT_LAYOUT_OK);
    assert(a == b && created == 1 && !owned.session);
    text[0] = 'd';
    assert(cui_textarea_visual_geometry(ui, cui_string(text), line, 14, (cui_text_style){0}, &owned,
                                        &b) == CUI_TEXT_LAYOUT_OK);
    assert(a != b && created == 2);
    cui_text_caret caret;
    size_t count;
    assert(a->ops->carets(a->session, 4, &caret, 1, &count) == CUI_TEXT_LAYOUT_OK && caret.x == 3);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 200, 200, 0));
    assert(released == 2);
    char *large = malloc(200001);
    assert(large);
    memset(large, 'a', 200000);
    large[200000] = 0;
    cui_textarea_line row = {.start = 0, .end = 4, .next = 4, .paragraph_end = 200000};
    for (size_t i = 0; i < 64; i++) {
        row.start = i * 4;
        row.end = row.next = row.start + 4;
        assert(cui_textarea_visual_geometry(ui, cui_string(large), row, 14, (cui_text_style){0},
                                            &owned, &b) == CUI_TEXT_LAYOUT_OK);
    }
    row.start = 256;
    row.end = row.next = 260;
    const cui_text_geometry *sentinel = a;
    assert(cui_textarea_visual_geometry(ui, cui_string(large), row, 14, (cui_text_style){0}, &owned,
                                        &sentinel) == CUI_TEXT_LAYOUT_CAPACITY);
    assert(sentinel == a);
    free(large);
    cui_end_frame(ui);
    cui_destroy(ui);
    assert(released == created);
}

static cui_text_layout_status bad_factory(void *user, cui_text_line_source source, float size,
                                          cui_text_style style, cui_text_geometry *out) {
    cui_text_layout_status status = factory(NULL, source, size, style, out);
    if (*(unsigned *)user == 1)
        return CUI_TEXT_LAYOUT_MEMORY;
    out->byte_length++;
    return status;
}
static cui_text_layout_status old_factory(void *user, cui_str text, float size,
                                          cui_text_style style, cui_text_geometry *out) {
    return factory(user, (cui_text_line_source){text, 0, text.length}, size, style, out);
}
static void factory_failures(void) {
    unsigned mode = 1;
    cui_context *ui =
        cui_create((cui_config){.measure_user = &mode, .text_geometry_line = bad_factory});
    assert(ui);
    cui_text_geometry out = {.session = (void *)(uintptr_t)42, .width = 88};
    cui_text_line_source source = {cui_string("abc"), 0, 3};
    assert(cui_get_text_geometry_line(ui, source, 14, (cui_text_style){0}, &out) ==
           CUI_TEXT_LAYOUT_MEMORY);
    assert(out.session == (void *)(uintptr_t)42 && out.width == 88);
    mode = 2;
    assert(cui_get_text_geometry_line(ui, source, 14, (cui_text_style){0}, &out) ==
           CUI_TEXT_LAYOUT_INVALID);
    assert(out.session == (void *)(uintptr_t)42 && out.width == 88 && created == released);
    cui_destroy(ui);
    ui = cui_create((cui_config){.text_geometry = old_factory});
    assert(ui);
    out = (cui_text_geometry){0};
    assert(cui_get_text_geometry_line(ui, source, 14, (cui_text_style){0}, &out) ==
           CUI_TEXT_LAYOUT_OK);
    void *session = out.session;
    source.byte_start = 1;
    assert(cui_get_text_geometry_line(ui, source, 14, (cui_text_style){0}, &out) ==
               CUI_TEXT_LAYOUT_UNAVAILABLE &&
           out.session == session);
    cui_release_text_geometry(&out);
    cui_destroy(ui);
    assert(created == released);
}
static void rebind(void) {
    char a[32] = "abc\nxyz", b[32] = "q";
    cui_edit *first =
        cui_edit_create((cui_edit_config){.buffer = a, .capacity = sizeof a, .multiline = true});
    cui_edit *second =
        cui_edit_create((cui_edit_config){.buffer = b, .capacity = sizeof b, .multiline = true});
    assert(first && second);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_textarea_line rows[8];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, rows, 8));
    cui_textarea_metrics metrics = {14, 20, 2, 2};
    assert(cui_textarea_layout(ui, first, &state, metrics, 100, false) == CUI_TEXTAREA_OK &&
           state.count == 2);
    cui_edit_destroy(first);
    assert(cui_textarea_layout(ui, second, &state, metrics, 100, false) == CUI_TEXTAREA_OK &&
           state.count == 1 && rows[0].end == 1);
    cui_edit_destroy(second);
    cui_destroy(ui);
}
int main(void) {
    descriptor();
    lifetime();
    factory_failures();
    rebind();
    return 0;
}
