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

static void expect_visual(cui_bridge_app *app, const char *text, bool pill, bool outline) {
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
            ++text_count;
        } else if (op->type == CUI_DRAW_RECT)
            ++fills;
        else if (op->type == CUI_DRAW_BORDER)
            ++borders;
    }
    assert(text_count == 1 && fills == (size_t)pill && borders == (size_t)outline);
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
    expect_visual(app, "Ctrl+Shift+Esc", false, false);
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
    expect_visual(app, "New literal", true, true);
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
    assert(cui_bridge_node_remove(app, replaced) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
