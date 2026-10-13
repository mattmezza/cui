/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include "cui/cui_textarea.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <hb-ft.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Audit executable: currently returns 1 for confirmed integration gaps, 77
 * when native bidi or an actual supported optional fixture is unavailable. */
static unsigned gaps;
static void comparison(bool equal, const char *message) {
    if (!equal) {
        fprintf(stderr, "GAP: %s\n", message);
        ++gaps;
    }
}
static const char *font_path(const char *env, const char *const *paths, size_t count) {
    const char *requested = getenv(env);
    if (requested) {
        cui_font *font = cui_font_open(requested);
        assert(font);
        cui_font_close(font);
        return requested;
    }
    for (size_t i = 0; i < count; ++i) {
        cui_font *font = cui_font_open(paths[i]);
        if (font) {
            cui_font_close(font);
            return paths[i];
        }
    }
    return NULL;
}
static bool metrics(void *user, float size, cui_text_style style, cui_font_metrics *out) {
    return cui_font_get_metrics(user, size, style, out);
}
typedef struct {
    uint64_t hash;
    unsigned glyphs, pixels;
} raster;
static void glyph(void *user, int x, int y, int width, int height, int pitch,
                  const unsigned char *data) {
    raster *r = user;
    ++r->glyphs;
    r->hash = r->hash * UINT64_C(1099511628211) + (unsigned)x;
    r->hash = r->hash * UINT64_C(1099511628211) + (unsigned)y;
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) {
            r->hash = r->hash * UINT64_C(1099511628211) + data[row * pitch + col];
            r->pixels += data[row * pitch + col] != 0;
        }
}
static cui_text_layout *layout(cui_font *font, cui_str text, float size, size_t start,
                               size_t count) {
    cui_text_layout *result = NULL;
    assert(cui_font_layout_line(font, text, size, (cui_text_style){0}, CUI_BIDI_AUTO_LTR, start,
                                count, NULL, &result) == CUI_TEXT_LAYOUT_OK);
    return result;
}
static void hb_reference(cui_font *font, const char *primary, const char *fallback, cui_str text) {
    cui_text_layout *line = layout(font, text, 14, 0, SIZE_MAX);
    cui_text_layout_view view;
    assert(cui_text_layout_get(line, &view) && view.run_count == 1 && (view.runs[0].level & 1));
    FT_Library library;
    FT_Face face;
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library, view.runs[0].face_index ? fallback : primary, 0, &face));
    assert(!FT_Set_Char_Size(face, 0, 14 * 64, 72, 72));
    hb_font_t *hb = hb_ft_font_create_referenced(face);
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_direction(buffer, HB_DIRECTION_RTL);
    hb_buffer_set_script(buffer, HB_SCRIPT_HEBREW);
    hb_buffer_set_language(buffer, hb_language_from_string("und", -1));
    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
    hb_buffer_set_flags(buffer, HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES | HB_BUFFER_FLAG_BOT |
                                    HB_BUFFER_FLAG_EOT);
    hb_buffer_add_utf8(buffer, text.data, (int)text.length, 0, (int)text.length);
    hb_shape(hb, buffer, NULL, 0);
    unsigned count;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t *position = hb_buffer_get_glyph_positions(buffer, NULL);
    assert(count == view.glyph_count);
    float pen = 0;
    for (unsigned i = 0; i < count; ++i) {
        assert(info[i].codepoint && info[i].codepoint == view.glyphs[i].glyph_id);
        assert(info[i].cluster == view.glyphs[i].cluster_byte);
        assert(fabsf(view.glyphs[i].x - pen - (float)position[i].x_offset / 64) < .01f);
        pen += (float)position[i].x_advance / 64;
    }
    assert(fabsf(pen - view.width) < .01f);
    raster actual = {0}, reference = {0};
    assert(cui_font_draw(font, text, 14, 0, 0, glyph, &actual));
    assert(cui_text_layout_draw(line, 0, 0, glyph, &reference));
    assert(actual.hash == reference.hash && actual.glyphs == count && actual.pixels);
    printf(
        "native Hebrew drawing verified against HarfBuzz and actual nonzero raster (%u pixels)\n",
        actual.pixels);
    hb_buffer_destroy(buffer);
    hb_font_destroy(hb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    cui_text_layout_destroy(line);
}
static void caret_audit(cui_context *ui, cui_font *font) {
    char buffer[64] = "אבג";
    cui_str text = cui_string(buffer);
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    assert(edit);
    assert(cui_edit_select_atomic(edit, 0, 0) == CUI_EDIT_OK);
    cui_textarea_line rows[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, rows, 16));
    cui_theme theme = cui_theme_light();
    cui_textarea_options options = {.width = 280, .no_wrap = true};
    for (unsigned frame = 0; frame < 2; ++frame) {
        if (frame)
            cui_focus(ui, 50);
        assert(cui_begin_frame(ui, 600, 400, .016f));
        assert(
            cui_textarea(ui, 50, &theme, cui_string("RTL editor"), edit, &state, options).error ==
            CUI_TEXTAREA_OK);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_text_layout *line = layout(font, text, state.font_size, 0, SIZE_MAX);
    cui_text_layout_view view;
    assert(cui_text_layout_get(line, &view));
    cui_text_caret caret;
    size_t count;
    assert(cui_text_layout_carets(line, 0, &caret, 1, &count) == CUI_TEXT_LAYOUT_OK && count == 1);
    float editor_x = cui_textarea_row_x(ui, edit, &state, 0, 0);
    printf("RTL byte 0 caret: editor %.3f, shaped %.3f\n", editor_x, caret.x);
    comparison(fabsf(editor_x - caret.x) < .01f,
               "Textarea byte0 caret uses logical prefix width instead of RTL visual edge");
    cui_rect ime;
    assert(cui_text_input_rect(ui, &ime));
    assert(fabsf(ime.x - state.viewport.x - editor_x + state.scroll_x) < .01f);
    cui_input_hit_geometry hit = {
        .ui = ui, .font_size = state.font_size, .viewport = {0, 0, 280, 20}};
    size_t input_byte = cui_base_input_hit_test(&hit, text, 0, 0);
    size_t textarea_byte = cui_textarea_row_offset(ui, edit, &state, 0, 0);
    assert(cui_text_layout_hit_test(line, 0, 0, CUI_TEXT_UPSTREAM, &caret) == CUI_TEXT_LAYOUT_OK);
    printf("RTL left edge: Input byte %zu, Textarea byte %zu, shaped byte %zu\n", input_byte,
           textarea_byte, caret.byte_offset);
    comparison(input_byte == caret.byte_offset && textarea_byte == caret.byte_offset,
               "Textarea visual left hit maps to logical start instead of RTL end");
    assert(cui_edit_select_atomic(edit, 0, 2) == CUI_EDIT_OK);
    theme.selection = (cui_color){17, 33, 49, 255};
    assert(cui_begin_frame(ui, 600, 400, .016f));
    assert(cui_textarea(ui, 50, &theme, cui_string("RTL editor"), edit, &state, options).error ==
           CUI_TEXTAREA_OK);
    cui_draw_list painted = cui_end_frame(ui);
    const cui_draw_command *highlight = NULL;
    for (size_t i = 0; i < painted.count; ++i)
        if (painted.commands[i].type == CUI_DRAW_RECT &&
            !memcmp(&painted.commands[i].color, &theme.selection, sizeof theme.selection))
            highlight = &painted.commands[i];
    assert(highlight);
    cui_text_selection_rect selected;
    assert(cui_text_layout_selection(line, 0, 2, &selected, 1, &count) == CUI_TEXT_LAYOUT_OK &&
           count == 1);
    float painted_x = highlight->bounds.x - state.viewport.x + state.scroll_x;
    printf("RTL first-character selection: widget x%.3f, shaped x%.3f\n", painted_x,
           selected.bounds.x);
    comparison(
        fabsf(painted_x - selected.bounds.x) < .01f,
        "Textarea selection highlight occupies logical prefix side rather than selected RTL glyph");
    cui_text_layout_destroy(line);
    cui_edit_destroy(edit);
}
static void wrap_audit(cui_context *ui, cui_font *font) {
    cui_str paragraph = cui_string("אבג 123 !");
    float width = cui_font_measure(font, cui_string("123 !"), 24).width + 1;
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_begin_box(ui, 80, (cui_box){.width = width, .fixed_width = true});
    cui_text(ui, paragraph, 24, (cui_color){20, 40, 60, 255});
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    const cui_draw_command *wrapped = NULL;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 5 &&
            !memcmp(draw.commands[i].text.data, "123 !", 5))
            wrapped = &draw.commands[i];
    assert(wrapped && cui_last_error(ui) == CUI_OK);
    cui_text_layout *contextual = layout(font, paragraph, 24, 4, 5);
    cui_text_layout *standalone = layout(font, wrapped->text, 24, 0, SIZE_MAX);
    cui_text_layout_view expected, actual;
    assert(cui_text_layout_get(contextual, &expected) && cui_text_layout_get(standalone, &actual));
    raster renderer = {0}, independent = {0}, reference = {0};
    /* SDL/GPU/Sokol all invoke this same native draw entry point per command. */
    assert(cui_font_draw_styled(font, wrapped->text, wrapped->font_size, wrapped->text_style, 0, 0,
                                glyph, &renderer));
    assert(cui_text_layout_draw(standalone, 0, 0, glyph, &independent));
    assert(cui_text_layout_draw(contextual, 0, 0, glyph, &reference));
    assert(renderer.hash == independent.hash && renderer.pixels && reference.pixels);
    printf("Core wrapped '123 !': standalone base %u, paragraph base %u, raster %s\n",
           actual.paragraph_level, expected.paragraph_level,
           renderer.hash == reference.hash ? "equal" : "different");
    comparison(
        renderer.hash == reference.hash,
        "Core native wrapped command resets paragraph bidi context and repositions punctuation");
    cui_text_layout_destroy(contextual);
    cui_text_layout_destroy(standalone);
}
static void input_case(cui_context *ui, cui_font *font, const char *sample) {
    char buffer[128];
    assert(strlen(sample) < sizeof buffer);
    strcpy(buffer, sample);
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit);
    assert(cui_edit_select_atomic(edit, 0, 0) == CUI_EDIT_OK);
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    theme.selection = (cui_color){17, 33, 49, 255};
    cui_input_options options = {.width = 280};
    for (unsigned frame = 0; frame < 2; ++frame) {
        if (frame)
            cui_focus(ui, 60);
        assert(cui_begin_frame(ui, 600, 400, .016f));
        assert(cui_input_ex(ui, 60, &theme, cui_string("RTL input"), edit, &state, options).error ==
               CUI_EDIT_OK);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_input_metrics input_metrics;
    assert(cui_input_measure(options, theme.font_size, &input_metrics));
    cui_text_layout *line = layout(font, cui_string(buffer), input_metrics.font_size, 0, SIZE_MAX);
    cui_text_caret edge;
    size_t count;
    assert(cui_text_layout_carets(line, 0, &edge, 1, &count) == CUI_TEXT_LAYOUT_OK);
    cui_rect viewport = cui_bounds(ui, cui_input_viewport_id(60)), ime;
    assert(cui_text_input_rect(ui, &ime));
    assert(fabsf(ime.x - viewport.x + state.scroll_x - edge.x) < .01f);
    cui_input_hit_geometry measured = {.ui = ui,
                                       .viewport = viewport,
                                       .font_size = input_metrics.font_size,
                                       .scroll_x = state.scroll_x};
    assert(cui_text_layout_hit_test(line, 0, 0, CUI_TEXT_DOWNSTREAM, &edge) == CUI_TEXT_LAYOUT_OK);
    size_t expected_hit = edge.byte_offset;
    assert(cui_base_input_hit_test(&measured, cui_string(buffer), viewport.x, viewport.y) ==
           expected_hit);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN, .x = viewport.x + .01f, .y = viewport.y + 2};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
    assert(cui_begin_frame(ui, 600, 400, .016f));
    assert(cui_input_ex(ui, 60, &theme, cui_string("RTL input"), edit, &state, options).error ==
           CUI_EDIT_OK);
    cui_end_frame(ui);
    assert(cui_edit_get_state(edit).cursor == expected_hit);
    if (!strcmp(sample, "abc אבג")) {
        cui_text_caret edges[2];
        size_t edge_count;
        assert(cui_text_layout_carets(line, 4, edges, 2, &edge_count) == CUI_TEXT_LAYOUT_OK &&
               edge_count == 2);
        assert(edges[0].x != edges[1].x);
        for (size_t e = 0; e < edge_count; ++e) {
            cui_text_caret chosen;
            assert(cui_text_layout_hit_test(line, edges[e].x, 0, CUI_TEXT_DOWNSTREAM, &chosen) ==
                   CUI_TEXT_LAYOUT_OK);
            assert(chosen.byte_offset == 4);
            viewport = cui_bounds(ui, cui_input_viewport_id(60));
            event = (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                .x = viewport.x + edges[e].x - state.scroll_x,
                                .y = viewport.y + 2};
            assert(cui_queue_event(ui, event));
            event.type = CUI_EVENT_POINTER_UP;
            assert(cui_queue_event(ui, event));
            for (unsigned idle = 0; idle < 2; ++idle) {
                assert(cui_begin_frame(ui, 600, 400, .016f));
                assert(cui_input_ex(ui, 60, &theme, cui_string("RTL input"), edit, &state, options)
                           .error == CUI_EDIT_OK);
                cui_end_frame(ui);
                viewport = cui_bounds(ui, cui_input_viewport_id(60));
                assert(cui_text_input_rect(ui, &ime));
                assert(cui_edit_get_state(edit).cursor == 4);
                assert(fabsf(ime.x - viewport.x + state.scroll_x - chosen.x) < .01f);
            }
        }
    }
    cui_text_layout_view view;
    assert(cui_text_layout_get(line, &view));
    size_t selection_end = 0;
    for (size_t i = 0; i < view.cluster_count; ++i)
        if (!view.clusters[i].byte_start)
            selection_end = view.clusters[i].byte_end;
    if (!strcmp(sample, "abc אבג 123"))
        selection_end = 10;
    assert(selection_end && cui_edit_select_atomic(edit, 0, selection_end) == CUI_EDIT_OK);
    assert(cui_begin_frame(ui, 600, 400, .016f));
    assert(cui_input_ex(ui, 60, &theme, cui_string("RTL input"), edit, &state, options).error ==
           CUI_EDIT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    cui_text_selection_rect selected[32];
    assert(cui_text_layout_selection(line, 0, selection_end, selected, 32, &count) ==
               CUI_TEXT_LAYOUT_OK &&
           count);
    viewport = cui_bounds(ui, cui_input_viewport_id(60));
    size_t painted_count = 0;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            !memcmp(&draw.commands[i].color, &theme.selection, sizeof theme.selection)) {
            assert(painted_count < count);
            assert(fabsf(draw.commands[i].bounds.x - viewport.x + state.scroll_x -
                         selected[painted_count].bounds.x) < .01f);
            assert(fabsf(draw.commands[i].bounds.w - selected[painted_count].bounds.w) < .01f);
            ++painted_count;
        }
    assert(painted_count == count);
    raster actual = {0};
    assert(cui_font_draw(font, cui_string(buffer), input_metrics.font_size, 0, 0, glyph, &actual));
    assert(actual.pixels);
    printf("Input geometry and actual raster passed: %s\n", sample);
    cui_text_layout_destroy(line);
    cui_edit_destroy(edit);
}

