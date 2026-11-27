/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>

#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_WITH_SDL 1
#include "../../src/bridge/cui_bridge.c"

static const char filename[] = "Delivery review with a very long filename for the workspace.pdf";
static const char details[] = "PDF document";
static const uint32_t title_color = UINT32_C(0x295dafff);
static const uint32_t preview_color = UINT32_C(0xff8040ff);

static struct {
    cui_rect image, title, description;
    uint64_t revision;
    unsigned preview_count;
    bool ellipsis, title_styled, percent, scrim, muted_description;
} paint;

static bool render_attachment(cui_software *target, const cui_draw_list *list, cui_color clear) {
    memset(&paint, 0, sizeof paint);
    cui_draw_command commands[256];
    size_t count = 0;
    for (size_t i = 0; i < list->count; i++) {
        const cui_draw_command *cmd = &list->commands[i];
        if (cmd->type == CUI_DRAW_TEXT) {
            if (bridge_pack_color(cmd->color) == title_color) {
                paint.title = cmd->bounds;
                paint.ellipsis =
                    cmd->text.length < strlen(filename) && cmd->text.length >= 3 &&
                    memcmp(cmd->text.data + cmd->text.length - 3, "\xe2\x80\xa6", 3) == 0;
                paint.title_styled = cmd->font_size == 18 &&
                                     cmd->text_style.weight == CUI_TEXT_WEIGHT_BOLD &&
                                     cmd->text_style.italic;
            }
            if (cmd->text.length == strlen(details) &&
                !memcmp(cmd->text.data, details, cmd->text.length)) {
                paint.description = cmd->bounds;
                paint.muted_description =
                    bridge_pack_color(cmd->color) == bridge_pack_color(cui_theme_light().muted);
            }
            if (cmd->text.length == sizeof "· 40%" - 1 &&
                !memcmp(cmd->text.data, "· 40%", cmd->text.length))
                paint.percent = true;
            continue; /* The software fixture has no font provider. */
        }
        if (cmd->type == CUI_DRAW_IMAGE && cmd->bitmap.bitmap.width == 4 &&
            cmd->bitmap.bitmap.height == 2) {
            paint.image = cmd->bounds;
            paint.revision = cmd->bitmap.bitmap.revision;
            paint.preview_count++;
        }
        if (cmd->type == CUI_DRAW_RECT && cmd->color.a == (uint8_t)(.55f * 255))
            paint.scrim = true;
        assert(count < sizeof commands / sizeof commands[0]);
        commands[count++] = *cmd;
    }
    cui_draw_list raster = *list;
    raster.commands = commands;
    raster.count = count;
    return cui_software_render(target, &raster, clear);
}

#define CUI_BRIDGE_TEST_RENDER render_attachment
#include "../fixtures/bridge_step_software.h"

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static cui_bridge_rect bounds(cui_bridge_app *app, cui_bridge_node node) {
    cui_bridge_rect result;
    assert(cui_bridge_node_bounds(app, node, &result) == CUI_BRIDGE_OK);
    return result;
}

static bool has_label(cui_bridge_app *app, const char *description) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; i++) {
        if (nodes[i].role == CUI_ROLE_LABEL && nodes[i].name.length == strlen(description) &&
            !memcmp(nodes[i].name.data, description, nodes[i].name.length))
            return true;
    }
    return false;
}

static uint32_t sample_preview(void) {
    unsigned width, height;
    const uint8_t *pixels = cui_software_pixels(software, &width, &height);
    unsigned x = (unsigned)(paint.image.x + paint.image.w / 2);
    unsigned y = (unsigned)(paint.image.y + paint.image.h / 2);
    assert(pixels && x < width && y < height);
    const uint8_t *p = pixels + ((size_t)y * width + x) * 4;
    return bridge_pack_color((cui_color){p[0], p[1], p[2], p[3]});
}

