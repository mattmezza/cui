/* SPDX-License-Identifier: MIT */
#include "../web/notification_source_motion_probe.c"
#include "cui/cui_sdl3.h"
#include "cui/cui_software.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
static SDL_Surface *captured;
static unsigned assertions;
static SDL_AssertState SDLCALL abort_assert(const SDL_AssertData *data, void *user) {
    (void)data;
    (void)user;
    assertions++;
    return SDL_ASSERTION_ABORT;
}
bool __real_SDL_RenderPresent(SDL_Renderer *renderer);
bool __wrap_SDL_RenderPresent(SDL_Renderer *renderer) {
    SDL_Surface *raw = SDL_RenderReadPixels(renderer, NULL);
    assert(raw);
    SDL_DestroySurface(captured);
    captured = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    assert(captured);
    return __real_SDL_RenderPresent(renderer);
}
int main(void) {
    SDL_SetAssertionHandler(abort_assert, NULL);
    assert(SDL_Init(SDL_INIT_VIDEO));
    cui_sdl3 *adapter = cui_sdl3_create("Notification source motion", 384, 384, NULL);
    assert(adapter);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_Renderer *renderer = NULL;
    for (int i = 0; i < count; i++)
        if (!strcmp(SDL_GetWindowTitle(windows[i]), "Notification source motion"))
            renderer = SDL_GetRenderer(windows[i]);
    SDL_free(windows);
    assert(renderer);
    printf("Notification motion SDL driver: %s\n", SDL_GetRendererName(renderer));
    cui_software *reference = cui_software_create(384, 384, NULL);
    assert(reference);
    for (unsigned scale = 1; scale <= 2; scale++)
        for (int anchor = 0; anchor < 8; anchor++)
            for (int closing = 0; closing < 2; closing++) {
                notification_source_motion_scene(anchor, closing, closing ? .05 : .1, scale, 0, 0);
                assert(SDL_SetRenderScale(renderer, scale, scale));
                assert(cui_sdl3_render(adapter, &list, (cui_color){255, 255, 255, 255}));
                cui_draw_command physical[1024];
                assert(list.count < 1024);
                memcpy(physical, list.commands, list.count * sizeof(*physical));
                for (size_t i = 0; i < list.count; i++) {
                    cui_draw_command *c = &physical[i];
                    c->bounds.x *= scale;
                    c->bounds.y *= scale;
                    c->bounds.w *= scale;
                    c->bounds.h *= scale;
                    c->radius *= scale;
                    c->stroke *= scale;
                    c->corner_radii.top_left *= scale;
                    c->corner_radii.top_right *= scale;
                    c->corner_radii.bottom_left *= scale;
                    c->corner_radii.bottom_right *= scale;
                    if (c->type == CUI_DRAW_DROP_SHADOW) {
                        c->shadow.caster.x *= scale;
                        c->shadow.caster.y *= scale;
                        c->shadow.caster.w *= scale;
                        c->shadow.caster.h *= scale;
                        c->shadow.layer.offset_x *= scale;
                        c->shadow.layer.offset_y *= scale;
                        c->shadow.layer.sigma *= scale;
                        c->shadow.layer.spread *= scale;
                        c->shadow.radii.top_left *= scale;
                        c->shadow.radii.top_right *= scale;
                        c->shadow.radii.bottom_left *= scale;
                        c->shadow.radii.bottom_right *= scale;
                    }
                }
                cui_draw_list projected = {physical, list.count, 384, 384};
                assert(cui_software_render(reference, &projected, (cui_color){255, 255, 255, 255}));
                unsigned x = (unsigned)floorf((fixture.entry[0].bounds.x + 16) * scale),
                         y = (unsigned)floorf((fixture.entry[0].bounds.y + 12) * scale);
                assert(x < 384 && y < 384);
                const unsigned char *expected =
                    cui_software_pixels(reference, NULL, NULL) + (y * 384 + x) * 4;
                const unsigned char *actual =
                    (unsigned char *)captured->pixels + y * captured->pitch + x * 4;
                for (unsigned k = 0; k < 4; k++)
                    assert(abs((int)actual[k] - (int)expected[k]) <= 2);
                assert(actual[0] <
                       230); /* Actual moving translucent dark card, not blank output. */
            }
    assert(assertions == 0);
    cui_software_destroy(reference);
    cui_destroy(fixture.ui);
    SDL_DestroySurface(captured);
    cui_sdl3_destroy(adapter);
    SDL_Quit();
    puts("PASS actual SDL source-motion32 controlled placement/enter/exit DPR1/2 pixels");
    return 0;
}
