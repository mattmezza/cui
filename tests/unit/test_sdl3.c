/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui/cui_time_field_v1.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    unsigned char *coverage;
    size_t ink;
} density_pixels;
static void density_glyph(void *user, int x, int y, int w, int h, int pitch,
                          const unsigned char *data) {
    density_pixels *p = user;
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            assert(x + col >= 0 && x + col < 320 && y + row >= 0 && y + row < 160);
            unsigned char *dest = &p->coverage[(y + row) * 320 + x + col];
            unsigned alpha = data[row * pitch + col];
            *dest = (unsigned char)(alpha + *dest * (255 - alpha) / 255);
            p->ink += alpha > 0;
        }
}
static void density_render(cui_sdl3 *s, cui_font *font) {
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    assert(SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    SDL_free(windows);
    assert(renderer);
    cui_draw_command text = {.type = CUI_DRAW_TEXT,
                             .bounds = {3, 5, 100, 25},
                             .text = cui_string("Retina fi"),
                             .font_size = 17,
                             .color = {255, 255, 255, 255}};
    size_t first_ink = 0;
    for (int pass = 0; pass < 4; pass++) {
        int logical_scale = pass == 1 ? 2 : 1;
        float scale = pass == 2 ? 1.5f : (float)logical_scale;
        assert(SDL_SetRenderLogicalPresentation(renderer, 320 / logical_scale, 160 / logical_scale,
                                                SDL_LOGICAL_PRESENTATION_STRETCH));
        assert(SDL_SetRenderScale(renderer, pass == 2 ? 1.5f : 1, pass == 2 ? 1.5f : 1));
        cui_draw_list list = {&text, 1, 320.0f / scale, 160.0f / scale};
        assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-density.bmp"));
        SDL_Surface *surface = SDL_LoadBMP("sdl-density.bmp");
        assert(surface && surface->w == 320 && surface->h == 160);
        /* Fractional destinations can interpolate coverage in the software renderer. */
        density_pixels pixels = {calloc(320 * 160, 1), 0};
        assert(pixels.coverage);
        assert(cui_font_draw_context_line_scaled(font, text.text, (cui_text_line_source){0}, 17,
                                                 (cui_text_style){0}, 3, 5, (float)scale,
                                                 density_glyph, &pixels));
        assert(pixels.ink);
        if (!pass)
            first_ink = pixels.ink;
        else if (scale > 1)
            assert(pixels.ink > first_ink);
        else
            assert(pixels.ink == first_ink);
        size_t rendered_ink = 0;
        for (int y = 0; y < surface->h; y++)
            for (int x = 0; x < surface->w; x++) {
                Uint8 r, g, b, a;
                assert(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
                if (scale == 1 || scale == 2)
                    assert(abs((int)r - pixels.coverage[y * 320 + x]) <= 2);
                assert(r == g && g == b);
                rendered_ink += r > 0;
            }
        assert(rendered_ink >= pixels.ink / 2);
        free(pixels.coverage);
        SDL_DestroySurface(surface);
        cui_draw_command clipped[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {0, 0, 20, 80}},
                                      text,
                                      {.type = CUI_DRAW_CLIP_POP}};
        list.commands = clipped;
        list.count = 3;
        assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-density.bmp"));
        surface = SDL_LoadBMP("sdl-density.bmp");
        assert(surface);
        for (int y = 0; y < surface->h; y++)
            for (int x = (int)(20 * scale); x < surface->w; x++) {
                Uint8 r, g, b, a;
                assert(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a) && !r && !g && !b);
            }
        SDL_DestroySurface(surface);
        if (scale == 2) {
            unsigned char image[32 * 32 * 4];
            for (int i = 0; i < 32 * 32; i++) {
                image[i * 4] = (unsigned char)((i % 32) * 7);
                image[i * 4 + 1] = image[i * 4 + 2] = 0;
                image[i * 4 + 3] = 255;
            }
            cui_draw_command artwork = {.type = CUI_DRAW_IMAGE,
                                        .bounds = {0, 0, 16, 16},
                                        .bitmap = {.bitmap = {image, 32, 32, 128, 1},
                                                   .source = {0, 0, 32, 32},
                                                   .opacity = 1}};
            list.commands = &artwork;
            list.count = 1;
            assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-density.bmp"));
            surface = SDL_LoadBMP("sdl-density.bmp");
            assert(surface);
            for (int x = 0; x < 32; x++) {
                Uint8 r, g, b, a;
                assert(SDL_ReadSurfacePixel(surface, x, 16, &r, &g, &b, &a) && r == x * 7);
            }
            SDL_DestroySurface(surface);
            artwork = (cui_draw_command){.type = CUI_DRAW_RECT,
                                         .bounds = {0, 0, 16, 16},
                                         .radius = 8,
                                         .color = {255, 255, 255, 255}};
            assert(cui_sdl3_render_capture(s, &list, (cui_color){0, 0, 0, 255}, "sdl-density.bmp"));
            surface = SDL_LoadBMP("sdl-density.bmp");
            assert(surface);
            unsigned covered[2] = {0};
            for (int y = 0; y < 2; y++)
                for (int x = 0; x < 32; x++) {
                    Uint8 r, g, b, a;
                    assert(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
                    covered[y] += r > 0;
                }
            assert(covered[1] > covered[0]);
            SDL_DestroySurface(surface);
        }
    }
    remove("sdl-density.bmp");
}
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
/* Queued SDL window signals exercise the adapter; this is not physical leave evidence. */
static void presence_scene(cui_context *ui) {
    assert(cui_begin_frame(ui, 320, 160, .016f));
    cui_begin_box(ui, 10, (cui_box){.width = 80, .height = 40});
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Presence"), false, false, 0);
    cui_set_cursor(ui, 10, CUI_CURSOR_GRAB);
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void pointer_presence(cui_sdl3 *s) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 128});
    assert(ui);
    presence_scene(ui);
    SDL_Event event = {.type = SDL_EVENT_WINDOW_MOUSE_ENTER};
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    bool enter = false;
    for (size_t i = 0; i < count; i++)
        enter |= events[i].type == CUI_EVENT_POINTER_ENTER;
    assert(enter);
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_MOTION};
    event.motion.x = 15;
    event.motion.y = 15;
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    assert(cui_hovered(ui, 10) && cui_pointer_cursor(ui) == CUI_CURSOR_GRAB);
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 15;
    event.button.y = 15;
    assert(SDL_PushEvent(&event));
    event = (SDL_Event){.type = SDL_EVENT_WINDOW_MOUSE_LEAVE};
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    assert(!cui_hovered(ui, 10) && cui_pressed(ui, 10) && cui_focused(ui) == 10 &&
           cui_pointer_cursor(ui) == CUI_CURSOR_GRAB);
    event = (SDL_Event){.type = SDL_EVENT_WINDOW_MOUSE_ENTER};
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    assert(cui_hovered(ui, 10) && cui_pressed(ui, 10));
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_MOTION};
    event.motion.x = 330;
    event.motion.y = 15;
    event.motion.state = SDL_BUTTON_LMASK;
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    events = cui_frame_events(ui, &count);
    bool outside_pair = false;
    for (size_t i = 0; i + 1 < count; i++)
        if (events[i].type == CUI_EVENT_POINTER_MOVE && events[i].x == 330)
            outside_pair = events[i + 1].type == CUI_EVENT_POINTER_LEAVE;
    assert(outside_pair && !cui_hovered(ui, 10) && cui_pressed(ui, 10));
    event = (SDL_Event){.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    events = cui_frame_events(ui, &count);
    bool focus_pair = false;
    for (size_t i = 0; i + 1 < count; i++)
        if (events[i].type == CUI_EVENT_CANCEL)
            focus_pair = events[i + 1].type == CUI_EVENT_POINTER_LEAVE;
    assert(focus_pair && !cui_pressed(ui, 10) && !cui_hovered(ui, 10) && cui_focused(ui) == 10 &&
           cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    event = (SDL_Event){.type = SDL_EVENT_WINDOW_FOCUS_GAINED};
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(s, ui));
    presence_scene(ui);
    assert(!cui_hovered(ui, 10));
    cui_destroy(ui);
}
typedef struct {
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 state;
    cui_time value;
    size_t commits;
    bool ordered;
} time_field_test;

