/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_radio_group.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size * .5f, size};
}
static cui_context *create(size_t elements, size_t commands) {
    cui_context *ui = cui_create(
        (cui_config){.max_elements = elements, .max_commands = commands, .measure = measure});
    assert(ui);
    return ui;
}
static uint64_t word(uint64_t hash, uint32_t value) {
    for (unsigned i = 0; i < 4; i++) {
        hash ^= (value >> (8 * i)) & 255;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
static uint64_t number(uint64_t hash, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return word(hash, bits);
}
static uint64_t commands(cui_draw_list list) {
    uint64_t h = word(UINT64_C(14695981039346656037), (uint32_t)list.count);
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        h = word(h, c->type);
        const float values[] = {c->bounds.x,
                                c->bounds.y,
                                c->bounds.w,
                                c->bounds.h,
                                c->radius,
                                c->stroke,
                                c->font_size,
                                c->corner_radii.top_left,
                                c->corner_radii.top_right,
                                c->corner_radii.bottom_left,
                                c->corner_radii.bottom_right};
        for (size_t j = 0; j < sizeof values / sizeof *values; j++)
            h = number(h, values[j]);
        h = word(h, c->color.r | (uint32_t)c->color.g << 8 | (uint32_t)c->color.b << 16 |
                        (uint32_t)c->color.a << 24);
        h = word(h, c->custom_corners);
        h = word(h, c->custom_border_edges);
        const float edges[] = {c->border_edges.left, c->border_edges.right, c->border_edges.top,
                               c->border_edges.bottom};
        for (size_t j = 0; j < 4; j++)
            h = number(h, edges[j]);
        h = word(h, c->text_style.weight);
        h = word(h, c->text_style.italic);
        h = word(h, c->text_style.monospace);
        h = word(h, c->text_style.family);
        h = number(h, c->text_style.letter_spacing);
        if (c->type == CUI_DRAW_IMAGE) {
            h = word(h, c->bitmap.bitmap.width);
            h = word(h, c->bitmap.bitmap.height);
            h = word(h, (uint32_t)c->bitmap.bitmap.revision);
            h = word(h, (uint32_t)(c->bitmap.bitmap.revision >> 32));
            const float image[] = {c->bitmap.source.x, c->bitmap.source.y, c->bitmap.source.w,
                                   c->bitmap.source.h, c->bitmap.opacity};
            for (size_t j = 0; j < 5; j++)
                h = number(h, image[j]);
            h = word(h, c->bitmap.grayscale);
            h = word(h, c->bitmap.custom_tint);
            h = word(h, c->bitmap.tint.r | (uint32_t)c->bitmap.tint.g << 8 |
                            (uint32_t)c->bitmap.tint.b << 16 | (uint32_t)c->bitmap.tint.a << 24);
            for (unsigned y = 0; y < c->bitmap.bitmap.height; y++)
                for (size_t x = 0; x < (size_t)c->bitmap.bitmap.width * 4; x++)
                    h = word(h, c->bitmap.bitmap.pixels[(size_t)y * c->bitmap.bitmap.stride + x]);
        }
        for (size_t j = 0; j < c->text.length; j++)
            h = word(h, (unsigned char)c->text.data[j]);
    }
    return h;
}
/* Actual software composition with text omitted; no fake glyph rasterization. */
static uint64_t pixels(cui_software *r, cui_draw_list list) {
    cui_draw_command filtered[512];
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT)
            filtered[n++] = list.commands[i];
    assert(n <= 512);
    cui_draw_list paint = {filtered, n, list.width, list.height};
    assert(cui_software_render(r, &paint, (cui_color){255, 255, 255, 255}));
    const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
    uint64_t h = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < 320 * 160 * 4; i++) {
        h ^= rgba[i];
        h *= UINT64_C(1099511628211);
    }
    return h;
}
static cui_radio_group_item items[] = {{11, CUI_STR("Alpha"), false}, {12, CUI_STR("Beta"), false}};
/* Loaded from an external frozen fixture captured from the pre-refactor library.
 * Never synthesize expected values from the implementation under test. */
