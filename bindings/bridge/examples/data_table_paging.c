/* SPDX-License-Identifier: MIT */
/* A retained inbox with deterministic, offline page delivery. */
#include "cui/cui_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t generation, frame;
} page_ticket;

typedef struct {
    cui_bridge_app *app;
    cui_bridge_node table, status;
    uint64_t generation, pending_frame;
    uint32_t capacity;
} inbox;

static const char *headers[] = {"Subject"};
static const char *messages[] = {"Release notes",    "Build report",  "Design review",
                                 "Schedule",         "Issue triage",  "Test feedback",
                                 "Workspace update", "Agenda",        "New project",
                                 "Bug report",       "Meeting notes", "Next steps"};
static const uint32_t keys[] = {101, 102, 103, 104, 105, 106, 107, 108, 301, 302, 303, 304};

static cui_bridge_status replace_messages(const inbox *owner, uint32_t count) {
    if (!owner->capacity)
        return cui_bridge_data_table_set_keyed(owner->app, owner->table, headers, 1, messages, keys,
                                               count);

    const char *rows[40];
    uint32_t row_keys[40];
    char more[28][24];
    for (uint32_t i = 0; i < count; i++) {
        if (i < 12) {
            rows[i] = messages[i];
            row_keys[i] = keys[i];
        } else {
            snprintf(more[i - 12], sizeof more[0], "Inbox message %u", (unsigned)(i + 1));
            rows[i] = more[i - 12];
            row_keys[i] = i + 500;
        }
    }
    return cui_bridge_data_table_set_keyed_capacity_v1(owner->app, owner->table, headers, 1, rows,
                                                       row_keys, count, owner->capacity);
}

static cui_bridge_status start_page(inbox *owner, page_ticket *ticket) {
    if (owner->pending_frame)
        return CUI_BRIDGE_EMPTY;
    cui_bridge_data_table_load_more_v1 request;
    cui_bridge_status status =
        cui_bridge_data_table_poll_load_more_v1(owner->app, owner->table, sizeof request, &request);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = cui_bridge_data_table_configure_load_more_v1(owner->app, owner->table, 0, 0);
    if (status != CUI_BRIDGE_OK)
        return status;
    owner->pending_frame = request.frame;
    *ticket = (page_ticket){owner->generation, request.frame};
    return cui_bridge_set_text(owner->app, owner->status, "Loading the next page");
}

static cui_bridge_status receive_page(inbox *owner, page_ticket ticket) {
    if (!owner->pending_frame || ticket.generation != owner->generation ||
        ticket.frame != owner->pending_frame)
        return CUI_BRIDGE_EMPTY;
    /* A production host supplies the newly combined snapshot here. */
    cui_bridge_status status = replace_messages(owner, owner->capacity ? 40 : 12);
    if (status != CUI_BRIDGE_OK)
        return status;
    owner->pending_frame = 0;
    /* The data source says this is the final page. Leave paging off. */
    return cui_bridge_set_text(owner->app, owner->status,
                               owner->capacity ? "40 messages. End of inbox."
                                               : "12 messages. End of inbox.");
}

static cui_bridge_status retry_page(inbox *owner) {
    /* Cancel the owner's logical request. The old transport may still complete. */
    owner->generation++;
    owner->pending_frame = 0;
    return cui_bridge_data_table_configure_load_more_v1(owner->app, owner->table, 1, 0);
}

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            fprintf(stderr, "Inbox check failed on line %d: %s\n", __LINE__, #expression);         \
            goto failed;                                                                           \
        }                                                                                          \
    } while (0)

int main(int argc, char **argv) {
    uint32_t capacity = 0;
    if (argc == 2 && strcmp(argv[1], "--larger") == 0) {
        capacity = cui_bridge_data_table_max_row_capacity_v1();
        if (capacity < 40)
            return EXIT_FAILURE;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--larger]\n", argv[0]);
        return EXIT_FAILURE;
    }
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 480,
                                  .height = 360};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    CHECK(app && status == CUI_BRIDGE_OK);
    inbox owner = {.app = app, .generation = 1, .capacity = capacity};
    cui_bridge_node root = {0};
    CHECK(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "inbox", &root) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(app, root, CUI_BRIDGE_DATA_TABLE, "messages", &owner.table) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_node_create(app, root, CUI_BRIDGE_LABEL, "status", &owner.status) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_text(app, owner.status, "8 messages") == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_visual_size(app, owner.table, 360, 160) == CUI_BRIDGE_OK);
    CHECK(replace_messages(&owner, 8) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_data_table_configure_load_more_v1(app, owner.table, 1, 0) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    page_ticket original, fresh;
    CHECK(start_page(&owner, &original) == CUI_BRIDGE_EMPTY);

    /* Drive the same viewport and selection events a UI host supplies. */
    cui_bridge_rect top, selected, after;
    CHECK(cui_bridge_data_table_cell_bounds(app, owner.table, 1, 0, &top) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_queue_scroll(app, top.x + 10, top.y + 10, 0, -20) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    CHECK(start_page(&owner, &original) == CUI_BRIDGE_OK);
    CHECK(start_page(&owner, &fresh) == CUI_BRIDGE_EMPTY);
    CHECK(cui_bridge_data_table_cell_bounds(app, owner.table, 6, 0, &selected) == CUI_BRIDGE_OK);
    float x = selected.x + selected.width * .5f;
    float y = selected.y + selected.height * .5f;
    CHECK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);

    CHECK(retry_page(&owner) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    CHECK(start_page(&owner, &fresh) == CUI_BRIDGE_OK);
    CHECK(receive_page(&owner, original) == CUI_BRIDGE_EMPTY);
    CHECK(receive_page(&owner, fresh) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t row, column, sort_column, direction;
    CHECK(cui_bridge_data_table_get_state(app, owner.table, &row, &column, &sort_column,
                                          &direction) == CUI_BRIDGE_OK);
    CHECK(row == keys[6]);
    if (capacity) {
        char copied[24];
        uint32_t required;
        CHECK(cui_bridge_data_table_get_cell(app, owner.table, 39, 0, copied, sizeof copied,
                                             &required) == CUI_BRIDGE_OK);
        CHECK(strcmp(copied, "Inbox message 40") == 0);
    }
    CHECK(cui_bridge_data_table_cell_bounds(app, owner.table, 6, 0, &after) == CUI_BRIDGE_OK);
    CHECK(after.y == selected.y);
    CHECK(cui_bridge_queue_scroll(app, x, y, 0, -20) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    CHECK(start_page(&owner, &fresh) == CUI_BRIDGE_EMPTY);
    puts("Appended the final page, kept selection and position, and ignored a cancelled response.");
    cui_bridge_destroy(app);
    return EXIT_SUCCESS;
failed:
    cui_bridge_destroy(app);
    return EXIT_FAILURE;
}
