/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef CUI_TEST_DATA_TABLE_ALLOCATIONS
/* Cover the actual bridge and Base allocations without production hooks. */
#define CUI_BRIDGE_BUILD 1
#include "../../../src/bridge/cui_bridge.c"

static struct {
    void *address;
    size_t bytes;
} retained[32];
static size_t calls, fail_at, live, peak;
static bool capture;

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);

static void *record(void *address, size_t bytes) {
    if (!address || !capture)
        return address;
    for (size_t i = 0; i < sizeof retained / sizeof *retained; i++) {
        if (retained[i].address)
            continue;
        retained[i].address = address;
        retained[i].bytes = bytes;
        live += bytes;
        if (live > peak)
            peak = live;
        return address;
    }
    abort();
}

void *__wrap_malloc(size_t bytes) {
    return capture && ++calls == fail_at ? NULL : record(__real_malloc(bytes), bytes);
}

void *__wrap_calloc(size_t count, size_t bytes) {
    return capture && ++calls == fail_at ? NULL
                                         : record(__real_calloc(count, bytes), count * bytes);
}

void __wrap_free(void *address) {
    for (size_t i = 0; address && i < sizeof retained / sizeof *retained; i++) {
        if (retained[i].address != address)
            continue;
        retained[i].address = NULL;
        live -= retained[i].bytes;
        break;
    }
    __real_free(address);
}

static size_t visible_cells(cui_bridge_app *app) {
    size_t count, cells = 0;
    const cui_semantic *semantics = cui_semantics(app->ui, &count);
    for (size_t i = 0; i < count; i++)
        if (semantics[i].role == CUI_ROLE_GRIDCELL)
            cells++;
    return cells;
}
#endif

static cui_bridge_app *make_app(cui_bridge_node *root) {
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_NONE,
                                  .width = 580,
                                  .height = 400};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "inbox", root) ==
           CUI_BRIDGE_OK);
    return app;
}

static cui_bridge_node make_table(cui_bridge_app *app, cui_bridge_node root, const char *key) {
    cui_bridge_node table;
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_DATA_TABLE, key, &table) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_visual_size(app, table, 340, 160) == CUI_BRIDGE_OK);
    return table;
}

static void step(cui_bridge_app *app) {
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
}

static void click(cui_bridge_app *app, cui_bridge_rect rect) {
    float x = rect.x + rect.width * .5f, y = rect.y + rect.height * .5f;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    step(app);
}

static cui_bridge_status page(cui_bridge_app *app, cui_bridge_node table,
                              cui_bridge_data_table_load_more_v1 *request) {
    return cui_bridge_data_table_poll_load_more_v1(app, table, sizeof *request, request);
}

static void expect_state(cui_bridge_app *app, cui_bridge_node table, uint32_t expected_row,
                         uint32_t expected_column, uint32_t expected_sort) {
    uint32_t row, column, sort, direction;
    assert(cui_bridge_data_table_get_state(app, table, &row, &column, &sort, &direction) ==
           CUI_BRIDGE_OK);
    assert(row == expected_row && column == expected_column && sort == expected_sort);
    assert(direction == (expected_sort ? 1u : 0u));
}