static uint64_t legacy[8][3][2];
#ifndef CUI_CAPTURE_LEGACY
static void load_legacy(const char *path) {
    FILE *file = fopen(path, "r");
    assert(file);
    char line[512];
    size_t n = 0;
    while (fgets(line, sizeof line, file)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        assert(n < 24);
        uint64_t command, pixel;
        assert(sscanf(line, "%" SCNx64 " %" SCNx64, &command, &pixel) == 2);
        legacy[n / 3][n % 3][0] = command;
        legacy[n / 3][n % 3][1] = pixel;
        n++;
    }
    assert(n == 24 && !ferror(file));
    fclose(file);
}
#endif
static void legacy_regressions(bool dump, bool named) {
    cui_theme theme = cui_theme_light();
    for (unsigned route = 0; route < 8; route++) {
        if (dump)
            printf("# route%u\n", route);
        for (unsigned variant = 0; variant < 3; variant++) {
            cui_context *ui = create(128, 512);
            cui_software *r = cui_software_create(320, 160, NULL);
            assert(r && cui_begin_frame(ui, 320, 160, .016f));
            cui_begin_box(ui, 900,
                          (cui_box){.padding = 8, .width = 300, .height = 140, .clip = true});
            cui_check_state check = (cui_check_state)variant;
            bool boolean = variant == 1;
            int radio = variant == 1 ? 1 : 0;
            cui_checkbox_options o = {.disabled = variant == 2};
            cui_id selected = variant == 1 ? 11 : variant == 2 ? 12 : 0;
            cui_radio_group_state group = {0};
#ifndef CUI_CAPTURE_LEGACY
            if (named) {
                cui_selection_indicator_parts part = {101}, parts[] = {{101}, {102}};
                if (route == 0)
                    (void)cui_checkbox_ex_with_parts(ui, 10, &theme, cui_string("Choice"), &check,
                                                     o, part);
                else if (route == 1)
                    (void)cui_checkbox_source_with_parts(ui, 10, &theme, cui_string("Choice"),
                                                         &check, o, part);
                else if (route == 2)
                    (void)cui_checkbox_with_parts(ui, 10, &theme, cui_string("Choice"), &boolean,
                                                  o.disabled, part);
                else if (route == 3)
                    (void)cui_checkbox_events_with_parts(ui, 10, &theme, cui_string("Choice"),
                                                         check, o, part,
                                                         (cui_component_check_handler){0});
                else if (route == 4)
                    (void)cui_radio_ex_with_parts(ui, 10, &theme, cui_string("Choice"), &radio, 1,
                                                  o, part);
                else if (route == 5)
                    (void)cui_radio_with_parts(ui, 10, &theme, cui_string("Choice"), &radio, 1,
                                               o.disabled, part);
                else if (route == 6)
                    assert(cui_radio_group_with_parts(
                               ui, 20, &theme, items, 2, &selected, &group,
                               (cui_radio_group_options){.disabled = o.disabled}, parts, 2)
                               .valid);
                else
                    assert(cui_radio_group_events_with_parts(
                               ui, 20, &theme, items, 2, &selected, &group,
                               (cui_radio_group_options){.disabled = o.disabled}, parts, 2,
                               (cui_selection_handler){0})
                               .legacy.valid);
            } else
#else
            (void)named;
#endif
                if (route == 0)
                (void)cui_checkbox_ex(ui, 10, &theme, cui_string("Choice"), &check, o);
            else if (route == 1)
                (void)cui_checkbox_source(ui, 10, &theme, cui_string("Choice"), &check, o);
            else if (route == 2)
                (void)cui_checkbox(ui, 10, &theme, cui_string("Choice"), &boolean, o.disabled);
            else if (route == 3)
                (void)cui_checkbox_events(ui, 10, &theme, cui_string("Choice"), check, o,
                                          (cui_component_check_handler){0});
            else if (route == 4)
                (void)cui_radio_ex(ui, 10, &theme, cui_string("Choice"), &radio, 1, o);
            else if (route == 5)
                (void)cui_radio(ui, 10, &theme, cui_string("Choice"), &radio, 1, o.disabled);
            else if (route == 6)
                (void)cui_radio_group(ui, 20, &theme, items, 2, &selected, &group,
                                      (cui_radio_group_options){.disabled = o.disabled});
            else
                (void)cui_radio_group_events(ui, 20, &theme, items, 2, &selected, &group,
                                             (cui_radio_group_options){.disabled = o.disabled},
                                             (cui_selection_handler){0});
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            uint64_t c = commands(list), p = pixels(r, list);
            if (dump)
                printf("%016" PRIx64 " %016" PRIx64 "\n", c, p);
            else
                assert(c == legacy[route][variant][0] && p == legacy[route][variant][1]);
            cui_software_destroy(r);
            cui_destroy(ui);
        }
    }
}
#ifndef CUI_CAPTURE_LEGACY
static unsigned shadow_count(cui_draw_list list) {
    unsigned n = 0;
    for (size_t i = 0; i < list.count; i++)
        n += list.commands[i].type == CUI_DRAW_DROP_SHADOW ||
             list.commands[i].type == CUI_DRAW_INSET_SHADOW;
    return n;
}
static void shadow(cui_context *ui, cui_id id, float radius) {
    cui_box_shadow layers[] = {
        {.kind = CUI_BOX_SHADOW_DROP,
         .layer = {.offset_y = 2, .spread = 1, .color = {7, 17, 27, 255}}},
        {.kind = CUI_BOX_SHADOW_INSET, .layer = {.spread = 2, .color = {50, 90, 130, 150}}}};
    assert(cui_element_set_box_shadows(ui, id, layers, 2,
                                       (cui_corner_radii){radius, radius, radius, radius}));
}
static void named_geometry(void) {
    cui_theme t = cui_theme_light();
    for (unsigned route = 0; route < 8; route++) {
        cui_context *ui = create(128, 512);
        cui_software *r = cui_software_create(320, 160, NULL);
        assert(r);
        cui_checkbox_state motion = {0};
        cui_radio_group_state group = {0};
        uint64_t plain = 0;
        for (unsigned frame = 0; frame < 3; frame++) {
            assert(cui_begin_frame(ui, 320, 160, .016f));
            cui_begin_box(ui, 900,
                          (cui_box){.padding = frame == 2 ? 18 : 8,
                                    .width = 300,
                                    .height = 140,
                                    .clip = true,
                                    .custom_opacity = true,
                                    .opacity = .75f});
            cui_check_state check = CUI_CHECK_MIXED;
            bool boolean = true;
            int radio = 1;
            cui_id selected = 11;
            cui_selection_indicator_parts part = {101}, parts[] = {{101}, {102}};
            cui_checkbox_options o = {.state = &motion, .width = 140};
            if (route == 0)
                (void)cui_checkbox_ex_with_parts(ui, 10, &t, cui_string("Choice"), &check, o, part);
            else if (route == 1)
                (void)cui_checkbox_source_with_parts(ui, 10, &t, cui_string("Choice"), &check, o,
                                                     part);
            else if (route == 2)
                (void)cui_checkbox_with_parts(ui, 10, &t, cui_string("Choice"), &boolean, false,
                                              part);
            else if (route == 3) {
                cui_component_action_result a =
                    cui_checkbox_events_with_parts(ui, 10, &t, cui_string("Choice"), check, o, part,
                                                   (cui_component_check_handler){0});
                assert(a.valid && a.submitted);
            } else if (route == 4)
                (void)cui_radio_ex_with_parts(ui, 10, &t, cui_string("Choice"), &radio, 1, o, part);
            else if (route == 5)
                (void)cui_radio_with_parts(ui, 10, &t, cui_string("Choice"), &radio, 1, false,
                                           part);
            else if (route == 6)
                assert(cui_radio_group_with_parts(ui, 20, &t, items, 2, &selected, &group,
                                                  (cui_radio_group_options){0}, parts, 2)
                           .valid);
            else
                assert(cui_radio_group_events_with_parts(ui, 20, &t, items, 2, &selected, &group,
                                                         (cui_radio_group_options){0}, parts, 2,
                                                         (cui_selection_handler){0})
                           .legacy.valid);
            cui_rect indicator = {0}, row = {0};
            assert(cui_track_bounds(ui, 101, &indicator));
            assert(cui_track_bounds(ui, route < 6 ? 10 : 11, &row));
            if (frame)
                shadow(ui, 101, route < 4 ? t.radius * .5f : 8);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK && indicator.w > 0 && row.w > indicator.w);
            assert(indicator.x >= (frame == 2 ? 18 : 8));
            assert(shadow_count(list) == (frame ? 2 : 0));
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_DROP_SHADOW ||
                    list.commands[i].type == CUI_DRAW_INSET_SHADOW)
                    assert(memcmp(&list.commands[i].shadow.caster, &indicator, sizeof indicator) ==
                           0);
            size_t count;
            const cui_semantic *semantics = cui_semantics(ui, &count);
            assert(count == (route < 6 ? 1 : 3));
            for (size_t i = 0; i < count; i++)
                assert(semantics[i].id != 101 && semantics[i].id != 102);
            uint64_t image = pixels(r, list);
            if (!frame)
                plain = image;
            else
                assert(image != plain);
        }
        cui_software_destroy(r);
        cui_destroy(ui);
    }
}

