/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include "cui/cui_software.h"
#include "cui/cui_video_controls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static cui_context *ui;
static cui_video_controls_state state;
static cui_video_view view;
static cui_video_player_options options;
static cui_video_request last;
static unsigned requests, custom_icons;
static void request(void *user, const cui_video_request *r) {
    assert(user == &requests);
    requests++;
    last = *r;
}
static bool custom(void *user, cui_str path, cui_str *svg) {
    (void)user;
    if (path.length == strlen("icons/rewind.svg") &&
        !memcmp(path.data, "icons/rewind.svg", path.length)) {
        custom_icons++;
        *svg = cui_string("<svg viewBox='0 0 24 24'><path d='M2 2H22V22H2Z'/></svg>");
        return true;
    }
    return false;
}
static cui_draw_list frame(cui_theme theme) {
    assert(cui_begin_frame(ui, 640, 600, .016f));
    assert(cui_video_player(ui, 10, &theme, &state, view, NULL, options));
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static const cui_semantic *find(const char *label) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (s[i].name.length == strlen(label) && !memcmp(s[i].name.data, label, strlen(label)))
            return &s[i];
    assert(false);
    return NULL;
}
static void activate(const char *label) {
    assert(cui_queue_accessibility_action(ui, find(label)->id, CUI_ACCESSIBILITY_ACTIVATE));
}
static void rendered_icons(cui_draw_list draw, unsigned expected) {
    cui_draw_command commands[256];
    cui_draw_list surfaces = {.commands = commands, .width = draw.width, .height = draw.height};
    unsigned icons = 0;
    assert(draw.count <= 256);
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *c = &draw.commands[i];
        if (c->type == CUI_DRAW_IMAGE) {
            assert(cui_bitmap_valid(c->bitmap.bitmap));
            assert(c->bounds.w <= 20.01f && c->bounds.h <= 20.01f);
            icons++;
        }
        if (c->type != CUI_DRAW_TEXT)
            commands[surfaces.count++] = *c;
    }
    assert(icons == expected);
    cui_software *software = cui_software_create(640, 600, NULL);
    assert(software && cui_software_render(software, &surfaces, (cui_color){255, 255, 255, 255}));
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(software, &width, &height);
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *c = &draw.commands[i];
        if (c->type != CUI_DRAW_IMAGE)
            continue;
        unsigned ink = 0;
        for (unsigned y = (unsigned)c->bounds.y; y < (unsigned)(c->bounds.y + c->bounds.h); y++)
            for (unsigned x = (unsigned)c->bounds.x; x < (unsigned)(c->bounds.x + c->bounds.w);
                 x++) {
                assert(x < width && y < height);
                const unsigned char *p = pixels + (y * width + x) * 4;
                ink += p[0] < 250 || p[1] < 250 || p[2] < 250;
            }
        assert(ink >= 8); /* Actual SVG pixels, not a fallback character. */
    }
    cui_software_destroy(software);
}
int main(void) {
    options = cui_video_player_defaults();
    assert(options.controls && options.surface.opacity == 1 && options.controls_options.compact);
    assert(options.surface.geometry.width == 320 && options.surface.geometry.height == 180);
    assert(options.surface.geometry.fit == CUI_IMAGE_CONTAIN);
    assert(options.controls_options.visible == 15 && options.controls_options.skip_seconds == 10);
    assert(!cui_video_controls_defaults().compact); /* Legacy layout remains opt-in. */
    cui_str names[] = {
        options.controls_options.play_icon,       options.controls_options.pause_icon,
        options.controls_options.back_icon,       options.controls_options.forward_icon,
        options.controls_options.fullscreen_icon, options.controls_options.leave_fullscreen_icon,
        options.controls_options.rate_icon};
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; i++) {
        assert(cui_icon_query(names[i]) == CUI_ICON_EMBEDDED);
        cui_icon_state icon = {0};
        assert(cui_icon_set_name(&icon, names[i], NULL, NULL));
        assert(cui_icon_prepare(&icon, 20, 20, cui_icon_identity()) == CUI_ICON_OK);
        cui_bitmap b = cui_icon_state_bitmap(&icon);
        unsigned ink = 0;
        for (unsigned y = 0; y < b.height; y++)
            for (unsigned x = 0; x < b.width; x++)
                ink += b.pixels[y * b.stride + x * 4 + 3] > 0;
        assert(ink >= 8);
        cui_icon_state_destroy(&icon);
    }
    ui = cui_create((cui_config){.width = 640, .height = 600});
    assert(ui);
    view =
        (cui_video_view){.source_id = 77, .capabilities = CUI_VIDEO_CAP_PLAY | CUI_VIDEO_CAP_SEEK};
    assert(cui_video_init(
        &view.timeline,
        (cui_video_config){.duration = 120, .duration_known = true, .seekable = true}));
    options.controls_options.on_request = request;
    options.controls_options.user = &requests;
    cui_theme theme = cui_theme_light();
    cui_draw_list draw = frame(theme);
    rendered_icons(draw, 4);
    assert(find("Enter fullscreen")->disabled);
    assert(!find("Play")->disabled && !find("Playback position")->disabled);
    const char *labels[] = {"Play", "Skip backward", "Skip forward", "Enter fullscreen",
                            "Playback position"};
    for (unsigned i = 0; i < 5; i++) {
        cui_rect b = cui_bounds(ui, find(labels[i])->id);
        assert(b.w >= 44 && b.h >= 44 && b.x >= 0 && b.x + b.w <= 320.01f);
    }
    activate("Skip backward");
    frame(theme);
    assert(requests == 1 && last.kind == CUI_VIDEO_SKIP && last.value.seconds == -10);
    assert(view.timeline.position == 0); /* requests do not acknowledge host success */
    activate("Skip forward");
    frame(theme);
    assert(requests == 2 && last.value.seconds == 10 && last.source_id == 77);
    view.capabilities |= CUI_VIDEO_CAP_FULLSCREEN;
    frame(theme);
    activate("Enter fullscreen");
    frame(theme);
    assert(requests == 3 && last.kind == CUI_VIDEO_FULLSCREEN && last.value.enabled);
    view.fullscreen = true;
    view.timeline.playing = true;
    draw = frame(theme);
    rendered_icons(draw, 4);
    assert(!find("Leave fullscreen")->disabled && !find("Pause")->disabled);
    options.controls_options.width = 160;
    options.surface.geometry.width = 160;
    options.surface.geometry.height = 90;
    draw = frame(theme);
    rendered_icons(draw, 4);
    for (unsigned i = 0; i < 3; i++) {
        const char *label = i == 0 ? "Pause" : i == 1 ? "Skip backward" : "Leave fullscreen";
        cui_rect b = cui_bounds(ui, find(label)->id);
        assert(b.w >= 44 && b.h >= 44 && b.x + b.w <= 160.01f);
    }
    options.controls_options.back_icon = cui_string("rewind");
    theme.icon_loader = custom;
    frame(theme);
    assert(custom_icons > 0);
    view.timeline.playing = false;
    view.status = CUI_VIDEO_FAILED;
    frame(theme);
    assert(find("Play")->disabled && find("Playback position")->disabled);
    assert(!find("Leave fullscreen")->disabled);
    cui_destroy(ui);
    puts("video player defaults: ok");
}