static void test_page_boundary(void) {
    const uint32_t limit = cui_bridge_data_table_max_row_capacity_v1();
    assert(limit == 10000);
    cui_bridge_node root;
    cui_bridge_app *app = make_app(&root);
    cui_bridge_node table = make_table(app, root, "page");
    const char *headers[] = {"Subject", "Owner"};
    const char *cells[66];
    uint32_t keys[33];
    char subject[] = "R\xc3\xa9sum\xc3\xa9";
    for (uint32_t i = 0; i < 33; i++) {
        cells[i * 2] = subject;
        cells[i * 2 + 1] = "Owner";
        keys[i] = i + 100;
    }
    keys[30] = UINT32_MAX;

    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, NULL, NULL, 0, 0) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, NULL, NULL, 0,
                                                       limit + 1) == CUI_BRIDGE_CAPACITY);
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, NULL, NULL, 0,
                                                       limit) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_configure_load_more_v1(app, table, 1, 33) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_configure_load_more_v1(app, table, 1, 0) == CUI_BRIDGE_OK);
    step(app);
    cui_bridge_data_table_load_more_v1 request;
    assert(page(app, table, &request) == CUI_BRIDGE_OK);
    assert(request.frame && !request.row_count && !request.last_row_key &&
           request.capacity == limit);
    assert(cui_bridge_data_table_set_selection_mode(app, table, 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, cells, keys, 32,
                                                       limit) == CUI_BRIDGE_OK);
    subject[0] = 'X';
    uint32_t required;
    char copied[129];
    assert(cui_bridge_data_table_get_cell(app, table, 17, 0, copied, sizeof copied, &required) ==
           CUI_BRIDGE_OK);
    assert(strcmp(copied, "R\xc3\xa9sum\xc3\xa9") == 0 && required == sizeof subject);
    step(app);
    cui_bridge_rect header, row;
    assert(cui_bridge_data_table_header_bounds(app, table, 1, &header) == CUI_BRIDGE_OK);
    click(app, header);
    assert(cui_bridge_data_table_cell_bounds(app, table, 1, 0, &row) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, row.x + 10, row.y + 10, 0, -100) == CUI_BRIDGE_OK);
    step(app);
    cui_bridge_rect selected;
    assert(cui_bridge_data_table_cell_bounds(app, table, 30, 1, &selected) == CUI_BRIDGE_OK);
    click(app, selected);
    expect_state(app, table, UINT32_MAX, 2, 2);
    cui_bridge_event_ex event;
    char payload[128];
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(strstr(payload, "\"reason\":\"sort\""));
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(event.value == UINT32_MAX && strstr(payload, "\"reason\":\"selection\""));

    uint32_t tail_key = keys[31];
    keys[31] = keys[30];
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, cells, keys, 33,
                                                       limit) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(page(app, table, &request) == CUI_BRIDGE_OK);
    assert(request.row_count == 32 && request.visible_end == 32 && request.visible_begin > 0 &&
           request.last_row_key == tail_key && request.capacity == limit);
    expect_state(app, table, UINT32_MAX, 2, 2);

    /* Apply a caller-sorted snapshot with a newly arrived tail. */
    keys[31] = tail_key;
    uint32_t first_key = keys[0];
    memmove(keys, keys + 1, 31 * sizeof *keys);
    keys[31] = first_key;
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, cells, keys, 33,
                                                       33) == CUI_BRIDGE_OK);
    assert(page(app, table, &request) == CUI_BRIDGE_EMPTY);
    step(app);
    expect_state(app, table, UINT32_MAX, 2, 2);
    assert(cui_bridge_data_table_cell_bounds(app, table, 29, 1, &row) == CUI_BRIDGE_OK);
    assert(row.y == selected.y);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_EMPTY);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_DOWN_ARROW, 0, 0) ==
           CUI_BRIDGE_OK);
    step(app);
    expect_state(app, table, tail_key, 2, 2);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CHANGE && event.value == tail_key);
    assert(cui_bridge_queue_scroll(app, row.x + 10, row.y + 10, 0, -100) == CUI_BRIDGE_OK);
    step(app);
    assert(page(app, table, &request) == CUI_BRIDGE_EMPTY);
    assert(cui_bridge_data_table_set_keyed_capacity_v1(app, table, headers, 2, cells, keys, 33,
                                                       limit) == CUI_BRIDGE_OK);
    step(app);
    assert(page(app, table, &request) == CUI_BRIDGE_OK && request.row_count == 33 &&
           request.visible_begin > 0 && request.visible_end == 33 &&
           request.last_row_key == keys[32] && request.capacity == limit);

    assert(cui_bridge_data_table_set(app, table, headers, 2, cells, 33) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_set_keyed(app, table, headers, 2, cells, keys, 33) ==
           CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_data_table_get_cell(app, table, 32, 0, copied, sizeof copied, &required) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_set(app, table, headers, 2, NULL, 0) == CUI_BRIDGE_OK);
    expect_state(app, table, 0, 0, 0);
    step(app);
    assert(page(app, table, &request) == CUI_BRIDGE_OK && request.capacity == 32);
    cui_bridge_destroy(app);
}

