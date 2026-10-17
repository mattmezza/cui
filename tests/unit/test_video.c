/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "cui/cui_video.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #x);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static bool equal(cui_video a, cui_video b) {
    return a.position == b.position && a.duration == b.duration && a.rate == b.rate &&
           a.generation == b.generation && a.duration_known == b.duration_known &&
           a.seekable == b.seekable && a.playing == b.playing && a.ended == b.ended &&
           a.loop == b.loop;
}
static void timeline(void) {
    cui_video s;
    CHECK(cui_video_init(
        &s, (cui_video_config){.duration = 10, .duration_known = true, .seekable = true}));
    CHECK(s.generation == 1 && s.rate == 1 && !s.playing);
    CHECK(cui_video_set_playing(&s, true) && cui_video_advance(&s, 3));
    CHECK(s.position == 3);
    CHECK(cui_video_set_playing(&s, false) && cui_video_advance(&s, 5) && s.position == 3);
    CHECK(cui_video_set_rate(&s, 2) && cui_video_set_playing(&s, true));
    CHECK(cui_video_advance(&s, 2) && s.position == 7);
    CHECK(cui_video_advance(&s, 2) && s.position == 10 && s.ended && !s.playing);
    CHECK(cui_video_set_playing(&s, true) && s.position == 0 && s.generation == 2);
    CHECK(cui_video_seek(&s, 9) && s.generation == 3 && s.playing);
    CHECK(cui_video_set_loop(&s, true) && cui_video_advance(&s, 1));
    CHECK(s.position == 1 && s.playing && !s.ended && s.generation == 4);
    CHECK(cui_video_advance(&s, 15) && s.position == 1 && s.generation == 5);
    cui_video before = s;
    CHECK(!cui_video_seek(&s, 11) && !cui_video_seek(&s, NAN));
    CHECK(!cui_video_set_rate(&s, 0) && !cui_video_set_rate(&s, INFINITY));
    CHECK(!cui_video_advance(&s, -1) && !cui_video_advance(&s, INFINITY));
    CHECK(!cui_video_advance(&s, DBL_MAX));
    CHECK(equal(s, before));
    s.generation = UINT64_MAX;
    before = s;
    CHECK(!cui_video_seek(&s, 0) && !cui_video_advance(&s, 5));
    CHECK(equal(s, before));
    CHECK(cui_video_init(&s, (cui_video_config){0}));
    CHECK(cui_video_set_playing(&s, true) && cui_video_advance(&s, 100));
    CHECK(s.position == 100 && !s.duration_known && !s.ended);
    CHECK(!cui_video_seek(&s, 1));
    CHECK(cui_video_finish(&s) && s.ended && !s.playing && s.position == 100);
    CHECK(!cui_video_set_playing(&s, true));
    before = s;
    CHECK(!cui_video_init(&s, (cui_video_config){.duration = 1}));
    CHECK(!cui_video_init(&s, (cui_video_config){.duration = NAN, .duration_known = true}));
    CHECK(equal(s, before));
    CHECK(cui_video_init(&s, (cui_video_config){.duration_known = true}));
    CHECK(cui_video_set_playing(&s, true) && s.ended && !s.playing);
    CHECK(cui_video_init(
        &s, (cui_video_config){
                .duration = DBL_MAX, .duration_known = true, .seekable = true, .loop = true}));
    CHECK(cui_video_seek(&s, DBL_MAX * .75) && cui_video_set_playing(&s, true));
    CHECK(cui_video_advance(&s, DBL_MAX * .75) && isfinite(s.position));
    CHECK(fabs(s.position / DBL_MAX - .5) < 1e-15);
}
static void surface(void) {
    const unsigned char red[] = {255, 0, 0, 255, 255, 0, 0, 255};
    const unsigned char blue[] = {0, 0, 255, 255, 0, 0, 255, 255};
    cui_video s;
    CHECK(cui_video_init(&s, (cui_video_config){.seekable = true}));
    cui_video_frame f = {.bitmap = {red, 2, 1, 8, 1}, .generation = s.generation};
    cui_video_surface_options o = {
        .geometry = {.width = 100, .height = 100, .fit = CUI_IMAGE_CONTAIN}, .opacity = 1};
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100});
    cui_software *renderer = cui_software_create(100, 100, NULL);
    CHECK(ui && renderer);
    for (unsigned i = 0; i < 3; ++i) {
        o.geometry.fit = i == 0 ? CUI_IMAGE_CONTAIN : i == 1 ? CUI_IMAGE_COVER : CUI_IMAGE_FILL;
        CHECK(cui_begin_frame(ui, 100, 100, .016f));
        CHECK(cui_video_surface(ui, 50, &s, &f, o));
        cui_draw_list list = cui_end_frame(ui);
        CHECK(cui_last_error(ui) == CUI_OK);
        unsigned images = 0;
        for (size_t n = 0; n < list.count; ++n) {
            if (list.commands[n].type != CUI_DRAW_IMAGE)
                continue;
            ++images;
            CHECK(list.commands[n].bitmap.bitmap.pixels == f.bitmap.pixels);
            CHECK(list.commands[n].bitmap.bitmap.revision == f.bitmap.revision);
            CHECK(list.commands[n].bounds.w == 100);
            CHECK(list.commands[n].bounds.h == (i == 0 ? 50 : 100));
            CHECK(list.commands[n].bounds.y == (i == 0 ? 25 : 0));
            CHECK(list.commands[n].bitmap.source.w == (i == 1 ? 1 : 2));
        }
        CHECK(images == 1);
        CHECK(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
        CHECK(w == 100 && h == 100 && pixels[3] == 255);
        if (i == 0)
            CHECK(pixels[0] == 0 && pixels[2] == 0);
        const unsigned char *center = pixels + (50 * w + 50) * 4;
        CHECK(center[i == 0 ? 0 : 2] == (i == 2 ? 128 : 255));
        cui_rect bounds = cui_bounds(ui, 50);
        CHECK(bounds.w == 100 && bounds.h == 100);
        f.bitmap.pixels = blue;
        ++f.bitmap.revision;
        if (i == 1)
            o.opacity = .5f;
    }
    CHECK(cui_video_seek(&s, 2));
    CHECK(cui_begin_frame(ui, 100, 100, .016f));
    CHECK(!cui_video_surface(ui, 50, &s, &f, o)); /* stale generation */
    f.generation = s.generation;
    f.timestamp = NAN;
    CHECK(!cui_video_surface(ui, 50, &s, &f, o));
    f.timestamp = 2;
    o.opacity = NAN;
    CHECK(!cui_video_surface(ui, 50, &s, &f, o));
    o.opacity = 1;
    o.geometry.fit = (cui_image_fit)99;
    CHECK(!cui_video_surface(ui, 50, &s, &f, o));
    o.geometry.fit = CUI_IMAGE_FILL;
    f.bitmap.stride = 1;
    CHECK(!cui_video_surface(ui, 50, &s, &f, o));
    cui_draw_list empty = cui_end_frame(ui);
    CHECK(empty.count == 0 && cui_last_error(ui) == CUI_OK);
    cui_software_destroy(renderer);
    cui_destroy(ui);
}
int main(void) {
    timeline();
    surface();
    return 0;
}
