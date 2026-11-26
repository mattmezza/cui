/* SPDX-License-Identifier: MIT */
#include "cui/cui_kbd.h"
#include "cui/cui_software.h"
#include "cui_kbd_uppercase_cases.inc"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static void formats(void) {
    const struct {
        const char *key, *normal, *mac;
    } names[] = {{"a", "A", "A"},
                 {"f12", "F12", "F12"},
                 {"ctrl", "Ctrl", "⌃"},
                 {"alt", "Alt", "⌥"},
                 {"shift", "Shift", "⇧"},
                 {"cmd", "Win", "⌘"},
                 {"space", "Space", "Space"},
                 {"backspace", "Backspace", "⌫"},
                 {"delete", "Delete", "⌫"},
                 {"escape", "Esc", "⎋"},
                 {"enter", "Enter", "⏎"},
                 {"pagedown", "Page Down", "Page Down"},
                 {"pageup", "Page Up", "Page Up"},
                 {"left", "Left", "←"},
                 {"right", "Right", "→"},
                 {"up", "Up", "↑"},
                 {"down", "Down", "↓"},
                 {"-", "-", "-"},
                 {"+", "+", "+"},
                 {"éclair", "Éclair", "Éclair"},
                 {"ßeta", "SSeta", "SSeta"},
                 {"ΐé", "Ϊ́é", "Ϊ́é"},
                 {"𐐨z", "𐐀z", "𐐀z"},
                 {"😀é", "😀é", "😀é"}};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
        for (unsigned mac = 0; mac < 2; mac++) {
            char output[160];
            const char *expected = mac ? names[i].mac : names[i].normal;
            cui_kbd_stroke s = {.key = cui_string(names[i].key)};
            CHECK(cui_kbd_format_stroke(s, mac != 0, output, sizeof output) == strlen(expected));
            CHECK(!strcmp(output, expected));
        }
    for (unsigned mac = 0; mac < 2; mac++) {
        cui_kbd_stroke stroke = {.modifiers =
                                     CUI_MOD_CTRL | CUI_MOD_ALT | CUI_MOD_SHIFT | CUI_MOD_META,
                                 .key = CUI_STR("enter")};
        const char *expected = mac ? "⌃⌥⇧⌘⏎" : "Ctrl+Alt+Shift+Win+Enter";
        size_t required = strlen(expected);
        CHECK(cui_kbd_format_stroke(stroke, mac != 0, NULL, 0) == required);
        for (size_t capacity = 0; capacity <= required + 3; capacity++) {
            unsigned char storage[80];
            memset(storage, 0xa5, sizeof storage);
            char *out = (char *)storage + 1;
            CHECK(cui_kbd_format_stroke(stroke, mac != 0, out, capacity) == required);
            CHECK(storage[0] == 0xa5 && storage[capacity + 1] == 0xa5);
            if (capacity) {
                size_t length = strlen(out);
                CHECK(length < capacity && length <= required);
                CHECK(!memcmp(out, expected, length));
                CHECK(cui_text_line_source_valid(
                    (cui_text_line_source){.paragraph = {out, length}, .byte_end = length}));
            } else
                CHECK(storage[1] == 0xa5);
        }
        stroke.key = (cui_str)CUI_STR("ΐé");
        expected = mac ? "⌃⌥⇧⌘Ϊ́é" : "Ctrl+Alt+Shift+Win+Ϊ́é";
        required = strlen(expected);
        CHECK(cui_kbd_format_stroke(stroke, mac != 0, NULL, 10) == required);
        for (size_t capacity = 1; capacity <= required + 1; ++capacity) {
            char storage[40];
            memset(storage, '!', sizeof storage);
            memcpy(storage, stroke.key.data, stroke.key.length);
            cui_kbd_stroke borrowed = {.modifiers = stroke.modifiers,
                                       .key = {storage, stroke.key.length}};
            CHECK(cui_kbd_format_stroke(borrowed, mac != 0, storage, capacity) == required);
            size_t written = capacity - 1;
            while (((unsigned char)expected[written] & 0xc0) == 0x80)
                --written;
            CHECK(strlen(storage) == written && !memcmp(storage, expected, written));
            if (capacity >= stroke.key.length)
                CHECK(storage[capacity] == '!');
        }
    }
    char invalid[160] = "untouched", long_key[129];
    memset(long_key, 'x', sizeof long_key);
    cui_kbd_stroke bad[] = {{.key = {NULL, 2}},
                            {.key = {"", 0}},
                            {.key = {long_key, sizeof long_key}},
                            {.modifiers = 1u << 30, .key = CUI_STR("a")},
                            {.key = {"a\0b", 3}},
                            {.key = CUI_STR("a\nb")},
                            {.key = {"\xed\xa0\x80", 3}},
                            {.key = {"\xe2\x80\xa8", 3}},
                            {.key = CUI_STR("\t")}};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        CHECK(cui_kbd_format_stroke(bad[i], false, invalid, sizeof invalid) == SIZE_MAX);
        CHECK(!strcmp(invalid, "untouched"));
    }
    char overlap[160] = "enter";
    CHECK(cui_kbd_format_stroke((cui_kbd_stroke){.key = {overlap, 5}}, true, overlap,
                                sizeof overlap) == 3);
    CHECK(!strcmp(overlap, "⏎"));
    char longest[128];
    memset(longest, 'x', sizeof longest);
    CHECK(cui_kbd_format_stroke((cui_kbd_stroke){.modifiers = 15, .key = {longest, 128}}, false,
                                invalid, sizeof invalid) == 147);
    memcpy(longest, "ΐ", 2);
    CHECK(cui_kbd_format_stroke((cui_kbd_stroke){.modifiers = 15, .key = {longest, 128}}, false,
                                invalid, sizeof invalid) == 151);
    CHECK(strlen(invalid) == 151 && !memcmp(invalid + 19, "Ϊ́", 6));
    char legacy[160];
    CHECK(cui_kbd_format(0, cui_string("escape"), false, legacy, sizeof legacy) == 6);
    CHECK(!strcmp(legacy, "escape")); /* additive formatting, old helper unchanged */
    CHECK(cui_kbd_format(0, cui_string("ßé"), false, legacy, sizeof legacy) == 4);
    CHECK(!strcmp(legacy, "ßé"));
}