typedef struct {
    const char *headers[16];
    const char **cells;
    uint32_t *keys;
    char text[129];
} matrix;

static void make_matrix(matrix *data, uint32_t rows, uint32_t columns) {
    data->cells = malloc((size_t)rows * columns * sizeof *data->cells);
    data->keys = malloc((size_t)rows * sizeof *data->keys);
    assert(data->cells && data->keys);
    memset(data->text, 'a', sizeof data->text - 1);
    data->text[sizeof data->text - 1] = '\0';
    for (uint32_t c = 0; c < columns; c++)
        data->headers[c] = "Column";
    for (uint32_t r = 0; r < rows; r++) {
        data->keys[r] = r + 1;
        for (uint32_t c = 0; c < columns; c++)
            data->cells[r * columns + c] = data->text;
    }
}

static cui_bridge_status bind(cui_bridge_app *app, cui_bridge_node table, const matrix *data,
                              uint32_t rows, uint32_t columns, uint32_t capacity) {
    return cui_bridge_data_table_set_keyed_capacity_v1(app, table, data->headers, columns,
                                                       data->cells, data->keys, rows, capacity);
}

static void test_large_budget(void) {
    const uint32_t limit = cui_bridge_data_table_max_row_capacity_v1();
    const uint32_t columns = 16;
    cui_bridge_node root;
    cui_bridge_app *app = make_app(&root);
    cui_bridge_node first = make_table(app, root, "first");
    cui_bridge_node second = make_table(app, root, "second");
    matrix data;
    make_matrix(&data, limit, columns);
    assert(bind(app, first, &data, 1000, columns, limit) == CUI_BRIDGE_OK);
#ifdef CUI_TEST_DATA_TABLE_ALLOCATIONS
    step(app);
    size_t cells = visible_cells(app);
    assert(cells > 0 && cells <= 32);
#endif
    assert(bind(app, first, &data, limit, columns, limit) == CUI_BRIDGE_OK);
#ifdef CUI_TEST_DATA_TABLE_ALLOCATIONS
    step(app);
    assert(visible_cells(app) == cells);
#endif
    assert(bind(app, first, &data, limit, columns, limit) == CUI_BRIDGE_OK);
    assert(bind(app, second, &data, limit, columns, limit) == CUI_BRIDGE_CAPACITY);
    assert(bind(app, second, &data, 1000, columns, limit) == CUI_BRIDGE_OK);
    char too_long[130];
    memset(too_long, 'x', sizeof too_long - 1);
    too_long[sizeof too_long - 1] = '\0';
    data.cells[(size_t)limit * columns - 1] = too_long;
    assert(bind(app, first, &data, limit, columns, limit) == CUI_BRIDGE_INVALID_ARGUMENT);
    char copied[129];
    uint32_t required;
    assert(cui_bridge_data_table_get_cell(app, first, limit - 1, columns - 1, copied, sizeof copied,
                                          &required) == CUI_BRIDGE_OK);
    assert(required == sizeof copied && strcmp(copied, data.text) == 0);
    data.cells[(size_t)limit * columns - 1] = data.text;
    step(app);
    cui_bridge_rect row;
    assert(cui_bridge_data_table_cell_bounds(app, first, 1, 0, &row) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_scroll(app, row.x + 10, row.y + 10, 0, -(float)limit) == CUI_BRIDGE_OK);
    step(app);
    assert(cui_bridge_data_table_cell_bounds(app, first, limit - 1, 0, &row) == CUI_BRIDGE_OK);
    assert(cui_bridge_data_table_cell_bounds(app, first, 1, 0, &row) == CUI_BRIDGE_INVALID_STATE);

    assert(cui_bridge_data_table_set_keyed(app, first, data.headers, columns, data.cells, data.keys,
                                           32) == CUI_BRIDGE_OK);
    assert(bind(app, second, &data, limit, columns, limit) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_remove(app, second) == CUI_BRIDGE_OK);
    assert(bind(app, first, &data, limit, columns, limit) == CUI_BRIDGE_OK);
    free(data.keys);
    free(data.cells);
    cui_bridge_destroy(app);
}

