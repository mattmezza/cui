/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"

static uint64_t icon_masks[8];
static size_t icon_count;
static bool render_disclosure(cui_software *target, const cui_draw_list *draw, cui_color clear) {
    icon_count = 0;
    cui_draw_command commands[128];
    size_t command_count = 0;
    for (size_t i = 0; i < draw->count; ++i) {
        const cui_draw_command *cmd = &draw->commands[i];
        /* This fixture runs on the portable build without a native font provider. */
        if (cmd->type == CUI_DRAW_TEXT)
            continue;
        assert(command_count < sizeof commands / sizeof commands[0]);
        commands[command_count++] = *cmd;
        if (cmd->type != CUI_DRAW_IMAGE)
            continue;
        cui_bitmap mask = cmd->bitmap.bitmap;
        uint64_t fingerprint = UINT64_C(14695981039346656037);
        for (unsigned y = 0; y < mask.height; ++y)
            for (unsigned x = 0; x < mask.width; ++x)
                fingerprint = (fingerprint ^ mask.pixels[y * mask.stride + x * 4 + 3]) *
                              UINT64_C(1099511628211);
        assert(icon_count < sizeof icon_masks / sizeof icon_masks[0]);
        icon_masks[icon_count++] = fingerprint;
    }
    cui_draw_list visual = *draw;
    visual.commands = commands;
    visual.count = command_count;
    return cui_software_render(target, &visual, clear);
}

#define CUI_BRIDGE_TEST_RENDER render_disclosure
#include "../fixtures/bridge_step_software.h"

static cui_bridge_node create(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                              const char *key) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    if (kind == CUI_BRIDGE_ITEM || kind == CUI_BRIDGE_LABEL || kind == CUI_BRIDGE_COLLAPSIBLE)
        assert(cui_bridge_set_text(app, node, key) == CUI_BRIDGE_OK);
    return node;
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect box;
    assert(cui_bridge_node_bounds(app, node, &box) == CUI_BRIDGE_OK);
    return box;
}

static void step_at(cui_bridge_app *app, int line) {
    cui_bridge_status status = cui_bridge_step(app, .016f);
    if (status != CUI_BRIDGE_OK)
        fprintf(stderr, "Disclosure test step on line %d: %s, CUI error %d, frame error %d\n", line,
                cui_bridge_status_message(status), cui_last_error(app->ui), app->frame_error);
    assert(status == CUI_BRIDGE_OK);
}
#define step(app) step_at(app, __LINE__)

static void pointer(cui_bridge_app *app, cui_bridge_node node, cui_bridge_pointer_phase phase) {
    cui_bridge_rect box = bounds(app, node);
    assert(cui_bridge_queue_pointer(app, phase, box.x + 6, box.y + box.height / 2) ==
           CUI_BRIDGE_OK);
}

static void activate(cui_bridge_app *app, cui_bridge_node node) {
    assert(cui_queue_accessibility_action(app->ui, app->nodes[node.slot].id,
                                          CUI_ACCESSIBILITY_ACTIVATE));
}

static uint32_t pixel(float x, float y) {
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(software, &width, &height);
    assert(pixels && x >= 0 && x < width && y >= 0 && y < height);
    const unsigned char *p = pixels + ((size_t)y * width + (unsigned)x) * 4;
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t row_color(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect box = bounds(app, node);
    return pixel(box.x + 6, box.y + box.height / 2);
}

static void expect_change(cui_bridge_app *app, cui_bridge_node node, uint32_t expanded) {
    cui_bridge_event_ex event;
    char payload[128];
    uint32_t needed;
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &needed) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.node.slot == node.slot &&
           event.node.generation == node.generation && event.checked == expanded);
    uint32_t value = !expanded;
    assert(cui_bridge_get_checked(app, node, &value) == CUI_BRIDGE_OK && value == expanded);
}

