/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void models(void) {
    char output[64];
    assert(cui_avatar_initials(cui_string(" ada   lovelace "), output) == 2 &&
           !strcmp(output, "AL"));
    assert(cui_avatar_initials(cui_string("Jean Paul Sartre"), output) == 2 &&
           !strcmp(output, "JS"));
    assert(cui_avatar_initials(cui_string("Éva 小"), output) == 5 && !strcmp(output, "É小"));
    assert(cui_avatar_initials(cui_string(""), output) == 1 && !strcmp(output, "?"));
    assert(cui_avatar_initials((cui_str){"\xed\xa0\x80", 3}, output) == 1 && !strcmp(output, "?"));
    assert(cui_avatar_initials((cui_str){"\xf0\x80\x80\x80", 4}, output) == 1);
    assert(cui_avatar_initials((cui_str){"\xc2", 1}, output) == 1);
    assert(cui_kbd_format(CUI_MOD_CTRL | CUI_MOD_SHIFT, cui_string("P"), false, output,
                          sizeof(output)) == 12);
    assert(!strcmp(output, "Ctrl+Shift+P"));
    assert(cui_kbd_format(CUI_MOD_META | CUI_MOD_ALT, cui_string("K"), true, output,
                          sizeof(output)) == 7);
    assert(!strcmp(output, "⌥⌘K"));
    char tiny[3] = {'!', '!', '!'};
    assert(cui_kbd_format(CUI_MOD_CTRL, cui_string("P"), false, tiny, 3) == 6);
    assert(tiny[0] == 'C' && tiny[1] == 't' && tiny[2] == 0);
    assert(cui_kbd_format(0, cui_string("P"), false, NULL, 0) == 1);
    assert(cui_kbd_format(CUI_MOD_META, cui_string("K"), true, tiny, 3) == 4);
    assert(tiny[0] == 0);
    assert(cui_kbd_format(0, cui_string("Éva"), false, tiny, 3) == 4);
    assert(!strcmp(tiny, "É"));
    assert(cui_loading_phase(1, 2) == .5f);
    assert(cui_loading_phase(5, 2) == .5f);
    assert(cui_loading_phase(NAN, 2) == 0 && cui_loading_phase(1, 0) == 0);
    cui_hover_state s = {0};
    assert(!cui_hover_update(&s, true, false, false, false, false, .1f, .3f, .2f));
    assert(!s.open && s.pending);
    assert(cui_hover_update(&s, true, false, false, false, false, .21f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, false, true, false, false, false, .5f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, false, false, false, false, false, .1f, .3f, .2f) && s.open);
    assert(!cui_hover_update(&s, true, false, false, false, false, .5f, .3f, .2f) && s.open &&
           !s.pending);
    assert(cui_hover_update(&s, false, false, false, true, false, 0, .3f, .2f) && !s.open);
    assert(!cui_hover_update(&s, false, false, true, false, false, 0, 0, .2f) && !s.open);
    assert(!cui_hover_update(&s, false, false, false, false, false, 0, 0, .2f));
    assert(cui_hover_update(&s, false, false, true, false, false, 0, 0, .2f) && s.open);
    assert(cui_hover_update(&s, true, true, true, false, true, .5f, .3f, .2f) && !s.open);
    assert(!cui_hover_update(&s, true, false, false, false, false, NAN, .3f, .2f) &&
           s.elapsed == 0);
    assert(!cui_hover_update(&s, false, false, false, false, false, 0, .3f, .2f) && !s.pending);
}
static cui_draw_list scene(cui_context *ui, const cui_theme *t, bool *visible, float phase,
                           bool reduced) {
    assert(cui_begin_frame(ui, 800, 1000, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 800, .gap = 4});
    cui_alert(ui, 2, t,
              (cui_alert_options){.kind = CUI_ALERT_ERROR,
                                  .title = cui_string("Failure"),
                                  .message = cui_string("Retry the operation"),
                                  .width = 400,
                                  .visible = visible,
                                  .dismissible = true});
    cui_avatar(ui, 3, t, cui_string("Ada Lovelace"), 48);
    cui_begin_bubble(ui, 4, t, CUI_BUBBLE_OUTLINE, 320);
    cui_text(ui, cui_string("Hello"), t->font_size, t->text);
    cui_end_bubble(ui);
    cui_empty(ui, 5, t, cui_string("No files"), cui_string("Upload a file to begin"), 320);
    cui_begin_group_box(ui, 6, t, cui_string("Preferences"), 320);
    cui_kbd(ui, 7, t, cui_string("Ctrl+P"));
    cui_end_group_box(ui);
    cui_spinner(ui, 8, t, 24, phase, reduced);
    cui_skeleton(ui, 9, t, 120, 16, 8, phase, false, reduced);
    cui_divider(ui, 10, t, 100, false);
    cui_divider(ui, 11, t, 20, true);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static cui_draw_list hover_scene(cui_context *ui, const cui_theme *t, cui_hover_state *s, bool card,
                                 float dt, bool disabled) {
    assert(cui_begin_frame(ui, 300, 180, dt));
    cui_begin_box(ui, 40, (cui_box){.width = 300, .height = 40, .padding = 4, .clip = true});
    cui_button(ui, 41, t, cui_string("Help"), disabled);
    cui_set_semantic_details(ui, 41, (cui_semantic_details){.required = true});
    cui_hover_options options = {.trigger = 41,
                                 .side = CUI_POPUP_ABOVE,
                                 .viewport = {0, 0, 300, 180},
                                 .width = 140,
                                 .height = 40,
                                 .gap = 8,
                                 .dt = dt,
                                 .open_delay = .2f,
                                 .close_delay = .2f,
                                 .disabled = disabled};
    if (card) {
        if (cui_begin_hover_card(ui, 42, t, s, cui_string("Details"), options)) {
            cui_text(ui, cui_string("Details body"), t->font_size, t->text);
            cui_end_hover_card(ui);
        }
    } else
        cui_tooltip(ui, 42, t, s, cui_string("Help text"), options);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static void intrinsic_hover_scene(cui_context *ui, const cui_theme *t, cui_hover_state *state,
                                  float width, float height, float maximum, uint64_t revision,
                                  float anchor_y, bool long_text) {
    assert(cui_begin_frame(ui, 300, 180, .016f));
    cui_begin_box(ui, 80, (cui_box){.width = 300, .height = 180});
    cui_begin_absolute_box(ui, 81, (cui_box){0}, (cui_rect){250, anchor_y, 40, 24});
    cui_interact(ui, 81, CUI_ROLE_BUTTON, cui_string("Profile"), false, false, 0);
    cui_end_box(ui);
    if (cui_begin_hover_card(ui, 82, t, state, cui_string("Profile card"),
                             (cui_hover_options){.trigger = 81,
                                                 .viewport = {0, 0, 300, 180},
                                                 .width = width,
                                                 .height = height,
                                                 .max_height = maximum,
                                                 .content_revision = revision,
                                                 .side = CUI_POPUP_BELOW,
                                                 .gap = 4})) {
        cui_label(ui, t, cui_string("Alex Morgan"));
        cui_label(
            ui, t,
            cui_string(long_text
                           ? "A composable floating card with useful profile details that reflow."
                           : "Two lines."));
        cui_end_hover_card(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void hover_intrinsic(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    cui_theme t = cui_theme_light();
    cui_hover_state state = {0};
    intrinsic_hover_scene(ui, &t, &state, 200, 0, 0, 0, 5, false);
    cui_focus(ui, 81);
    for (unsigned i = 0; i < 3; i++)
        intrinsic_hover_scene(ui, &t, &state, 200, 0, 0, 0, 5, false);
    cui_rect small = cui_bounds(ui, 82);
    assert(state.open && small.h > 30 && small.h < 100);
    assert(small.x >= 0 && small.x + small.w <= 300);
    assert(!cui_needs_redraw(ui));
    /* The first estimate fits below, but the real card must flip above. Track
     * unclipped content height rather than learning the temporary clipped cap. */
    cui_context *edge = cui_create((cui_config){.max_elements = 256});
    assert(edge);
    cui_hover_state edge_state = {0};
    intrinsic_hover_scene(edge, &t, &edge_state, 200, 0, 0, 0, 100, false);
    cui_focus(edge, 81);
    for (unsigned i = 0; i < 3; i++)
        intrinsic_hover_scene(edge, &t, &edge_state, 200, 0, 0, 0, 100, false);
    cui_rect middle = cui_bounds(edge, 82);
    assert(fabsf(middle.h - small.h) < .02f && middle.y + middle.h <= 100);
    cui_destroy(edge);
    for (unsigned i = 0; i < 3; i++)
        intrinsic_hover_scene(ui, &t, &state, 220, 0, 0, 1, 145, true);
    cui_rect wrapped = cui_bounds(ui, 82);
    assert(wrapped.h > small.h && wrapped.y >= 0 && wrapped.y + wrapped.h <= 180);
    assert(wrapped.y + wrapped.h <= cui_bounds(ui, 81).y + .02f);
    for (unsigned i = 0; i < 3; i++)
        intrinsic_hover_scene(ui, &t, &state, 160, 0, 0, 1, 145, true);
    cui_rect narrow = cui_bounds(ui, 82);
    assert(narrow.h > wrapped.h && narrow.y >= 0 && narrow.y + narrow.h <= 180);
    for (unsigned i = 0; i < 3; i++)
        intrinsic_hover_scene(ui, &t, &state, 200, 0, 50, 2, 5, false);
    assert(cui_bounds(ui, 82).h <= 50.02f);
    intrinsic_hover_scene(ui, &t, &state, 200, 90, 30, 3, 5, false);
    assert(fabsf(cui_bounds(ui, 82).h - 90) < .02f);
    /* Closed/omitted content does not retain caller storage or pin resources. */
    cui_focus(ui, 0);
    state.open = false;
    intrinsic_hover_scene(ui, &t, &state, 200, 0, 0, 4, 5, false);
    intrinsic_hover_scene(ui, &t, &state, 200, 0, 0, 4, 5, false);
    assert(cui_bounds(ui, 82).w == 0);
    cui_destroy(ui);
}
static void hover_panels(void) {
    cui_theme t = cui_theme_light();
    for (unsigned card = 0; card < 2; ++card) {
        cui_context *ui = cui_create((cui_config){.width = 300, .height = 180});
        assert(ui);
        cui_hover_state s = {0};
        hover_scene(ui, &t, &s, card, 0, false);
        cui_rect trigger = cui_bounds(ui, 41);
        cui_queue_event(
            ui,
            (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = trigger.x + 2, .y = trigger.y + 2});
        hover_scene(ui, &t, &s, card, .1f, false);
        assert(!s.open && s.pending);
        cui_draw_list list = hover_scene(ui, &t, &s, card, .11f, false);
        assert(s.open);
        cui_rect panel = cui_bounds(ui, 42);
        assert(panel.w == 140 && panel.h == 40 && panel.y >= 40);
        bool visible = false;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_TEXT && c->bounds.y >= 40) {
                visible = true;
                if (!card) {
                    assert(fabsf(c->bounds.x + c->bounds.w / 2 - panel.x - panel.w / 2) < .01f);
                    assert(fabsf(c->bounds.y + c->bounds.h / 2 - panel.y - panel.h / 2) < .01f);
                }
            }
        }
        assert(visible);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == 2 && sem[1].details.parent == 41 && sem[0].details.required);
        if (!card)
            assert(sem[0].details.description.length == 9);
        assert(sem[1].tab_index == -1);
        cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = panel.x + 2, .y = panel.y + 2});
        hover_scene(ui, &t, &s, card, .1f, false);
        hover_scene(ui, &t, &s, card, .11f, false);
        assert(s.open == (bool)card);
        if (card) {
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE});
            hover_scene(ui, &t, &s, true, 0, false);
            assert(!s.open && s.suppressed);
        }
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 290, .y = 170});
        hover_scene(ui, &t, &s, card, 0, false);
        assert(!s.suppressed);
        cui_focus(ui, 41);
        hover_scene(ui, &t, &s, card, .3f, false);
        assert(s.open);
        hover_scene(ui, &t, &s, card, 0, true);
        assert(!s.open);
        cui_destroy(ui);
    }
}

