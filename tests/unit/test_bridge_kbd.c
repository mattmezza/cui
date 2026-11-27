/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

static void expect_label(cui_bridge_app *app, cui_bridge_node handle, const char *name) {
    cui_id id = app->nodes[handle.slot].id;
    size_t count = 0;
    const cui_semantic *nodes = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; ++i) {
        if (nodes[i].id != id)
            continue;
        assert(nodes[i].role == CUI_ROLE_LABEL && nodes[i].tab_stop_disabled);
        assert(nodes[i].name.length == strlen(name) &&
               !memcmp(nodes[i].name.data, name, nodes[i].name.length));
        return;
    }
    assert(!"missing shortcut label");
}

static void expect_visual_with_text(cui_bridge_app *app, const char *text, bool pill, bool outline,
                                    float size, cui_color color) {
    assert(cui_begin_frame(app->ui, app->width, app->height, .016f));
    bridge_render_node(app, app->root);
    cui_draw_list draw = cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK);
    size_t text_count = 0, fills = 0, borders = 0;
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *op = &draw.commands[i];
        if (op->type == CUI_DRAW_TEXT) {
            assert(op->text.length == strlen(text) &&
                   !memcmp(op->text.data, text, op->text.length));
            if (size) {
                assert(op->font_size == size);
                assert(op->color.r == color.r && op->color.g == color.g && op->color.b == color.b &&
                       op->color.a == color.a);
            }
            ++text_count;
        } else if (op->type == CUI_DRAW_RECT)
            ++fills;
        else if (op->type == CUI_DRAW_BORDER)
            ++borders;
    }
    assert(text_count == 1 && fills == (size_t)pill && borders == (size_t)outline);
}

static void expect_visual(cui_bridge_app *app, const char *text, bool pill, bool outline) {
    expect_visual_with_text(app, text, pill, outline, 0, (cui_color){0});
}

static cui_bridge_rect expect_styled(cui_bridge_app *app, cui_bridge_node hint, float width,
                                     float pill_width, size_t lines) {
    app->width = width;
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds;
    assert(cui_bridge_node_bounds(app, hint, &bounds) == CUI_BRIDGE_OK);
    assert(bounds.x == 16 && bounds.width == pill_width);
    assert(cui_begin_frame(app->ui, app->width, app->height, .016f));
    bridge_render_node(app, app->root);
    cui_draw_list draw = cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK);
    size_t text_count = 0;
    bool fill = false, border = false;
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *cmd = &draw.commands[i];
        if (cmd->type == CUI_DRAW_TEXT) {
            assert(cmd->font_size == 14 && cmd->bounds.h == 20);
            assert(cmd->text_style.monospace && cmd->text_style.italic &&
                   cmd->text_style.weight == CUI_TEXT_WEIGHT_BOLD);
            assert(cmd->text_paint.kind == CUI_TEXT_PAINT_LINEAR &&
                   cmd->text_paint.stop_count == 2);
            assert(cmd->text_paint.stops[0].color.r == 242 &&
                   cmd->text_paint.stops[1].color.b == 211);
            assert(cmd->bounds.x >= bounds.x &&
                   cmd->bounds.x + cmd->bounds.w <= bounds.x + bounds.width + .1f);
            ++text_count;
        } else if (cmd->type == CUI_DRAW_RECT) {
            assert(cmd->color.r == 28 && cmd->color.g == 50 && cmd->radius == 5);
            fill = true;
        } else if (cmd->type == CUI_DRAW_BORDER) {
            assert(cmd->color.g == 160 && cmd->stroke == 2);
            border = true;
        }
    }
    assert(fill && border && text_count == lines);
    return bounds;
}

