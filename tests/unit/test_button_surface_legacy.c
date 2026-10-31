/* SPDX-License-Identifier: MIT */
/* External old-library oracle: never recapture after rebuilding production. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "button_surface_test_helpers.h"
#include "cui/cui_button_styles.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned order[64], order_count, slots;
static void click(void *user) {
    (void)user;
    assert(order_count < 64);
    order[order_count++] = 1;
}
static void typed(void *user, const cui_button_click_event *event) {
    (void)user;
    assert(event->target == 10 && order_count < 64);
    order[order_count++] = 2 + (unsigned)event->origin;
}
static void handler(void *user, const cui_component_event_metadata *event) {
    (void)user;
    assert(event->target == 10 && order_count < 64);
    order[order_count++] = 8;
}
static void content(cui_context *ui, void *user) {
    (void)user;
    ++slots;
    cui_begin_box(ui, 33, (cui_box){.width = 20, .height = 10, .background = {90, 10, 200, 255}});
    cui_end_box(ui);
}
static void old_style(void *user, const cui_button_style_info *info,
                      const cui_button_style_paint *current, cui_button_paint_patch *out) {
    (void)user;
    (void)current;
    if (info->phase == CUI_BUTTON_STYLE_INSTANCE) {
        out->fields = CUI_BUTTON_PAINT_RADIUS;
        out->paint.root.radius = 3;
    } else if (info->phase == CUI_BUTTON_STYLE_HOVER) {
        out->fields = CUI_BUTTON_PAINT_BACKGROUND;
        out->paint.root.background = (cui_color){20, 120, 80, 255};
    }
}
static void queue(cui_context *ui, unsigned stage) {
    cui_rect b = cui_bounds(ui, 10);
    cui_event e = {.x = b.x + b.w / 2, .y = b.y + b.h / 2};
    if (stage >= 1 && stage <= 3) {
        e.type = stage == 1   ? CUI_EVENT_POINTER_MOVE
                 : stage == 2 ? CUI_EVENT_POINTER_DOWN
                              : CUI_EVENT_POINTER_UP;
        assert(cui_queue_event(ui, e));
    } else if (stage == 4) {
        for (unsigned i = 0; i < 2; ++i)
            assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    } else if (stage == 5) {
        cui_focus(ui, 10);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
    }
}
static uint64_t state(cui_context *ui, bool result) {
    uint64_t h = word(UINT64_C(14695981039346656037), result);
    h = word(h, slots);
    for (unsigned i = 0; i < order_count; ++i)
        h = word(h, order[i]);
    size_t n;
    const cui_event *events = cui_frame_events(ui, &n);
    for (size_t i = 0; i < n; ++i) {
        h = word(h, events[i].type);
        h = word(h, cui_event_consumed(ui, i));
        h = word(h, (uint32_t)cui_event_accepted_activation(ui, i));
    }
    const cui_semantic *sem = cui_semantics(ui, &n);
    h = word(h, (uint32_t)n);
    for (size_t i = 0; i < n; ++i) {
        h = word(h, (uint32_t)sem[i].id);
        h = word(h, sem[i].role);
        h = number(h, sem[i].bounds.x);
        h = number(h, sem[i].bounds.y);
        h = number(h, sem[i].bounds.w);
        h = number(h, sem[i].bounds.h);
        h = word(h, sem[i].disabled);
        h = word(h, sem[i].checked);
        h = word(h, sem[i].focused);
        h = number(h, sem[i].value);
        h = word(h, (uint32_t)sem[i].tab_index);
        for (size_t j = 0; j < sem[i].name.length; ++j)
            h = word(h, (unsigned char)sem[i].name.data[j]);
    }
    return h;
}
int main(int argc, char **argv) {
    assert(argc == 2);
#ifdef CUI_CAPTURE_LEGACY
    FILE *file = fopen(argv[1], "w");
#else
    FILE *file = fopen(argv[1], "r");
#endif
    assert(file);
    for (unsigned route = 0; route < 6; ++route) {
        cui_context *ui = create(256, 1024);
        cui_set_reduced_motion(ui, true);
        cui_software *r = cui_software_create(320, 160, NULL);
        assert(r);
        cui_theme t = cui_theme_light();
        cui_button_state owner = {0};
        cui_button_group_style_workspace *workspace = calloc(1, sizeof *workspace);
        assert(workspace);
        bool selected[2] = {false, true};
        order_count = slots = 0;
        for (unsigned stage = 0; stage < 10; ++stage) {
            queue(ui, stage);
            assert(cui_begin_frame(ui, 320, 160, .016f));
            cui_begin_box(ui, 900,
                          (cui_box){.width = 300, .height = 140, .padding = 8, .clip = true});
            cui_button_options o = {.width = 110,
                                    .state = &owner,
                                    .on_click = click,
                                    .on_click_event = typed,
                                    .selected = stage == 6,
                                    .open = stage == 7,
                                    .loading = stage == 8,
                                    .disabled = stage == 9,
                                    .dropdown_caret = true};
            if (stage == 7)
                o.content = content;
            bool result = false;
            if (route == 0)
                result = cui_button_ex(ui, 10, &t, cui_string("Save"), o);
            else if (route == 1)
                result = cui_button_styled_ex(
                    ui, 10, &t, cui_string("Save"), o,
                    (cui_button_styles){.instance = old_style, .hovered = old_style});
            else if (route == 2) {
                (void)cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Save"), o.disabled, false,
                                   0);
                cui_button_paint(ui, 10, &t, cui_string("Save"), o);
            } else if (route == 3)
                result = cui_button_events(ui, 10, &t, cui_string("Save"), o,
                                           (cui_component_activation_handler){handler, NULL})
                             .activation_count != 0;
            else if (route == 4) {
                cui_button_group_styled_item items[] = {{10, CUI_STR("Save"), o},
                                                        {11, CUI_STR("Other"), {.width = 80}}};
                cui_button_group_result group;
                assert(cui_button_group_ex(ui, 100, &t, items, selected, 2, workspace,
                                           (cui_button_group_style_options){0}, &group));
                result = group.accepted_activations != 0;
            } else
                result = cui_button(ui, 10, &t, cui_string("Save"), o.disabled);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            uint64_t actual[] = {commands(list), pixels(r, list), state(ui, result)};
#ifdef CUI_CAPTURE_LEGACY
            assert(fprintf(file, "%016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n", actual[0],
                           actual[1], actual[2]) > 0);
#else
            uint64_t expected[3];
            assert(fscanf(file, "%" SCNx64 " %" SCNx64 " %" SCNx64, &expected[0], &expected[1],
                          &expected[2]) == 3);
            if (memcmp(actual, expected, sizeof actual)) {
                fprintf(stderr, "Button legacy mismatch route=%u stage=%u\n", route, stage);
                abort();
            }
#endif
        }
        free(workspace);
        cui_software_destroy(r);
        cui_destroy(ui);
    }
    assert(!ferror(file));
#ifndef CUI_CAPTURE_LEGACY
    unsigned char trailing;
    assert(fscanf(file, " %c", &trailing) == EOF);
#endif
    assert(fclose(file) == 0);
    puts("Button legacy: 60 frozen command/software/state triples pass");
    return 0;
}
