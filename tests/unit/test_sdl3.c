/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void bitmap_pixels(cui_sdl3 *s) {
    unsigned char pixels[] = {255, 0, 0, 255, 0, 255, 0, 128};
    cui_draw_command c = {
        .type = CUI_DRAW_IMAGE,
        .bounds = {0, 0, 40, 20},
        .bitmap = {.bitmap = {pixels, 2, 1, 8, 1}, .source = {0, 0, 2, 1}, .opacity = 1}};
    cui_draw_list list = {&c, 1, 320, 160};
    for (int pass = 0; pass < 3; pass++) {
        if (pass == 2) {
            pixels[0] = 0;
            pixels[2] = 255;
            c.bitmap.bitmap.revision++;
        }
        assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-bitmap.bmp"));
        SDL_Surface *surface = SDL_LoadBMP("sdl-bitmap.bmp");
        assert(surface);
        Uint8 red, green, blue, alpha;
        assert(SDL_ReadSurfacePixel(surface, 5, 5, &red, &green, &blue, &alpha));
        assert(pass == 2 ? blue == 255 && red == 0 : red == 255 && blue == 0);
        assert(SDL_ReadSurfacePixel(surface, 30, 5, &red, &green, &blue, &alpha));
        assert(abs(green - 128) <= 1);
        SDL_DestroySurface(surface);
    }
    c.bitmap.source = (cui_rect){1, 0, 1, 1};
    c.bitmap.grayscale = true;
    c.radius = 10;
    assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-bitmap.bmp"));
    SDL_Surface *surface = SDL_LoadBMP("sdl-bitmap.bmp");
    assert(surface);
    Uint8 red, green, blue, alpha;
    assert(SDL_ReadSurfacePixel(surface, 0, 0, &red, &green, &blue, &alpha) && red == 0);
    assert(SDL_ReadSurfacePixel(surface, 20, 10, &red, &green, &blue, &alpha));
    assert(abs(red - 91) <= 1 && red == green && green == blue);
    SDL_DestroySurface(surface);
}
static void shapes(cui_sdl3 *s) {
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = {8, 8, 48, 48},
                          .color = {255, 0, 0, 128},
                          .custom_corners = true,
                          .corner_radii = {12, 0, 0, 6},
                          .custom_border_edges = true,
                          .border_edges = {0, 4, 2, 6}};
    cui_draw_list list = {&c, 1, 320, 160};
    assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-shapes.bmp"));
    SDL_Surface *surface = SDL_LoadBMP("sdl-shapes.bmp");
    assert(surface);
    Uint8 red, green, blue, alpha;
    assert(SDL_ReadSurfacePixel(surface, 8, 8, &red, &green, &blue, &alpha) && red == 0);
    assert(SDL_ReadSurfacePixel(surface, 55, 8, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    assert(SDL_ReadSurfacePixel(surface, 8, 55, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    assert(SDL_ReadSurfacePixel(surface, 55, 55, &red, &green, &blue, &alpha) && red == 0);
    SDL_DestroySurface(surface);
    c.type = CUI_DRAW_BORDER;
    assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-shapes.bmp"));
    surface = SDL_LoadBMP("sdl-shapes.bmp");
    assert(surface);
    assert(SDL_ReadSurfacePixel(surface, 8, 32, &red, &green, &blue, &alpha) && red == 0);
    assert(SDL_ReadSurfacePixel(surface, 54, 32, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    assert(SDL_ReadSurfacePixel(surface, 32, 8, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    assert(SDL_ReadSurfacePixel(surface, 32, 54, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    assert(SDL_ReadSurfacePixel(surface, 32, 32, &red, &green, &blue, &alpha) && red == 0);
    SDL_DestroySurface(surface);
    cui_draw_command clipped[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {32, 0, 32, 64}}, c, {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list clip_list = {clipped, 3, 320, 160};
    assert(cui_sdl3_render_capture(s, &clip_list, (cui_color){0, 0, 0, 255}, "sdl-shapes.bmp"));
    surface = SDL_LoadBMP("sdl-shapes.bmp");
    assert(surface);
    assert(SDL_ReadSurfacePixel(surface, 20, 8, &red, &green, &blue, &alpha) && red == 0);
    assert(SDL_ReadSurfacePixel(surface, 40, 8, &red, &green, &blue, &alpha) &&
           abs(red - 128) <= 1);
    SDL_DestroySurface(surface);
}
int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: test_sdl3 [FONT]\n");
        return 2;
    }
    const char *paths[] = {
        argc == 2 ? argv[1] : getenv("CUI_TEST_FONT"), "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    cui_font *font = NULL;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]) && !font; i++)
        if (paths[i])
            font = cui_font_open(paths[i]);
    if (!font) {
        fprintf(stderr, "SKIP: set CUI_TEST_FONT to a test font file\n");
        return 77;
    }
    cui_sdl3 *s = cui_sdl3_create("CUI SDL adapter test", 320, 160, font);
    assert(s);
    shapes(s);
    bitmap_pixels(s);
    const char text[] = "repeat repeat caf\xc3\xa9";
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 320, 160}, .color = {30, 40, 50, 255}},
        {.type = CUI_DRAW_TEXT,
         .bounds = {10, 10, 300, 30},
         .font_size = 18,
         .color = {255, 255, 255, 255},
         .text = {text, sizeof(text) - 1}}};
    cui_draw_list list = {commands, 2, 320, 160};
    assert(cui_sdl3_render(s, &list, (cui_color){0}));
    cui_sdl3_cache_stats first = cui_sdl3_glyph_cache_stats(s);
    assert(first.entries > 0 && first.hits > 0 && first.misses > 0);
    commands[1].color = (cui_color){255, 50, 20, 128}; /* tint does not allocate new glyphs */
    assert(cui_sdl3_render(s, &list, (cui_color){0}));
    cui_sdl3_cache_stats second = cui_sdl3_glyph_cache_stats(s);
    assert(second.entries == first.entries && second.misses == first.misses &&
           second.hits > first.hits);
    assert(second.coverage_bytes <= 4u * 1024u * 1024u);
    for (int size = 8; size < 50; size++) {
        commands[1].font_size = (float)size;
        assert(cui_sdl3_render(s, &list, (cui_color){0}));
    }
    cui_sdl3_cache_stats stressed = cui_sdl3_glyph_cache_stats(s);
    assert(stressed.misses > 256 && stressed.entries <= 256 &&
           stressed.coverage_bytes <= 4u * 1024u * 1024u);
    cui_platform platform = cui_sdl3_clipboard_interface(s);
    assert(platform.capabilities == CUI_PLATFORM_CLIPBOARD);
    assert(platform.clipboard_set(platform.user, (cui_str){text, sizeof(text) - 1}));
    cui_str copy = platform.clipboard_get(platform.user);
    assert(copy.length == sizeof(text) - 1 && !memcmp(copy.data, text, copy.length));
    assert(!platform.clipboard_set(platform.user, (cui_str){"a\0b", 3}));
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 160});
    assert(ui);
    assert(!cui_sdl3_set_cursor(NULL, CUI_CURSOR_DEFAULT));
    assert(!cui_sdl3_set_cursor(s, (cui_cursor)999));
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0) {
        for (int cursor = CUI_CURSOR_DEFAULT; cursor <= CUI_CURSOR_NOT_ALLOWED; cursor++) {
            assert(cui_sdl3_set_cursor(s, (cui_cursor)cursor));
            assert(SDL_GetCursor());
        }
        assert(cui_sdl3_set_cursor(s, CUI_CURSOR_DEFAULT));
        SDL_Cursor *first = SDL_GetCursor();
        assert(cui_sdl3_set_cursor(s, CUI_CURSOR_RESIZE_HORIZONTAL));
        assert(SDL_GetCursor() != first);
        assert(cui_sdl3_set_cursor(s, CUI_CURSOR_DEFAULT) && SDL_GetCursor() == first);
    } else {
        assert(!cui_sdl3_set_cursor(s, CUI_CURSOR_RESIZE_HORIZONTAL));
    }
    /* Every alphanumeric shortcut survives the native host translation. */
    const char *shortcut_chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; shortcut_chars[i]; i++) {
        SDL_Event shortcut = {.type = SDL_EVENT_KEY_DOWN};
        shortcut.key.key = (SDL_Keycode)shortcut_chars[i];
        assert(SDL_PushEvent(&shortcut));
    }
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    size_t shortcut_count, shortcut_index = 0;
    const cui_event *shortcut_events = cui_frame_events(ui, &shortcut_count);
    for (size_t i = 0; i < shortcut_count; i++)
        if (shortcut_events[i].type == CUI_EVENT_KEY_DOWN) {
            assert(shortcut_index < strlen(shortcut_chars));
            assert(shortcut_events[i].key ==
                   cui_key_from_character(shortcut_chars[shortcut_index++]));
        }
    assert(shortcut_index == strlen(shortcut_chars));
    cui_end_frame(ui);
    SDL_Event composition = {.type = SDL_EVENT_TEXT_EDITING};
    composition.edit.text = "preedit";
    assert(SDL_PushEvent(&composition));
    SDL_Event composing_key = {.type = SDL_EVENT_KEY_DOWN};
    composing_key.key.key = SDLK_B;
    assert(SDL_PushEvent(&composing_key));
    composition.edit.text = "";
    assert(SDL_PushEvent(&composition));
    assert(SDL_PushEvent(&composing_key));
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    size_t composition_count;
    const cui_event *composition_events = cui_frame_events(ui, &composition_count);
    assert(composition_count == 2);
    assert(composition_events[0].key == CUI_KEY_B && composition_events[0].ime_composing);
    assert(composition_events[1].key == CUI_KEY_B && !composition_events[1].ime_composing);
    cui_end_frame(ui);
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_TAB;
    event.key.mod = SDL_KMOD_SHIFT;
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, 0));
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; i++)
        if (events[i].type == CUI_EVENT_KEY_DOWN && events[i].key == CUI_KEY_TAB &&
            events[i].modifiers == CUI_MOD_SHIFT)
            found = true;
    assert(found);
    cui_end_frame(ui);
    const Uint8 buttons[] = {SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE};
    for (unsigned i = 0; i < 3; ++i) {
        event = (SDL_Event){0};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = buttons[i];
        event.button.clicks = (Uint8)(i + 1);
        event.button.x = 11;
        event.button.y = 12;
        assert(SDL_PushEvent(&event));
    }
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, 0));
    events = cui_frame_events(ui, &count);
    unsigned observed = 0;
    for (size_t i = 0; i < count; ++i)
        if (events[i].type == CUI_EVENT_POINTER_DOWN && events[i].x == 11 && events[i].y == 12) {
            assert(events[i].pointer_button == (cui_pointer_button)observed);
            assert(events[i].click_count == observed + 1);
            observed++;
        }
    assert(observed == 3);
    cui_end_frame(ui);
    printf("SDL driver=%s; glyph misses=%llu then %llu; persistent cache and "
           "clipboard/event roundtrips passed\n",
           cui_sdl3_driver(s), (unsigned long long)first.misses, (unsigned long long)second.misses);
    cui_window_host window = cui_sdl3_window_interface(s);
    cui_window_info info = window.query(window.user);
    assert(info.actions & (1u << CUI_WINDOW_CLOSE));
    assert(!(info.actions & (1u << CUI_WINDOW_MOVE)));
    assert(!window.perform(window.user, CUI_WINDOW_MOVE, 10, 10));
    assert(!window.perform(window.user, CUI_WINDOW_MENU, 10, 10));
    const char *driver = cui_sdl3_video_driver();
    if (driver && (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen")))
        assert(info.actions == (1u << CUI_WINDOW_CLOSE));
    bool decorated = cui_sdl3_set_decorated(s, false);
    if (driver && (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen")))
        assert(!decorated);
    if (driver && strcmp(driver, "dummy") && strcmp(driver, "offscreen"))
        assert(decorated);
    if (decorated) {
        info = window.query(window.user);
#ifndef SDL_PLATFORM_MACOS
        assert(info.decoration == CUI_WINDOW_CLIENT_DECORATED);
#endif
        assert(cui_sdl3_set_decorated(s, true));
    }
    assert(window.perform(window.user, CUI_WINDOW_CLOSE, 0, 0));
    assert(!cui_sdl3_poll(s, ui));
    cui_destroy(ui);
    cui_sdl3_destroy(s);
    cui_font_close(font);
    return 0;
}