static void semantic_colors(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark()};
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    for (size_t theme = 0; theme < 2; theme++) {
        const cui_theme *t = &themes[theme];
        cui_color accents[] = {t->info, t->success, t->warning, t->danger};
        cui_color bubble_colors[4];
        for (unsigned kind = 0; kind < 4; kind++) {
            cui_begin_frame(ui, 500, 300, .016f);
            cui_alert(ui, 100, t,
                      (cui_alert_options){.kind = (cui_alert_kind)kind,
                                          .message = cui_string("Status"),
                                          .width = 400});
            cui_draw_list draw = cui_end_frame(ui);
            bool border = false, svg = false;
            for (size_t i = 0; i < draw.count; i++) {
                if (draw.commands[i].type == CUI_DRAW_BORDER && draw.commands[i].bounds.w == 400) {
                    assert(!memcmp(&draw.commands[i].color, &accents[kind], sizeof(cui_color)));
                    border = true;
                }
                svg |= draw.commands[i].type == CUI_DRAW_IMAGE;
            }
            assert(border && svg);
            cui_begin_frame(ui, 500, 300, .016f);
            cui_begin_bubble(ui, 101, t, (cui_bubble_variant)(CUI_BUBBLE_INFO + kind), 400);
            cui_text(ui, cui_string("Status"), t->font_size, t->text);
            cui_end_bubble(ui);
            draw = cui_end_frame(ui);
            bool found = false;
            for (size_t i = 0; i < draw.count; i++)
                if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w == 400) {
                    bubble_colors[kind] = draw.commands[i].color;
                    found = true;
                }
            assert(found);
        }
        for (unsigned a = 0; a < 4; a++)
            for (unsigned b = a + 1; b < 4; b++)
                assert(memcmp(&bubble_colors[a], &bubble_colors[b], sizeof(cui_color)));
    }
    cui_destroy(ui);
}
static size_t avatar_stack_frame(cui_context *ui, cui_avatar_image *image, cui_bitmap source,
                                 float width, bool disabled) {
    cui_avatar_item people[8] = {0};
    const char *names[] = {"Alex", "Jamie", "Robin", "Four", "Five", "Six", "Seven", "Eight"};
    for (size_t i = 0; i < 8; i++)
        people[i] = (cui_avatar_item){
            .key = i + 1, .name = cui_string(names[i]), .image = source, .image_state = image};
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 200, 100, .016f));
    size_t selected = cui_avatar_stack(ui, 1000, &t, people, 8,
                                       (cui_avatar_stack_options){.size = 48,
                                                                  .width = width,
                                                                  .ring_width = 2,
                                                                  .interactive = true,
                                                                  .disabled = disabled});
    cui_end_frame(ui);
    return selected;
}
static void image_avatars(void) {
    unsigned char source[8 * 4 * 4];
    for (size_t y = 0; y < 4; y++)
        for (size_t x = 0; x < 8; x++) {
            size_t n = (y * 8 + x) * 4;
            source[n] = x < 2 || x > 5 ? 255 : 0;
            source[n + 1] = x >= 2 && x <= 5 ? 200 : 0;
            source[n + 2] = 20;
            source[n + 3] = 128;
        }
    unsigned char pixels[16 * 16 * 4];
    cui_avatar_image image = {0}, saved;
    assert(!cui_avatar_image_init(&image, pixels, sizeof pixels, 513));
    assert(!cui_avatar_image_init(&image, pixels, 10, 16));
    assert(cui_avatar_image_init(&image, pixels, sizeof pixels, 16));
    cui_bitmap bitmap = {source, 8, 4, 8 * 4, 1};
    assert(cui_avatar_image_prepare(&image, bitmap));
    assert(image.bitmap.pixels == pixels && image.bitmap.width == 16 && pixels[3] == 0);
    size_t center = (8 * 16 + 8) * 4;
    assert(pixels[center] == 0 && pixels[center + 1] == 200 && pixels[center + 3] == 128);
    uint64_t revision = image.bitmap.revision;
    assert(cui_avatar_image_prepare(&image, bitmap) && image.bitmap.revision == revision);
    saved = image;
    assert(!cui_avatar_image_prepare(&image, (cui_bitmap){pixels, 16, 16, 64, 1}));
    assert(memcmp(&image, &saved, sizeof image) == 0);
    source[(2 * 8 + 4) * 4 + 1] = 50;
    bitmap.revision++;
    assert(cui_avatar_image_prepare(&image, bitmap) && image.bitmap.revision == revision + 1);
    assert(pixels[center + 1] < 200);
    for (size_t i = 0; i < sizeof source; i += 4) {
        source[i] = 240;
        source[i + 1] = 10;
        source[i + 2] = 20;
        source[i + 3] = 255;
    }
    bitmap.revision++;
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui);
    assert(avatar_stack_frame(ui, &image, bitmap, 200, false) == SIZE_MAX);
    cui_rect bounds = cui_bounds(ui, 1000);
    assert(bounds.w == 156 && bounds.h == 48);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    cui_id jamie = 0;
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == 5 && !memcmp(sem[i].name.data, "Jamie", 5)) {
            jamie = sem[i].id;
            assert(sem[i].bounds.x == 36 && sem[i].bounds.w == 48 &&
                   sem[i].role == CUI_ROLE_BUTTON);
        }
    assert(jamie);
    cui_draw_list draw;
    /* Use a separate render frame to inspect the genuine masked bitmap output. */
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_theme t = cui_theme_light();
    cui_avatar_ex(
        ui, 3000, &t, cui_string("Photo"),
        (cui_avatar_options){.size = 48, .ring_width = 2, .image = bitmap, .image_state = &image});
    draw = cui_end_frame(ui);
    size_t photos = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            photos++;
            assert(draw.commands[i].bitmap.bitmap.pixels[3] == 0);
        }
    assert(photos == 1);
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &draw, (cui_color){255, 255, 255, 255}));
    const unsigned char *rendered = cui_software_pixels(renderer, NULL, NULL);
    assert(rendered[0] == 255 && rendered[1] == 255 && rendered[2] == 255);
    assert(rendered[(24 * 100 + 24) * 4] > 230 && rendered[(24 * 100 + 24) * 4 + 1] < 20);
    /* A previously prepared photo must not leak into a failed/missing frame.
     * Recovering with the same source reuses the untouched cache. */
    const cui_bitmap failures[] = {{0}, {source, 8, 4, 1, 9}, {0}};
    for (size_t failure = 0; failure < sizeof failures / sizeof failures[0]; failure++) {
        saved = image;
        assert(cui_begin_frame(ui, 100, 100, .016f));
        cui_avatar_ex(ui, 3000, &t, failure == 2 ? cui_string("") : cui_string("Ada Lovelace"),
                      (cui_avatar_options){.size = 48, .interactive = failure == 1,
                                           .image = failures[failure],
                                           .image_state = &image});
        draw = cui_end_frame(ui);
        bool initials = false;
        for (size_t i = 0; i < draw.count; i++) {
            assert(draw.commands[i].type != CUI_DRAW_IMAGE);
            if (draw.commands[i].type == CUI_DRAW_TEXT &&
                draw.commands[i].text.length == (failure == 2 ? 1 : 2) &&
                !memcmp(draw.commands[i].text.data, failure == 2 ? "?" : "AL",
                        failure == 2 ? 1 : 2))
                initials = true;
        }
        assert(initials && memcmp(&image, &saved, sizeof image) == 0);
        sem = cui_semantics(ui, &count);
        bool named = false;
        for (size_t i = 0; i < count; i++)
            if (sem[i].id == 3000) {
                const char *expected = failure == 2 ? "Anonymous" : "Ada Lovelace";
                size_t length = strlen(expected);
                named = sem[i].role == (failure == 1 ? CUI_ROLE_BUTTON : CUI_ROLE_IMAGE) &&
                        sem[i].name.length == length &&
                        !memcmp(sem[i].name.data, expected, length);
            }
        assert(named);
        /* Text commands were checked above; paint the fill without a host font. */
        cui_draw_command shapes[64];
        cui_draw_list fill = draw;
        fill.commands = shapes;
        fill.count = 0;
        assert(draw.count <= sizeof shapes / sizeof shapes[0]);
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type != CUI_DRAW_TEXT)
                shapes[fill.count++] = draw.commands[i];
        assert(cui_software_render(renderer, &fill, (cui_color){255, 255, 255, 255}));
        rendered = cui_software_pixels(renderer, NULL, NULL);
        /* Away from the initials, the failed frame paints the fallback fill. */
        size_t sample = (24 * 100 + 12) * 4;
        assert(rendered[sample] == t.primary.r && rendered[sample + 1] == t.primary.g);
        assert(cui_begin_frame(ui, 100, 100, .016f));
        cui_avatar_ex(ui, 3000, &t, cui_string("Ada Lovelace"),
                      (cui_avatar_options){.size = 48, .image = bitmap, .image_state = &image});
        draw = cui_end_frame(ui);
        photos = 0;
        for (size_t i = 0; i < draw.count; i++)
            photos += draw.commands[i].type == CUI_DRAW_IMAGE;
        assert(photos == 1 && image.bitmap.revision == saved.bitmap.revision);
        assert(cui_software_render(renderer, &draw, (cui_color){255, 255, 255, 255}));
        rendered = cui_software_pixels(renderer, NULL, NULL);
        assert(rendered[(24 * 100 + 24) * 4] > 230);
    }
    cui_software_destroy(renderer);
    avatar_stack_frame(ui, &image, bitmap, 200, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 24}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 40, .y = 24}));
    assert(avatar_stack_frame(ui, &image, bitmap, 200, false) == 1);
    cui_focus(ui, jamie);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(avatar_stack_frame(ui, &image, bitmap, 200, false) == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = 40,
                                           .y = 24,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 1}));
    avatar_stack_frame(ui, &image, bitmap, 200, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = 60,
                                           .y = 24,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 1}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = 40,
                                           .y = 24,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 1}));
    assert(avatar_stack_frame(ui, &image, bitmap, 200, false) == SIZE_MAX);
    avatar_stack_frame(ui, &image, bitmap, 200, true);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 24}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 40, .y = 24}));
    assert(avatar_stack_frame(ui, &image, bitmap, 200, true) == SIZE_MAX);
    avatar_stack_frame(ui, &image, bitmap, 90, false);
    bounds = cui_bounds(ui, 1000);
    assert(bounds.w == 84 && bounds.h == 48);
    sem = cui_semantics(ui, &count);
    bool overflow = false;
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == 13 && !memcmp(sem[i].name.data, "7 more people", 13))
            overflow = true;
    assert(overflow);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    const cui_color background = {17, 39, 61, 255}, foreground = {219, 211, 203, 255};
    cui_avatar_ex(ui, 4000, &t, cui_string("Sam Patel"),
                  (cui_avatar_options){.size = 40,
                                       .custom_background = true,
                                       .custom_foreground = true,
                                       .background = background,
                                       .foreground = foreground});
    draw = cui_end_frame(ui);
    bool initials = false, surface = false;
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *command = &draw.commands[i];
        if (command->type == CUI_DRAW_RECT && command->bounds.w == 40) {
            assert(!memcmp(&command->color, &background, sizeof background));
            surface = true;
        }
        if (command->type == CUI_DRAW_TEXT && command->text.length == 2 &&
            !memcmp(command->text.data, "SP", 2)) {
            assert(!memcmp(&command->color, &foreground, sizeof foreground));
            initials = true;
        }
    }
    assert(initials && surface);
    cui_destroy(ui);
}
int main(void) {
    image_avatars();
    semantic_colors();
    hover_panels();
    hover_intrinsic();
    models();
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 1000});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; ++i) {
        bool visible = true;
        cui_draw_list list = scene(ui, &themes[i], &visible, 0, true);
        bool initials = false, skeleton = false, vertical = false, outline = false;
        for (size_t j = 0; j < list.count; ++j) {
            const cui_draw_command *c = &list.commands[j];
            if (c->type == CUI_DRAW_TEXT && c->text.length == 2 && !memcmp(c->text.data, "AL", 2)) {
                initials = true;
                cui_rect avatar = cui_bounds(ui, 3);
                assert(fabsf(c->bounds.x + c->bounds.w / 2 - avatar.x - avatar.w / 2) < .01f);
                assert(fabsf(c->bounds.y + c->bounds.h / 2 - avatar.y - avatar.h / 2) < .01f);
            }
            if (c->type == CUI_DRAW_RECT && c->bounds.w == 120 && c->bounds.h == 16 &&
                c->color.a == themes[i].border.a)
                skeleton = true;
            if (c->type == CUI_DRAW_RECT && c->bounds.w == 1 && c->bounds.h == 20)
                vertical = true;
            if (c->type == CUI_DRAW_BORDER && c->bounds.w == 320 && c->stroke == 1)
                outline = true;
        }
        assert(initials && skeleton && vertical && outline);
        assert(cui_bounds(ui, 3).w == 48 && cui_bounds(ui, 3).h == 48);
        assert(!cui_focused(ui));
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 7);
        assert(sem[0].id == 2 && sem[0].name.length == 19);
        cui_rect dismiss = cui_bounds(ui, sem[1].id);
        assert(cui_queue_event(
            ui,
            (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = dismiss.x + 2, .y = dismiss.y + 2}));
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = dismiss.x + 2, .y = dismiss.y + 2}));
        scene(ui, &themes[i], &visible, .5f, true);
        assert(!visible);
        scene(ui, &themes[i], &visible, 0, true);
        sem = cui_semantics(ui, &count);
        assert(count == 5 && sem[0].id == 3);
        assert(!cui_focused(ui));
        list = scene(ui, &themes[i], &visible, .5f, true);
        bool opaque = false;
        for (size_t j = 0; j < list.count; ++j)
            if (list.commands[j].bounds.w == 120 && list.commands[j].type == CUI_DRAW_RECT)
                opaque = list.commands[j].color.a == themes[i].border.a;
        assert(opaque);
        scene(ui, &themes[i], &visible, .5f, false);
        assert(cui_needs_redraw(ui));
    }
    cui_destroy(ui);
    return 0;
}
