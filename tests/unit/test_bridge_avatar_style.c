/* SPDX-License-Identifier: MIT */
/* Compile the real portable bridge in this fixture to capture its private
 * renderer's draw list without adding a public testing/renderer API. Link cui,
 * not cui_bridge; no SDL backend or native window is created. */
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>

static cui_draw_list frame(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_begin_frame(app->ui, app->width, app->height, .016f));
    cui_begin_box(app->ui, 1, (cui_box){.width = -1, .height = -1});
    bridge_render_node(app, app->root);
    cui_end_box(app->ui);
    cui_draw_list list = cui_end_frame(app->ui);
    assert(cui_last_error(app->ui) == CUI_OK);
    return list;
}
static bool color_equal(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void diameter(cui_bridge_app *app, cui_bridge_node node, float expected) {
    cui_bridge_rect bounds;
    assert(cui_bridge_node_bounds(app, node, &bounds) == CUI_BRIDGE_OK);
    assert(bounds.width == expected && bounds.height == expected);
}
int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 800, .height = 800};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, avatar, button;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_AVATAR, "avatar", &avatar) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "button", &button) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, avatar, "Ada Lovelace") == CUI_BRIDGE_OK);
    (void)frame(app);
    diameter(app, avatar, 40);
    /* Legacy pixels stay unbounded by the new 512px styled-avatar cap. */
    assert(cui_bridge_set_visual_size(app, avatar, 600, 600) == CUI_BRIDGE_OK);
    (void)frame(app);
    diameter(app, avatar, 600);
    app->width = 300;
    (void)frame(app);
    cui_bridge_rect compact;
    assert(cui_bridge_node_bounds(app, avatar, &compact) == CUI_BRIDGE_OK);
    assert(compact.width == 600); /* Positive legacy sizes stay fixed in narrow layouts. */
    assert(cui_bridge_set_avatar_identity(app, avatar, 1, 0, 0) == CUI_BRIDGE_OK);
    (void)frame(app);
    diameter(app, avatar, 512);
    app->width = 800;
    assert(cui_bridge_set_avatar_identity(app, avatar, 0, 0, 0) == CUI_BRIDGE_OK);
    (void)frame(app);
    diameter(app, avatar, 600);
    assert(cui_bridge_set_visual_size(app, avatar, 60, 60) == CUI_BRIDGE_OK);
    (void)frame(app);
    diameter(app, avatar, 60);
    assert(cui_bridge_set_design_size(app, avatar, CUI_BRIDGE_SIZE_MD) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_avatar_identity(app, avatar, 1, 1, 4) == CUI_BRIDGE_OK);
    cui_draw_list list = frame(app);
    diameter(app, avatar, 48);
    cui_avatar_palette palette;
    assert(cui_avatar_identity(cui_string("Ada Lovelace"), app->theme.dark, true, 4, &palette));
    bool fill = false, initials = false;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *command = &list.commands[i];
        fill |= command->type == CUI_DRAW_RECT && command->bounds.w == 48 && color_equal(command->color, palette.background);
        initials |= command->type == CUI_DRAW_TEXT && command->text.length == 2 && !memcmp(command->text.data, "AL", 2) && color_equal(command->color, palette.foreground);
    }
    assert(fill && initials);
    assert(cui_bridge_set_design_size(app, root, CUI_BRIDGE_SIZE_MD) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_design_size(app, avatar, (cui_bridge_design_size)99) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_button_size(app, avatar, CUI_BRIDGE_SIZE_MD) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_avatar_identity(app, button, 1, 0, 0) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_set_avatar_identity(app, avatar, 2, 0, 0) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_avatar_identity(app, avatar, 1, 1, 12) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_design_size(app, button, CUI_BRIDGE_SIZE_LG) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_button_size(app, button, CUI_BRIDGE_SIZE_BASE) == CUI_BRIDGE_OK);
    char name[] = "user";
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 1, name) == CUI_BRIDGE_OK);
    name[0] = 'X'; /* The bridge owns the resource name. */
    assert(!strcmp(app->nodes[avatar.slot].avatar_placeholder, "user"));
    char too_long[4098]; memset(too_long, 'x', sizeof too_long - 1); too_long[sizeof too_long - 1] = 0;
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 0, too_long) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 0, "\x80") == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 0, NULL) == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 2, "") == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(!strcmp(app->nodes[avatar.slot].avatar_placeholder, "user") && app->nodes[avatar.slot].avatar_anonymous);
    /* Named people keep initials even when a placeholder is configured. */
    list = frame(app);
    for (size_t i = 0; i < list.count; ++i) assert(list.commands[i].type != CUI_DRAW_IMAGE);
    assert(cui_bridge_set_text(app, avatar, "") == CUI_BRIDGE_OK);
    list = frame(app);
    size_t images = 0;
    for (size_t i = 0; i < list.count; ++i) images += list.commands[i].type == CUI_DRAW_IMAGE;
    assert(images == 1); /* Runtime SVG rasterization through the named icon cache. */
    size_t count, avatar_semantics = 0;
    const cui_semantic *semantics = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (semantics[i].id == app->nodes[avatar.slot].id) {
            assert(semantics[i].role == CUI_ROLE_IMAGE && !semantics[i].details.focusable);
            assert(semantics[i].name.length == 9 && !memcmp(semantics[i].name.data, "Anonymous", 9));
            ++avatar_semantics;
        }
    assert(avatar_semantics == 1);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 1, "") == CUI_BRIDGE_OK);
    list = frame(app);
    images = 0;
    for (size_t i = 0; i < list.count; ++i) images += list.commands[i].type == CUI_DRAW_IMAGE;
    assert(images == 1); /* Empty resource name selects the default user mask. */
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 0, "") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_design_size(app, avatar, CUI_BRIDGE_SIZE_INHERIT) == CUI_BRIDGE_OK);
    list = frame(app);
    diameter(app, avatar, 60);
    for (size_t i = 0; i < list.count; ++i) assert(list.commands[i].type != CUI_DRAW_IMAGE);
    assert(cui_bridge_node_remove(app, avatar) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 1, "user") == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_set_avatar_identity(app, avatar, 1, 0, 0) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_set_design_size(app, avatar, CUI_BRIDGE_SIZE_MD) == CUI_BRIDGE_STALE_HANDLE);
    assert(cui_bridge_request_close(app) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_design_size(app, button, CUI_BRIDGE_SIZE_BASE) == CUI_BRIDGE_CLOSED);
    assert(cui_bridge_set_avatar_placeholder(app, avatar, 1, "") == CUI_BRIDGE_CLOSED);
    cui_bridge_destroy(app);
    puts("bridge Avatar style tests passed");
    return 0;
}
