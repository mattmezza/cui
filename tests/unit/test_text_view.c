/* SPDX-License-Identifier: MIT */
#include "cui/cui_text_view.h"
#include <assert.h>
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
    assert(f.blocks[1].text_inset == 29 && f.blocks[2].text_inset == 29);
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
    assert(f.glyphs[0].bounds.x == 12);
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
    test_markdown();
    test_unicode_ranges_and_stream();
    test_html();
    test_source_copy();
    test_table_copy_alignment();
    test_paragraphs_and_numbered_markers();
    test_bounded_parser();
#ifndef CUI_TEXT_VIEW_BASE_ONLY
    test_styled_selection();
    test_links_and_images();
    test_theme_scroll_and_reveal();
#endif
    return 0;
}
