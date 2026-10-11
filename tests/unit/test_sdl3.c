/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
  if (argc > 2) {
    fprintf(stderr, "Usage: test_sdl3 [FONT]\n");
    return 2;
  }
  const char *paths[] = {argc == 2 ? argv[1] : getenv("CUI_TEST_FONT"),
                         "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                         "/usr/share/fonts/TTF/DejaVuSans.ttf"};
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
  const char text[] = "repeat repeat caf\xc3\xa9";
  cui_draw_command commands[] = {{.type = CUI_DRAW_RECT,
                                  .bounds = {0, 0, 320, 160},
                                  .color = {30, 40, 50, 255}},
                                 {.type = CUI_DRAW_TEXT,
                                  .bounds = {10, 10, 300, 30},
                                  .font_size = 18,
                                  .color = {255, 255, 255, 255},
                                  .text = {text, sizeof(text) - 1}}};
  cui_draw_list list = {commands, 2, 320, 160};
  assert(cui_sdl3_render(s, &list, (cui_color){0}));
  cui_sdl3_cache_stats first = cui_sdl3_glyph_cache_stats(s);
  assert(first.entries > 0 && first.hits > 0 && first.misses > 0);
  commands[1].color =
      (cui_color){255, 50, 20, 128}; /* tint does not allocate new glyphs */
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
  assert(
      platform.clipboard_set(platform.user, (cui_str){text, sizeof(text) - 1}));
  cui_str copy = platform.clipboard_get(platform.user);
  assert(copy.length == sizeof(text) - 1 &&
         !memcmp(copy.data, text, copy.length));
  assert(!platform.clipboard_set(platform.user, (cui_str){"a\0b", 3}));
  cui_context *ui = cui_create((cui_config){.width = 320, .height = 160});
  assert(ui);
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
  cui_destroy(ui);
  printf("SDL driver=%s; glyph misses=%llu then %llu; persistent cache and "
         "clipboard/event roundtrips passed\n",
         cui_sdl3_driver(s), (unsigned long long)first.misses,
         (unsigned long long)second.misses);
  cui_sdl3_destroy(s);
  cui_font_close(font);
  return 0;
}