static void unicode_mappings(void) {
    for (size_t i = 0; i < sizeof kbd_uppercase_cases / sizeof kbd_uppercase_cases[0]; ++i) {
        char key[8], expected[8], output[8];
        /* A lowercase suffix catches accidental uppercasing of the entire key. */
        snprintf(key, sizeof key, "%sx", kbd_uppercase_cases[i].key);
        snprintf(expected, sizeof expected, "%sx", kbd_uppercase_cases[i].upper);
        cui_kbd_stroke stroke = {.key = cui_string(key)};
        CHECK(cui_kbd_format_stroke(stroke, false, output, sizeof output) == strlen(expected));
        CHECK(!strcmp(output, expected));
    }
}
static const cui_draw_command *find(cui_draw_list d, cui_draw_type type) {
    for (size_t i = 0; i < d.count; i++)
        if (d.commands[i].type == type)
            return &d.commands[i];
    return NULL;
}
static cui_context *create(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 160});
    CHECK(ui);
    return ui;
}
static cui_draw_list hint(cui_context *ui, cui_theme theme, cui_kbd_options o) {
    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    CHECK(cui_kbd_ex(ui, 10, &theme,
                     (cui_kbd_stroke){.modifiers = CUI_MOD_CTRL, .key = CUI_STR("a")}, o));
    cui_draw_list draw = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void pixel(cui_draw_list draw, cui_color color) {
    cui_draw_command commands[64];
    cui_draw_list copy = {.commands = commands, .width = 320, .height = 160};
    CHECK(draw.count <= 64);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *software = cui_software_create(320, 160, NULL);
    CHECK(software && cui_software_render(software, &copy, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *p = cui_software_pixels(software, &w, &h);
    CHECK(w == 320 && h == 160);
    CHECK(p[(7 * 320 + 7) * 4] == color.r && p[(7 * 320 + 7) * 4 + 1] == color.g &&
          p[(7 * 320 + 7) * 4 + 2] == color.b);
    cui_software_destroy(software);
}
static void render(void) {
    cui_context *ui = create();
    for (unsigned dark = 0; dark < 2; dark++) {
        cui_theme t = dark ? cui_theme_dark() : cui_theme_light();
        for (unsigned mode = 0; mode < 3; mode++) {
            cui_kbd_options o = {.plain = mode == 2, .outline = mode != 0};
            cui_draw_list d = hint(ui, t, o);
            const cui_draw_command *text = find(d, CUI_DRAW_TEXT);
            CHECK(text);
            CHECK(text->text.length == 6 && !memcmp(text->text.data, "Ctrl+A", 6));
            CHECK(text->font_size == (mode == 2 ? t.font_size : 12));
            cui_color foreground = mode == 2 ? t.text : t.muted;
            CHECK(!memcmp(&text->color, &foreground, sizeof foreground));
            if (mode == 2) {
                CHECK(!find(d, CUI_DRAW_RECT));
                CHECK(!find(d, CUI_DRAW_BORDER));
            } else {
                cui_color bg = mode == 1 ? t.background : t.muted_background;
                CHECK(find(d, CUI_DRAW_RECT));
                pixel(d, bg);
                CHECK((find(d, CUI_DRAW_BORDER) != NULL) == (mode == 1));
                CHECK(cui_bounds(ui, 10).h == (mode == 1 ? 18 : 16));
            }
            size_t n;
            const cui_semantic *s = cui_semantics(ui, &n);
            CHECK(n == 1 && s[0].role == CUI_ROLE_LABEL && s[0].tab_stop_disabled);
            CHECK(s[0].name.length == 6 && !memcmp(s[0].name.data, "Ctrl+A", 6));
            cui_rect bounds = cui_bounds(ui, 10);
            CHECK(!memcmp(&s[0].bounds, &bounds, sizeof bounds));
        }
        t.font_size = 32;
        cui_draw_list d = hint(ui, t, (cui_kbd_options){0});
        CHECK(find(d, CUI_DRAW_TEXT)->font_size == 24 && cui_bounds(ui, 10).h == 32);
    }
    cui_theme t = cui_theme_light();
    hint(ui, t, (cui_kbd_options){.mac_symbols = true, .accessible_name = CUI_STR("Control A")});
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    CHECK(n == 1 && s[0].name.length == 9);
    cui_rect hit = cui_bounds(ui, 10);
    CHECK(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = hit.x + 4, .y = hit.y + 4}));
    CHECK(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = hit.x + 4, .y = hit.y + 4}));
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    hint(ui, t, (cui_kbd_options){0});
    CHECK(!cui_interaction_state(ui, 10).activated);

    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    cui_begin_box(ui, 20, (cui_box){.row = true, .gap = 8});
    CHECK(cui_kbd_ex(ui, 21, &t, (cui_kbd_stroke){.key = CUI_STR("escape")}, (cui_kbd_options){0}));
    CHECK(cui_kbd_ex(ui, 22, &t, (cui_kbd_stroke){.key = CUI_STR("enter")},
                     (cui_kbd_options){.outline = true}));
    cui_end_box(ui);
    cui_end_frame(ui);
    cui_rect a = cui_bounds(ui, 21), b = cui_bounds(ui, 22);
    CHECK(b.x >= a.x + a.w + 7.99f);
    s = cui_semantics(ui, &n);
    CHECK(n == 2); /* independent labels, no synthetic group binding */
    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    CHECK(cui_kbd_ex(ui, 30, &t, (cui_kbd_stroke){.key = CUI_STR("a")}, (cui_kbd_options){0}));
    cui_end_frame(ui);
    CHECK(cui_bounds(ui, 30).w == 20);
    char key[] = "enter", name[] = "Enter key";
    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    CHECK(cui_kbd_ex(ui, 40, &t, (cui_kbd_stroke){.key = {key, 5}},
                     (cui_kbd_options){.accessible_name = {name, 9}}));
    memset(key, 'x', 5);
    memset(name, 'x', 9);
    cui_draw_list copied = cui_end_frame(ui);
    CHECK(find(copied, CUI_DRAW_TEXT)->text.length == 5 &&
          !memcmp(find(copied, CUI_DRAW_TEXT)->text.data, "Enter", 5));
    s = cui_semantics(ui, &n);
    CHECK(n == 1 && !memcmp(s[0].name.data, "Enter key", 9));
    cui_destroy(ui);
}
static void failures(void) {
    cui_context *ui = create();
    cui_theme t = cui_theme_light();
    CHECK(!cui_kbd_ex(ui, 10, &t, (cui_kbd_stroke){.key = CUI_STR("a")}, (cui_kbd_options){0}));
    CHECK(cui_begin_frame(ui, 320, 160, .016f));
    cui_kbd_stroke stroke = {.key = CUI_STR("a")};
    CHECK(!cui_kbd_ex(ui, 0, &t, stroke, (cui_kbd_options){0}));
    CHECK(!cui_kbd_ex(ui, 10, &t, (cui_kbd_stroke){.key = CUI_STR("bad\n")}, (cui_kbd_options){0}));
    CHECK(!cui_kbd_ex(ui, 10, &t, stroke, (cui_kbd_options){.accessible_name = {"a\0b", 3}}));
    t.radius = NAN;
    CHECK(!cui_kbd_ex(ui, 10, &t, stroke, (cui_kbd_options){0}));
    t = cui_theme_light();
    t.font_size = INFINITY;
    CHECK(!cui_kbd_ex(ui, 10, &t, stroke, (cui_kbd_options){0}));
    cui_draw_list d = cui_end_frame(ui);
    CHECK(!d.count);
    size_t n;
    cui_semantics(ui, &n);
    CHECK(!n && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}

static cui_size measure_styled(void *user, cui_str text, float font, cui_text_style style) {
    (void)user;
    return (cui_size){text.length * font * (style.monospace ? .7f : .5f), font + 3};
}

static void styled_hints(void) {
    cui_context *ui = cui_create((cui_config){.measure_styled = measure_styled});
    CHECK(ui);
    cui_theme theme = cui_theme_light();
    theme.font_size = 16;
    cui_kbd_stroke stroke = {.key = CUI_STR("pagedown")};
    cui_box box = {.width = .5f,
                   .width_percent = true,
                   .padding = 4,
                   .background = {52, 34, 92, 255},
                   .radius = 8};
    cui_typography_token typography = {
        .style = {.weight = CUI_TEXT_WEIGHT_BOLD, .monospace = true, .family = 7},
        .size = 10,
        .line_height = 19,
        .align = CUI_TEXT_ALIGN_RIGHT,
        .wrap = true};
    cui_text_gradient_stop stops[] = {{0, {245, 120, 180, 255}}, {1, {230, 244, 255, 255}}};
    cui_text_paint paint = {
        .kind = CUI_TEXT_PAINT_LINEAR, .x1 = 1, .stops = stops, .stop_count = 2};
    cui_kbd_style style = {.box = &box, .typography = &typography, .text_paint = &paint};
    float widths[] = {240, 112, 240};
    float height = 0;
    for (size_t i = 0; i < sizeof widths / sizeof widths[0]; ++i) {
        CHECK(cui_begin_frame(ui, widths[i], 160, .016f));
        cui_begin_box(ui, 12, (cui_box){.width = widths[i], .fixed_width = true});
        CHECK(cui_kbd_styled(ui, 10, &theme, stroke, (cui_kbd_options){.outline = true}, &style));
        CHECK(cui_element_set_shadow_preset(ui, 10, CUI_SHADOW_XS, (cui_corner_radii){8, 8, 8, 8}));
        stops[0].color.r = 0;
        cui_end_box(ui);
        cui_draw_list draw = cui_end_frame(ui);
        stops[0].color.r = 245;
        CHECK(cui_last_error(ui) == CUI_OK);
        cui_rect bounds = cui_bounds(ui, 10);
        CHECK(fabsf(bounds.w - widths[i] * .5f) < .1f);
        if (!i)
            height = bounds.h;
        else
            CHECK(fabsf(bounds.h - height - (i == 1 ? typography.line_height : 0)) < .1f);
        CHECK(!find(draw, CUI_DRAW_BORDER));
        CHECK(find(draw, CUI_DRAW_DROP_SHADOW));
        bool has_fill = false;
        size_t rows = 0;
        for (size_t j = 0; j < draw.count; ++j) {
            const cui_draw_command *text = &draw.commands[j];
            if (text->type == CUI_DRAW_RECT && text->color.r == 52 &&
                text->corner_radii.top_left == 8)
                has_fill = true;
            if (text->type != CUI_DRAW_TEXT)
                continue;
            ++rows;
            CHECK(text->font_size == 10 && text->bounds.h == typography.line_height);
            CHECK(text->text_style.monospace && text->text_style.weight == CUI_TEXT_WEIGHT_BOLD &&
                  text->text_style.family == 7);
            CHECK(text->text_paint.kind == CUI_TEXT_PAINT_LINEAR &&
                  text->text_paint.stops != stops && text->text_paint.stops[0].color.r == 245);
            CHECK(fabsf(text->bounds.x + text->bounds.w - (bounds.x + bounds.w - box.padding)) <
                  .2f);
        }
        CHECK(has_fill);
        CHECK(rows == (i == 1 ? 2u : 1u));
        size_t count;
        const cui_semantic *semantic = cui_semantics(ui, &count);
        CHECK(count == 1 && semantic[0].role == CUI_ROLE_LABEL && semantic[0].tab_stop_disabled);
        CHECK(semantic[0].name.length == 9 && !memcmp(semantic[0].name.data, "Page Down", 9));
    }

    CHECK(cui_begin_frame(ui, 240, 160, .016f));
    cui_text_paint solid = {.kind = CUI_TEXT_PAINT_SOLID, .color = {32, 65, 120, 255}};
    CHECK(cui_kbd_styled(ui, 10, &theme, stroke, (cui_kbd_options){0},
                         &(cui_kbd_style){.typography = &typography, .text_paint = &solid}));
    cui_draw_list draw = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK);
    CHECK(fabsf(cui_bounds(ui, 10).w - 71) < .1f && cui_bounds(ui, 10).h == 23);
    CHECK(find(draw, CUI_DRAW_TEXT)->color.b == 120);

    CHECK(cui_begin_frame(ui, 240, 160, .016f));
    typography.style.letter_spacing = 1;
    CHECK(!cui_kbd_styled(ui, 11, &theme, stroke, (cui_kbd_options){0}, &style));
    typography.style.letter_spacing = 0;
    paint.x1 = 0;
    CHECK(!cui_kbd_styled(ui, 11, &theme, stroke, (cui_kbd_options){0}, &style));
    typography.size = NAN;
    CHECK(cui_kbd_styled(ui, 10, &theme, stroke, (cui_kbd_options){.plain = true}, &style));
    draw = cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK && draw.count == 1);
    CHECK(find(draw, CUI_DRAW_TEXT)->font_size == theme.font_size);
    size_t count;
    const cui_semantic *semantic = cui_semantics(ui, &count);
    CHECK(count == 1 && semantic[0].id == 10);
    cui_destroy(ui);
}

int main(void) {
    formats();
    unicode_mappings();
    render();
    failures();
    styled_hints();
    puts("Kbd source slice: ok");
    return 0;
}
