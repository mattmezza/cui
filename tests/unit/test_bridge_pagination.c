/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdlib.h>

static unsigned reject_allocations;
static void *test_bridge_calloc(size_t count, size_t size) {
    if (reject_allocations) {
        --reject_allocations;
        return NULL;
    }
    return calloc(count, size);
}

#define calloc test_bridge_calloc
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#undef calloc

static cui_bridge_node create(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind,
                              const char *key) {
    cui_bridge_node node;
    assert(cui_bridge_node_create(app, parent, kind, key, &node) == CUI_BRIDGE_OK);
    return node;
}
static cui_bridge_pagination_config_v1 empty_config(void) {
    return (cui_bridge_pagination_config_v1){.struct_size = sizeof(cui_bridge_pagination_config_v1),
                                             .schema_version = CUI_BRIDGE_PAGINATION_V1};
}
static const cui_semantic *semantic(cui_bridge_app *app, cui_role role, const char *name) {
    size_t count;
    const cui_semantic *items = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (items[i].role == role && items[i].name.length == strlen(name) &&
            !memcmp(items[i].name.data, name, items[i].name.length))
            return &items[i];
    return NULL;
}
static cui_bridge_rect part(cui_bridge_app *app, cui_bridge_node pager, unsigned index) {
    cui_bridge_rect box;
    assert(cui_bridge_numeric_part_bounds(app, pager, index, &box) == CUI_BRIDGE_OK);
    return box;
}
static void press(cui_bridge_app *app, cui_bridge_rect bounds) {
    assert(bounds.width > 0 && bounds.height > 0);
    float x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
}
static void press_semantic(cui_bridge_app *app, cui_role role, const char *name) {
    const cui_semantic *item = semantic(app, role, name);
    assert(item);
    cui_rect b = cui_bounds(app->ui, item->id);
    press(app, (cui_bridge_rect){b.x, b.y, b.w, b.h});
}
static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}
static void page_is(cui_bridge_app *app, cui_bridge_node pager, double page, bool changed) {
    double value;
    assert(cui_bridge_get_value(app, pager, &value) == CUI_BRIDGE_OK && value == page);
    cui_bridge_notification_v1 event;
    char text[1];
    uint32_t required;
    if (changed) {
        assert(cui_bridge_poll_value_notification_v1(app, sizeof event, CUI_BRIDGE_VALUE_SCALAR_V1,
                                                     &event, text, sizeof text,
                                                     &required) == CUI_BRIDGE_OK);
        assert(event.kind == CUI_BRIDGE_EVENT_CHANGE && event.node_kind == CUI_BRIDGE_PAGINATION &&
               event.node.slot == pager.slot && event.node.generation == pager.generation &&
               event.payload_kind == CUI_BRIDGE_PAYLOAD_SCALAR &&
               event.payload.scalar_value == page && event.legacy.value == page);
    }
    assert(cui_bridge_poll_value_notification_v1(app, sizeof event, CUI_BRIDGE_VALUE_SCALAR_V1,
                                                 &event, text, sizeof text,
                                                 &required) == CUI_BRIDGE_EMPTY);
}

