/* SPDX-License-Identifier: MIT */
#include "cui/cui_kbd.h"
#include "cui/cui_software.h"
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
                 {"é", "é", "é"}};
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
    char legacy[160];
    CHECK(cui_kbd_format(0, cui_string("escape"), false, legacy, sizeof legacy) == 6);
    CHECK(!strcmp(legacy, "escape")); /* additive formatting, old helper unchanged */
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
int main(void) {
    formats();
    render();
    failures();
    puts("Kbd source slice: ok");
    return 0;
}
