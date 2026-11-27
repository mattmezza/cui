/* SPDX-License-Identifier: MIT */
/* A virtual inbox or clip timeline that keeps your place during updates. */
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
    bool horizontal;
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

static cui_bridge_status prepend_item(inbox *view) {
    if (view->count == INBOX_CAPACITY)
        return CUI_BRIDGE_OK;
    char label[32];
    snprintf(label, sizeof label, view->horizontal ? "New clip %u" : "New message %u",
             (unsigned)view->count);
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
    view->rows[0] = (cui_bridge_virtual_list_row_v1){row, view->horizontal ? 100 : 60};
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
            status = prepend_item(view);
        else if (event.node.slot == view->resize.slot) {
            float before = view->rows[0].extent;
            float small = view->horizontal ? 100 : 60;
            float large = view->horizontal ? 136 : 86;
            view->rows[0].extent = before == large ? small : large;
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
    bool horizontal = argc > 1 && strcmp(argv[1], "--horizontal") == 0;
    int font_arg = horizontal ? 2 : 1;
    if (argc > font_arg + 1) {
        fprintf(stderr, "Usage: %s [--horizontal] [font.ttf]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *font = argc > font_arg ? argv[font_arg] : NULL;
    const char *title_text = horizontal ? "Clip timeline" : "Live inbox";
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend =
                                      font ? CUI_BRIDGE_BACKEND_SDL : CUI_BRIDGE_BACKEND_NONE,
                                  .title_utf8 = title_text,
                                  .font_path_utf8 = font,
                                  .width = 480,
                                  .height = 320};
    cui_bridge_status status;
    inbox view = {.app = cui_bridge_create(&options, &status), .horizontal = horizontal};
    CHECK(view.app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, title, controls;
    CHECK(make_node(&view, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "inbox", "", &root));
    CHECK(make_node(&view, root, CUI_BRIDGE_LABEL, "heading", title_text, &title));
    CHECK(make_node(&view, root, CUI_BRIDGE_ROW, "controls", "", &controls));
    CHECK(make_node(&view, controls, CUI_BRIDGE_BUTTON, "prepend",
                    horizontal ? "Prepend clip" : "Prepend message", &view.prepend));
    CHECK(make_node(&view, controls, CUI_BRIDGE_BUTTON, "resize",
                    horizontal ? "Resize first clip" : "Resize first row", &view.resize));
    CHECK(make_node(&view, root, CUI_BRIDGE_VIRTUAL_LIST, "messages", "", &view.list));
    CHECK(cui_bridge_set_visual_size(view.app, view.list, 380, horizontal ? 96 : 176) ==
          CUI_BRIDGE_OK);
    if (horizontal)
        CHECK(cui_bridge_virtual_list_set_axis_v1(
                  view.app, view.list, CUI_BRIDGE_VIRTUAL_LIST_HORIZONTAL) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_virtual_list_preserve_anchor_v1(view.app, view.list, 1) == CUI_BRIDGE_OK);
    for (; view.count < 12; ++view.count) {
        char label[32];
        snprintf(label, sizeof label, horizontal ? "Clip %u" : "Inbox message %u",
                 (unsigned)(view.count + 1));
        if (horizontal)
            view.rows[view.count].extent = (float)(96 + (view.count % 3) * 24);
        CHECK(make_node(&view, view.list, CUI_BRIDGE_ITEM, label, label,
                        &view.rows[view.count].item));
    }
    CHECK(submit_rows(&view) == CUI_BRIDGE_OK);
    /* Position the viewport 9.5 px into the fourth item in either layout. */
    double initial_offset = 9.5;
    for (int i = 0; i < 3; i++)
        initial_offset += horizontal ? view.rows[i].extent : 32;
    CHECK(cui_bridge_set_value(view.app, view.list, initial_offset) == CUI_BRIDGE_OK);
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
              offset == initial_offset + (horizontal ? 136 : 86));
        CHECK(cui_bridge_node_bounds(view.app, view.list, &list) == CUI_BRIDGE_OK);
        CHECK(cui_bridge_virtual_list_item_bounds(view.app, view.list, anchor, &row) ==
              CUI_BRIDGE_OK);
        float within_item = horizontal ? list.x - row.x : list.y - row.y;
        CHECK(fabsf(within_item - 9.5f) < .001f);
        printf("Item 4 is still first at %.1f pixels from its leading edge\n", (double)within_item);
    }
    cui_bridge_destroy(view.app);
    return EXIT_SUCCESS;
failed:
    cui_bridge_destroy(view.app);
    return EXIT_FAILURE;
}
