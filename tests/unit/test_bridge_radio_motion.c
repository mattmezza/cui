/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <stdlib.h>

static unsigned allocations_until_failure;
static void *radio_test_calloc(size_t count, size_t size) {
    if (allocations_until_failure && !--allocations_until_failure)
        return NULL;
    return calloc(count, size);
}

#define calloc radio_test_calloc
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"
#undef calloc

static cui_draw_list painted;
static bool paint(cui_software *renderer, const cui_draw_list *list, cui_color clear) {
    painted = *list;
    cui_draw_command commands[32];
    size_t count = 0;
    for (size_t i = 0; i < list->count; i++)
        if (list->commands[i].type != CUI_DRAW_TEXT) {
            assert(count < sizeof commands / sizeof commands[0]);
            commands[count++] = list->commands[i];
        }
    cui_draw_list shapes = *list;
    shapes.commands = commands;
    shapes.count = count;
    return cui_software_render(renderer, &shapes, clear);
}
#define CUI_BRIDGE_TEST_RENDER paint
#include "../fixtures/bridge_step_software.h"

static cui_bridge_node node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                            const char *key) {
    cui_bridge_node result;
    assert(cui_bridge_node_create(app, parent, kind, key, &result) == CUI_BRIDGE_OK);
    return result;
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_rect box;
    assert(cui_bridge_node_bounds(app, handle, &box) == CUI_BRIDGE_OK);
    return box;
}

static unsigned mark(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_rect box = bounds(app, handle);
    for (size_t i = 0; i < painted.count; i++) {
        const cui_draw_command *cmd = &painted.commands[i];
        if (cmd->type == CUI_DRAW_IMAGE && cmd->bounds.y >= box.y &&
            cmd->bounds.y < box.y + box.height)
            return cmd->bitmap.tint.a;
    }
    return 0;
}

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static void click(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_rect box = bounds(app, handle);
    float x = box.x + box.width / 2, y = box.y + box.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    step(app);
}

static cui_bridge_status poll(cui_bridge_app *app, cui_bridge_notification_v1 *event) {
    char text[16];
    uint32_t required;
    return cui_bridge_poll_value_notification_v1(app, sizeof *event, CUI_BRIDGE_VALUE_SELECTION_V1,
                                                 event, text, sizeof text, &required);
}

