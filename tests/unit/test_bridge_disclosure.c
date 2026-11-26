/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"

static uint64_t icon_masks[8];
static size_t icon_count;
typedef struct {
    cui_rect bounds;
    cui_text_style style;
    float font_size;
    cui_color color;
} text_sample;
static text_sample text_samples[128];
static size_t text_count;
static bool render_disclosure(cui_software *target, const cui_draw_list *draw, cui_color clear) {
    icon_count = 0;
    text_count = 0;
    cui_draw_command commands[128];
    size_t command_count = 0;
    for (size_t i = 0; i < draw->count; ++i) {
        const cui_draw_command *cmd = &draw->commands[i];
        /* This fixture runs on the portable build without a native font provider. */
        if (cmd->type == CUI_DRAW_TEXT) {
            assert(text_count < sizeof text_samples / sizeof text_samples[0]);
            text_samples[text_count++] =
                (text_sample){cmd->bounds, cmd->text_style, cmd->font_size, cmd->color};
            continue;
        }
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

static size_t heading_lines(cui_bridge_app *app, cui_bridge_node node,
                            cui_bridge_disclosure_heading_v1 expected) {
    cui_bridge_rect row = bounds(app, node);
    size_t lines = 0;
    for (size_t i = 0; i < text_count; ++i) {
        const text_sample *draw = &text_samples[i];
        if (draw->bounds.y < row.y || draw->bounds.y >= row.y + row.height)
            continue;
        assert(draw->font_size == expected.font_size && draw->style.weight == expected.weight &&
               draw->style.italic == !!(expected.text_flags & CUI_BRIDGE_DISCLOSURE_TITLE_ITALIC) &&
               draw->style.monospace ==
                   !!(expected.text_flags & CUI_BRIDGE_DISCLOSURE_TITLE_MONOSPACE));
        assert(bridge_pack_color(draw->color) == expected.title_rgba);
        ++lines;
    }
    return lines;
}

static void test_heading_appearance(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 750,
                                  .height = 620};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "Root");
    cui_bridge_node group = create(app, root, CUI_BRIDGE_ACCORDION, "Accordion");
    cui_bridge_node first = create(app, group, CUI_BRIDGE_ITEM, "Workspace and access preferences");
    cui_bridge_node panel = create(app, first, CUI_BRIDGE_LABEL, "Invite a colleague");
    cui_bridge_node second = create(app, group, CUI_BRIDGE_ITEM, "Notifications");
    cui_bridge_node solo = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "Advanced");
    cui_bridge_disclosure_config_v1 card = {
        .struct_size = sizeof card,
        .schema_version = 1,
        .flags = CUI_BRIDGE_DISCLOSURE_JOINED_V1 | CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1,
        .width = -1,
    };
    const cui_bridge_disclosure_heading_v1 normal = {
        .struct_size = sizeof normal,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY | CUI_BRIDGE_DISCLOSURE_HEADING_COLOR,
        .font_size = 22,
        .line_height = 30,
        .weight = 1,
        .align = 2,
        .text_flags = CUI_BRIDGE_DISCLOSURE_TITLE_WRAP,
        .title_rgba = 0x445588ff,
    };
    cui_bridge_disclosure_heading_v1 open = normal;
    open.fields |= CUI_BRIDGE_DISCLOSURE_HEADING_BOX;
    open.width = -1;
    open.padding = 15;
    open.gap = 8;
    open.background_rgba = 0x163b4eff;
    open.font_size = 26;
    open.line_height = 34;
    open.weight = 3;
    open.title_rgba = 0xfffaeaff;
    cui_bridge_disclosure_heading_v1 disabled = normal;
    disabled.fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY;
    disabled.font_size = 18;
    disabled.text_flags |=
        CUI_BRIDGE_DISCLOSURE_TITLE_ITALIC | CUI_BRIDGE_DISCLOSURE_TITLE_MONOSPACE;
    const cui_bridge_disclosure_heading_v1 item_color = {
        .struct_size = sizeof item_color,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_COLOR,
        .font_size = NAN, /* Inactive and ignored. */
        .title_rgba = 0,
    };
    const cui_bridge_disclosure_divider_v1 divider = {
        .struct_size = sizeof divider,
        .schema_version = 1,
        .thickness = 4,
        .inset_left = 12,
        .inset_right = 18,
        .color_rgba = 0x347a97ff,
    };

    assert(cui_bridge_disclosure_set_heading_v1(app, root, 0, &normal) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 3, &normal) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 0, &normal) == CUI_BRIDGE_OK);
    cui_bridge_disclosure_heading_v1 copied = open;
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 1, &copied) == CUI_BRIDGE_OK);
    memset(&copied, 0, sizeof copied);
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 2, &disabled) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_heading_v1(app, second, 0, &item_color) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_heading_v1(app, solo, 0, &normal) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_divider_v1(app, solo, &divider) == CUI_BRIDGE_INVALID_STATE);
    step(app);
    float unjoined_y = bounds(app, second).y;
    cui_bridge_disclosure_divider_v1 reset = divider;
    assert(cui_bridge_disclosure_set_divider_v1(app, group, &reset) == CUI_BRIDGE_OK);
    memset(&reset, 0, sizeof reset);
    step(app);
    expect_empty(app);
    assert(bounds(app, second).y == unjoined_y); /* Dormant before JOINED. */
    assert(cui_bridge_disclosure_configure_v1(app, group, &card) == CUI_BRIDGE_OK);

    cui_bridge_disclosure_heading_v1 invalid = normal;
    invalid.letter_spacing = 1;
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 0, &invalid) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = normal;
    invalid.struct_size -= 4;
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 0, &invalid) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = open;
    invalid.padding = NAN;
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 1, &invalid) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    reset = divider;
    reset.inset_left = -1;
    assert(cui_bridge_disclosure_set_divider_v1(app, group, &reset) == CUI_BRIDGE_INVALID_ARGUMENT);
    step(app);
    expect_empty(app);
    assert(heading_lines(app, first, normal) == 1);
    cui_bridge_disclosure_heading_v1 transparent = normal;
    transparent.title_rgba = 0;
    assert(heading_lines(app, second, transparent) == 1);
    assert(heading_lines(app, solo, normal) == 1);
    cui_bridge_rect first_row = bounds(app, first), second_row = bounds(app, second);
    assert(second_row.y == first_row.y + first_row.height + divider.thickness);
    float seam = second_row.y - 2;
    assert(pixel(first_row.x + 12, seam) == divider.color_rgba);
    assert(pixel(first_row.x + 11, seam) != divider.color_rgba);
    assert(pixel(first_row.x + first_row.width - 19, seam) == divider.color_rgba);
    assert(pixel(first_row.x + first_row.width - 18, seam) != divider.color_rgba);

    app->width = 270;
    step(app);
    assert(bounds(app, first).height > first_row.height && heading_lines(app, first, normal) > 1);
    activate(app, first);
    step(app);
    expect_change(app, first, 1);
    expect_empty(app);
    assert(heading_lines(app, first, open) > 1);
    assert(bounds(app, first).width == bounds(app, group).width);
    assert(row_color(app, first) == open.background_rgba);
    second_row = bounds(app, second);
    cui_bridge_rect body = bounds(app, panel);
    assert(body.y >= bounds(app, first).y + bounds(app, first).height);
    assert(pixel(second_row.x + 13, second_row.y - 2) == divider.color_rgba);
    assert(cui_bridge_set_disabled(app, first, 1) == CUI_BRIDGE_OK);
    step(app);
    disabled.title_rgba = open.title_rgba; /* Disabled typography inherits the expanded color. */
    assert(heading_lines(app, first, disabled) > 1);
    expect_empty(app);

    activate(app, solo);
    step(app);
    expect_change(app, solo, 1);
    assert(heading_lines(app, solo, normal) == 1);
    expect_empty(app);

    second_row = bounds(app, second);
    reset = divider;
    reset.thickness = 0;
    assert(cui_bridge_disclosure_set_divider_v1(app, group, &reset) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, second).y == second_row.y - divider.thickness);
    assert(cui_bridge_disclosure_set_divider_v1(app, group, NULL) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, second).y == second_row.y - divider.thickness + 1);

    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_heading_v1(app, second, 0, NULL) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node replacement = create(app, group, CUI_BRIDGE_ITEM, "Delivery");
    assert(replacement.slot == second.slot);
    assert(cui_bridge_disclosure_set_heading_v1(app, solo, 0, NULL) == CUI_BRIDGE_OK);
    step(app);
    assert(heading_lines(app, replacement, normal) == 1);
    assert(!app->nodes[solo.slot].disclosure_headings);
    expect_empty(app);
    cui_bridge_destroy(app);
}