int main(void) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 560,
                                  .height = 480,
                                  .max_nodes = 8};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_pagination_versions() == 1 && cui_bridge_pagination_config_size_v1() == 916);
    cui_bridge_node root = create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root");
    cui_bridge_node pager = create(app, root, CUI_BRIDGE_PAGINATION, "pager");
    cui_bridge_pagination_config_v1 config = empty_config(), out = empty_config();
    assert(cui_bridge_pagination_get_config_v1(app, pager, &out) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_pagination_configure_v1(app, root, &config) == CUI_BRIDGE_INVALID_STATE);
    reject_allocations = 1;
    assert(cui_bridge_pagination_configure_v1(app, pager, &config) == CUI_BRIDGE_OUT_OF_MEMORY);
    assert(!app->pagination);
    assert(cui_bridge_set_total(app, pager, 400) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, pager, 10) == CUI_BRIDGE_OK);
    step(app);
    assert(semantic(app, CUI_ROLE_BUTTON, "First") && semantic(app, CUI_ROLE_BUTTON, "Last"));
    assert(part(app, pager, 1).y == part(app, pager, 4).y);
    press(app, part(app, pager, 3));
    step(app);
    page_is(app, pager, 11, true);

    press(app, part(app, pager, 4)); /* Old Last activation queued before reconfiguration. */
    config.flags = CUI_BRIDGE_PAGINATION_CHEVRONS | CUI_BRIDGE_PAGINATION_JUMP_MENU;
    config.visible_pages = 5;
    config.available_width = 224;
    strcpy(config.group, "Pages de résultats");
    strcpy(config.previous, "Précédente");
    strcpy(config.next, "Suivante");
    strcpy(config.later_pages, "Pages suivantes");
    config.next[20] = 1; /* Bytes beyond a terminated label do not persist. */
    assert(cui_bridge_pagination_configure_v1(app, pager, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_design_size(app, pager, CUI_BRIDGE_SIZE_2XL) == CUI_BRIDGE_OK);
    memset(&config, 0xcc, sizeof config);
    assert(cui_bridge_pagination_get_config_v1(app, pager, &out) == CUI_BRIDGE_OK);
    assert(!strcmp(out.next, "Suivante") && !out.next[20] && out.available_width == 224);
    cui_bridge_pagination_config_v1 rejected = out;
    memset(rejected.first, 'A', sizeof rejected.first);
    assert(cui_bridge_pagination_configure_v1(app, pager, &rejected) == CUI_BRIDGE_CAPACITY);
    rejected = out;
    strcpy(rejected.next, "\xc0\x80");
    assert(cui_bridge_pagination_configure_v1(app, pager, &rejected) == CUI_BRIDGE_INVALID_UTF8);
    strcpy(rejected.next, "Next\nPage");
    assert(cui_bridge_pagination_configure_v1(app, pager, &rejected) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    rejected = out;
    rejected.visible_pages = 33;
    assert(cui_bridge_pagination_configure_v1(app, pager, &rejected) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    rejected = out;
    rejected.available_width = INFINITY;
    assert(cui_bridge_pagination_configure_v1(app, pager, &rejected) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    app->width = 316; /* Mock the native host reporting its current logical viewport. */
    step(app);
    page_is(app, pager, 11, false);
    cui_bridge_rect previous = part(app, pager, 2), next = part(app, pager, 3);
    assert(next.y > previous.y && next.height == 56 && previous.height == 56);
    assert(semantic(app, CUI_ROLE_GROUP, "Pages de résultats"));
    assert(semantic(app, CUI_ROLE_BUTTON, "Précédente"));
    assert(!semantic(app, CUI_ROLE_BUTTON, "First") && !semantic(app, CUI_ROLE_BUTTON, "Last"));

    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_MOVE, next.x + next.width / 2,
                                    next.y + next.height / 2) == CUI_BRIDGE_OK);
    for (unsigned i = 0; i < 32; ++i)
        step(app);
    assert(semantic(app, CUI_ROLE_LABEL, "Suivante"));
    page_is(app, pager, 11, false);

    press_semantic(app, CUI_ROLE_BUTTON, "Pages suivantes");
    step(app);
    page_is(app, pager, 11, false);
    const cui_semantic *menu = semantic(app, CUI_ROLE_MENU, "Pages suivantes");
    assert(menu && menu->details.scrollable);
    assert(!semantic(app, CUI_ROLE_LABEL, "Suivante"));
    cui_rect popup = cui_bounds(app->ui, menu->id);
    assert(popup.w > 0 && popup.h > 0 && popup.h <= 240 && popup.x + popup.w <= app->width);
    press_semantic(app, CUI_ROLE_MENUITEM, "13");
    step(app);
    page_is(app, pager, 13, true);
    assert(!semantic(app, CUI_ROLE_MENU, "Pages suivantes"));

    press(app, part(app, pager, 3));
    assert(cui_bridge_set_disabled(app, pager, 1) == CUI_BRIDGE_OK);
    step(app);
    page_is(app, pager, 13, false);
    assert(cui_bridge_set_disabled(app, pager, 0) == CUI_BRIDGE_OK);
    assert(cui_bridge_pagination_configure_v1(app, pager, NULL) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_design_size(app, pager, CUI_BRIDGE_SIZE_INHERIT) == CUI_BRIDGE_OK);
    assert(cui_bridge_pagination_get_config_v1(app, pager, &out) == CUI_BRIDGE_EMPTY);
    assert(!strcmp(out.next, "Suivante")); /* Failed read leaves the output alone. */
    app->width = 560;
    step(app);
    assert(semantic(app, CUI_ROLE_BUTTON, "Previous") && semantic(app, CUI_ROLE_BUTTON, "First"));

    assert(cui_bridge_pagination_configure_v1(app, pager, &out) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, pager) == CUI_BRIDGE_OK);
    assert(!bridge_pagination_node(app, pager.slot));
    assert(cui_bridge_pagination_get_config_v1(app, pager, &out) == CUI_BRIDGE_STALE_HANDLE);
    cui_bridge_node replacement = create(app, root, CUI_BRIDGE_PAGINATION, "pager");
    assert(replacement.slot == pager.slot && replacement.generation != pager.generation);
    assert(cui_bridge_pagination_get_config_v1(app, replacement, &out) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_pagination_configure_v1(app, replacement, &out) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