static void time_field_changed(void *user, const cui_time_change *event) {
    time_field_test *field = user;
    assert(event->event.target == 20 && event->previous.minute != event->value.minute);
    assert(event->value.minute == field->value.minute);
    field->commits++;
}
static void time_field_frame(cui_context *ui, time_field_test *field, const char *preedit,
                             unsigned minute, size_t commits) {
    cui_theme theme = cui_theme_light();
    cui_time_handler handler = {time_field_changed, field};
    cui_time_field_result_v1 result;
    if (field->ordered) {
        cui_time_field_frame_entry_v1 entry = {20, &field->state, &field->value, &field->options};
        assert(cui_begin_time_field_frame_v1(ui, 320, 160, .016f, &entry, 1) == CUI_TIME_TEXT_OK);
        result = cui_time_field_from_frame_v1(ui, 20, &theme, cui_string("Time"), CUI_DESIGN_BASE,
                                              handler);
        assert(cui_focused(ui) == 20 || !cui_focused(ui));
    } else {
        assert(cui_begin_frame(ui, 320, 160, .016f));
        result = cui_time_field_v1(ui, 20, &theme, cui_string("Time"), &field->state,
                                   &field->value, &field->options, handler);
    }
    assert(result.status == CUI_TIME_TEXT_OK && result.validation == CUI_TIME_TEXT_OK);
    cui_id segment = 0;
    cui_str pending = cui_time_field_preedit_v1(ui, 20, &segment);
    if (preedit) {
        assert(segment == cui_time_segment_id(20, CUI_TIME_FIELD_MINUTE_V1));
        assert(pending.length == strlen(preedit) && !memcmp(pending.data, preedit, pending.length));
    } else
        assert(!segment && !pending.length);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(field->value.minute == minute && field->commits == commits);
}
static void time_field_ime(cui_sdl3 *s, bool ordered) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 160});
    assert(ui);
    time_field_test field = {.value = {9, 30, 0}, .ordered = ordered};
    assert(cui_time_field_options_init_v1(&field.options) == CUI_TIME_TEXT_OK);
    cui_time_locale_v1 locale;
    assert(cui_time_locale_init_v1(&locale) == CUI_TIME_TEXT_OK);
    const char *digits[] = {"٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"};
    for (unsigned i = 0; i < 10; i++)
        locale.digits[i] = cui_string(digits[i]);
    field.options.text.locale = &locale;
    field.options.flags = CUI_TIME_FIELD_IME_V1;
    assert(cui_time_field_state_init_v1(&field.state, field.value, &field.options) ==
           CUI_TIME_TEXT_OK);
    if (ordered)
        field.state.segment = CUI_TIME_FIELD_MINUTE_V1;
    time_field_frame(ui, &field, NULL, 30, 0);
    cui_id minute = cui_time_segment_id(20, CUI_TIME_FIELD_MINUTE_V1);
    cui_focus(ui, ordered ? 20 : minute);
    time_field_frame(ui, &field, NULL, 30, 0);
    cui_text_input_state input;
    assert(cui_get_text_input_config(ui, &input) && input.owner == minute);
    assert(cui_sdl3_poll(s, ui));

    SDL_Event update = {.type = SDL_EVENT_TEXT_EDITING};
    SDL_Event clear = {.type = SDL_EVENT_TEXT_EDITING};
    SDL_Event commit = {.type = SDL_EVENT_TEXT_INPUT};
    clear.edit.text = "";
    update.edit.text = commit.text.text = "٢١";
    assert(SDL_PushEvent(&update) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, "٢١", 30, 0);
    assert(SDL_PushEvent(&clear) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 30, 0);
    time_field_frame(ui, &field, NULL, 30, 0);
    assert(SDL_PushEvent(&commit) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 21, 1);

    update.edit.text = commit.text.text = "٤٢";
    assert(SDL_PushEvent(&update) && SDL_PushEvent(&clear) && SDL_PushEvent(&commit));
    assert(cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 42, 2);

    update.edit.text = commit.text.text = "٥٥";
    assert(SDL_PushEvent(&update) && SDL_PushEvent(&clear) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 42, 2);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    assert(SDL_PushEvent(&lost) && SDL_PushEvent(&commit) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 42, 2);
    SDL_Event gained = {.type = SDL_EVENT_WINDOW_FOCUS_GAINED};
    assert(SDL_PushEvent(&gained) && cui_sdl3_poll(s, ui));
    time_field_frame(ui, &field, NULL, 42, 2);
    if (ordered) {
        assert(SDL_PushEvent(&update) && SDL_PushEvent(&clear) && cui_sdl3_poll(s, ui));
        time_field_frame(ui, &field, NULL, 42, 2);
        SDL_Event left = {.type = SDL_EVENT_KEY_DOWN};
        left.key.key = SDLK_LEFT;
        assert(SDL_PushEvent(&left) && SDL_PushEvent(&commit) && cui_sdl3_poll(s, ui));
        time_field_frame(ui, &field, NULL, 42, 2);
        assert(field.value.hour == 9 && cui_focused(ui) == 20 &&
               cui_get_text_input_config(ui, &input) && input.owner != minute);
    }
    cui_focus(ui, 0);
    time_field_frame(ui, &field, NULL, 42, 2);
    assert(cui_sdl3_poll(s, ui));
    cui_destroy(ui);
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
        for (int cursor = CUI_CURSOR_DEFAULT; cursor <= CUI_CURSOR_HELP; cursor++) {
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
    /* Native normalized contact coordinates become logical window pixels and
     * touch-generated compatibility mouse events are excluded. */
    SDL_Event finger = {.type = SDL_EVENT_FINGER_DOWN};
    finger.tfinger.fingerID = 41;
    finger.tfinger.touchID = 11;
    finger.tfinger.x = .25f;
    finger.tfinger.y = .5f;
    assert(SDL_PushEvent(&finger));
    finger.tfinger.touchID = 22;
    assert(SDL_PushEvent(&finger)); /* Same finger ID on another native device. */
    finger.type = SDL_EVENT_FINGER_MOTION;
    finger.tfinger.fingerID = 42;
    finger.tfinger.touchID = 11;
    assert(SDL_PushEvent(&finger));
    SDL_Event ghost = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    ghost.button.which = SDL_TOUCH_MOUSEID;
    ghost.button.button = SDL_BUTTON_LEFT;
    assert(SDL_PushEvent(&ghost));
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    size_t native_count;
    const cui_event *native = cui_frame_events(ui, &native_count);
    unsigned finger_count = 0;
    for (size_t n = 0; n < native_count; n++) {
        assert(native[n].type != CUI_EVENT_POINTER_DOWN ||
               native[n].pointer_type == CUI_POINTER_TOUCH);
        if (native[n].pointer_type == CUI_POINTER_TOUCH) {
            assert(native[n].pointer_id == (finger_count == 2 ? 42 : 41));
            assert(native[n].pointer_device == (finger_count == 1 ? 22 : 11));
            assert(native[n].x == 80 && native[n].y == 80);
            finger_count++;
        }
    }
    assert(finger_count == 3);
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    cui_end_frame(ui);
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
    composition.edit.text = "\xf0\x9f\x98\x80\xc3\xa9";
    composition.edit.start = 1;
    composition.edit.length = 1;
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
    assert(composition_count == 4);
    assert(composition_events[0].type == CUI_EVENT_COMPOSITION_UPDATE);
    assert(composition_events[0].composition_anchor == 4 &&
           composition_events[0].composition_cursor == 6);
    assert(composition_events[0].text_length == 6 &&
           !memcmp(composition_events[0].text_data, "\xf0\x9f\x98\x80\xc3\xa9", 6));
    assert(composition_events[1].key == CUI_KEY_B && composition_events[1].ime_composing);
    assert(composition_events[2].type == CUI_EVENT_COMPOSITION_CANCEL);
    assert(composition_events[3].key == CUI_KEY_B && !composition_events[3].ime_composing);
    cui_end_frame(ui);
    char long_commit[301];
    memset(long_commit, 'x', 300);
    long_commit[300] = 0;
    SDL_Event ime_commit = {.type = SDL_EVENT_TEXT_INPUT};
    ime_commit.text.text = long_commit;
    assert(SDL_PushEvent(&ime_commit));
    assert(cui_sdl3_poll(s, ui));
    long_commit[0] = 'y';
    assert(cui_begin_frame(ui, 320, 160, .016f));
    const cui_event *committed = cui_frame_events(ui, &composition_count);
    assert(composition_count == 1 && committed[0].type == CUI_EVENT_COMPOSITION_COMMIT &&
           committed[0].text_length == 300 && committed[0].text_data[0] == 'x');
    cui_end_frame(ui);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    assert(SDL_PushEvent(&lost) && SDL_PushEvent(&ime_commit));
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    committed = cui_frame_events(ui, &composition_count);
    assert(composition_count == 2 && committed[0].type == CUI_EVENT_CANCEL &&
           committed[1].type == CUI_EVENT_POINTER_LEAVE);
    cui_end_frame(ui);
    SDL_Event gained = {.type = SDL_EVENT_WINDOW_FOCUS_GAINED};
    assert(SDL_PushEvent(&gained) && SDL_PushEvent(&ime_commit));
    assert(cui_sdl3_poll(s, ui));
    assert(cui_begin_frame(ui, 320, 160, .016f));
    committed = cui_frame_events(ui, &composition_count);
    assert(composition_count == 1 && committed[0].type == CUI_EVENT_TEXT &&
           committed[0].text_length == 300);
    cui_end_frame(ui);
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_TAB;
    event.key.mod = SDL_KMOD_SHIFT;
    assert(SDL_PushEvent(&event));
    /* An event wakeup must preserve the key for the normal dispatcher. */
    assert(cui_sdl3_wait_events(s, 1000));
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
    cui_window_decoration decorated_mode = driver && !strcmp(driver, "cocoa")
                                               ? CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS
                                               : CUI_WINDOW_SERVER_DECORATED;
    assert(info.decoration == decorated_mode);
    bool decorated = cui_sdl3_set_decorated(s, false);
    if (driver && (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen")))
        assert(!decorated);
    if (driver && strcmp(driver, "dummy") && strcmp(driver, "offscreen"))
        assert(decorated);
    if (decorated) {
        info = window.query(window.user);
        assert(info.decoration == CUI_WINDOW_CLIENT_DECORATED);
        assert(cui_sdl3_set_decorated(s, true));
        assert(window.query(window.user).decoration == decorated_mode);
    }
    assert(cui_sdl3_frame_delta(NULL) == 0);
    assert(cui_sdl3_frame_delta(s) == 0);
    Uint64 started = SDL_GetTicksNS();
    SDL_Delay(30);
    float elapsed = cui_sdl3_frame_delta(s);
    double measured = (double)(SDL_GetTicksNS() - started) / 1e9;
    assert(elapsed >= .025f && elapsed <= measured + .01);
    /* The idle deadline expires with no event, instead of consuming CPU. */
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    started = SDL_GetTicksNS();
    assert(!cui_sdl3_wait_events(s, 20));
    assert(SDL_GetTicksNS() - started >= 15000000);
    /* Idle time must not fast-forward a newly started animation. */
    float resumed = cui_sdl3_frame_delta(s);
    double waited_and_resumed = (double)(SDL_GetTicksNS() - started) / 1e9;
    assert(resumed <= waited_and_resumed - .015);
    density_render(s, font);
    pointer_presence(s);
    time_field_ime(s, false);
    time_field_ime(s, true);
    assert(window.perform(window.user, CUI_WINDOW_CLOSE, 0, 0));
    assert(!cui_sdl3_poll(s, ui));
    cui_destroy(ui);
    cui_sdl3_destroy(s);
    cui_font_close(font);
    return 0;
}