static bool drew_text(const char *value) {
    size_t length = strlen(value);
    for (size_t i = 0; i < painted.count; i++) {
        const cui_draw_command *cmd = &painted.commands[i];
        if (cmd->type == CUI_DRAW_TEXT && cmd->text.length == length &&
            !memcmp(cmd->text.data, value, length))
            return true;
    }
    return false;
}

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 320,
                                  .height = 240,
                                  .max_nodes = 12};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node radio = node(app, root, CUI_BRIDGE_RADIO, "radio");
    cui_bridge_node group = node(app, root, CUI_BRIDGE_RADIO_GROUP, "group");
    cui_bridge_node first = node(app, group, CUI_BRIDGE_ITEM, "first");
    cui_bridge_node second = node(app, group, CUI_BRIDGE_ITEM, "second");
    assert(cui_bridge_set_checked(app, radio, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_selected(app, group, first) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_set_motion_v1(app, radio, 0) == CUI_BRIDGE_OK && !app->radio_motions);
    assert(cui_bridge_radio_set_motion_v1(app, radio, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_radio_set_motion_v1(app, first, 1) == CUI_BRIDGE_INVALID_STATE);
    for (unsigned fail = 1; fail <= 2; fail++) {
        allocations_until_failure = fail;
        assert(cui_bridge_radio_set_motion_v1(app, group, 1) == CUI_BRIDGE_OUT_OF_MEMORY);
        assert(!app->radio_motions);
    }
    assert(cui_bridge_radio_set_motion_v1(app, group, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_set_motion_v1(app, radio, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(cui_bridge_set_text(app, second, "Priority") == CUI_BRIDGE_OK);
    step(app);
    float ordinary_height = bounds(app, second).height;
    assert(cui_bridge_set_detail_text(app, second, "Tracked delivery") == CUI_BRIDGE_OK);
    step(app);
    float described_height = bounds(app, second).height;
    assert(described_height > ordinary_height);
    cui_bridge_node decoration = node(app, second, CUI_BRIDGE_COLUMN, "decoration");
    cui_bridge_node caption = node(app, decoration, CUI_BRIDGE_LABEL, "caption");
    cui_bridge_node avatar = node(app, decoration, CUI_BRIDGE_AVATAR, "avatar");
    cui_bridge_node action = node(app, decoration, CUI_BRIDGE_BUTTON, "hidden-action");
    assert(cui_bridge_set_text(app, caption, "Tracking included") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, avatar, "Delivery team") == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_item_set_content_v1(app, group, 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_INVALID_STATE);
    step(app);
    cui_bridge_rect hidden;
    assert(cui_bridge_node_bounds(app, caption, &hidden) == CUI_BRIDGE_INVALID_STATE);
    assert(!drew_text("Tracking included") && bounds(app, second).height == described_height);
    assert(cui_bridge_node_remove(app, action) == CUI_BRIDGE_OK);
    cui_bridge_badge_config_v1 badge_config = {.struct_size = sizeof badge_config,
                                               .schema_version = 1};
    assert(cui_bridge_badge_default_config_v1(&badge_config) == CUI_BRIDGE_OK);
    assert(cui_bridge_badge_create_v1(app, decoration, "configured", &badge_config, NULL, NULL, 0,
                                      NULL, 0, &action) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_node_remove(app, action) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, decoration, CUI_BRIDGE_BUTTON, "hidden-action", &action) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_badge_create_v1(app, decoration, "configured", &badge_config, NULL, NULL, 0,
                                      NULL, 0, &action) == CUI_BRIDGE_INVALID_STATE);
    step(app);
    assert(drew_text("Tracking included") && bounds(app, second).height > described_height);
    assert(bounds(app, avatar).width > 0);
    size_t semantic_count;
    const cui_semantic *sem = cui_semantics(app->ui, &semantic_count);
    bool described = false;
    for (size_t i = 0; i < semantic_count; i++) {
        assert(sem[i].id != app->nodes[decoration.slot].id &&
               sem[i].id != app->nodes[caption.slot].id && sem[i].id != app->nodes[avatar.slot].id);
        if (sem[i].id != app->nodes[second.slot].id)
            continue;
        assert(sem[i].role == CUI_ROLE_RADIO && sem[i].name.length == 8 &&
               sem[i].details.description.length == 16 &&
               !memcmp(sem[i].details.description.data, "Tracked delivery", 16));
        described = true;
    }
    assert(described);
    unsigned full = app->theme.on_primary.a;
    assert(mark(app, radio) == 0 && mark(app, first) == full && mark(app, second) == 0);

    click(app, caption);
    assert(cui_focused(app->ui) == app->nodes[second.slot].id);
    unsigned first_alpha = mark(app, first), second_alpha = mark(app, second);
    assert(first_alpha > 0 && first_alpha < full && second_alpha > 0 && second_alpha < full);
    cui_bridge_notification_v1 event;
    assert(poll(app, &event) == CUI_BRIDGE_OK && event.kind == CUI_BRIDGE_NOTIFY_CHANGE &&
           event.node.slot == group.slot && event.payload_kind == CUI_BRIDGE_PAYLOAD_SELECTION &&
           event.payload.selected.slot == second.slot);
    cui_bridge_node selected;
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == second.slot);
    assert(cui_bridge_radio_set_motion_v1(app, group, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(mark(app, first) < first_alpha && mark(app, second) > second_alpha);
    assert(poll(app, &event) == CUI_BRIDGE_EMPTY);

    click(app, radio);
    assert(mark(app, radio) > 0 && mark(app, radio) < full);
    assert(poll(app, &event) == CUI_BRIDGE_OK && event.node.slot == radio.slot &&
           event.payload_kind == CUI_BRIDGE_PAYLOAD_LEGACY && event.legacy.checked == 1);
    assert(cui_bridge_radio_set_motion_v1(app, radio, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(mark(app, radio) == full && !bridge_radio_motion(app, radio.slot));
    for (unsigned i = 0; i < 90; i++)
        step(app);
    assert(mark(app, first) == 0 && mark(app, second) == full && !cui_needs_redraw(app->ui));
    assert(poll(app, &event) == CUI_BRIDGE_EMPTY);

    cui_set_reduced_motion(app->ui, true);
    cui_focus(app->ui, app->nodes[second.slot].id);
    step(app);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_UP_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(mark(app, first) == full && mark(app, second) == 0);
    assert(poll(app, &event) == CUI_BRIDGE_OK && event.node.slot == group.slot &&
           event.payload.selected.slot == first.slot);
    assert(poll(app, &event) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 0) == CUI_BRIDGE_OK);
    action = node(app, decoration, CUI_BRIDGE_BUTTON, "legacy-child");
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_INVALID_STATE);
    step(app);
    assert(cui_bridge_node_bounds(app, caption, &hidden) == CUI_BRIDGE_INVALID_STATE);
    assert(!drew_text("Tracking included"));
    assert(cui_bridge_node_remove(app, action) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(drew_text("Tracking included"));
    assert(cui_bridge_get_selected(app, group, &selected) == CUI_BRIDGE_OK &&
           selected.slot == first.slot && poll(app, &event) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_node_remove(app, group) == CUI_BRIDGE_OK);
    assert(cui_bridge_radio_item_set_content_v1(app, second, 1) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_radio_set_motion_v1(app, group, 1) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node next = node(app, root, CUI_BRIDGE_RADIO, "replacement");
    assert(next.slot == group.slot && !bridge_radio_motion(app, next.slot));
    cui_bridge_destroy(app);
}