int main(int argc, char **argv) {
    bool audit = argc == 2 && !strcmp(argv[1], "--audit");
    if (argc > 1 && !audit) {
        fprintf(stderr, "usage: %s [--audit]\n", argv[0]);
        return 2;
    }
    const char *const primary_paths[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                         "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *const hebrew_paths[] = {"/usr/share/fonts/noto/NotoSansHebrew-Regular.ttf",
                                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                        "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *primary = font_path("CUI_TEST_FONT", primary_paths, 3);
    const char *hebrew = font_path("CUI_TEST_HEBREW_FONT", hebrew_paths, 3);
    if (!primary || !hebrew) {
        puts("SKIP: set supported CUI_TEST_FONT/CUI_TEST_HEBREW_FONT fixtures");
        return 77;
    }
    cui_font *font = cui_font_open(primary), *fallback = cui_font_open(hebrew);
    assert(font && fallback);
    const char *const arabic_paths[] = {"/usr/share/fonts/noto/NotoSansArabic-Regular.ttf",
                                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                        "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    const char *arabic_path = font_path("CUI_TEST_ARABIC_FONT", arabic_paths, 3);
    cui_font *arabic = arabic_path ? cui_font_open(arabic_path) : NULL;
    cui_font *chain[] = {fallback, arabic};
    assert(cui_font_set_fallbacks(font, chain, arabic ? 2 : 1));
    cui_text_layout *probe = NULL;
    cui_text_layout_status status =
        cui_font_layout_line(font, cui_string("אבג 123 !"), 24, (cui_text_style){0},
                             CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL, &probe);
    if (status == CUI_TEXT_LAYOUT_UNAVAILABLE || status == CUI_TEXT_LAYOUT_MISSING_GLYPH) {
        puts("SKIP: native bidi disabled or selected faces lack actual sample glyphs");
        cui_font_close(font);
        cui_font_close(fallback);
        cui_font_close(arabic);
        return 77;
    }
    assert(status == CUI_TEXT_LAYOUT_OK);
    cui_text_layout_destroy(probe);
    printf("font fixtures: primary %s, registered Hebrew fallback %s\n", primary, hebrew);
    hb_reference(font, primary, hebrew, cui_string("אבג"));
    cui_context *ui = cui_create((cui_config){.width = 600,
                                              .height = 400,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .font_metrics = metrics,
                                              .measure_user = font,
                                              .text_geometry = cui_font_text_geometry});
    assert(ui);
    input_case(ui, font, "אבג");
    input_case(ui, font, "שָׁלוֹם");
    input_case(ui, font, "abc אבג 123");
    input_case(ui, font, "abc אבג");
    if (arabic) {
        cui_text_layout *arabic_probe = NULL;
        if (cui_font_layout_line(font, cui_string("السَّلَامُ"), 14, (cui_text_style){0},
                                 CUI_BIDI_AUTO_LTR, 0, SIZE_MAX, NULL,
                                 &arabic_probe) == CUI_TEXT_LAYOUT_OK) {
            cui_text_layout_destroy(arabic_probe);
            input_case(ui, font, "السَّلَامُ");
        } else
            puts("Arabic Input geometry untested: selected fixtures lack actual glyphs");
    } else
        puts("Arabic Input geometry untested: set CUI_TEST_ARABIC_FONT");
    if (audit) {
        caret_audit(ui, font);
        wrap_audit(ui, font);
    }
    cui_destroy(ui);
    cui_font_close(font);
    cui_font_close(fallback);
    cui_font_close(arabic);
    fprintf(stderr, "native text integration audit: %u observed gap(s)\n", gaps);
    return gaps ? 1 : 0;
}
