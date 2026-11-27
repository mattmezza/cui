/* SPDX-License-Identifier: MIT */
#undef NDEBUG
#include "cui/cui_software.h"
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"

static cui_draw_list rendered;
static bool capture_draw(cui_software *renderer, const cui_draw_list *draw, cui_color clear) {
    (void)renderer;
    (void)clear;
    rendered = *draw;
    return true;
}

#define CUI_BRIDGE_TEST_RENDER capture_draw
#include "../fixtures/bridge_step_software.h"

static cui_bridge_node add(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                           const char *key) {
    cui_bridge_node out;
    assert(cui_bridge_node_create(app, parent, kind, key, &out) == CUI_BRIDGE_OK);
    if (kind == CUI_BRIDGE_BUTTON)
        assert(cui_bridge_set_text(app, out, key) == CUI_BRIDGE_OK);
    return out;
}

static void frame(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static void press(cui_bridge_app *app, cui_bridge_key key) {
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, key, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, key, 0, 0) == CUI_BRIDGE_OK);
    frame(app);
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect result;
    assert(cui_bridge_node_bounds(app, node, &result) == CUI_BRIDGE_OK);
    return result;
}

static bool band(cui_bridge_app *app, cui_bridge_rect owner, float expansion, float stroke,
                 cui_color color) {
    assert(owner.width > 0 && owner.height > 0 && cui_last_error(app->ui) == CUI_OK);
    for (size_t i = 0; i < rendered.count; ++i) {
        const cui_draw_command *draw = &rendered.commands[i];
        if (draw->type == CUI_DRAW_BORDER && draw->stroke == stroke &&
            fabsf(draw->bounds.x - (owner.x - expansion)) < .01f &&
            fabsf(draw->bounds.y - (owner.y - expansion)) < .01f &&
            fabsf(draw->bounds.w - (owner.width + 2 * expansion)) < .01f &&
            fabsf(draw->bounds.h - (owner.height + 2 * expansion)) < .01f &&
            draw->color.r == color.r && draw->color.g == color.g && draw->color.b == color.b &&
            draw->color.a == color.a)
            return true;
    }
    return false;
}

static void expect_event(cui_bridge_app *app, cui_bridge_node node, cui_bridge_event_type type) {
    cui_bridge_event_ex event;
    char text[8];
    uint32_t required;
    assert(cui_bridge_poll_event_ex(app, &event, text, sizeof text, &required) == CUI_BRIDGE_OK);
    assert(event.node.slot == node.slot && event.node.generation == node.generation &&
           event.type == type);
}

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 400,
                                  .height = 300};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = add(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node save = add(app, root, CUI_BRIDGE_BUTTON, "Save");
    app->theme.focus = (cui_color){80, 100, 220, 255};
    cui_color halo = app->theme.focus;
    halo.a /= 2;
    assert(cui_bridge_set_button_focus_v1(app, root, CUI_BRIDGE_BUTTON_FOCUS_VARIANT_V1) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_button_focus_v1(app, save, CUI_BRIDGE_BUTTON_FOCUS_OUTER_RING_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_focus_v1(app, save, UINT32_MAX) == CUI_BRIDGE_INVALID_ARGUMENT);
    frame(app);
    cui_bridge_rect original = bounds(app, save);
    assert(!band(app, original, 3, 3, halo));
    press(app, CUI_BRIDGE_KEY_TAB);
    assert(cui_focused(app->ui) == app->nodes[save.slot].id && band(app, original, 3, 3, halo));
    assert(cui_bridge_set_button_focus_v1(app, save, CUI_BRIDGE_BUTTON_FOCUS_VARIANT_V1) ==
           CUI_BRIDGE_OK);
    frame(app);
    cui_color ink = app->theme.buttons[CUI_THEME_BUTTON_PRIMARY].foreground;
    ink.a = (uint8_t)(ink.a * .6f);
    assert(band(app, original, -app->theme.font_size / 8, 1, ink));
    press(app, CUI_BRIDGE_KEY_ENTER);
    expect_event(app, save, CUI_BRIDGE_EVENT_CLICK);
    cui_bridge_event other;
    assert(cui_bridge_poll_event(app, &other) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_button_focus_v1(app, save, CUI_BRIDGE_BUTTON_FOCUS_LEGACY_V1) ==
           CUI_BRIDGE_OK);
    frame(app);
    assert(band(app, original, 0, 2, app->theme.focus));

    cui_bridge_node dialog = add(app, root, CUI_BRIDGE_DIALOG, "Settings");
    cui_bridge_dialog_config_v1 config = {
        .flags = CUI_BRIDGE_DIALOG_CENTERED_V1 | CUI_BRIDGE_DIALOG_HIDE_CLOSE_V1, .width = 280};
    assert(cui_bridge_dialog_configure_v1(app, dialog, sizeof config, &config) == CUI_BRIDGE_OK);
    cui_bridge_node footer;
    assert(cui_bridge_dialog_slot_create_v1(app, dialog, CUI_BRIDGE_DIALOG_FOOTER_V1, "footer",
                                            &footer) == CUI_BRIDGE_OK);
    cui_bridge_node accept = add(app, footer, CUI_BRIDGE_BUTTON, "Apply");
    assert(cui_bridge_set_button_focus_v1(app, accept, CUI_BRIDGE_BUTTON_FOCUS_OUTER_RING_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_dialog_set_action_v1(app, accept, CUI_BRIDGE_DIALOG_ACTION_CONFIRM_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, dialog, 1) == CUI_BRIDGE_OK);
    frame(app);
    press(app, CUI_BRIDGE_KEY_TAB);
    assert(cui_focused(app->ui) == app->nodes[accept.slot].id);
    assert(band(app, bounds(app, accept), 3, 3, halo));
    press(app, CUI_BRIDGE_KEY_ENTER);
    expect_event(app, accept, CUI_BRIDGE_EVENT_CLICK);
    expect_event(app, dialog, CUI_BRIDGE_EVENT_CLOSE);
    assert(cui_bridge_poll_event(app, &other) == CUI_BRIDGE_EMPTY);
    uint32_t open;
    assert(cui_bridge_get_open(app, dialog, &open) == CUI_BRIDGE_OK && !open);
    assert(cui_focused(app->ui) == app->nodes[save.slot].id);

    cui_bridge_node notice = add(app, root, CUI_BRIDGE_NOTIFICATION, "Updates");
    cui_bridge_node review;
    assert(cui_bridge_set_text(app, notice, "Updates") == CUI_BRIDGE_OK);
    assert(cui_bridge_notification_action_create_v1(app, notice, "review", &review) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, review, "Review") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_focus_v1(app, review, CUI_BRIDGE_BUTTON_FOCUS_OUTER_RING_V1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, notice, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .4f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .4f) == CUI_BRIDGE_OK);
    cui_focus(app->ui, app->nodes[review.slot].id);
    frame(app);
    assert(band(app, bounds(app, review), 3, 3, halo));
    press(app, CUI_BRIDGE_KEY_ENTER);
    expect_event(app, review, CUI_BRIDGE_EVENT_CLICK);
    assert(cui_bridge_poll_event(app, &other) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_get_open(app, notice, &open) == CUI_BRIDGE_OK && open);

    assert(cui_bridge_node_remove(app, save) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_focus_v1(app, save, CUI_BRIDGE_BUTTON_FOCUS_VARIANT_V1) ==
           CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