int main(void) {
    const cui_bridge_options options = {.struct_size = sizeof options,
                                        .abi_version = CUI_BRIDGE_ABI_VERSION,
                                        .backend = CUI_BRIDGE_BACKEND_SDL,
                                        .width = 480,
                                        .height = 500};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    app->theme = cui_theme_light();
    cui_bridge_node root, card;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "files", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_ATTACHMENT, "delivery", &card) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, card, filename) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, card, details) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, card, 176, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_UPLOADING) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, card, .4) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, card).width == 176 && !paint.preview_count && paint.percent);
    double value;
    assert(cui_bridge_get_value(app, card, &value) == CUI_BRIDGE_OK && value == .4);

    cui_bridge_attachment_config_v1 config = {
        .struct_size = sizeof config,
        .schema_version = 1,
        .flags =
            CUI_BRIDGE_ATTACHMENT_TITLE_STATUS_V1 | CUI_BRIDGE_ATTACHMENT_DESCRIPTION_STATUS_V1,
        .title_status = CUI_BRIDGE_ATTACHMENT_COMPLETE,
        .description_status = CUI_BRIDGE_ATTACHMENT_COMPLETE,
        .title = {.flags = CUI_BRIDGE_ATTACHMENT_TEXT_STYLE_V1 |
                           CUI_BRIDGE_ATTACHMENT_TEXT_COLOR_V1 |
                           CUI_BRIDGE_ATTACHMENT_TEXT_ITALIC_V1,
                  .weight = CUI_TEXT_WEIGHT_BOLD,
                  .font_size = 18,
                  .color_rgba = title_color},
    };
    assert(cui_bridge_set_detail_text(app, card, "Two\nlines") == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_detail_text(app, card, details) == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_configure_v1(app, root, &config) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, card, "Two\nlines") == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_text(app, card, "Two\rlines") == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_attachment_config_v1 invalid = config;
    invalid.title.letter_spacing = 1; /* The software fixture declares no tracking capability. */
    assert(cui_bridge_attachment_configure_v1(app, card, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = config;
    invalid.media_aspect = NAN;
    assert(cui_bridge_attachment_configure_v1(app, card, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = config;
    invalid.title_status = 9;
    assert(cui_bridge_attachment_configure_v1(app, card, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = config;
    invalid.struct_size -= 4;
    assert(cui_bridge_attachment_configure_v1(app, card, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);

    uint8_t rgba[40];
    memset(rgba, 0, sizeof rgba);
    for (size_t y = 0; y < 2; y++)
        for (size_t x = 0; x < 4; x++)
            memcpy(rgba + y * 20 + x * 4, (uint8_t[]){255, 128, 64, 255}, 4);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, root, rgba, 4, 2, 20, sizeof rgba) ==
           CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, rgba, 4, 2, 20, sizeof rgba) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, rgba, 4, 2, 12, 24) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, NULL, 0, 0, 0, 1) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    memset(rgba, 0, sizeof rgba);
    step(app);
    assert(paint.preview_count == 1 && paint.ellipsis && paint.title_styled && paint.percent);
    assert(paint.title.x >= paint.image.x + paint.image.w);
    assert(has_label(app, filename) && has_label(app, details));

    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(!paint.percent && sample_preview() == preview_color);
    uint64_t first_revision = paint.revision;
    float horizontal_height = bounds(app, card).height;

    config.flags |= CUI_BRIDGE_ATTACHMENT_VERTICAL_V1 | CUI_BRIDGE_ATTACHMENT_MEDIA_V1;
    config.media_aspect = 2;
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_OK);
    config.media_aspect = 1; /* The app already copied the 2:1 aspect. */
    step(app);
    assert(fabsf(paint.image.w / paint.image.h - 2.f) < .001f);
    assert(bounds(app, card).height > horizontal_height &&
           paint.title.y >= paint.image.y + paint.image.h);
    assert(paint.revision == first_revision && sample_preview() == preview_color);

    for (size_t i = 0; i < sizeof rgba / 4; i++)
        memcpy(rgba + i * 4, (uint8_t[]){16, 128, 32, 255}, 4);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, rgba, 4, 2, 16, 32) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(paint.revision > first_revision && sample_preview() == UINT32_C(0x108020ff));

    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_FAILED) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(paint.scrim && paint.muted_description); /* The description's own status wins. */
    config.flags |= CUI_BRIDGE_ATTACHMENT_INDETERMINATE_V1;
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_UPLOADING) ==
           CUI_BRIDGE_OK);
    step(app);
    assert(!paint.percent && has_label(app, details));

    assert(cui_bridge_set_visual_size(app, card, 8, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, card).width == 8 && !paint.preview_count && has_label(app, filename));
    assert(cui_bridge_set_visual_size(app, card, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_design_size(app, card, CUI_BRIDGE_SIZE_XS) == CUI_BRIDGE_OK);
    step(app);
    float small = bounds(app, card).width;
    assert(cui_bridge_set_design_size(app, card, CUI_BRIDGE_SIZE_3XL) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, card).width > small && paint.preview_count == 1);

    assert(cui_bridge_set_text(app, card, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_detail_text(app, card, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_attachment_status(app, card, CUI_BRIDGE_ATTACHMENT_COMPLETE) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, card, 162, 0) == CUI_BRIDGE_OK);
    config.media_aspect = 2;
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_OK);
    step(app);
    assert(bounds(app, card).height == 82 && paint.image.w == 160 && paint.image.h == 80);

    cui_bridge_node overlay, play;
    assert(cui_bridge_attachment_slot_create_v1(app, card, CUI_BRIDGE_ATTACHMENT_SLOT_OVERLAY_V1,
                                                "overlay", 0, &overlay) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, overlay, CUI_BRIDGE_BUTTON, "play", &play) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, play, "View") == CUI_BRIDGE_OK);
    cui_color overlay_color = {57, 108, 239, 255};
    app->theme.buttons[CUI_THEME_BUTTON_PRIMARY].normal = overlay_color;
    step(app);
    assert(paint.preview_count == 1 && sample_preview() == bridge_pack_color(overlay_color));
    assert(cui_bridge_node_remove(app, overlay) == CUI_BRIDGE_OK);
    step(app);
    assert(sample_preview() == UINT32_C(0x108020ff));

    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, NULL, 0, 0, 0, 0) == CUI_BRIDGE_OK);
    step(app);
    assert(!paint.preview_count);
    assert(cui_bridge_attachment_configure_v1(app, card, NULL) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, card, "Two\nlines") == CUI_BRIDGE_OK);
    step(app);
    cui_bridge_event event;
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_set_text(app, card, filename) == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, card) == CUI_BRIDGE_OK);
    assert(cui_bridge_attachment_configure_v1(app, card, &config) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_attachment_set_preview_rgba_v1(app, card, NULL, 0, 0, 0, 0) ==
           CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
}