#ifdef CUI_TEST_DATA_TABLE_ALLOCATIONS
static void test_allocation_failures(void) {
    cui_bridge_node root;
    cui_bridge_app *app = make_app(&root);
    cui_bridge_node table = make_table(app, root, "faults");
    matrix data;
    make_matrix(&data, 2, 1);
    capture = true;
    assert(bind(app, table, &data, 0, 1, 10000) == CUI_BRIDGE_OK);
    capture = false;
    size_t initial = live;
    assert(initial && initial == app->data_table_capacity_bytes && initial < 4096);
    for (fail_at = 1;; fail_at++) {
        assert(fail_at < 20);
        assert(cui_bridge_data_table_configure_load_more_v1(app, table, 0, 0) == CUI_BRIDGE_OK);
        assert(cui_bridge_data_table_configure_load_more_v1(app, table, 1, 0) == CUI_BRIDGE_OK);
        step(app);
        calls = 0;
        capture = true;
        cui_bridge_status status = bind(app, table, &data, 2, 1, 10000);
        capture = false;
        if (status == CUI_BRIDGE_OK) {
            assert(fail_at == calls + 1 && calls > 5);
            break;
        }
        assert(status == CUI_BRIDGE_OUT_OF_MEMORY && calls >= fail_at);
        assert(live == initial && live == app->data_table_capacity_bytes);
        cui_bridge_data_table_load_more_v1 request;
        assert(page(app, table, &request) == CUI_BRIDGE_OK && request.capacity == 10000 &&
               !request.row_count);
    }
    assert(live > initial && live == app->data_table_capacity_bytes);
    cui_bridge_destroy(app);
    assert(!live);
    free(data.keys);
    free(data.cells);
}

static void measure(uint32_t rows, uint32_t columns, bool legacy) {
    matrix data;
    make_matrix(&data, rows, columns);
    cui_bridge_node root;
    cui_bridge_app *app = make_app(&root);
    cui_bridge_node table = make_table(app, root, "measure");
    peak = calls = fail_at = 0;
    const unsigned iterations = rows <= 32 ? 2000 : rows <= 1000 ? 100 : 20;
    clock_t start = clock();
    for (unsigned i = 0; i < iterations; i++) {
        capture = true;
        cui_bridge_status status =
            legacy ? cui_bridge_data_table_set_keyed(app, table, data.headers, columns, data.cells,
                                                     data.keys, rows)
                   : bind(app, table, &data, rows, columns, 10000);
        capture = false;
        assert(status == CUI_BRIDGE_OK);
    }
    double bind_us = (double)(clock() - start) * 1e6 / CLOCKS_PER_SEC / iterations;
    assert(live && peak >= live * 2 && (legacy || live == app->data_table_capacity_bytes));
    step(app);
    size_t painted = visible_cells(app);
    assert(painted > 0 && painted <= 32);
    start = clock();
    for (unsigned i = 0; i < 500; i++)
        step(app);
    double step_us = (double)(clock() - start) * 1e6 / CLOCKS_PER_SEC / 500;
    printf("%s %u x %u: bind %.1f us, step %.1f us, retained %zu B, peak %zu B, "
           "%zu semantic cells\n",
           legacy ? "legacy setter" : "capacity setter", rows, columns, bind_us, step_us, live,
           peak, painted);
    cui_bridge_destroy(app);
    assert(!live);
    free(data.keys);
    free(data.cells);
}
#endif

int main(void) {
#ifdef CUI_TEST_DATA_TABLE_ALLOCATIONS
    test_allocation_failures();
    if (getenv("CUI_TEST_DATA_TABLE_BENCHMARK")) {
        for (uint32_t columns = 2; columns <= 16; columns *= 8) {
            measure(32, columns, true);
            measure(32, columns, false);
            measure(1000, columns, false);
            measure(10000, columns, false);
        }
    }
#endif
    test_page_boundary();
    test_large_budget();
}
