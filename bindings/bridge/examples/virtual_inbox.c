/* SPDX-License-Identifier: MIT */
/* An inbox that keeps your place when new messages arrive. */
#include <cui/cui_bridge.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INBOX_CAPACITY = 64 };
typedef struct {
    cui_bridge_app *app;
    cui_bridge_node list, prepend, resize;
    cui_bridge_virtual_list_row_v1 rows[INBOX_CAPACITY];
    uint32_t count;
} inbox;

static bool make_node(inbox *view, cui_bridge_node parent, cui_bridge_kind kind, const char *key,
                      const char *text, cui_bridge_node *out) {
    return cui_bridge_node_create(view->app, parent, kind, key, out) == CUI_BRIDGE_OK &&
           (!*text || cui_bridge_set_text(view->app, *out, text) == CUI_BRIDGE_OK);
}

static cui_bridge_status submit_rows(const inbox *view) {
    return cui_bridge_virtual_list_set_rows_v1(view->app, view->list, sizeof view->rows[0],
                                               view->rows, view->count);
}

static cui_bridge_status prepend_message(inbox *view) {
    if (view->count == INBOX_CAPACITY)
        return CUI_BRIDGE_OK;
    char label[32];
    snprintf(label, sizeof label, "New message %u", (unsigned)view->count);
    cui_bridge_node row;
    cui_bridge_status status =
        cui_bridge_node_create(view->app, view->list, CUI_BRIDGE_ITEM, label, &row);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = cui_bridge_set_text(view->app, row, label);
    if (status != CUI_BRIDGE_OK) {
        (void)cui_bridge_node_remove(view->app, row);
        return status;
    }
    memmove(view->rows + 1, view->rows, view->count * sizeof view->rows[0]);
    view->rows[0] = (cui_bridge_virtual_list_row_v1){row, 60};
    view->count++;
    status = submit_rows(view);
    if (status != CUI_BRIDGE_OK) {
        view->count--;
        memmove(view->rows, view->rows + 1, view->count * sizeof view->rows[0]);
        (void)cui_bridge_node_remove(view->app, row);
        return status;
    }
    if (view->count == INBOX_CAPACITY)
        return cui_bridge_set_disabled(view->app, view->prepend, 1);
    return CUI_BRIDGE_OK;
}

static cui_bridge_status after_frame(void *user) {
    inbox *view = user;
    cui_bridge_event event;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_event(view->app, &event)) == CUI_BRIDGE_OK) {
        if (event.type != CUI_BRIDGE_EVENT_CLICK)
            continue;
        if (event.node.slot == view->prepend.slot)
            status = prepend_message(view);
        else if (event.node.slot == view->resize.slot) {
            float before = view->rows[0].extent;
            view->rows[0].extent = before == 86 ? 60 : 86;
            status = submit_rows(view);
            if (status != CUI_BRIDGE_OK)
                view->rows[0].extent = before;
        }
        if (status != CUI_BRIDGE_OK)
            return status;
    }
    return status == CUI_BRIDGE_EMPTY ? CUI_BRIDGE_OK : status;
}

static bool press(inbox *view, cui_bridge_node button) {
    cui_bridge_app *app = view->app;
    cui_bridge_rect bounds;
    if (cui_bridge_node_bounds(app, button, &bounds) != CUI_BRIDGE_OK)
        return false;
    float x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
    return cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK &&
           cui_bridge_step(app, .016f) == CUI_BRIDGE_OK && after_frame(view) == CUI_BRIDGE_OK;
}

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            fprintf(stderr, "Virtual inbox failed on line %d: %s\n", __LINE__, #expression);       \
            goto failed;                                                                           \
        }                                                                                          \
    } while (0)

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [font.ttf]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *font = argc == 2 ? argv[1] : NULL;
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend =
                                      font ? CUI_BRIDGE_BACKEND_SDL : CUI_BRIDGE_BACKEND_NONE,
                                  .title_utf8 = "Live inbox",
                                  .font_path_utf8 = font,
                                  .width = 480,
                                  .height = 320};
    cui_bridge_status status;
    inbox view = {.app = cui_bridge_create(&options, &status)};
    CHECK(view.app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, title, controls;
    CHECK(make_node(&view, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "inbox", "", &root));
    CHECK(make_node(&view, root, CUI_BRIDGE_LABEL, "heading", "Live inbox", &title));
    CHECK(make_node(&view, root, CUI_BRIDGE_ROW, "controls", "", &controls));
    CHECK(
        make_node(&view, controls, CUI_BRIDGE_BUTTON, "prepend", "Prepend message", &view.prepend));
    CHECK(
        make_node(&view, controls, CUI_BRIDGE_BUTTON, "resize", "Resize first row", &view.resize));
    CHECK(make_node(&view, root, CUI_BRIDGE_VIRTUAL_LIST, "messages", "", &view.list));
    CHECK(cui_bridge_set_visual_size(view.app, view.list, 380, 176) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_virtual_list_preserve_anchor_v1(view.app, view.list, 1) == CUI_BRIDGE_OK);
    for (; view.count < 12; ++view.count) {
        char label[32];
        snprintf(label, sizeof label, "Inbox message %u", (unsigned)(view.count + 1));
        CHECK(make_node(&view, view.list, CUI_BRIDGE_ITEM, label, label,
                        &view.rows[view.count].item));
    }
    CHECK(submit_rows(&view) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_value(view.app, view.list, 105.5) == CUI_BRIDGE_OK);
    if (font) {
        status = cui_bridge_run(view.app, after_frame, &view);
        CHECK(status == CUI_BRIDGE_OK || status == CUI_BRIDGE_CLOSED);
    } else {
        cui_bridge_node anchor = view.rows[3].item;
        CHECK(cui_bridge_step(view.app, .016f) == CUI_BRIDGE_OK);
        CHECK(press(&view, view.prepend));
        CHECK(press(&view, view.resize));
        CHECK(cui_bridge_step(view.app, .016f) == CUI_BRIDGE_OK);
        double offset;
        cui_bridge_rect list, row;
        CHECK(cui_bridge_get_value(view.app, view.list, &offset) == CUI_BRIDGE_OK &&
              offset == 191.5);
        CHECK(cui_bridge_node_bounds(view.app, view.list, &list) == CUI_BRIDGE_OK);
        CHECK(cui_bridge_virtual_list_item_bounds(view.app, view.list, anchor, &row) ==
              CUI_BRIDGE_OK);
        CHECK(fabsf(row.y - list.y + 9.5f) < .001f);
        printf("Message 4 is still first at %.1f pixels into the row\n", (double)(list.y - row.y));
    }
    cui_bridge_destroy(view.app);
    return EXIT_SUCCESS;
failed:
    cui_bridge_destroy(view.app);
    return EXIT_FAILURE;
}