static void test_style(cui_bridge_app *app, cui_bridge_node root, cui_bridge_node hint) {
    const uint32_t fields = CUI_BRIDGE_KBD_STYLE_FILL | CUI_BRIDGE_KBD_STYLE_WRAP |
                            CUI_BRIDGE_KBD_STYLE_PADDING | CUI_BRIDGE_KBD_STYLE_RADIUS |
                            CUI_BRIDGE_KBD_STYLE_BACKGROUND | CUI_BRIDGE_KBD_STYLE_BORDER |
                            CUI_BRIDGE_KBD_STYLE_FONT_SIZE | CUI_BRIDGE_KBD_STYLE_LINE_HEIGHT |
                            CUI_BRIDGE_KBD_STYLE_WEIGHT | CUI_BRIDGE_KBD_STYLE_ITALIC |
                            CUI_BRIDGE_KBD_STYLE_MONOSPACE | CUI_BRIDGE_KBD_STYLE_TEXT_GRADIENT;
    cui_bridge_kbd_style_v1 style = {.struct_size = sizeof style,
                                     .schema_version = 1,
                                     .fields = fields,
                                     .background_rgba = 0x1c3260ff,
                                     .border_rgba = 0x50a060ff,
                                     .weight = CUI_TEXT_WEIGHT_BOLD,
                                     .padding_x = 4,
                                     .padding_y = 3,
                                     .radius = 5,
                                     .border_width = 2,
                                     .font_size = 14,
                                     .line_height = 20,
                                     .stop_count = 2,
                                     .x1 = 1,
                                     .stops = {{0, 0xf26530ff}, {1, 0x4030d3ff}}};
    assert(cui_bridge_set_text(app, hint, "Keyboard shortcuts") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_kbd_style_v1(app, root, &style) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_kbd_style_v1(app, hint, &style) == CUI_BRIDGE_OK);
    cui_bridge_kbd_style_v1 invalid = style;
    invalid.reserved = 1;
    assert(cui_bridge_set_kbd_style_v1(app, hint, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = style;
    invalid.fields |= CUI_BRIDGE_KBD_STYLE_TEXT_COLOR;
    assert(cui_bridge_set_kbd_style_v1(app, hint, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = style;
    invalid.stop_count = 9;
    assert(cui_bridge_set_kbd_style_v1(app, hint, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid.stop_count = 2;
    invalid.stops[1].position = NAN;
    assert(cui_bridge_set_kbd_style_v1(app, hint, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    invalid = style;
    invalid.font_size = INFINITY;
    assert(cui_bridge_set_kbd_style_v1(app, hint, &invalid) == CUI_BRIDGE_INVALID_ARGUMENT);
    cui_bridge_kbd_style_v1 capped = style;
    capped.fields |= CUI_BRIDGE_KBD_STYLE_MAX_WIDTH;
    capped.max_width = 105;
    memset(&style, 0, sizeof style);
    cui_bridge_rect wide = expect_styled(app, hint, 240, 208, 1);
    cui_bridge_rect narrow = expect_styled(app, hint, 133, 101, 2);
    assert(narrow.height - wide.height == 20);
    expect_label(app, hint, "Keyboard shortcuts");
    assert(cui_bridge_set_kbd_style_v1(app, hint, &capped) == CUI_BRIDGE_OK);
    (void)expect_styled(app, hint, 240, 105, 2);

    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, CUI_BRIDGE_KBD_PLAIN_V1, "enter", 5, NULL,
                                        0) == CUI_BRIDGE_OK);
    expect_visual(app, "Enter", false, false);
    expect_label(app, hint, "Enter");
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "pagedown", 8, NULL, 0) == CUI_BRIDGE_OK);
    (void)expect_styled(app, hint, 240, 105, 1);
    expect_label(app, hint, "Page Down");
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, NULL, 0, NULL, 0) == CUI_BRIDGE_OK);
    (void)expect_styled(app, hint, 240, 105, 2);
    expect_label(app, hint, "Keyboard shortcuts");
    assert(cui_bridge_set_text(app, hint, "Reset") == CUI_BRIDGE_OK);
    cui_bridge_kbd_style_v1 solid = {
        .struct_size = sizeof solid,
        .schema_version = 1,
        .fields = CUI_BRIDGE_KBD_STYLE_MAX_WIDTH | CUI_BRIDGE_KBD_STYLE_TEXT_COLOR,
        .max_width = 30,
        .text_rgba = 0x001647b0,
    };
    assert(cui_bridge_set_kbd_style_v1(app, hint, &solid) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds;
    assert(cui_bridge_node_bounds(app, hint, &bounds) == CUI_BRIDGE_OK && bounds.width == 30);
    assert(cui_begin_frame(app->ui, app->width, app->height, .016f));
    bridge_render_node(app, app->root);
    cui_draw_list draw = cui_end_frame(app->ui);
    bool clipped = false, colored = false;
    for (size_t i = 0; i < draw.count; ++i) {
        const cui_draw_command *cmd = &draw.commands[i];
        if (cmd->type == CUI_DRAW_CLIP_PUSH && cmd->bounds.w == bounds.width)
            clipped = true;
        if (cmd->type == CUI_DRAW_TEXT)
            colored = cmd->color.r == 0 && cmd->color.g == 22 && cmd->color.b == 71 &&
                      cmd->color.a == 176;
    }
    assert(clipped && colored);
    assert(cui_bridge_set_kbd_style_v1(app, hint, NULL) == CUI_BRIDGE_OK);
    expect_visual(app, "Reset", true, true);
    cui_bridge_event event;
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);
}

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 640,
                                  .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, hint = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_KBD, "key", &hint) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, hint, "Manual") == CUI_BRIDGE_OK);
    expect_visual(app, "Manual", true, true);

    char key[] = "ß";
    char name[] = "Open search";
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, CUI_BRIDGE_MOD_ALT,
                                        CUI_BRIDGE_KBD_OUTLINE_V1 | CUI_BRIDGE_KBD_MAC_SYMBOLS_V1,
                                        key, sizeof key - 1, name,
                                        sizeof name - 1) == CUI_BRIDGE_OK);
    memset(key, '?', sizeof key);
    memset(name, '?', sizeof name);
    assert(cui_bridge_set_text(app, hint, "New literal") == CUI_BRIDGE_OK);
    char literal[16];
    uint32_t required;
    assert(cui_bridge_get_text(app, hint, literal, sizeof literal, &required) == CUI_BRIDGE_OK);
    assert(!strcmp(literal, "New literal") && required == sizeof "New literal");
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    expect_label(app, hint, "Open search");
    expect_visual(app, "⌥SS", true, true);

    char long_name[1025];
    memset(long_name, 'a', sizeof long_name);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "A", 1, long_name, sizeof long_name) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "A", 1,
                                        "line\xe2\x80\xa8"
                                        "end",
                                        10) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "a\0b", 3, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "\xc0\xaf", 2, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 16, 0, "A", 1, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 8, "A", 1, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "", 0, NULL, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_kbd_stroke_v1(app, root, 0, 0, "A", 1, NULL, 0) ==
           CUI_BRIDGE_INVALID_STATE);
    expect_visual(app, "⌥SS", true, true);
    expect_label(app, hint, "Open search");

    assert(cui_bridge_set_kbd_stroke_v1(app, hint, CUI_BRIDGE_MOD_CTRL | CUI_BRIDGE_MOD_SHIFT,
                                        CUI_BRIDGE_KBD_PLAIN_V1 | CUI_BRIDGE_KBD_OUTLINE_V1,
                                        "escape", 6, NULL, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text_defaults_v1(app, root, 27, 0x1c6578ff, 1) == CUI_BRIDGE_OK);
    expect_visual_with_text(app, "Ctrl+Shift+Esc", false, false, 27,
                            (cui_color){28, 101, 120, 255});
    expect_label(app, hint, "Ctrl+Shift+Esc");
    cui_bridge_rect hit;
    assert(cui_bridge_node_bounds(app, hint, &hit) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, hit.x + 1, hit.y + 1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, hit.x + 1, hit.y + 1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_event event;
    assert(cui_bridge_poll_event(app, &event) == CUI_BRIDGE_EMPTY);

    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, NULL, 0, NULL, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, NULL, 0, NULL, 0) == CUI_BRIDGE_OK);
    expect_visual_with_text(app, "New literal", true, true, app->theme.font_size * .8f,
                            app->theme.text);
    assert(cui_bridge_set_text_defaults_v1(app, root, 0, 0, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "enter", 5, NULL, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, hint) == CUI_BRIDGE_OK);
    cui_bridge_node replaced = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_KBD, "replacement", &replaced) ==
           CUI_BRIDGE_OK);
    assert(replaced.slot == hint.slot && replaced.generation != hint.generation);
    assert(cui_bridge_set_kbd_stroke_v1(app, hint, 0, 0, "escape", 6, NULL, 0) ==
           CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_set_text(app, replaced, "Replacement") == CUI_BRIDGE_OK);
    expect_visual(app, "Replacement", true, true);
    assert(cui_bridge_set_kbd_stroke_v1(app, replaced, 0, 0, "enter", 5, NULL, 0) == CUI_BRIDGE_OK);
    expect_visual(app, "Enter", true, false);
    assert(cui_bridge_set_kbd_stroke_v1(app, replaced, 0, 0, NULL, 0, NULL, 0) == CUI_BRIDGE_OK);
    test_style(app, root, replaced);
    const cui_bridge_kbd_style_v1 cleanup = {
        .struct_size = sizeof cleanup, .schema_version = 1, .fields = CUI_BRIDGE_KBD_STYLE_WRAP};
    assert(cui_bridge_set_kbd_style_v1(app, replaced, &cleanup) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, replaced) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_kbd_style_v1(app, replaced, &cleanup) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_destroy(app);
    return 0;
}
