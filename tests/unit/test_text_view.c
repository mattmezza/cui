/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_view.h"
#ifndef CUI_TEXT_VIEW_BASE_ONLY
#include "cui/cui_components.h"
#endif
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
static cui_str text(const char *s) {
    return (cui_str){s, strlen(s)};
}
typedef struct {
    cui_text_view_state state;
    char source[4096], rendered[4096];
    cui_text_view_mapping mapping[4096];
    cui_text_view_span spans[512];
    cui_text_view_block blocks[256];
    cui_text_view_glyph glyphs[4096];
    cui_text_view_highlight highlights[32];
} fixture;
static void setup(fixture *f, cui_text_view_format format) {
    assert(cui_text_view_init(&f->state,
                              (cui_text_view_workspace){.source = f->source,
                                                        .source_capacity = sizeof f->source,
                                                        .rendered = f->rendered,
                                                        .rendered_capacity = sizeof f->rendered,
                                                        .mapping = f->mapping,
                                                        .mapping_capacity = 4096,
                                                        .spans = f->spans,
                                                        .span_capacity = 512,
                                                        .blocks = f->blocks,
                                                        .block_capacity = 256,
                                                        .glyphs = f->glyphs,
                                                        .glyph_capacity = 4096,
                                                        .highlights = f->highlights,
                                                        .highlight_capacity = 32},
                              format));
}
static cui_size measure(void *u, cui_str t, float font, unsigned flags) {
    (void)u;
    (void)flags;
    size_t count = 0;
    for (size_t i = 0; i < t.length; ++i)
        if (((unsigned char)t.data[i] & 0xc0) != 0x80)
            ++count;
    return (cui_size){(float)count * font * .5f, font};
}
static void test_image_font_metrics(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("![alt](asset)\n\nnext")) == CUI_TEXT_VIEW_OK);
    f.blocks[0].image_ready = true;
    f.blocks[0].image = (cui_bitmap){.width = 2, .height = 1};
    cui_text_view_layout_options o = {.width = 200,
                                      .font_size = 14,
                                      .line_height = 20,
                                      .paragraph_gap = 16,
                                      .font_metrics_available = true,
                                      .body_metrics = {8, 2, 10}};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.y == 12 && f.blocks[0].bounds.h == 1 &&
           f.blocks[0].image_line.h == 20 && f.blocks[1].bounds.y == 36);
    o.body_metrics = (cui_font_metrics){4, 6, 10};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.y == 9);
    f.blocks[0].image.height = 40;
    o.body_metrics = (cui_font_metrics){8, 2, 10};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.y == 0 && f.blocks[0].image_line.h == 40 &&
           f.blocks[1].bounds.y == 56);
    size_t count = f.state.glyph_count;
    o.body_metrics.ascent = NAN;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_INVALID);
    assert(f.state.glyph_count == count && f.blocks[1].bounds.y == 56);
    o.body_metrics.ascent = FLT_MAX;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_INVALID);
    o.font_metrics_available = false;
    f.blocks[0].image.height = 1;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.y == 0 && f.blocks[0].image_line.h == 1);
}
static void test_markdown(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    cui_text_view_state *s = &f.state;
    const char *source =
        "# Title\n\nHello **bold** and *italic*, `code`, [site](https://example.test).\n\n- "
        "alpha\n- beta\n\n```c\nint x;\n```\n\n| Name | Value |\n| --- | --- |\n| One | Two |\n";
    assert(cui_text_view_set_text(s, text(source)) == CUI_TEXT_VIEW_OK);
    cui_str rendered = cui_text_view_rendered_text(s);
    assert(strstr(rendered.data, "Hello bold and italic, code, site."));
    assert(!strstr(rendered.data, "**"));
    assert(strstr(rendered.data, "Name Value\nOne Two"));
    assert(s->block_count == 9);
    bool bold = false, italic = false, code = false, link = false;
    for (size_t i = 0; i < s->span_count; ++i) {
        bold |= (f.spans[i].flags & CUI_TEXT_VIEW_BOLD) != 0;
        italic |= (f.spans[i].flags & CUI_TEXT_VIEW_ITALIC) != 0;
        code |= (f.spans[i].flags & CUI_TEXT_VIEW_CODE) != 0;
        link |= f.spans[i].link.end > f.spans[i].link.begin;
    }
    assert(bold && italic && code && link);
    size_t offset = (size_t)(strstr(source, "**bold**") - source);
    cui_text_view_range range;
    assert(cui_text_view_range_for_source(s, (cui_text_view_range){offset, offset + 8}, &range));
    assert(range.end - range.begin == 4 && !memcmp(f.rendered + range.begin, "bold", 4));
    assert(!cui_text_view_range_for_source(s, (cui_text_view_range){offset, offset + 2}, &range));
    assert(
        cui_text_view_range_for_source(s, (cui_text_view_range){offset + 2, offset + 6}, &range));
    assert(cui_text_view_select(s, range.begin, range.end));
    assert(cui_text_view_selected_text(s).length == 4);
    cui_text_view_range selected;
    assert(cui_text_view_selected_source_range(s, &selected));
    assert(selected.begin == offset + 2 && selected.end == offset + 6);
    assert(cui_text_view_set_highlights(s, (cui_text_view_highlight[]){{range, {255, 100, 0, 100}}},
                                        1) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_set_highlights(
               s, (cui_text_view_highlight[]){{{range.end, range.begin}, {0}}}, 1) ==
           CUI_TEXT_VIEW_INVALID);
    assert(s->highlight_count == 1);
    assert(cui_text_view_layout(
               s,
               (cui_text_view_layout_options){
                   .width = 240, .font_size = 14, .line_height = 20, .paragraph_gap = 16},
               measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(s->height > 100 && s->glyph_count > 0);
    assert(f.blocks[5].kind == CUI_TEXT_VIEW_TABLE_CELL && f.blocks[5].bounds.x == 0);
    assert(f.blocks[6].bounds.x > 0 && f.blocks[6].bounds.y == f.blocks[5].bounds.y);
    assert(cui_text_view_reveal(s, range) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_layout(s,
                                (cui_text_view_layout_options){.width = 240,
                                                               .font_size = 14,
                                                               .line_height = 20,
                                                               .paragraph_gap = 16,
                                                               .max_lines = 2},
                                measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(s->clamped && s->height == 40 && !s->reveal_pending);
    char saved[4096];
    memcpy(saved, f.rendered, sizeof saved);
    size_t old = s->text_length;
    assert(cui_text_view_set_text(s, (cui_str){"\xc0\xaf", 2}) == CUI_TEXT_VIEW_INVALID);
    assert(s->text_length == old && !memcmp(saved, f.rendered, old));
    s->workspace.span_capacity = 1;
    assert(cui_text_view_set_text(s, text("**one** *two* `three`")) == CUI_TEXT_VIEW_CAPACITY);
    assert(s->text_length == old);
    s->workspace.span_capacity = 512;
}
static void test_unicode_ranges_and_stream(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    cui_text_view_state *s = &f.state;
    assert(cui_text_view_set_text(s, text("hé &amp; \\*")) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(f.rendered, "hé & *\n"));
    assert(!cui_text_view_select(s, 2, 3));
    cui_text_view_range r;
    assert(cui_text_view_range_for_source(s, (cui_text_view_range){5, 6}, &r));
    assert(r.end - r.begin == 1 && f.rendered[r.begin] == '&');
    assert(cui_text_view_set_highlights(s, (cui_text_view_highlight[]){{{0, 3}, {1, 2, 3, 4}}},
                                        1) == CUI_TEXT_VIEW_OK);
    s->stream_fade = true;
    s->time = 1;
    assert(cui_text_view_set_text(s, text("hé &amp; \\* tail")) == CUI_TEXT_VIEW_OK);
    assert(s->highlight_count == 1 && f.highlights[0].range.begin == 0 &&
           f.highlights[0].range.end == 3);
    assert(cui_text_view_advance(s, .1f));
    assert(!cui_text_view_advance(s, .4f));
    assert(cui_text_view_set_text(s, text("prefix hé &amp; \\* tail")) == CUI_TEXT_VIEW_OK);
    assert(s->highlight_count == 1 && f.highlights[0].range.begin == 7 &&
           f.highlights[0].range.end == 10);
    assert(!cui_text_view_advance(s, 0));
    assert(cui_text_view_set_text(s, text("prefix hi &amp; \\* tail")) == CUI_TEXT_VIEW_OK);
    assert(s->highlight_count == 0);
    assert(cui_text_view_set_text(s, text("**bo")) == CUI_TEXT_VIEW_OK);
    s->time = 2;
    assert(cui_text_view_set_text(s, text("**bold**")) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(f.rendered, "bold\n"));
    assert(f.mapping[0].born == 2 && f.mapping[0].flags & CUI_TEXT_VIEW_BOLD);
    s->reduced_motion = true;
    assert(!cui_text_view_advance(s, 0));
}
static void test_paragraphs_and_numbered_markers(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("one\ntwo  \nthree\n\n12. item\n99. next")) ==
           CUI_TEXT_VIEW_OK);
    assert(!strcmp(f.rendered, "one two\nthree\nitem\nnext\n"));
    assert(f.state.block_count == 3);
    assert(!strcmp(f.blocks[1].marker_text, "12.") && !strcmp(f.blocks[2].marker_text, "13."));
    cui_text_view_range r = f.blocks[1].list_marker;
    assert(r.end - r.begin == 3 && !memcmp(f.source + r.begin, "12.", 3));
    assert(cui_text_view_range_for_source(&f.state, (cui_text_view_range){3, 4}, &r));
    assert(r.end - r.begin == 1 && f.rendered[r.begin] == ' ');
    assert(cui_text_view_layout(
               &f.state,
               (cui_text_view_layout_options){
                   .width = 160, .font_size = 14, .line_height = 20, .paragraph_gap = 16},
               measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[1].text_inset == 28 && f.blocks[2].text_inset == 28);
    char output[256];
    size_t length;
    assert(cui_text_view_select(&f.state, 0, f.blocks[0].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "one two  \nthree"));
    assert(cui_text_view_select(&f.state, f.blocks[1].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "12. item\n13. next"));
}
static void test_source_copy(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    const char *source = "# title\n\nhello **bold** [link](https://x.test) `a`";
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    char output[256];
    size_t length;
    size_t a = (size_t)(strstr(f.rendered, "bold") - f.rendered);
    assert(cui_text_view_select(&f.state, a + 1, a + 3));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "**ol**") && length == 6);
    memcpy(output, "unchanged", 10);
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, 3, &length) ==
           CUI_TEXT_VIEW_CAPACITY);
    assert(!strcmp(output, "unchanged") && length == 6);
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, NULL, 0, &length) ==
               CUI_TEXT_VIEW_CAPACITY &&
           length == 6);
    a = (size_t)(strstr(f.rendered, "link") - f.rendered);
    assert(cui_text_view_select(&f.state, a, a + 4));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "[link](https://x.test)"));
    assert(cui_text_view_select(&f.state, 0, f.state.text_length));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(length == strlen(source) && !strcmp(output, source));
    assert(cui_text_view_set_text(&f.state, text("before &amp;amp; \\` end")) == CUI_TEXT_VIEW_OK);
    a = (size_t)(strstr(f.rendered, "&amp;") - f.rendered);
    assert(cui_text_view_select(&f.state, a, a + 7));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "\\&amp; \\`"));
    setup(&f, CUI_TEXT_VIEW_HTML);
    assert(cui_text_view_set_text(&f.state, text("<b>hello</b>")) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_select(&f.state, 0, 5));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "hello"));
}
static void test_table_copy_alignment(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state,
                                  text("| A | B | C |\n| :-- | :-: | --: |\n| x | y | z |")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].alignment == CUI_TEXT_VIEW_ALIGN_LEFT &&
           f.blocks[1].alignment == CUI_TEXT_VIEW_ALIGN_CENTER &&
           f.blocks[2].alignment == CUI_TEXT_VIEW_ALIGN_RIGHT);
    assert(cui_text_view_layout(
               &f.state,
               (cui_text_view_layout_options){
                   .width = 300, .font_size = 14, .line_height = 20, .paragraph_gap = 16},
               measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.glyphs[0].bounds.x == 8);
    assert(f.glyphs[1].bounds.x > 140 && f.glyphs[1].bounds.x < 160);
    assert(f.glyphs[2].bounds.x > 280);
    size_t a = (size_t)(strstr(f.rendered, "y") - f.rendered);
    assert(cui_text_view_select(&f.state, a, a + 1));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "|  |  |  |\n| :-- | :-: | --: |\n|  | y |  |"));
}
static void test_fenced_code_content(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("```` c metadata\na\n```\nb\n````")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 1 && f.blocks[0].kind == CUI_TEXT_VIEW_CODE_BLOCK);
    cui_text_view_range r = f.blocks[0].text;
    assert(r.end - r.begin == 7 && !memcmp(f.rendered + r.begin, "a\n```\nb", 7));
    r = f.blocks[0].resource;
    assert(r.end - r.begin == 1 && f.source[r.begin] == 'c');
    assert(cui_text_view_select(&f.state, 0, f.blocks[0].text.end));
    char output[128];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "````c\na\n```\nb\n````"));
    assert(cui_text_view_set_text(&f.state, text("```c\na\n```")) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].text.end - f.blocks[0].text.begin == 1);
    assert(cui_text_view_layout(&f.state,
                                (cui_text_view_layout_options){.width = 200,
                                                               .font_size = 14,
                                                               .line_height = 20,
                                                               .paragraph_gap = 16,
                                                               .code_actions_height = 32},
                                measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.h == 44 && f.glyphs[0].font_size == 14);
}
static void test_proportional_table_defaults(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state,
                                  text("| A | abcdefghijklmnopqrst |\n| --- | --- |\n| x | y |")) ==
           CUI_TEXT_VIEW_OK);
    cui_text_view_layout_options o = {
        .width = 300, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.w == 64 && f.blocks[1].bounds.w == 236);
    assert(f.blocks[0].bounds.h == 28 && f.blocks[2].bounds.w == 64);
    assert(f.blocks[0].table_columns == 2 && !(f.glyphs[0].flags & CUI_TEXT_VIEW_SEMIBOLD));
    o.width = 100;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.w == 64 && f.blocks[1].bounds.x == 64 && f.blocks[1].bounds.w == 64);
    size_t glyph_count = f.state.glyph_count;
    cui_rect bounds = f.blocks[0].bounds;
    o.width = FLT_MAX;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_INVALID);
    assert(f.state.glyph_count == glyph_count && f.blocks[0].bounds.w == bounds.w);
    o.width = 10000000;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    for (size_t b = 0; b < f.state.block_count; ++b)
        assert(isfinite(f.blocks[b].bounds.x) && isfinite(f.blocks[b].bounds.w));
    o.table_actions_height = FLT_MAX;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_INVALID);
    o.table_actions_height = 0;
    char source[256];
    memcpy(source, "| A | ", 6);
    memset(source + 6, 'x', 200);
    strcpy(source + 206, " |\n| --- | --- |");
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    o.width = 2000;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(fabsf(f.blocks[0].bounds.w - (2000.f * 5 / 155)) < .01f);
}
static cui_size table_measure(void *user, cui_str str, float font, unsigned flags) {
    (void)user;
    (void)font;
    float width = 0;
    for (size_t i = 0; i < str.length; ++i)
        width += str.data[i] == 'W' ? 100 : 1;
    if (flags & CUI_TEXT_VIEW_BOLD)
        width *= 2;
    return (cui_size){width, 20};
}
static void test_adaptive_table_model(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("| WWWW | iiii |\n| --- | --- |\n| x | y |")) ==
           CUI_TEXT_VIEW_OK);
    cui_text_view_layout_options o = {.width = 100,
                                      .font_size = 14,
                                      .line_height = 20,
                                      .table_mode = CUI_TEXT_VIEW_TABLE_ADAPTIVE};
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.w == 208.5f && f.blocks[1].bounds.w == 48);
    assert(f.blocks[0].bounds.h > 28);
    o.width = 1000;
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(fabsf(f.blocks[0].bounds.w - 998.f * 417 / 465) < .01f);
    assert(fabsf(f.blocks[1].bounds.x + f.blocks[1].bounds.w - 999) < .01f);
    o.width = 100;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.w == 417 && f.blocks[1].bounds.w == 48);
    assert(f.blocks[0].bounds.h == 28);
    assert(cui_text_view_set_text(&f.state, text("| **WWWW** | iiii |\n| --- | --- |")) ==
           CUI_TEXT_VIEW_OK);
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.w == 817);
    assert(cui_text_view_set_text(&f.state, text("| **WWWW** | iiii |\n| --- | --- |\n| x |")) ==
           CUI_TEXT_VIEW_OK);
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].table_track_width == 867 && f.blocks[0].table_frame.h == 58);
    o.table_mode = (cui_text_view_table_mode)-1;
    assert(cui_text_view_layout(&f.state, o, table_measure, NULL) == CUI_TEXT_VIEW_INVALID);
}
static void test_task_list_model(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("- [ ] todo\n- [x] done\n3. [X] ordered\n- [q] literal\n- [x]")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 5);
    assert(f.blocks[0].task && !f.blocks[0].checked);
    assert(f.blocks[1].task && f.blocks[1].checked);
    assert(f.blocks[2].task && f.blocks[2].checked && f.blocks[2].list_number == 3);
    assert(!f.blocks[3].task && f.blocks[4].task && f.blocks[4].checked);
    assert(!memcmp(f.rendered, "todo\ndone\nordered\n[q] literal\n", 30));
    assert(f.mapping[0].source.begin == 6);
    assert(cui_text_view_select(&f.state, 0, f.blocks[1].text.end));
    char output[128];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- [ ] todo\n- [x] done"));
    assert(cui_text_view_select(&f.state, f.blocks[2].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "3. [x] ordered"));
    assert(cui_text_view_layout(
               &f.state,
               (cui_text_view_layout_options){.width = 200, .font_size = 14, .line_height = 20},
               measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[2].text_inset == 20);
    assert(f.blocks[3].text_inset == 14 && !strcmp(f.blocks[3].prefix_text, "\xe2\x80\xa2 "));
}
static void test_resource_decoding(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(&f.state, text("[site](https://x.test?a=1&amp;b=\\!&#x1f642;)")) ==
        CUI_TEXT_VIEW_OK);
    char output[128] = "untouched";
    size_t length;
    cui_text_view_range resource = f.spans[0].link;
    assert(cui_text_view_decode_resource(&f.state, resource, output, 2, &length) ==
           CUI_TEXT_VIEW_CAPACITY);
    assert(!strcmp(output, "untouched") && length > 2);
    assert(cui_text_view_decode_resource(&f.state, resource, output, sizeof output, &length) ==
           CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "https://x.test?a=1&b=!\xf0\x9f\x99\x82"));
    assert(length == strlen(output));
    assert(cui_text_view_decode_resource(
               &f.state,
               (cui_text_view_range){f.state.source_length + 1, f.state.source_length + 1}, output,
               sizeof output, &length) == CUI_TEXT_VIEW_INVALID);
    setup(&f, CUI_TEXT_VIEW_HTML);
    assert(cui_text_view_set_text(&f.state, text("<a href='x\\!&amp;y'>site</a>")) ==
           CUI_TEXT_VIEW_OK);
    assert(cui_text_view_decode_resource(&f.state, f.spans[0].link, output, sizeof output,
                                         &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "x\\!&y"));
    setup(&f, CUI_TEXT_VIEW_PLAIN);
    assert(cui_text_view_set_text(&f.state, text("x\\!&amp;y")) == CUI_TEXT_VIEW_OK);
    assert(cui_text_view_decode_resource(&f.state, (cui_text_view_range){0, f.state.source_length},
                                         output, sizeof output, &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "x\\!&amp;y"));
}
static void test_nested_list_model(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state,
               text("1. parent\n   4. child\n      2. grand\n   9. sibling\n9. next\n\nlast")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].list_parent == SIZE_MAX && f.blocks[1].list_parent == 0 &&
           f.blocks[2].list_parent == 1 && f.blocks[3].list_parent == 0);
    assert(f.blocks[1].list_depth == 1 && f.blocks[2].list_depth == 2);
    assert(f.blocks[3].list_number == 5 && f.blocks[4].list_number == 2);
    cui_text_view_layout_options o = {.width = 300, .font_size = 14, .line_height = 20};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[1].bounds.x == 16 && f.blocks[2].bounds.x == 32);
    assert(!strcmp(f.blocks[1].prefix_text, "D. ") && !strcmp(f.blocks[2].prefix_text, "b. ") &&
           !strcmp(f.blocks[3].prefix_text, "E. "));
    char output[256];
    size_t length;
    assert(cui_text_view_select(&f.state, f.blocks[2].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "1. \n   4. \n      2. grand"));
    assert(cui_text_view_select(&f.state, f.blocks[1].text.begin, f.blocks[3].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "1. \n   4. child\n      2. grand\n   5. sibling"));
    assert(
        cui_text_view_set_text(&f.state, text("- [x]\n  - child\n    0. zero\n       1. below")) ==
        CUI_TEXT_VIEW_OK);
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(f.blocks[1].prefix_text, "\xe2\x97\xa6 ") &&
           !strcmp(f.blocks[2].prefix_text, "0. ") && !strcmp(f.blocks[3].prefix_text, "a. "));
    assert(cui_text_view_select(&f.state, f.blocks[1].text.begin, f.blocks[1].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- [x]\n  - child"));
    char nested[2048];
    size_t at = 0;
    for (unsigned depth = 0; depth <= CUI_TEXT_VIEW_LIST_DEPTH_LIMIT; ++depth) {
        memset(nested + at, ' ', depth * 2);
        at += depth * 2;
        memcpy(nested + at, "- item\n", 7);
        at += 7;
    }
    nested[at] = 0;
    assert(cui_text_view_set_text(&f.state, text(nested)) == CUI_TEXT_VIEW_INVALID);
    assert(f.state.block_count == 4 && f.blocks[1].list_parent == 0);
}
static void test_first_list_table(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    const char *source =
        "10. | a | b |\n    | --- | --- |\n    | x | y |\n    - child\n\n    after\n11. next";
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 7 && f.blocks[0].kind == CUI_TEXT_VIEW_TABLE_CELL &&
           f.blocks[0].list_item && f.blocks[0].list_ordered && f.blocks[0].list_number == 10);
    assert(f.blocks[1].list_parent == 0 && f.blocks[3].list_parent == 0 &&
           f.blocks[4].list_parent == 0 && f.blocks[5].list_parent == 0 &&
           f.blocks[6].list_number == 11);
    assert(cui_text_view_select(&f.state, 0, f.blocks[6].text.end));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(
        output,
        "10. | a | b |\n    | :-- | :-- |\n    | x | y |\n    after\n    - child\n11. next"));
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].table_frame.x == 28 && f.blocks[0].table_frame.w == 372 &&
           f.blocks[0].text_inset == 8 && f.blocks[4].bounds.y == f.blocks[0].table_frame.h);
    assert(
        cui_text_view_set_text(
            &f.state, text("- | abcdefghijklmnopqrstuvwxyz | b |\n  | --- | --- |\n  | x | y |")) ==
        CUI_TEXT_VIEW_OK);
    o.width = 100;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    f.blocks[0].table_scroll.x = 3;
    assert(cui_text_view_set_text(&f.state, text("- | abcdefghijklmnopqrstuvwxyz | b |\n  | --- | "
                                                 "--- |\n  | x | y |\n  | next | row |")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].table_scroll.x == 3);
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].table_frame.x == 14 && f.blocks[0].table_frame.w == 86 &&
           f.blocks[0].table_track_width > 86);
}
static void test_nested_table_structure(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("10. parent\n    - child\n\n    | a | b |\n    | --- | --- |\n    | "
                              "x | y |\n\n    after\n11. next")) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 8 && f.blocks[2].kind == CUI_TEXT_VIEW_TABLE_CELL &&
           f.blocks[2].list_parent == 0 && f.blocks[5].list_parent == 0 &&
           f.blocks[6].list_parent == 0 && f.blocks[7].list_number == 11);
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[2].table_frame.x == 16 && f.blocks[2].table_frame.w == 384 &&
           f.blocks[2].bounds.x == 16 && f.blocks[3].bounds.x == 208 &&
           f.blocks[6].bounds.y == f.blocks[2].table_frame.y + f.blocks[2].table_frame.h);
    assert(cui_text_view_select(&f.state, 0, f.blocks[7].text.end));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "10. parent\n    | a | b |\n    | :-- | :-- |\n    | x | y |\n    "
                           "after\n    - child\n11. next"));
    assert(cui_text_view_set_text(&f.state, text("| a | b |\n| --- | --- |\n| x | y |")) ==
           CUI_TEXT_VIEW_OK);
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.state.height == f.blocks[0].table_frame.h);
}
static void test_list_image_structure(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(
            &f.state, text("10. ![alt](asset)\n    - child\n\n    ![second](asset)\n11. next")) ==
        CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 4 && f.blocks[0].kind == CUI_TEXT_VIEW_IMAGE &&
           f.blocks[0].list_item && f.blocks[0].list_ordered && f.blocks[0].list_number == 10);
    assert(f.blocks[1].list_parent == 0 && f.blocks[2].kind == CUI_TEXT_VIEW_IMAGE &&
           f.blocks[2].list_continuation && f.blocks[2].list_parent == 0 &&
           f.blocks[3].list_number == 11);
    assert(cui_text_view_select(&f.state, 0, f.blocks[3].text.end));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "10. ![alt](asset)\n    ![second](asset)\n    - child\n11. next"));
    assert(cui_text_view_select(&f.state, f.blocks[2].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "10. ![second](asset)"));
}
static void test_quote_paragraph_lines(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    const char *source = "> first  \n> second\n> third\n\noutside";
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 2 && f.blocks[0].kind == CUI_TEXT_VIEW_QUOTE);
    assert(!memcmp(f.rendered, "first\nsecond third", 18));
    assert(cui_text_view_select(&f.state, 0, f.blocks[0].text.end));
    cui_text_view_range selected;
    assert(cui_text_view_selected_source_range(&f.state, &selected));
    assert(selected.begin == 2 && selected.end == (size_t)(strstr(source, "third") - source) + 5);
    char output[128];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "> first  \n> second third"));
    assert(cui_text_view_set_text(&f.state, text("- > first  \n  > second\n  > third\n- next")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 2 && f.blocks[0].list_item &&
           f.blocks[0].kind == CUI_TEXT_VIEW_QUOTE);
    assert(cui_text_view_select(&f.state, 0, f.blocks[0].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- > first  \n  > second third"));
    assert(cui_text_view_set_text(&f.state, text("- parent\n\n  > first  \n  > second")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 2 && f.blocks[1].list_continuation);
    assert(cui_text_view_select(&f.state, 0, f.blocks[1].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- parent\n  > first  \n  > second"));
}
static void test_list_nonparagraph_blocks(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("- ## heading\n  - child\n\n  > quote\n\n  after\n- next")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 5 && f.blocks[0].kind == CUI_TEXT_VIEW_HEADING &&
           f.blocks[0].level == 2 && f.blocks[0].list_item && !f.blocks[0].list_ordered);
    assert(f.blocks[1].list_parent == 0 && f.blocks[2].kind == CUI_TEXT_VIEW_QUOTE &&
           f.blocks[2].list_continuation && f.blocks[2].list_parent == 0);
    assert(f.blocks[3].list_continuation && !f.blocks[3].list_repeat_marker);
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.x == 14 && f.blocks[0].bounds.h == 30 && f.blocks[2].bounds.x == 16 &&
           f.blocks[2].text_inset == 12);
    assert(cui_text_view_select(&f.state, f.blocks[0].text.begin, f.blocks[4].text.end));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- ## heading\n  > quote\n  after\n  - child\n- next"));
    assert(cui_text_view_set_text(&f.state, text("10. > quote\n\n    # subheading\n11. next")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].kind == CUI_TEXT_VIEW_QUOTE && f.blocks[0].list_ordered &&
           f.blocks[1].kind == CUI_TEXT_VIEW_HEADING && f.blocks[1].list_parent == 0 &&
           f.blocks[2].list_number == 11);
    assert(cui_text_view_select(&f.state, f.blocks[0].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "10. > quote\n    # subheading\n11. next"));
}
static void test_first_list_fence(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    const char *source = "- ```rust\n  one\n  two\n  ```\n  - child\n\n  after";
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 3 && f.blocks[0].kind == CUI_TEXT_VIEW_CODE_BLOCK &&
           f.blocks[0].list_item && f.blocks[1].list_parent == 0 && f.blocks[2].list_parent == 0);
    assert(!memcmp(f.rendered, "one\ntwo\nchild\nafter", 19));
    cui_text_view_range selected;
    assert(cui_text_view_select(&f.state, 0, 3));
    assert(cui_text_view_selected_source_range(&f.state, &selected));
    assert(selected.begin == 12 && selected.end == 15);
    assert(cui_text_view_select(&f.state, 4, 7));
    assert(cui_text_view_selected_source_range(&f.state, &selected));
    assert(selected.begin == 18 && selected.end == 21);
    assert(cui_text_view_select(&f.state, 0, 7));
    char output[128];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- ```rust\n  one\n  two\n  ```"));
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.x == 14 && f.blocks[0].list_prefix_width == 14 &&
           f.blocks[0].text_inset == 12);
    assert(cui_text_view_set_text(&f.state, text("10. ~~~\n    a\\\\b\n    ~~~\n11. next")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].list_item && f.blocks[0].list_number == 10 && f.blocks[1].list_number == 11);
    assert(!memcmp(f.rendered, "a\\\\b", 4));
    assert(f.mapping[1].source.begin == 13 && f.mapping[2].source.begin == 14);
}
static void test_nested_fenced_continuation(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    const char *source = "1. parent\n   - child\n\n   ```rust\n   one\n   two\n   ```\n\n   after";
    assert(cui_text_view_set_text(&f.state, text(source)) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 4 && f.blocks[2].kind == CUI_TEXT_VIEW_CODE_BLOCK &&
           f.blocks[2].list_parent == 0 && f.blocks[2].list_continuation);
    assert(f.blocks[3].list_parent == 0 && !f.blocks[3].list_repeat_marker);
    size_t code = f.blocks[2].text.begin;
    assert(!memcmp(f.rendered + code, "one\ntwo", 7));
    cui_text_view_range selected;
    assert(cui_text_view_select(&f.state, code, code + 3));
    assert(cui_text_view_selected_source_range(&f.state, &selected));
    assert(selected.begin == (size_t)(strstr(source, "one") - source) &&
           selected.end == selected.begin + 3);
    assert(cui_text_view_select(&f.state, code + 4, code + 7));
    assert(cui_text_view_selected_source_range(&f.state, &selected));
    assert(selected.begin == (size_t)(strstr(source, "two") - source) &&
           selected.end == selected.begin + 3);
    assert(cui_text_view_select(&f.state, code, code + 7));
    char output[256];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "1. ```rust\n   one\n   two\n   ```"));
    assert(cui_text_view_set_text(&f.state, text("  ```\n  a\\\\b\n  ```")) == CUI_TEXT_VIEW_OK);
    assert(!memcmp(f.rendered, "a\\\\b", 4));
    assert(f.mapping[0].source.begin == 8 && f.mapping[1].source.begin == 9 &&
           f.mapping[2].source.begin == 10);
}
static void test_nested_list_continuations(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state,
                                  text("- parent\n  - child\n\n  resumed\n\n  final\n- sibling")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 5 && f.blocks[2].list_parent == 0 &&
           f.blocks[3].list_parent == 0);
    assert(f.blocks[2].list_repeat_marker && !f.blocks[3].list_repeat_marker);
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[2].bounds.x == 0 && f.blocks[2].text_inset == f.blocks[0].text_inset &&
           f.blocks[3].bounds.x == 16);
    char output[256];
    size_t length;
    assert(cui_text_view_select(&f.state, f.blocks[0].text.begin, f.blocks[4].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- parent\n  resumed\n  final\n  - child\n- sibling"));
    assert(cui_text_view_select(&f.state, f.blocks[1].text.begin, f.blocks[2].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- resumed\n  - child"));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_PLAIN, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "child\nresumed"));
    assert(
        cui_text_view_set_text(
            &f.state, text("10. parent\n    - child\n\n    resumed\n    - second\n\n    final")) ==
        CUI_TEXT_VIEW_OK);
    assert(f.blocks[2].list_repeat_marker && f.blocks[4].list_repeat_marker &&
           f.blocks[4].list_parent == 0);
    assert(cui_text_view_select(&f.state, f.blocks[0].text.begin, f.blocks[4].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "10. parent\n    resumed\n    final\n    - child\n    - second"));
}
static void test_setext_headings(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("first\nsecond\n===\n\n---\nSome text\n---")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 3);
    assert(f.blocks[0].kind == CUI_TEXT_VIEW_HEADING && f.blocks[0].level == 1);
    assert(f.blocks[1].kind == CUI_TEXT_VIEW_RULE);
    assert(f.blocks[2].kind == CUI_TEXT_VIEW_HEADING && f.blocks[2].level == 2);
    char output[128];
    size_t length;
    assert(cui_text_view_select(&f.state, f.blocks[0].text.begin, f.blocks[0].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "# first second"));
    assert(cui_text_view_set_text(&f.state, text("ordinary\n    ===")) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].kind != CUI_TEXT_VIEW_HEADING);
}
static void test_list_continuations(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("12. first\nlazy continuation  \nwith hard\n\n    second paragraph\n "
                              "   wrap continues\n\noutside")) == CUI_TEXT_VIEW_OK);
    assert(f.state.block_count == 3 && f.blocks[1].list_continuation &&
           f.blocks[1].list_parent == 0 && !f.blocks[2].list_continuation);
    assert(!memcmp(f.rendered,
                   "first lazy continuation\nwith hard\nsecond paragraph wrap continues\noutside\n",
                   f.state.text_length));
    cui_text_view_layout_options o = {
        .width = 400, .font_size = 14, .line_height = 20, .paragraph_gap = 16};
    assert(cui_text_view_layout(&f.state, o, measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].bounds.h == 40 && f.blocks[1].bounds.x == 16 && f.blocks[1].bounds.y == 40 &&
           f.blocks[2].bounds.y == 76);
    char output[256];
    size_t length;
    assert(cui_text_view_select(&f.state, 0, f.blocks[1].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(
        output,
        "12. first lazy continuation  \n    with hard\n    second paragraph wrap continues"));
    assert(cui_text_view_select(&f.state, f.blocks[1].text.begin, f.blocks[1].text.end));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "12. second paragraph wrap continues"));
    strcpy(output, "untouched");
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, 3, &length) ==
           CUI_TEXT_VIEW_CAPACITY);
    assert(!strcmp(output, "untouched") && length == strlen("12. second paragraph wrap continues"));
}
static void test_html(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_HTML);
    cui_text_view_state *s = &f.state;
    assert(cui_text_view_set_text(s, text("<h2>Title</h2><p>Hello <strong>bold</strong> &amp; <a "
                                          "href='https://x.test'>link</a><br>line</"
                                          "p><script>secret()</script><table><tr><th>A</th><th>B</"
                                          "th></tr><tr><td>1</td><td>2</td></tr></table>")) ==
           CUI_TEXT_VIEW_OK);
    assert(strstr(f.rendered, "Hello bold & link\nline"));
    assert(!strstr(f.rendered, "secret"));
    assert(s->block_count == 6);
    bool link = false;
    for (size_t i = 0; i < s->span_count; ++i)
        link |= f.spans[i].link.end > f.spans[i].link.begin;
    assert(link);
    assert(cui_text_view_set_highlights(s, NULL, 0) == CUI_TEXT_VIEW_UNSUPPORTED);
    assert(!cui_text_view_range_for_source(s, (cui_text_view_range){0, 5}, NULL));
    assert(cui_text_view_layout(
               s,
               (cui_text_view_layout_options){
                   .width = 160, .font_size = 14, .line_height = 20, .paragraph_gap = 16},
               measure, NULL) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[2].table_header && f.blocks[3].table_header && !f.blocks[4].table_header);
}
static void test_table_highlight_edits_and_nested_html(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, (cui_str){0}) == CUI_TEXT_VIEW_OK);
    const char *table = "| A | B |\n| --- | --- |\n| x | y |\n| m | n |";
    assert(cui_text_view_set_text(&f.state, text(table)) == CUI_TEXT_VIEW_OK);
    cui_text_view_highlight h[6];
    for (size_t i = 0; i < 6; ++i)
        h[i] = (cui_text_view_highlight){.range = f.blocks[i].text, .background = {255, 0, 0, 100}};
    assert(cui_text_view_set_highlights(&f.state, h, 6) == CUI_TEXT_VIEW_OK);
    assert(
        cui_text_view_set_text(&f.state, text("| A | B |\n| --- | --- |\n| X | y |\n| m | n |")) ==
        CUI_TEXT_VIEW_OK);
    assert(f.state.highlight_count == 2);
    assert(f.highlights[0].range.begin == f.blocks[0].text.begin &&
           f.highlights[1].range.begin == f.blocks[1].text.begin);
    assert(cui_text_view_set_text(
               &f.state, text("intro\n\n| A | B |\n| --- | --- |\n| X | y |\n| m | n |")) ==
           CUI_TEXT_VIEW_OK);
    assert(f.state.highlight_count == 2 && f.highlights[0].range.begin == f.blocks[1].text.begin);
    setup(&f, CUI_TEXT_VIEW_HTML);
    assert(cui_text_view_set_text(
               &f.state,
               text("<strong>a<strong>b</strong>c</strong>d<SCRIPT>secret</SCRIPT ><p>safe</p>")) ==
           CUI_TEXT_VIEW_OK);
    assert(strstr(f.rendered, "abcd") && strstr(f.rendered, "safe") &&
           !strstr(f.rendered, "secret"));
    assert(f.mapping[0].flags & CUI_TEXT_VIEW_BOLD);
    assert(f.mapping[1].flags & CUI_TEXT_VIEW_BOLD);
    assert(f.mapping[2].flags & CUI_TEXT_VIEW_BOLD);
    assert(!(f.mapping[3].flags & CUI_TEXT_VIEW_BOLD));
}
static void test_bounded_parser(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    cui_text_view_state invalid;
    assert(!cui_text_view_init(&invalid, f.state.workspace, (cui_text_view_format)-1));
    unsigned seed = 123;
    char input[257];
    const char alphabet[] = "abc _*~`[]()<>!|#&;\\\n0123";
    for (unsigned k = 0; k < 300; ++k) {
        for (size_t i = 0; i < 256; ++i) {
            seed = seed * 1664525u + 1013904223u;
            input[i] = alphabet[seed % (sizeof alphabet - 1)];
        }
        input[256] = 0;
        cui_text_view_error e = cui_text_view_set_text(&f.state, text(input));
        assert(e == CUI_TEXT_VIEW_OK || e == CUI_TEXT_VIEW_CAPACITY || e == CUI_TEXT_VIEW_INVALID);
        assert(f.state.text_length < sizeof f.rendered && f.state.span_count <= 512 &&
               f.state.block_count <= 256);
    }
}
#ifndef CUI_TEXT_VIEW_BASE_ONLY
static cui_draw_list frame(cui_context *ui, fixture *f, cui_text_view_options o) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_text_view(ui, 77, &theme, &f->state, o));
    cui_draw_list d = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return d;
}
static unsigned clicks;
static cui_pointer_button clicked_button;
static void link_clicked(cui_str url, const cui_event *e, void *u) {
    assert(u == &clicks);
    assert(url.length == strlen("https://x.test") &&
           !memcmp(url.data, "https://x.test", url.length));
    ++clicks;
    clicked_button = e->pointer_button;
}
static bool image_loaded(cui_str uri, cui_bitmap *out, void *u) {
    static const unsigned char pixels[] = {255, 0, 0, 255, 0, 255, 0, 255};
    (void)u;
    assert(uri.length == 5 && !memcmp(uri.data, "asset", 5));
    *out = (cui_bitmap){.pixels = pixels, .width = 2, .height = 1, .stride = 8, .revision = 1};
    return true;
}
static bool image_metrics(void *user, float size, cui_text_style style, cui_font_metrics *out) {
    assert(size == 14 && style.weight == CUI_TEXT_WEIGHT_NORMAL);
    *out = *(cui_font_metrics *)user;
    return true;
}
static void test_image_metric_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("- ![alt](asset)\n- next")) == CUI_TEXT_VIEW_OK);
    cui_font_metrics metrics = {8, 2, 10};
    cui_context *ui = cui_create(
        (cui_config){.max_elements = 512, .font_metrics = image_metrics, .measure_user = &metrics});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 200;
    o.image = image_loaded;
    cui_draw_list d = frame(ui, &f, o);
    bool image = false;
    for (size_t i = 0; i < d.count; ++i) {
        if (d.commands[i].type == CUI_DRAW_IMAGE) {
            image = true;
            assert(d.commands[i].bounds.y == f.state.viewport.y + 12 &&
                   d.commands[i].bounds.h == 1);
        }
    }
    assert(image && f.blocks[0].image_line.y == 0 && f.blocks[1].bounds.y == 20);
    metrics.ascent = NAN;
    d = frame(ui, &f, o);
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_IMAGE)
            assert(d.commands[i].bounds.y == f.state.viewport.y);
    cui_destroy(ui);
}
static void test_links_and_images(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("[site](https://x.test)\n\n![alt](asset)")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 200;
    o.image = image_loaded;
    o.on_link = link_clicked;
    o.link_user = &clicks;
    cui_draw_list d = frame(ui, &f, o);
    bool bitmap = false;
    for (size_t i = 0; i < d.count; ++i)
        bitmap |= d.commands[i].type == CUI_DRAW_IMAGE;
    assert(bitmap);
    frame(ui, &f, o);
    float x = f.state.viewport.x + f.glyphs[0].bounds.x + 2;
    float y = f.state.viewport.y + f.glyphs[0].bounds.y + 2;
    for (unsigned button = 0; button < 2; ++button) {
        cui_pointer_button b = button ? CUI_POINTER_SECONDARY : CUI_POINTER_PRIMARY;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .pointer_button = b,
                                               .x = x,
                                               .y = y,
                                               .modifiers = CUI_MOD_CTRL}));
        frame(ui, &f, o);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .pointer_button = b,
                                               .x = x,
                                               .y = y,
                                               .modifiers = CUI_MOD_CTRL}));
        frame(ui, &f, o);
        assert(clicks == button + 1 && clicked_button == b);
    }
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x + 20, .y = y}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + 20,
                                           .y = y}));
    frame(ui, &f, o);
    assert(clicks == 2 && !f.state.selecting);
    cui_destroy(ui);
}
static void test_theme_scroll_and_reveal(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state,
               text("first paragraph\n\nsecond paragraph\n\nthird paragraph\n\nlast paragraph")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.height = 40;
    o.scrollable = true;
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t n = 0; n < 4; ++n) {
        cui_text_view_style mapped = cui_text_view_style_from_theme(&themes[n]);
        assert(!memcmp(&mapped.link, &themes[n].primary, sizeof(cui_color)));
        assert(!memcmp(&mapped.table_background, &themes[n].surface, sizeof(cui_color)));
        assert(cui_begin_frame(ui, 400, 300, .016f));
        assert(cui_text_view(ui, 77, &themes[n], &f.state, o));
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && f.state.height > 40);
    }
    assert(cui_text_view_reveal(
               &f.state, (cui_text_view_range){f.state.text_length - 3, f.state.text_length - 1}) ==
           CUI_TEXT_VIEW_OK);
    frame(ui, &f, o);
    assert(f.state.scroll.y > 0 && !f.state.reveal_pending);
    float old = f.state.scroll.y;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = f.state.viewport.x + 10,
                                           .y = f.state.viewport.y + 10,
                                           .dy = 1,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.state.scroll.y < old);
    o.disabled = true;
    frame(ui, &f, o);
    assert(!f.state.selecting);
    cui_destroy(ui);
}
static char clipboard_text[512];
static bool clipboard_write(void *user, cui_str value) {
    assert(user == clipboard_text && value.length < sizeof clipboard_text);
    memcpy(clipboard_text, value.data, value.length);
    clipboard_text[value.length] = 0;
    return true;
}
static cui_id action_buttons[4];
static unsigned action_count, action_clicks;
static void code_controls(cui_context *ui, cui_id id, const cui_theme *theme, cui_str code,
                          cui_str language, void *user) {
    (void)user;
    assert(code.length == 1 && language.length == 1 && language.data[0] == 'c');
    assert(action_count < 4);
    cui_id button = id ^ UINT64_C(0x91831);
    action_buttons[action_count++] = button;
    if (cui_button_ex(ui, button, theme, text("Copy"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL}))
        ++action_clicks;
}
static void table_controls(cui_context *ui, cui_id id, const cui_theme *theme,
                           const cui_text_view_state *state, unsigned table, void *user) {
    (void)state;
    (void)table;
    (void)user;
    assert(action_count < 4);
    cui_id button = id ^ UINT64_C(0x91831);
    action_buttons[action_count++] = button;
    if (cui_button_ex(ui, button, theme, text("Save"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL}))
        ++action_clicks;
}
static void test_action_composition(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state,
               text("```c\na\n```\n\n```c\nb\n```\n\n| A | B |\n| --- | --- |\n| x | y |")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 200;
    o.code_actions = code_controls;
    o.table_actions = table_controls;
    action_count = 0;
    frame(ui, &f, o);
    assert(action_count == 3 && action_buttons[0] != action_buttons[1] &&
           action_buttons[1] != action_buttons[2]);
    cui_rect button = cui_bounds(ui, action_buttons[0]);
    assert(fabsf(button.y - (f.state.viewport.y + 8)) < .01f);
    assert(fabsf(button.x + button.w - (f.state.viewport.x + 192)) < .01f);
    assert(f.blocks[0].bounds.h == 44);
    cui_rect footer = cui_bounds(ui, action_buttons[2]);
    assert(fabsf(footer.y -
                 (f.state.viewport.y + f.blocks[5].bounds.y + f.blocks[5].bounds.h + 4)) < .01f);
    float x = button.x + button.w * .5f, y = button.y + button.h * .5f;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(!f.state.selecting);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(action_clicks == 1 && !f.state.selecting);
    cui_destroy(ui);
}
static void test_table_edges_and_clip(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("| A | [site](https://x.test) |\n| --- | --- |\n| x | y |")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.on_link = link_clicked;
    o.link_user = &clicks;
    cui_draw_list d = frame(ui, &f, o);
    bool internal_edge = false, cell_clip = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *c = &d.commands[i];
        if (c->type == CUI_DRAW_BORDER && c->custom_border_edges && c->border_edges.left == 0 &&
            c->border_edges.top == 0 && c->border_edges.right == 1 && c->border_edges.bottom == 1)
            internal_edge = true;
        if (c->type == CUI_DRAW_CLIP_PUSH && c->bounds.w == 64)
            cell_clip = true;
    }
    assert(internal_edge && cell_clip);
    unsigned before = clicks;
    float x = f.state.viewport.x + 110, y = f.state.viewport.y + 10;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    frame(ui, &f, o);
    assert(clicks == before && !f.state.selecting);
    cui_destroy(ui);
}
static void test_horizontal_table_interaction(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state,
                                  text("| abcdefghijklmnopqrstuvwxyz | [site](https://x.test) |\n| "
                                       "--- | --- |\n| x | y |")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    o.on_link = link_clicked;
    o.link_user = &clicks;
    frame(ui, &f, o);
    frame(ui, &f, o);
    assert(f.blocks[0].table_frame.w == 100 && f.blocks[0].table_track_width > 100);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        assert(semantics[i].role != CUI_ROLE_SCROLLBAR);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -40,
                                           .dy = -100,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -500,
                                           .dy = -1,
                                           .scroll_precise = true}));
    cui_draw_list d = frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x > 0);
    bool link_painted = false;
    float link_x = 0, link_y = 0;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == 4 &&
            !memcmp(d.commands[i].text.data, "site", 4)) {
            link_x = d.commands[i].bounds.x + 2;
            link_y = d.commands[i].bounds.y + 4;
            link_painted = link_x > 0 && link_x < 100;
        }
    assert(link_painted);
    unsigned before = clicks;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = link_x,
                                           .y = link_y}));
    frame(ui, &f, o);
    assert(f.state.anchor == f.spans[1].text.begin);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = link_x,
                                           .y = link_y}));
    frame(ui, &f, o);
    assert(clicks == before + 1 && !f.state.selecting);
    f.blocks[0].table_scroll.x = 0;
    assert(cui_text_view_reveal(&f.state, f.spans[1].text) == CUI_TEXT_VIEW_OK);
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x > 0 && !f.state.reveal_pending);
    float scroll = f.blocks[0].table_scroll.x;
    assert(cui_text_view_set_text(&f.state,
                                  text("| abcdefghijklmnopqrstuvwxyz | [site](https://x.test) |\n| "
                                       "--- | --- |\n| x | y |\n\nappend")) == CUI_TEXT_VIEW_OK);
    assert(f.blocks[0].table_scroll.x == scroll);
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == scroll);
    o.scrollable = true;
    o.height = 35;
    frame(ui, &f, o);
    frame(ui, &f, o);
    float horizontal = f.blocks[0].table_scroll.x;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -1,
                                           .dy = -2,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.state.scroll.y == 2 && f.blocks[0].table_scroll.x == horizontal);
    f.blocks[0].table_scroll.x = 0;
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -2,
                                           .dy = -1,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.state.scroll.y == 2 && f.blocks[0].table_scroll.x == 2);
    f.state.scroll.y = 0;
    f.blocks[0].table_scroll.x = f.blocks[0].table_track_width;
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.state.scroll.y == 0);
    o.width = 400;
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.state.scroll.y == 0 && f.blocks[0].table_scroll.x == 0);
    cui_destroy(ui);
}
static void test_passive_task_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("- [x] done\n- [ ] todo")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_terminal(),
                          cui_theme_high_contrast()};
    for (size_t theme = 0; theme < 4; ++theme) {
        assert(cui_begin_frame(ui, 400, 300, .016f));
        assert(cui_text_view(ui, 77, &themes[theme], &f.state, cui_text_view_default()));
        cui_draw_list d = cui_end_frame(ui);
        unsigned boxes = 0, checks = 0;
        for (size_t i = 0; i < d.count; ++i) {
            const cui_draw_command *c = &d.commands[i];
            if (c->type == CUI_DRAW_BORDER && c->bounds.w == 14 && c->bounds.h == 14) {
                ++boxes;
                float offset = fmaxf(
                    0, (cui_text_view_style_from_theme(&themes[theme]).line_height - 14) * .5f);
                assert(c->bounds.y == f.blocks[0].bounds.y + offset ||
                       c->bounds.y == f.blocks[1].bounds.y + offset);
            }
            if (c->type == CUI_DRAW_IMAGE) {
                ++checks;
                assert(c->bounds.w == 10 && c->bounds.h == 10 && c->bitmap.custom_tint);
            }
        }
        assert(boxes == 2 && checks == 1);
        size_t count;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        for (size_t i = 0; i < count; ++i)
            assert(semantics[i].role != CUI_ROLE_CHECKBOX && semantics[i].role != CUI_ROLE_BUTTON);
    }
    cui_destroy(ui);
}
static unsigned decoded_link_clicks, decoded_image_calls;
static void decoded_link(cui_str uri, const cui_event *event, void *user) {
    (void)user;
    assert(event->type == CUI_EVENT_POINTER_UP);
    assert(uri.length == strlen("https://x.test?a=1&b=2") &&
           !memcmp(uri.data, "https://x.test?a=1&b=2", uri.length));
    ++decoded_link_clicks;
}
static bool decoded_image(cui_str uri, cui_bitmap *bitmap, void *user) {
    (void)user;
    static const unsigned char pixel[] = {255, 0, 0, 255};
    assert(uri.length == strlen("asset?a=1&b=2") && !memcmp(uri.data, "asset?a=1&b=2", uri.length));
    *bitmap = (cui_bitmap){.pixels = pixel, .width = 1, .height = 1, .stride = 4, .revision = 1};
    ++decoded_image_calls;
    return true;
}
static void test_decoded_resource_callbacks(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_HTML);
    assert(cui_text_view_set_text(&f.state,
                                  text("<a href='https://x.test?a=1&amp;b=2'>site</a><img "
                                       "src='asset?a=1&amp;b=2' alt='alt'>")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.on_link = decoded_link;
    o.image = decoded_image;
    frame(ui, &f, o);
    assert(decoded_image_calls == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = 2,
                                           .y = 4}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = 2,
                                           .y = 4}));
    frame(ui, &f, o);
    assert(decoded_link_clicks == 1 && decoded_image_calls == 3);
    cui_destroy(ui);
}
static void test_nested_list_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("1. parent\n   4. child\n      2. grand")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_draw_list d = frame(ui, &f, cui_text_view_default());
    bool upper = false, lower = false;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == 3) {
            if (!memcmp(d.commands[i].text.data, "D. ", 3))
                upper = d.commands[i].bounds.x == 16;
            if (!memcmp(d.commands[i].text.data, "b. ", 3))
                lower = d.commands[i].bounds.x == 32;
        }
    assert(upper && lower);
    cui_destroy(ui);
}
static void test_list_continuation_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("- [x] first\n\n  second paragraph\n\noutside")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    cui_draw_list d = frame(ui, &f, o);
    unsigned checks = 0;
    bool continuation = false;
    for (size_t i = 0; i < d.count; ++i) {
        if (d.commands[i].type == CUI_DRAW_IMAGE)
            ++checks;
        if (d.commands[i].type == CUI_DRAW_TEXT &&
            d.commands[i].text.length == strlen("second paragraph") &&
            !memcmp(d.commands[i].text.data, "second paragraph", d.commands[i].text.length))
            continuation = d.commands[i].bounds.x == 16 && d.commands[i].bounds.y >= 20 &&
                           d.commands[i].bounds.y < 22;
    }
    assert(checks == 1 && continuation);
    cui_destroy(ui);
}
static void nested_code_controls(cui_context *ui, cui_id id, const cui_theme *theme, cui_str code,
                                 cui_str language, void *user) {
    (void)user;
    assert(code.length == 7 && !memcmp(code.data, "one\ntwo", 7));
    assert(language.length == 4 && !memcmp(language.data, "rust", 4));
    assert(action_count < 4);
    cui_id button = id ^ UINT64_C(0x91831);
    action_buttons[action_count++] = button;
    if (cui_button_ex(ui, button, theme, text("Copy"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL}))
        ++action_clicks;
}
static void test_first_list_table_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(
            &f.state, text("- | abcdefghijklmnopqrstuvwxyz | b |\n  | --- | --- |\n  | x | y |")) ==
        CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    cui_draw_list d = frame(ui, &f, o);
    bool marker = false, clip = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_TEXT && command->text.length == 4 &&
            !memcmp(command->text.data, "\xe2\x80\xa2 ", 4))
            marker = command->bounds.x == 0 && command->bounds.y < 2;
        if (command->type == CUI_DRAW_CLIP_PUSH && command->bounds.x == f.blocks[0].table_frame.x &&
            command->bounds.w == f.blocks[0].table_frame.w)
            clip = true;
    }
    assert(marker && clip && f.blocks[0].table_frame.x == f.blocks[0].list_prefix_width &&
           f.blocks[0].table_track_width > f.blocks[0].table_frame.w);
    frame(ui, &f, o);
    float x = f.blocks[0].table_frame.x + 4;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = x,
                                           .y = 10,
                                           .dx = -5,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED}));
    d = frame(ui, &f, o);
    marker = false;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == 4 &&
            !memcmp(d.commands[i].text.data, "\xe2\x80\xa2 ", 4))
            marker = d.commands[i].bounds.x == 0;
    assert(f.blocks[0].table_scroll.x == 5 && marker);
    assert(
        cui_text_view_set_text(&f.state, text("- [x] | a | b |\n  | --- | --- |\n  | x | y |")) ==
        CUI_TEXT_VIEW_OK);
    o.table_mode = CUI_TEXT_VIEW_TABLE_WRAP;
    o.width = 400;
    d = frame(ui, &f, o);
    unsigned checks = 0;
    for (size_t i = 0; i < d.count; ++i)
        checks += d.commands[i].type == CUI_DRAW_IMAGE;
    assert(checks == 1 && f.blocks[0].table_frame.x == 20 && f.blocks[0].bounds.x == 20 &&
           f.blocks[0].text_inset == 8);
    cui_destroy(ui);
}
static void test_nested_table_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state,
               text("- parent\n  - child\n\n  | abcdefghijklmnopqrstuvwxyz | "
                    "abcdefghijklmnopqrstuvwxyz |\n  | --- | --- |\n  | x | y |\n\n  after")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    o.table_actions = table_controls;
    action_count = 0;
    cui_draw_list d = frame(ui, &f, o);
    assert(action_count == 1 && f.blocks[2].table_frame.x == 16 && f.blocks[2].table_frame.w == 84);
    bool frame_clip = false;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_CLIP_PUSH && d.commands[i].bounds.x == 16 &&
            d.commands[i].bounds.w == 84)
            frame_clip = true;
    assert(frame_clip);
    cui_rect button = cui_bounds(ui, action_buttons[0]);
    assert(button.x >= 16 && button.x + button.w <= 100 &&
           button.y == f.blocks[2].table_frame.y + f.blocks[2].table_frame.h + 4);
    unsigned before = action_clicks;
    float x = button.x + button.w * .5f, y = button.y + button.h * .5f;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(action_clicks == before + 1);
    o.table_actions = NULL;
    o.scrollable = true;
    o.height = 50;
    frame(ui, &f, o);
    frame(ui, &f, o);
    f.state.scroll.y = f.blocks[2].table_frame.y;
    frame(ui, &f, o);
    float scroll_before = f.state.scroll.y;
    float mask_x = f.state.viewport.x + f.blocks[2].table_frame.x + 4;
    float mask_y = f.state.viewport.y + f.blocks[2].table_frame.y - scroll_before + 5;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = mask_x,
                                           .y = mask_y,
                                           .dx = -2,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED}));
    frame(ui, &f, o);
    assert(f.blocks[2].table_scroll.x == 2 && f.state.scroll.y == scroll_before);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = mask_x,
                                           .y = mask_y,
                                           .dy = -2,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED}));
    frame(ui, &f, o);
    assert(f.blocks[2].table_scroll.x == 2 && f.state.scroll.y == scroll_before + 2);
    cui_destroy(ui);
}
static void test_wrapped_link_decoration_ids(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("~~[abc](https://x.test)~~")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 9;
    o.on_link = link_clicked;
    o.link_user = &clicks;
    cui_draw_list d = frame(ui, &f, o);
    unsigned text_lines = 0, decorations = 0;
    for (size_t i = 0; i < d.count; ++i) {
        text_lines += d.commands[i].type == CUI_DRAW_TEXT;
        if (d.commands[i].type == CUI_DRAW_RECT && d.commands[i].bounds.h == 1)
            ++decorations;
    }
    assert(text_lines == 3 && decorations == 6 && f.blocks[0].bounds.h == 60);
    frame(ui, &f, o);
    unsigned before = clicks;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = 4,
                                           .y = 30}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = 4,
                                           .y = 30}));
    frame(ui, &f, o);
    assert(clicks == before + 1);
    cui_destroy(ui);
}
static void test_list_image_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(
            &f.state, text("10. ![alt](asset)\n    - child\n\n    ![second](asset)\n11. next")) ==
        CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    o.image = image_loaded;
    cui_draw_list d = frame(ui, &f, o);
    unsigned images = 0;
    bool first = false, continuation = false, marker = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_IMAGE) {
            ++images;
            assert(command->bitmap.bitmap.width == 2 && command->bitmap.bitmap.height == 1 &&
                   command->bounds.w == 2 && command->bounds.h == 1);
            assert(command->bitmap.bitmap.pixels[0] == 255 &&
                   command->bitmap.bitmap.pixels[5] == 255);
            first |= command->bounds.x == f.blocks[0].list_prefix_width && command->bounds.y == 0;
            continuation |= command->bounds.x == 16;
        }
        if (command->type == CUI_DRAW_TEXT && command->text.length == 4 &&
            !memcmp(command->text.data, "10. ", 4))
            marker = command->bounds.x == 0;
    }
    assert(images == 2 && first && continuation && marker && f.blocks[1].bounds.y == 20);
    o.image = NULL;
    d = frame(ui, &f, o);
    bool alt = false;
    images = 0;
    for (size_t i = 0; i < d.count; ++i) {
        images += d.commands[i].type == CUI_DRAW_IMAGE;
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == 3 &&
            !memcmp(d.commands[i].text.data, "alt", 3))
            alt = d.commands[i].bounds.x == f.blocks[0].list_prefix_width;
    }
    assert(!images && alt && !f.blocks[0].image_ready && f.blocks[3].list_number == 11);
    cui_destroy(ui);
}
static void test_quote_paragraph_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("> first  \n> second\n> third\n\noutside")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    cui_draw_list d = frame(ui, &f, o);
    bool first = false, continuation = false, border = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_TEXT && command->text.length == 5 &&
            !memcmp(command->text.data, "first", 5))
            first = command->bounds.x == 12 && command->bounds.y < 2;
        if (command->type == CUI_DRAW_TEXT && command->text.length == 12 &&
            !memcmp(command->text.data, "second third", 12))
            continuation =
                command->bounds.x == 12 && command->bounds.y >= 20 && command->bounds.y < 22;
        if (command->type == CUI_DRAW_RECT && command->bounds.x == 0 && command->bounds.w == 3 &&
            command->bounds.h == 40)
            border = true;
    }
    assert(first && continuation && border && f.blocks[1].bounds.y == 56);
    cui_destroy(ui);
}
static void test_list_nonparagraph_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("- ## heading\n  - child\n\n  > quote")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    cui_draw_list d = frame(ui, &f, o);
    bool heading = false, prefix = false, quote = false, quote_border = false;
    unsigned prefix_count = 0;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_TEXT && command->text.length == 7 &&
            !memcmp(command->text.data, "heading", 7))
            heading = command->font_size == 21 &&
                      command->text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD &&
                      command->bounds.x == f.blocks[0].list_prefix_width;
        if (command->type == CUI_DRAW_TEXT && command->text.length == 4 &&
            !memcmp(command->text.data, "\xe2\x80\xa2 ", 4)) {
            ++prefix_count;
            prefix = command->font_size == 14 && command->bounds.x == 0 &&
                     command->text_style.weight == CUI_TEXT_WEIGHT_NORMAL;
        }
        if (command->type == CUI_DRAW_TEXT && command->text.length == 5 &&
            !memcmp(command->text.data, "quote", 5))
            quote = command->bounds.x == 28;
        if (command->type == CUI_DRAW_RECT && command->bounds.x == 16 && command->bounds.w == 3)
            quote_border = true;
    }
    assert(heading && prefix && quote && quote_border && prefix_count == 1);
    cui_destroy(ui);
}
static void test_first_list_fence_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("- ```rust\n  one\n  two\n  ```\n- next")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 300;
    o.code_actions = nested_code_controls;
    action_count = 0;
    cui_draw_list d = frame(ui, &f, o);
    bool marker = false, code = false, background = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_TEXT && command->text.length == 4 &&
            !memcmp(command->text.data, "\xe2\x80\xa2 ", 4))
            marker |= command->bounds.x == 0 && command->bounds.y < 2;
        if (command->type == CUI_DRAW_TEXT && command->text.length == 3 &&
            !memcmp(command->text.data, "one", 3))
            code = command->text_style.monospace &&
                   command->bounds.x == f.blocks[0].list_prefix_width + 12 &&
                   command->bounds.y >= 12 && command->bounds.y < 20;
        if (command->type == CUI_DRAW_RECT && command->bounds.x == f.blocks[0].list_prefix_width &&
            command->bounds.y == 0 && command->bounds.w == 300 - f.blocks[0].list_prefix_width)
            background = true;
    }
    assert(marker && code && background && action_count == 1);
    assert(f.blocks[0].list_prefix_width > 0 && f.blocks[1].bounds.y == f.blocks[0].bounds.h);
    assert(cui_text_view_set_text(&f.state, text("- [x] ```rust\n  one\n  two\n  ```")) ==
           CUI_TEXT_VIEW_OK);
    action_count = 0;
    d = frame(ui, &f, o);
    unsigned checks = 0;
    for (size_t i = 0; i < d.count; ++i)
        checks += d.commands[i].type == CUI_DRAW_IMAGE;
    assert(checks == 1 && f.blocks[0].bounds.x == 20 && f.blocks[0].text_inset == 12);
    cui_destroy(ui);
}
static void test_nested_fenced_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(
            &f.state, text("- parent\n  - child\n\n  ```rust\n  one\n  two\n  ```\n\n  after")) ==
        CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    o.code_actions = nested_code_controls;
    action_count = 0;
    cui_draw_list d = frame(ui, &f, o);
    assert(action_count == 1 && f.blocks[2].bounds.x == 16 && f.blocks[3].bounds.x == 16);
    bool code = false;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text.length == 3 &&
            !memcmp(d.commands[i].text.data, "one", 3))
            code = d.commands[i].text_style.monospace && d.commands[i].bounds.x == 28;
    assert(code);
    cui_rect button = cui_bounds(ui, action_buttons[0]);
    assert(button.x >= 16 && button.x + button.w <= 400);
    unsigned clicks_before = action_clicks;
    float x = button.x + button.w * .5f, y = button.y + button.h * .5f;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(!f.state.selecting);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x,
                                           .y = y}));
    action_count = 0;
    frame(ui, &f, o);
    assert(action_clicks == clicks_before + 1 && !f.state.selecting);
    cui_destroy(ui);
}
static void test_nested_continuation_paint(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(
        cui_text_view_set_text(&f.state, text("- [x] parent\n  - child\n\n  resumed\n\n  final")) ==
        CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 400;
    cui_draw_list d = frame(ui, &f, o);
    unsigned checks = 0;
    bool resumed = false, final = false;
    for (size_t i = 0; i < d.count; ++i) {
        const cui_draw_command *command = &d.commands[i];
        if (command->type == CUI_DRAW_IMAGE)
            ++checks;
        if (command->type == CUI_DRAW_TEXT && command->text.length == 7 &&
            !memcmp(command->text.data, "resumed", 7))
            resumed = command->bounds.x == 20 && command->bounds.y >= 40 && command->bounds.y < 42;
        if (command->type == CUI_DRAW_TEXT && command->text.length == 5 &&
            !memcmp(command->text.data, "final", 5))
            final = command->bounds.x == 16 && command->bounds.y >= 60 && command->bounds.y < 62;
    }
    assert(checks == 2 && resumed && final);
    frame(ui, &f, o);
    cui_rect first = {0}, last = {0};
    for (size_t i = 0; i < f.state.glyph_count; ++i) {
        if (f.glyphs[i].text.begin == f.blocks[1].text.begin)
            first = f.glyphs[i].bounds;
        if (f.glyphs[i].text.end == f.blocks[2].text.end)
            last = f.glyphs[i].bounds;
    }
    float x = f.state.viewport.x, y = f.state.viewport.y;
    assert(first.w > 0 && last.w > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + first.x,
                                           .y = y + first.y + 2}));
    frame(ui, &f, o);
    assert(f.state.selecting && f.state.anchor == f.blocks[1].text.begin);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = x + last.x + last.w,
                                           .y = y + last.y + 2}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + last.x + last.w,
                                           .y = y + last.y + 2}));
    frame(ui, &f, o);
    assert(!f.state.selecting && f.state.cursor == f.blocks[2].text.end);
    char output[128];
    size_t length;
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_SOURCE, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "- [x] resumed\n  - child"));
    assert(cui_text_view_copy_selection(&f.state, CUI_TEXT_VIEW_COPY_PLAIN, output, sizeof output,
                                        &length) == CUI_TEXT_VIEW_OK);
    assert(!strcmp(output, "child\nresumed"));
    cui_destroy(ui);
}
static void test_horizontal_table_gesture(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(
               &f.state, text("| abcdefghijklmnopqrstuvwxyz | abcdefghijklmnopqrstuvwxyz |\n| --- "
                              "| --- |\n| x | y |\n\ntrailing paragraph")) == CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 100;
    o.height = 35;
    o.scrollable = true;
    o.table_mode = CUI_TEXT_VIEW_TABLE_NOWRAP;
    frame(ui, &f, o);
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_STARTED}));
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == 40 && f.state.scroll.y == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -10,
                                           .dy = -15,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_MOVED}));
    frame(ui, &f, o);
