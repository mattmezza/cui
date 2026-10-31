/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_surface.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <string.h>

static cui_size measure(void *user, cui_str text, float font) {
    (void)user;
    return (cui_size){(float)text.length * font * .5f, font};
}
static cui_context *create(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256, .max_commands = 1024,
                                            .measure = measure});
    assert(ui);
    return ui;
}
static unsigned shadows(cui_draw_list list) {
    unsigned n = 0;
    for (size_t i = 0; i < list.count; i++)
        n += list.commands[i].type == CUI_DRAW_DROP_SHADOW ||
             list.commands[i].type == CUI_DRAW_INSET_SHADOW;
    return n;
}
/* Font-free software oracle retains every nontext command and actual clipping. */
static uint64_t pixels(cui_software *r, cui_draw_list list) {
    cui_draw_command filtered[1024];
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT)
            filtered[n++] = list.commands[i];
    cui_draw_list paint = {filtered, n, list.width, list.height};
    assert(cui_software_render(r, &paint, (cui_color){255, 255, 255, 255}));
    const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < 400 * 200 * 4; i++) {
        hash ^= rgba[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
static void style(cui_context *ui, cui_id id, bool enabled) {
    cui_box_shadow layers[] = {
        {.layer = {.offset_x = 3, .offset_y = 3, .spread = 1,
                    .color = {19, 41, 83, 220}}, .kind = CUI_BOX_SHADOW_DROP},
        {.layer = {.spread = 2, .color = {131, 17, 219, 180}}, .kind = CUI_BOX_SHADOW_INSET}};
    assert(cui_element_set_box_shadows(ui, id, enabled ? layers : NULL,
                                       enabled ? 2 : 0, (cui_corner_radii){3, 3, 3, 3}));
}
static void child(cui_context *ui, void *user) {
    (void)user;
    cui_begin_box(ui, 901, (cui_box){.width = 70, .height = 24,
                                    .background = {180, 190, 200, 255}});
    cui_end_box(ui);
}
enum { BUTTON, INPUT, BARE_INPUT, AVATAR, PHOTO, BADGE, SWITCH_TRACK, SWITCH_THUMB,
       SLIDER_TRACK, BADGE_COUNT, BADGE_DOT, BADGE_ICON, CASE_COUNT };
static void controls(void) {
    static const unsigned char image[] = {255, 255, 255, 255};
    for (unsigned kind = 0; kind < CASE_COUNT; kind++) {
        cui_context *ui = create();
        cui_software *r = cui_software_create(400, 200, NULL);
        cui_theme t = cui_theme_light();
        char buffer[32] = "value";
        cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
        cui_input_state input = {0};
        cui_avatar_image photo;
        unsigned char image_storage[4];
        assert(r && edit && cui_avatar_image_init(&photo, image_storage, sizeof image_storage, 1));
        uint64_t baseline = 0;
        cui_draw_command old[128];
        size_t old_count = 0, old_semantic_count = 0;
        cui_id old_semantics[64];
        cui_rect retained = {0}, previous = {0};
        for (unsigned frame = 0; frame < 4; frame++) {
            float pad = frame == 2 ? 15 : 8;
            float width = frame == 2 ? 170 : 140;
            assert(cui_begin_frame(ui, 400, 200, .016f));
            cui_begin_box(ui, 1, (cui_box){.width = 360, .height = 150, .padding = pad,
                                            .clip = true, .custom_opacity = true, .opacity = .75f});
            cui_id target = 101;
            bool on = true;
            float value = .5f;
            if (kind == BUTTON)
                (void)cui_button_ex(ui, 101, &t, cui_string("Button"),
                    (cui_button_options){.width = width, .rounding = CUI_BUTTON_ROUNDED_CUSTOM,
                                          .radius = 11});
            else if (kind == INPUT || kind == BARE_INPUT)
                (void)cui_input_ex(ui, 101, &t, cui_string("Input"), edit, &input,
                    (cui_input_options){.width = width, .no_appearance = kind == BARE_INPUT});
            else if (kind == AVATAR || kind == PHOTO)
                (void)cui_avatar_ex(ui, 101, &t, cui_string("Avatar"),
                    (cui_avatar_options){.size = frame == 2 ? 56 : 40,
                       .image_state = kind == PHOTO ? &photo : NULL,
                       .image = {image, 1, 1, 4, 1}, .ring_width = 2});
            else if (kind == BADGE)
                cui_badge(ui, 101, &t, cui_string("Badge"));
            else if (kind == SWITCH_TRACK || kind == SWITCH_THUMB) {
                (void)cui_switch_ex(ui, 101, &t, cui_string("Switch"), &on, NULL,
                                     (cui_switch_options){.width = width});
                target = kind == SWITCH_TRACK ? cui_switch_track_id(101) : cui_switch_thumb_id(101);
            } else if (kind == SLIDER_TRACK) {
                (void)cui_slider(ui, 101, &t, cui_string("Slider"), &value, 0, 1, .1f, false);
                target = cui_slider_track_id(101);
            } else {
                cui_badge_options o = cui_badge_defaults();
                o.variant = kind == BADGE_COUNT ? CUI_BADGE_COUNT
                            : kind == BADGE_DOT ? CUI_BADGE_DOT : CUI_BADGE_ICON;
                o.count = 2;
                o.icon = cui_string("!");
                assert(cui_badge_ex(ui, 101, &t, o, child, NULL));
                target = cui_badge_indicator_id(101);
            }
            assert(cui_track_bounds(ui, target, &retained));
            if (frame)
                style(ui, target, frame != 3);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK && retained.w > 0 && retained.h > 0);
            size_t semantic_count;
            const cui_semantic *semantic = cui_semantics(ui, &semantic_count);
            uint64_t hash = pixels(r, list);
            if (!frame) {
                baseline = hash;
                old_count = list.count;
                assert(old_count <= 128 && semantic_count <= 64);
                memcpy(old, list.commands, old_count * sizeof *old);
                old_semantic_count = semantic_count;
                for (size_t i = 0; i < semantic_count; i++)
                    old_semantics[i] = semantic[i].id;
                previous = retained;
            } else {
                assert(semantic_count == old_semantic_count);
                for (size_t i = 0; i < semantic_count; i++)
                    assert(semantic[i].id == old_semantics[i]);
                if (frame != 3) {
                    assert(shadows(list) == 2 && hash != baseline);
                    unsigned found = 0;
                    for (size_t i = 0; i < list.count; i++) {
                        const cui_draw_command *c = &list.commands[i];
                        if (c->type == CUI_DRAW_DROP_SHADOW || c->type == CUI_DRAW_INSET_SHADOW) {
                            assert(memcmp(&c->shadow.caster, &retained, sizeof retained) == 0);
                            assert(c->shadow.layer.color.a == (c->type == CUI_DRAW_DROP_SHADOW ? 165 : 135));
                            found++;
                        }
                    }
                    assert(found == 2);
                    if (frame == 2)
                        assert(retained.x != previous.x || retained.y != previous.y);
                } else {
                    assert(shadows(list) == 0 && hash == baseline && list.count == old_count);
                    for (size_t i = 0; i < old_count; i++) {
                        assert(list.commands[i].type == old[i].type);
                        assert(memcmp(&list.commands[i].bounds, &old[i].bounds, sizeof(cui_rect)) == 0);
                    }
                }
            }
        }
        cui_edit_destroy(edit);
        cui_software_destroy(r);
        cui_destroy(ui);
    }
}
static void getters_and_absence(void) {
    /* Frozen identities emitted before this accessor slice, not recomputed salts. */
    assert(cui_badge_indicator_id(101) == UINT64_C(0x2780a7c4f39251be));
    assert(cui_badge_indicator_id(102) == UINT64_C(0x2780a7c4f39251bd));
    assert(cui_slider_track_id(101) == UINT64_C(0x39c1a2170384edcf));
    assert(cui_slider_track_id(102) == UINT64_C(0x39c1a2170384edcc));
    assert(cui_badge_indicator_id(0) == 0 && cui_slider_track_id(0) == 0);
    assert(cui_badge_indicator_id(UINT64_C(0x2780a7c4f39251db)) == 0);
    assert(cui_slider_track_id(UINT64_C(0x39c1a2170384edaa)) == 0);
    cui_context *ui = create();
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_badge_options o = cui_badge_defaults();
    assert(cui_badge_ex(ui, 101, &t, o, child, NULL));
    assert(!cui_element_set_shadow_preset(ui, cui_badge_indicator_id(101), CUI_SHADOW_MD,
                                          (cui_corner_radii){0}));
    assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    assert(shadows(cui_end_frame(ui)) == 0);
    cui_destroy(ui);
    /* An existing part colliding with an application ID retains Core rejection. */
    ui = create();
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_begin_box(ui, cui_slider_track_id(101), (cui_box){.width = 8, .height = 8});
    cui_end_box(ui);
    float value = .5f;
    (void)cui_slider(ui, 101, &t, cui_string("Slider"), &value, 0, 1, .1f, false);
    assert(cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
    assert(!cui_element_set_shadow_preset(ui, cui_slider_track_id(101), CUI_SHADOW_MD,
                                          (cui_corner_radii){0}));
    (void)cui_end_frame(ui);
    cui_destroy(ui);
}
static size_t ordinary(cui_draw_list list, cui_draw_command *out) {
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_DROP_SHADOW &&
            list.commands[i].type != CUI_DRAW_INSET_SHADOW)
            out[n++] = list.commands[i];
    return n;
}
static void panels(void) {
    for (unsigned kind = 0; kind < 3; kind++) {
        cui_context *ui = create();
        cui_theme t = cui_theme_light();
        cui_software *r = cui_software_create(400, 200, NULL);
        assert(r);
        cui_surface_state surface;
        cui_surface_init(&surface, 200, kind == 0 ? CUI_SURFACE_POPOVER : CUI_SURFACE_DIALOG);
        cui_hover_state hover = {0};
        /* Real preceding trigger layout is needed by Tooltip placement/hover. */
        assert(cui_begin_frame(ui, 400, 200, .016f));
        (void)cui_button(ui, 101, &t, cui_string("Trigger"), false);
        (void)cui_end_frame(ui);
        cui_focus(ui, 101);
        if (kind < 2)
            cui_surface_open(ui, &surface, 101);
        cui_draw_command baseline[1024], current[1024];
        size_t baseline_count = 0, baseline_semantic_count = 0;
        cui_id semantic_ids[64];
        cui_role semantic_roles[64];
        uint64_t baseline_pixels = 0;
        unsigned char baseline_rgba[400 * 200 * 4];
        for (int frame = -1; frame < 4; frame++) {
            assert(cui_begin_frame(ui, 400, 200, .016f));
            (void)cui_button(ui, 101, &t, cui_string("Trigger"), false);
            if (kind < 2) {
                cui_surface_options o = {.viewport = {0, 0, 400, 200},
                    .width = frame == 2 ? 190 : 170, .height = 140,
                    .anchor = {10, 10, 80, 20}, .no_outside_dismiss = true,
                    .no_escape_dismiss = true};
                bool opened = kind == 0
                    ? cui_popover_begin(ui, &t, &surface, cui_string("Panel"), cui_string("Description"), o,
                        (cui_popover_position){.mode = CUI_POPOVER_POSITION_ANCHOR,
                                               .anchor = CUI_POPOVER_TOP_LEFT, .arrow = true})
                    : cui_surface_begin(ui, &t, &surface, cui_string("Panel"), cui_string("Description"), o);
                assert(opened);
                cui_text(ui, cui_string("Body"), 14, t.text);
                cui_surface_end(ui);
            } else {
                bool opened = cui_tooltip(ui, 200, &t, &hover, cui_string("Tooltip"),
                    (cui_hover_options){.trigger = 101, .viewport = {0, 0, 400, 200},
                       .width = frame == 2 ? 150 : 130, .height = 40,
                       .side = CUI_POPUP_BELOW, .dt = .1f});
                assert(opened);
            }
            cui_rect panel = {0}, trigger = {0};
            assert(cui_track_bounds(ui, 200, &panel) && cui_track_bounds(ui, 101, &trigger));
            if (frame > 0)
                style(ui, 200, frame != 3);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK && panel.w > 0 && panel.h > 0);
            size_t semantic_count;
            const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
            uint64_t hash = pixels(r, list);
            size_t n = ordinary(list, current);
            if (frame < 0)
                continue; /* Settle first-open focus and popup composition. */
            if (!frame) {
                baseline_count = n;
                baseline_semantic_count = semantic_count;
                assert(semantic_count <= 64);
                for (size_t i = 0; i < semantic_count; i++) {
                    semantic_ids[i] = semantics[i].id;
                    semantic_roles[i] = semantics[i].role;
                }
                memcpy(baseline, current, n * sizeof *baseline);
                baseline_pixels = hash;
                memcpy(baseline_rgba, cui_software_pixels(r, NULL, NULL), sizeof baseline_rgba);
            } else {
                assert(semantic_count == baseline_semantic_count);
                for (size_t i = 0; i < semantic_count; i++) {
                    assert(semantics[i].id == semantic_ids[i]);
                    assert(semantics[i].role == semantic_roles[i]);
                }
                assert(shadows(list) == (frame == 3 ? 0 : 2));
                for (size_t i = 0; i < list.count; i++) {
                    const cui_draw_command *c = &list.commands[i];
                    if (c->type == CUI_DRAW_DROP_SHADOW || c->type == CUI_DRAW_INSET_SHADOW) {
                        assert(memcmp(&c->shadow.caster, &panel, sizeof panel) == 0);
                        assert(memcmp(&c->shadow.caster, &trigger, sizeof trigger) != 0);
                    }
                }
                if (frame == 1 || frame == 3) {
                    assert(n == baseline_count);
                    size_t a = 0, b = 0;
                    while (a < n && b < n) {
                        /* Explicit custom shadow paint precedes the owner's
                         * child-overflow clip. Ordinary background-only paint
                         * can sit inside that clip. Compare actual paint order
                         * without requiring identical clip-wrapper placement
                         * during opt-in; cleared/unmodified frames match fully. */
                        if (frame == 1) {
                            while (a < n && (current[a].type == CUI_DRAW_CLIP_PUSH ||
                                             current[a].type == CUI_DRAW_CLIP_POP))
                                a++;
                            while (b < n && (baseline[b].type == CUI_DRAW_CLIP_PUSH ||
                                             baseline[b].type == CUI_DRAW_CLIP_POP))
                                b++;
                            if (a == n || b == n)
                                break;
                        }
                        assert(current[a].type == baseline[b].type);
                        assert(memcmp(&current[a].bounds, &baseline[b].bounds, sizeof(cui_rect)) == 0);
                        assert(memcmp(&current[a].color, &baseline[b].color, sizeof(cui_color)) == 0);
                        a++;
                        b++;
                    }
                    assert(a == n && b == n);
                    assert((hash == baseline_pixels) == (frame == 3));
                    /* Independent ordinary-only software image: every pixel,
                     * including ancestor masks, trigger/backdrop and arrow,
                     * must match without any shadow commands in the oracle. */
                    (void)pixels(r, (cui_draw_list){current, n, list.width, list.height});
                    assert(memcmp(baseline_rgba, cui_software_pixels(r, NULL, NULL),
                                  sizeof baseline_rgba) == 0);
                }
            }
            assert(kind == 2 ? hover.open : surface.overlay.open);
        }
        if (kind == 2) {
            assert(cui_begin_frame(ui, 400, 200, .016f));
            (void)cui_button(ui, 101, &t, cui_string("Trigger"), false);
            assert(!cui_tooltip(ui, 200, &t, &hover, cui_string("Tooltip"),
                                 (cui_hover_options){.trigger = 101, .disabled = true}));
            assert(!cui_element_set_shadow_preset(ui, 200, CUI_SHADOW_MD, (cui_corner_radii){0}));
            assert(shadows(cui_end_frame(ui)) == 0);
        }
        cui_software_destroy(r);
        cui_destroy(ui);
    }
}
static void root_activation(void) {
    for (unsigned decorated = 0; decorated < 2; decorated++) {
        cui_context *ui = create();
        cui_theme t = cui_theme_light();
        bool checked = false;
        cui_rect track = {0};
        for (unsigned frame = 0; frame < 2; frame++) {
            if (frame) {
                float x = track.x + track.w / 2, y = track.y + track.h / 2;
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
                assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
            }
            assert(cui_begin_frame(ui, 400, 200, .016f));
            bool changed = cui_switch_ex(ui, 101, &t, cui_string("Switch"), &checked, NULL,
                                          (cui_switch_options){0});
            assert(changed == (frame != 0) && checked == (frame != 0));
            assert(cui_track_bounds(ui, cui_switch_track_id(101), &track));
            if (decorated)
                style(ui, cui_switch_track_id(101), true);
            (void)cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
        }
        cui_destroy(ui);
    }
}
int main(void) {
    getters_and_absence();
    controls();
    panels();
    root_activation();
    return 0;
}
