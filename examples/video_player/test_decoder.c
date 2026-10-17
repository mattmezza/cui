/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "decoder.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <threads.h>
static void tick(void) {
    struct timespec wait = {0, 1000000};
    thrd_sleep(&wait, NULL);
}
static video_lease acquire(video_decoder *d, uint64_t generation, double due) {
    video_lease lease = {0};
    for (unsigned i = 0; i < 10000; ++i) {
        assert(!video_decoder_poll(d).failed);
        if (video_decoder_acquire(d, generation, due, &lease))
            return lease;
        tick();
    }
    assert(!"decoded frame timeout");
    return lease;
}
static void pixel(video_lease lease, bool blue) {
    const unsigned char *p = lease.frame.frame.bitmap.pixels;
    assert(lease.frame.frame.bitmap.width == 64 && lease.frame.frame.bitmap.height == 32);
    assert(p[blue ? 2 : 0] == 255 && p[blue ? 0 : 2] == 0 && p[1] == 0 && p[3] == 255);
}
static void render(cui_context *ui, cui_software *r, cui_video_view view, video_lease lease,
                   bool blue) {
    cui_theme theme = cui_theme_light();
    cui_video_controls_state state = {0};
    assert(cui_begin_frame(ui, 128, 64, .016f));
    assert(cui_video_player(
        ui, 40, &theme, &state, view, &lease.frame,
        (cui_video_player_options){
            .surface = {.geometry = {.width = 128, .height = 64, .fit = CUI_IMAGE_FILL},
                        .opacity = 1}}));
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_software_render(r, &list, (cui_color){0, 0, 0, 255}));
    /* Direct renderer output verifies decoded content, not only packet metadata. */
    const unsigned char *p = cui_software_pixels(r, NULL, NULL);
    assert(p[blue ? 2 : 0] == 255 && p[blue ? 0 : 2] == 0 && p[3] == 255);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    video_decoder *d = video_decoder_open(argv[1], 91, 1);
    assert(d);
    for (unsigned i = 0; i < 10000 && !video_decoder_poll(d).ready; ++i) {
        assert(!video_decoder_poll(d).failed);
        tick();
    }
    video_decoder_status status = video_decoder_poll(d);
    assert(status.ready && status.duration_known && fabs(status.duration - 2) < .01);
    cui_video_view view = {.source_id = 91};
    assert(cui_video_init(
        &view.timeline,
        (cui_video_config){.duration = status.duration, .duration_known = true, .seekable = true}));
    cui_context *ui = cui_create((cui_config){.width = 128, .height = 64});
    cui_software *renderer = cui_software_create(128, 64, NULL);
    assert(ui && renderer);
    video_lease red = acquire(d, 1, 0);
    pixel(red, false);
    render(ui, renderer, view, red, false);
    unsigned char saved[4];
    memcpy(saved, red.frame.frame.bitmap.pixels, 4);
    /* Pausing the host clock cannot advance presentation; same lease survives
     * worker prefetch and remains byte-identical while a seek is in flight. */
    for (unsigned i = 0; i < 10; ++i) {
        tick();
        assert(!memcmp(saved, red.frame.frame.bitmap.pixels, 4));
    }
    assert(!video_decoder_close(d)); /* outstanding lease blocks destruction */
    assert(cui_video_seek(&view.timeline, 1.2));
    assert(video_decoder_seek(d, view.timeline.generation, 1.2));
    assert(!video_decoder_seek(d, view.timeline.generation, 1.3));
    video_lease blue = acquire(d, 2, 1.2);
    pixel(blue, true);
    assert(blue.frame.source_id == 91 && blue.frame.frame.generation == 2 &&
           blue.frame.frame.timestamp >= 1.2);
    assert(!memcmp(saved, red.frame.frame.bitmap.pixels, 4));
    /* Old lease remains safe but cannot be drawn with the new view. */
    cui_theme theme = cui_theme_light();
    cui_video_controls_state controls = {0};
    assert(cui_begin_frame(ui, 128, 64, .016f));
    assert(!cui_video_player(
        ui, 40, &theme, &controls, view, &red.frame,
        (cui_video_player_options){
            .surface = {.geometry = {.width = 128, .height = 64}, .opacity = 1}}));
    cui_end_frame(ui);
    render(ui, renderer, view, blue, true);
    assert(video_decoder_release(d, &red));
    assert(!video_decoder_release(d, &red));
    assert(video_decoder_release(d, &blue));
    for (unsigned i = 0; i < 10000; ++i) {
        video_lease next = {0};
        if (video_decoder_acquire(d, 2, 2, &next)) {
            pixel(next, true);
            assert(video_decoder_release(d, &next));
        }
        status = video_decoder_poll(d);
        assert(!status.failed && status.queued + status.leased <= 3);
        if (status.eof && !status.queued)
            break;
        tick();
    }
    status = video_decoder_poll(d);
    assert(status.eof && !status.queued && !status.leased && status.acquired == status.released);
    assert(video_decoder_close(d));
    cui_software_destroy(renderer);
    cui_destroy(ui);
    /* Decode/open errors are observable, with clean worker shutdown. */
    d = video_decoder_open("/nonexistent/cui-video-file", 92, 1);
    assert(d);
    for (unsigned i = 0; i < 10000 && !video_decoder_poll(d).failed; ++i)
        tick();
    assert(video_decoder_poll(d).failed && video_decoder_close(d));
    return 0;
}