static void expect_empty(cui_bridge_app *app) {
    cui_bridge_event_ex event;
    uint32_t needed;
    assert(cui_bridge_poll_event_ex(app, &event, NULL, 0, &needed) == CUI_BRIDGE_EMPTY);
}

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 360,
                                  .height = 400};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "Root");
    cui_bridge_node accordion = create(app, root, CUI_BRIDGE_ACCORDION, "Accordion");
    cui_bridge_node first = create(app, accordion, CUI_BRIDGE_ITEM, "Account");
    cui_bridge_node body = create(app, first, CUI_BRIDGE_LABEL, "Email delivery");
    cui_bridge_node second = create(app, accordion, CUI_BRIDGE_ITEM, "Permissions");
    cui_bridge_node disclosure = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "Advanced");
    step(app);
    float legacy_height = bounds(app, first).height;
    activate(app, first);
    activate(app, second);
    step(app);
    expect_change(app, first, 1);
    expect_change(app, second, 1);
    activate(app, first);
    step(app);
    expect_empty(app); /* Legacy open items stay open. */
    assert(cui_bridge_set_checked(app, first, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_checked(app, second, 0) == CUI_BRIDGE_OK);

    cui_bridge_disclosure_config_v1 config = {
        .struct_size = sizeof config,
        .schema_version = 1,
        .flags = CUI_BRIDGE_DISCLOSURE_JOINED_V1 | CUI_BRIDGE_DISCLOSURE_SINGLE_V1 |
                 CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1 | CUI_BRIDGE_DISCLOSURE_CUSTOM_CARD_V1 |
                 CUI_BRIDGE_DISCLOSURE_CLIP_CARD_V1,
        .width = -1,
        .design_size = CUI_BRIDGE_SIZE_XL,
        .card_radius = 8,
        .card_background_rgba = 0x223344ff,
        .card_border_rgba = 0xf4a810ff,
        .card_border_width = 2,
        .card_padding = 6,
        .collapsed_icon = "plus",
        .expanded_icon = "minus",
    };
    assert(cui_bridge_disclosure_configure_v1(app, root, &config) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_disclosure_configure_v1(app, disclosure, &config) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_disclosure_configure_v1(app, accordion, &config) == CUI_BRIDGE_OK);

    cui_bridge_disclosure_trigger_style_v1 style = {
        .struct_size = sizeof style,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HOVER_V1 | CUI_BRIDGE_DISCLOSURE_PRESSED_V1 |
                  CUI_BRIDGE_DISCLOSURE_EXPANDED_V1 | CUI_BRIDGE_DISCLOSURE_DISABLED_V1 |
                  CUI_BRIDGE_DISCLOSURE_FOCUS_V1 | CUI_BRIDGE_DISCLOSURE_CURSOR_V1,
        .hover_rgba = 0x185030ff,
        .pressed_rgba = 0x278044ff,
        .expanded_rgba = 0x3762c1ff,
        .disabled_rgba = 0x80684aff,
        .focus_rgba = 0xb13287ff,
        .cursor = CUI_BRIDGE_DISCLOSURE_CURSOR_CROSSHAIR,
    };
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, accordion, &style) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, first, &style) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, disclosure, &style) == CUI_BRIDGE_OK);

    /* Errors and edits to caller buffers leave installed values intact. */
    config.width = NAN;
    assert(cui_bridge_disclosure_configure_v1(app, accordion, &config) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    config.width = 200;
    memset(config.collapsed_icon, 'z', sizeof config.collapsed_icon);
    assert(cui_bridge_disclosure_configure_v1(app, accordion, &config) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    strcpy(config.collapsed_icon, "not-a-catalog-icon");
    assert(cui_bridge_disclosure_configure_v1(app, accordion, &config) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    style.cursor = 999;
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, first, &style) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    step(app);
    expect_empty(app);
    cui_bridge_rect group = bounds(app, accordion), row = bounds(app, first);
    assert(group.width == 328 && row.width == group.width - 12 && row.height > legacy_height);
    assert(row_color(app, first) == 0x223344ff);
    assert(pixel(group.x + group.width / 2, group.y + 1) == 0xf4a810ff);
    assert(icon_count == 3);
    uint64_t collapsed = icon_masks[0];

    app->width = 300;
    step(app);
    assert(bounds(app, accordion).width == 268 && bounds(app, first).width == 256);
    pointer(app, first, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    assert(row_color(app, first) == style.hover_rgba);
    assert(cui_pointer_cursor(app->ui) == CUI_CURSOR_CROSSHAIR);
    pointer(app, first, CUI_BRIDGE_POINTER_DOWN);
    step(app);
    assert(row_color(app, first) == style.pressed_rgba);
    row = bounds(app, first);
    assert(pixel(row.x + row.width / 2, row.y + 1) == style.focus_rgba);
    pointer(app, first, CUI_BRIDGE_POINTER_UP);
    step(app);
    expect_change(app, first, 1);
    expect_empty(app);
    assert(icon_count == 3 && icon_masks[0] != collapsed);
    cui_bridge_rect next = bounds(app, second), content = bounds(app, body);
    assert(content.y >= row.y + row.height && content.y < next.y - 1);
    assert(pixel(row.x + row.width / 2, next.y - 1) == bridge_pack_color(app->theme.border));

    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, 0, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(row_color(app, first) == style.expanded_rgba);
    activate(app, second);
    step(app);
    expect_change(app, first, 0);
    expect_change(app, second, 1);
    expect_empty(app);
    activate(app, second);
    step(app);
    expect_change(app, second, 0);

    assert(cui_bridge_set_disabled(app, first, 1) == CUI_BRIDGE_OK);
    pointer(app, first, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    assert(row_color(app, first) == style.disabled_rgba);
    assert(cui_pointer_cursor(app->ui) != CUI_CURSOR_CROSSHAIR);
    assert(!cui_accessibility_action_supported(app->ui, app->nodes[first.slot].id,
                                               CUI_ACCESSIBILITY_ACTIVATE));
    pointer(app, first, CUI_BRIDGE_POINTER_DOWN);
    pointer(app, first, CUI_BRIDGE_POINTER_UP);
    step(app);
    expect_empty(app);

    /* Standalone style uses the same paint and its existing queued change. */
    pointer(app, disclosure, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    assert(row_color(app, disclosure) == style.hover_rgba);
    assert(cui_queue_accessibility_action(app->ui, app->nodes[disclosure.slot].id,
                                          CUI_ACCESSIBILITY_FOCUS));
    step(app);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_SPACE, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(row_color(app, disclosure) == style.pressed_rgba);
    expect_empty(app);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_UP, CUI_BRIDGE_KEY_SPACE, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    expect_change(app, disclosure, 1);
    expect_empty(app);
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, disclosure, NULL) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_configure_v1(app, accordion, NULL) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, first).height == legacy_height);
    assert(row_color(app, first) == style.disabled_rgba); /* Independent reset. */
    assert(row_color(app, disclosure) != style.hover_rgba);
    expect_empty(app);

    assert(cui_bridge_node_remove(app, first) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_trigger_style_v1(app, first, NULL) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node replacement = create(app, accordion, CUI_BRIDGE_ITEM, "Replacement");
    assert(replacement.slot == first.slot && replacement.generation != first.generation);
    assert(!app->nodes[replacement.slot].disclosure_style);
    step(app);
    assert(row_color(app, replacement) != style.disabled_rgba);
    cui_bridge_destroy(app);
}