typedef struct {
    cui_check_state value;
    cui_selection_indicator_parts parts;
    unsigned calls, style_order;
} check_owner;
static void request(void *user, cui_check_state value, const cui_activation_snapshot *event) {
    check_owner *o = user;
    assert(event && event->target == 10);
    o->calls++;
    o->value = value;
    o->parts.indicator = 777;
}
static void observed(void *user, const cui_component_check_request *event) {
    check_owner *o = user;
    assert(event->event.target == 10 && event->current == CUI_CHECK_UNCHECKED);
    o->calls++;
    o->value = event->requested;
    o->parts.indicator = 777;
}
static void click(cui_context *ui, cui_rect row) {
    float x = row.x + 3, y = row.y + row.h / 2;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y}));
}
static void requests_and_failures(void) {
    cui_theme t = cui_theme_light();
    /* Accepted source/events callbacks mutate external descriptor storage; the
     * submitted indicator must retain its entry snapshot101. */
    for (unsigned mode = 0; mode < 8; mode++) {
        cui_context *ui = create(128, 512);
        cui_id indicator_id = mode == 7 ? 76894 : 101;
        check_owner o = {.parts = {indicator_id}};
        /* Bounds outputs are borrowed until end_frame, including across the
         * conditional registration below. */
        cui_rect row = {0}, tracked_indicator = {0};
        for (unsigned frame = 0; frame < 2; frame++) {
            if (frame)
                click(ui, row);
            assert(cui_begin_frame(ui, 320, 160, .016f));
            if ((mode == 3 || mode == 7) && frame) {
                cui_begin_box(ui, mode == 7 ? 86448 : 101, (cui_box){.width = 5, .height = 5});
                cui_end_box(ui);
            }
            if (frame && mode == 1)
                o.parts.indicator = 0;
            if (frame && mode == 2)
                o.parts.indicator = 10;
            bool boolean = o.value == CUI_CHECK_CHECKED;
            int radio = o.value == CUI_CHECK_CHECKED ? 1 : 0;
            if (mode == 4) {
                assert(cui_checkbox_with_parts(ui, 10, &t, cui_string("Check"), &boolean, false,
                                               o.parts) == (frame != 0));
                o.value = boolean ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED;
            } else if (mode == 5) {
                assert(cui_radio_with_parts(ui, 10, &t, cui_string("Radio"), &radio, 1, false,
                                            o.parts) == (frame != 0));
                o.value = radio ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED;
            } else if (mode == 0) {
                cui_component_action_result r = cui_checkbox_events_with_parts(
                    ui, 10, &t, cui_string("Check"), CUI_CHECK_UNCHECKED, (cui_checkbox_options){0},
                    o.parts, (cui_component_check_handler){observed, &o});
                assert(r.valid && r.request_count == frame);
            } else
                (void)cui_checkbox_source_with_parts(
                    ui, 10, &t, cui_string("Check"), &o.value,
                    (cui_checkbox_options){.on_change = request, .change_user = &o}, o.parts);
            if (!frame || mode == 0 || (mode >= 4 && mode != 7))
                assert(cui_track_bounds(ui, indicator_id, &tracked_indicator));
            if (!frame || mode == 0 || (mode >= 4 && mode != 7))
                assert(cui_track_bounds(ui, 10, &row));
            if (frame && (mode == 3 || mode == 7))
                assert(!cui_element_set_shadow_preset(ui, indicator_id, CUI_SHADOW_MD,
                                                      (cui_corner_radii){0}));
            cui_draw_list list = cui_end_frame(ui);
            (void)list;
            if (!frame || mode == 0 || (mode >= 4 && mode != 7))
                assert(tracked_indicator.w > 0 && tracked_indicator.h > 0);
            if (frame && (mode == 1 || mode == 2)) {
                assert(o.calls == 0 && o.value == CUI_CHECK_UNCHECKED);
                size_t n;
                (void)cui_semantics(ui, &n);
                assert(n == 0 && cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
            } else if (frame && (mode == 3 || mode == 7)) {
                assert(o.calls == 1 && o.value == CUI_CHECK_CHECKED);
                assert(cui_last_error(ui) == CUI_ERROR_DUPLICATE_ID);
            } else {
                assert(cui_last_error(ui) == CUI_OK);
                assert(o.value == (frame ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED));
                if (mode == 0 || mode == 6)
                    assert(o.calls == frame);
            }
        }
        cui_destroy(ui);
    }
}
typedef struct {
    cui_id selected;
    cui_selection_indicator_parts parts[2];
    unsigned owner_calls, observer_calls, order;
} group_owner;
static void group_request(void *user, cui_id next) {
    group_owner *o = user;
    assert(o->order++ == 0);
    o->owner_calls++;
    o->selected = next;
    o->parts[0].indicator = 777;
    o->parts[1].indicator = 778;
}
static void group_observed(void *user, const cui_selection_change *change) {
    group_owner *o = user;
    assert(o->order++ == 1 && change->value == o->selected);
    o->observer_calls++;
}
static void group_snapshot(void) {
    cui_context *ui = create(128, 512);
    cui_theme t = cui_theme_light();
    cui_radio_group_state state = {0};
    group_owner o = {.selected = 11, .parts = {{101}, {102}}};
    cui_rect target = {0};
    for (unsigned frame = 0; frame < 3; frame++) {
        if (frame == 1)
            click(ui, target);
        assert(cui_begin_frame(ui, 320, 160, .016f));
        cui_radio_group_item ordered[] = {items[frame == 2 ? 1 : 0], items[frame == 2 ? 0 : 1]};
        o.parts[0].indicator = frame == 2 ? 102 : 101;
        o.parts[1].indicator = frame == 2 ? 101 : 102;
        cui_radio_group_event_result r = cui_radio_group_events_with_parts(
            ui, 20, &t, ordered, 2, &o.selected, &state,
            (cui_radio_group_options){.on_change = group_request, .change_user = &o}, o.parts, 2,
            (cui_selection_handler){group_observed, &o});
        assert(r.legacy.valid && r.events.change_count == (frame == 1 ? 1 : 0));
        assert(cui_track_bounds(ui, 12, &target));
        cui_rect indicator = {0};
        assert(cui_track_bounds(ui, 102, &indicator));
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && list.count && indicator.w > 0);
        if (frame == 1)
            assert(o.parts[0].indicator == 777 && o.parts[1].indicator == 778);
        assert(o.owner_calls == (frame ? 1 : 0) && o.observer_calls == (frame ? 1 : 0));
        assert(o.selected == (frame ? 12 : 11));
        if (frame == 2)
            assert(indicator.x == target.x);
    }
    cui_destroy(ui);
    for (unsigned invalid = 0; invalid < 5; invalid++) {
        ui = create(128, 512);
        o = (group_owner){.selected = 11, .parts = {{101}, {102}}};
        if (invalid == 0)
            o.parts[1].indicator = 101;
        if (invalid == 1)
            o.parts[1].indicator = 11;
        if (invalid == 2)
            o.parts[0].indicator = 20;
        if (invalid == 3)
            o.parts[0].indicator = 0;
        assert(cui_begin_frame(ui, 320, 160, .016f));
        cui_radio_group_result r = cui_radio_group_with_parts(
            ui, 20, &t, items, 2, &o.selected, &state,
            (cui_radio_group_options){.on_change = group_request, .change_user = &o}, o.parts,
            invalid == 4 ? 1 : 2);
        assert(!r.valid && !o.owner_calls && o.selected == 11);
        size_t n;
        (void)cui_semantics(ui, &n);
        assert(n == 0 && cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
        assert(cui_end_frame(ui).count == 0);
        cui_destroy(ui);
    }
}
static void capacity_parity(void) {
    cui_theme t = cui_theme_light();
    for (unsigned group_mode = 0; group_mode < 2; group_mode++)
        for (size_t budget = 1; budget <= 16; budget++) {
            cui_error expected_error = CUI_OK;
            size_t expected_commands = 0;
            for (unsigned named = 0; named < 2; named++) {
                cui_context *ui = create(budget, 128);
                assert(cui_begin_frame(ui, 320, 160, .016f));
                cui_check_state value = CUI_CHECK_MIXED;
                cui_radio_group_state state = {0};
                cui_id selected = 11;
                cui_selection_indicator_parts parts[] = {{101}, {102}};
                if (group_mode) {
                    if (named)
                        (void)cui_radio_group_with_parts(ui, 20, &t, items, 2, &selected, &state,
                                                         (cui_radio_group_options){0}, parts, 2);
                    else
                        (void)cui_radio_group(ui, 20, &t, items, 2, &selected, &state,
                                              (cui_radio_group_options){0});
                } else if (named)
                    (void)cui_checkbox_ex_with_parts(ui, 10, &t, cui_string("Label"), &value,
                                                     (cui_checkbox_options){0}, parts[0]);
                else
                    (void)cui_checkbox_ex(ui, 10, &t, cui_string("Label"), &value,
                                          (cui_checkbox_options){0});
                cui_draw_list list = cui_end_frame(ui);
                if (!named) {
                    expected_error = cui_last_error(ui);
                    expected_commands = list.count;
                } else {
                    assert(cui_last_error(ui) == expected_error && list.count == expected_commands);
                }
                cui_destroy(ui);
            }
        }
}
static void refinement(cui_context *ui, void *user, cui_check_state state, bool disabled,
                       cui_checkbox_paint *paint) {
    (void)ui;
    (void)state;
    (void)disabled;
    check_owner *o = user;
    assert(o->style_order++ == 0);
    o->parts.indicator = 999;
    paint->indicator.radius = 0;
}
static void custom_indicator(cui_context *ui, void *user, cui_checkbox_indicator_info info) {
    check_owner *o = user;
    assert(o->style_order++ == 1 && info.value == CUI_CHECK_MIXED);
    cui_begin_box(ui, 333, (cui_box){.width = 3, .height = 3, .background = {1, 2, 3, 255}});
    cui_end_box(ui);
}
static void content(cui_context *ui, void *user) {
    check_owner *o = user;
    assert(o->style_order++ == 2);
    cui_text(ui, cui_string("Content"), 12, (cui_color){0, 0, 0, 255});
}
static void custom_snapshot(void) {
    cui_context *ui = create(128, 512);
    cui_theme t = cui_theme_light();
    check_owner o = {.value = CUI_CHECK_MIXED, .parts = {101}};
    assert(cui_begin_frame(ui, 320, 160, .016f));
    (void)cui_checkbox_ex_with_parts(
        ui, 10, &t, cui_string("Label"), &o.value,
        (cui_checkbox_options){.styles = {.instance = refinement, .user = &o},
                               .indicator = custom_indicator,
                               .indicator_user = &o,
                               .content = content,
                               .content_user = &o,
                               .width = 130},
        o.parts);
    assert(o.style_order == 3 && o.parts.indicator == 999);
    cui_rect indicator = {0}, custom = {0};
    assert(cui_track_bounds(ui, 101, &indicator) && cui_track_bounds(ui, 333, &custom));
    shadow(ui, 101, 0);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && shadow_count(list) == 2 && custom.w == 3);
    assert(custom.x >= indicator.x && custom.y >= indicator.y);
    cui_destroy(ui);
}
#endif
int main(int argc, char **argv) {
#ifdef CUI_CAPTURE_LEGACY
    (void)argc;
    (void)argv;
    legacy_regressions(true, false);
#else
    assert(argc == 2);
    load_legacy(argv[1]);
    legacy_regressions(false, false);
    legacy_regressions(false, true);
    named_geometry();
    requests_and_failures();
    group_snapshot();
    custom_snapshot();
    capacity_parity();
#endif
    return 0;
}