static void test_interaction_headings(void) {
    const cui_bridge_options options = {.struct_size = sizeof options,
                                        .abi_version = CUI_BRIDGE_ABI_VERSION,
                                        .backend = CUI_BRIDGE_BACKEND_SDL,
                                        .width = 420,
                                        .height = 400};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "Root");
    cui_bridge_node group = create(app, root, CUI_BRIDGE_ACCORDION, "Accordion");
    cui_bridge_node first = create(app, group, CUI_BRIDGE_ITEM, "Notifications");
    cui_bridge_node second = create(app, group, CUI_BRIDGE_ITEM, "Delivery");
    cui_bridge_node fold = create(app, root, CUI_BRIDGE_COLLAPSIBLE, "Advanced");
    const cui_bridge_disclosure_heading_v1 normal = {
        .struct_size = sizeof normal,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY | CUI_BRIDGE_DISCLOSURE_HEADING_COLOR,
        .font_size = 15,
        .line_height = 22,
        .title_rgba = 0x124071ff,
    };
    cui_bridge_disclosure_heading_v1 hover = normal;
    hover.weight = 2;
    const cui_bridge_disclosure_heading_v1 focus = {
        .struct_size = sizeof focus,
        .schema_version = 1,
        .fields = CUI_BRIDGE_DISCLOSURE_HEADING_COLOR,
        .title_rgba = 0xb02050ff,
    };
    cui_bridge_disclosure_heading_v1 press = normal;
    press.fields = CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY;
    press.text_flags = CUI_BRIDGE_DISCLOSURE_TITLE_ITALIC;
    press.weight = 3;
    cui_bridge_disclosure_heading_v1 item = focus;
    item.title_rgba = 0;
    const uint32_t hovered = CUI_BRIDGE_DISCLOSURE_TITLE_HOVERED;
    const uint32_t focused = CUI_BRIDGE_DISCLOSURE_TITLE_FOCUS_VISIBLE;
    const uint32_t pressed = CUI_BRIDGE_DISCLOSURE_TITLE_PRESSED;
    assert(cui_bridge_disclosure_set_heading_v1(app, group, 0, &normal) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_heading_v1(app, fold, 0, &normal) == CUI_BRIDGE_OK);
    cui_bridge_disclosure_heading_v1 copied = hover;
    assert(cui_bridge_disclosure_set_title_state_v1(app, group, hovered, &copied) == CUI_BRIDGE_OK);
    memset(&copied, 0, sizeof copied);
    assert(cui_bridge_disclosure_set_title_state_v1(app, group, focused, &focus) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_title_state_v1(app, group, pressed, &press) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_title_state_v1(app, second, hovered, &item) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_title_state_v1(app, fold, hovered, &hover) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_title_state_v1(app, root, hovered, &hover) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_disclosure_set_title_state_v1(app, group, pressed + 1, &hover) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    copied = hover;
    copied.fields |= CUI_BRIDGE_DISCLOSURE_HEADING_BOX;
    assert(cui_bridge_disclosure_set_title_state_v1(app, group, hovered, &copied) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    step(app);
    assert(heading_lines(app, first, normal) == 1);

    pointer(app, first, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    assert(heading_lines(app, first, hover) == 1);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_TAB, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    hover.title_rgba = focus.title_rgba; /* A color-only focus layer inherits hover typography. */
    assert(heading_lines(app, first, hover) == 1);
    expect_empty(app);

    pointer(app, second, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    hover.title_rgba = 0;
    assert(heading_lines(app, second, hover) == 1);
    pointer(app, second, CUI_BRIDGE_POINTER_DOWN);
    step(app);
    press.title_rgba = 0;
    assert(heading_lines(app, second, press) == 1);
    expect_empty(app);
    pointer(app, second, CUI_BRIDGE_POINTER_UP);
    step(app);
    assert(heading_lines(app, second, hover) == 1);
    expect_change(app, second, 1);
    expect_empty(app);

    assert(cui_bridge_disclosure_set_title_state_v1(app, second, hovered, NULL) == CUI_BRIDGE_OK);
    assert(!app->nodes[second.slot].disclosure_titles);
    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    assert(cui_bridge_disclosure_set_title_state_v1(app, second, hovered, NULL) ==
           CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node replacement = create(app, group, CUI_BRIDGE_ITEM, "Billing");
    assert(replacement.slot == second.slot && !app->nodes[replacement.slot].disclosure_titles);

    step(app);
    pointer(app, fold, CUI_BRIDGE_POINTER_MOVE);
    step(app);
    hover.title_rgba = normal.title_rgba;
    assert(heading_lines(app, fold, hover) == 1);
    assert(cui_bridge_set_disabled(app, fold, 1) == CUI_BRIDGE_OK);
    step(app);
    assert(heading_lines(app, fold, normal) == 1);
    expect_empty(app);
    cui_bridge_destroy(app);
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
    test_heading_appearance();
    test_interaction_headings();
}