#ifndef __wasm__
    assert(f.blocks[0].table_scroll.x == 50 && f.state.scroll.y == 0);
#else
    assert(f.blocks[0].table_scroll.x == 40 && f.state.scroll.y == 15);
#endif
    float x = f.blocks[0].table_scroll.x;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -10,
                                           .dy = -25,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_MOVED}));
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == x && f.state.scroll.y >= 25);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .scroll_precise = true,
                                           .scroll_phase = CUI_SCROLL_PHASE_ENDED}));
    frame(ui, &f, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 20,
                                           .y = 10,
                                           .dx = -2,
                                           .dy = -1,
                                           .scroll_precise = true}));
    frame(ui, &f, o);
    assert(f.blocks[0].table_scroll.x == x + 2);
    cui_destroy(ui);
}
static void test_styled_selection(void) {
    fixture f;
    setup(&f, CUI_TEXT_VIEW_MARKDOWN);
    assert(cui_text_view_set_text(&f.state, text("**bold** *italic* `code` héllo")) ==
           CUI_TEXT_VIEW_OK);
    cui_context *ui = cui_create((cui_config){.max_elements = 512});
    assert(ui);
    cui_text_view_options o = cui_text_view_default();
    o.width = 300;
    cui_draw_list d = frame(ui, &f, o);
    bool bold = false, italic = false, mono = false;
    for (size_t i = 0; i < d.count; ++i) {
        bold |= d.commands[i].type == CUI_DRAW_TEXT &&
                d.commands[i].text_style.weight == CUI_TEXT_WEIGHT_BOLD;
        italic |= d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text_style.italic;
        mono |= d.commands[i].type == CUI_DRAW_TEXT && d.commands[i].text_style.monospace;
    }
    assert(bold && italic && mono);
    frame(ui, &f, o);
    cui_rect first = f.glyphs[0].bounds, last = f.glyphs[3].bounds;
    float x = f.state.viewport.x, y = f.state.viewport.y;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + first.x + 1,
                                           .y = y + first.y + 2}));
    frame(ui, &f, o);
    assert(f.state.selecting);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = x + last.x + last.w,
                                           .y = y + last.y + 2}));
    frame(ui, &f, o);
    assert(f.state.dragged && f.state.cursor > f.state.anchor);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + last.x + last.w,
                                           .y = y + last.y + 2}));
    frame(ui, &f, o);
    assert(!f.state.selecting && cui_focused(ui) == 77);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_A, .modifiers = CUI_MOD_CTRL}));
    frame(ui, &f, o);
    assert(f.state.anchor == 0 && f.state.cursor == f.state.text_length);
    char copy_buffer[512];
    cui_platform platform = {.user = clipboard_text,
                             .capabilities = CUI_PLATFORM_CLIPBOARD,
                             .clipboard_set = clipboard_write};
    o.clipboard = &platform;
    o.copy_format = CUI_TEXT_VIEW_COPY_SOURCE;
    o.copy_buffer = copy_buffer;
    o.copy_capacity = sizeof copy_buffer;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_C, .modifiers = CUI_MOD_CTRL}));
    frame(ui, &f, o);
    assert(!strcmp(clipboard_text, f.source));
    assert(cui_text_view_select(&f.state, 1, 3));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_C, .modifiers = CUI_MOD_CTRL}));
    frame(ui, &f, o);
    assert(!strcmp(clipboard_text, "**ol**"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    frame(ui, &f, o);
    assert(f.state.anchor == f.state.cursor);
    o.max_lines = 1;
    o.width = 45;
    frame(ui, &f, o);
    assert(f.state.clamped);
    o.width = NAN;
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme theme = cui_theme_light();
    assert(!cui_text_view(ui, 77, &theme, &f.state, o));
    (void)cui_end_frame(ui);
    cui_destroy(ui);
}
#endif
int main(void) {
    test_image_font_metrics();
    test_markdown();
    test_unicode_ranges_and_stream();
    test_html();
    test_source_copy();
    test_proportional_table_defaults();
    test_adaptive_table_model();
    test_task_list_model();
    test_resource_decoding();
    test_nested_list_model();
    test_list_continuations();
    test_setext_headings();
    test_nested_list_continuations();
    test_nested_fenced_continuation();
    test_first_list_fence();
    test_list_nonparagraph_blocks();
    test_quote_paragraph_lines();
    test_list_image_structure();
    test_nested_table_structure();
    test_first_list_table();
    test_fenced_code_content();
    test_table_copy_alignment();
    test_paragraphs_and_numbered_markers();
    test_bounded_parser();
    test_table_highlight_edits_and_nested_html();
#ifndef CUI_TEXT_VIEW_BASE_ONLY
    test_styled_selection();
    test_image_metric_paint();
    test_links_and_images();
    test_theme_scroll_and_reveal();
    test_action_composition();
    test_table_edges_and_clip();
    test_horizontal_table_interaction();
    test_passive_task_paint();
    test_decoded_resource_callbacks();
    test_nested_list_paint();
    test_list_continuation_paint();
    test_nested_continuation_paint();
    test_nested_fenced_paint();
    test_first_list_fence_paint();
    test_list_nonparagraph_paint();
    test_quote_paragraph_paint();
    test_list_image_paint();
    test_wrapped_link_decoration_ids();
    test_nested_table_paint();
    test_first_list_table_paint();
    test_horizontal_table_gesture();
#endif
    return 0;
}
