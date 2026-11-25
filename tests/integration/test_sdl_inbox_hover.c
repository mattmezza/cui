/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui/cui_table.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 520, HEIGHT = 288, TABLE = 800, ROW_HEIGHT = 32, SAMPLES = 90 };
enum { SENDER = 10001, SUBJECT = 10002, RECEIVED = 10003 };
static const cui_color hover_fill = {197, 79, 151, 255};
static const cui_table_options table_options = {.width = WIDTH,
                                                .height = 8 * ROW_HEIGHT,
                                                .row_height = ROW_HEIGHT,
                                                .header_height = ROW_HEIGHT,
                                                .overscan = 1,
                                                .striped = true};

typedef struct {
    cui_context *ui;
    cui_sdl3 *host;
    SDL_WindowID window;
    cui_table_model *model;
    cui_table_state state;
    size_t cells;
    cui_id highlighted;
} inbox;

typedef struct {
    double poll_us, compose_us, present_us;
    size_t cells, commands;
} sample;

static cui_str label(void *user, size_t row, cui_id column) {
    const inbox *test = user;
    cui_table_row message;
    assert(cui_table_get_row(test->model, row, &message));
    if (column == SENDER)
        return cui_string(message.key % 2 ? "Sam Rivera" : "Alex Morgan");
    if (column == SUBJECT)
        return cui_string(message.key % 2 ? "Notes for the upcoming release"
                                          : "Review the latest build");
    return cui_string("09:30");
}

static void cell(cui_context *ui, const cui_theme *theme, void *user, size_t row, cui_id column) {
    inbox *test = user;
    ++test->cells;
    cui_table_row message;
    assert(cui_table_get_row(test->model, row, &message));
    if (column == SENDER && cui_hovered(ui, cui_table_row_id(TABLE, message.key))) {
        assert(!test->highlighted);
        test->highlighted = message.key;
        cui_begin_box(ui, 0, (cui_box){.width = 5, .height = 15, .background = hover_fill});
        cui_end_box(ui);
    }
    cui_text(ui, label(user, row, column), theme->font_size, theme->text);
}

static sample frame(inbox *test, const cui_theme *theme, cui_id expected_hover) {
    test->cells = 0;
    test->highlighted = 0;
    Uint64 start = SDL_GetTicksNS();
    assert(cui_sdl3_poll(test->host, test->ui));
    Uint64 polled = SDL_GetTicksNS();
    assert(cui_begin_frame(test->ui, WIDTH, HEIGHT, .016f));
    cui_table_result result = cui_data_table(test->ui, TABLE, theme, test->model, &test->state,
                                             table_options, label, cell, test);
    cui_draw_list draw = cui_end_frame(test->ui);
    Uint64 composed = SDL_GetTicksNS();
    assert(cui_sdl3_render(test->host, &draw, theme->background));
    Uint64 presented = SDL_GetTicksNS();
    assert(result.error == CUI_TABLE_OK && cui_last_error(test->ui) == CUI_OK);
    assert(result.rows.end - result.rows.begin <= 9 && test->cells <= 33);
    if (expected_hover) {
        assert(test->highlighted == expected_hover);
        size_t markers = 0;
        cui_rect row = cui_bounds(test->ui, cui_table_row_id(TABLE, expected_hover));
        for (size_t i = 0; i < draw.count; i++) {
            const cui_draw_command *c = &draw.commands[i];
            if (c->type == CUI_DRAW_RECT && c->color.r == hover_fill.r &&
                c->color.g == hover_fill.g && c->color.b == hover_fill.b &&
                c->color.a == hover_fill.a) {
                assert(c->bounds.y >= row.y && c->bounds.y + c->bounds.h <= row.y + row.h);
                ++markers;
            }
        }
        assert(markers == 1);
    }
    return (sample){(polled - start) / 1000.0, (composed - polled) / 1000.0,
                    (presented - composed) / 1000.0, test->cells, draw.count};
}

static void move(inbox *test, cui_id row) {
    cui_rect cell = cui_bounds(test->ui, cui_table_cell_id(TABLE, row, SENDER));
    assert(cell.w > 0 && cell.h > 0);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_MOTION};
    event.motion.windowID = test->window;
    event.motion.x = cell.x + 20;
    event.motion.y = cell.y + cell.h / 2;
    assert(SDL_PushEvent(&event));
}

static int compare_time(const void *left, const void *right) {
    double a = *(const double *)left, b = *(const double *)right;
    return (a > b) - (a < b);
}

static void update_inbox(inbox *test, const cui_theme *theme, cui_table_row *rows, size_t count,
                         cui_table_column *columns) {
    cui_table_row_anchor anchor =
        cui_table_capture_row_anchor(test->model, test->state.scroll_y, table_options);
    assert(anchor.key && anchor.within == 16);
    cui_id selected = anchor.key + 2;
    test->state.row = test->state.cursor_row = selected;
    test->state.cursor_column = SENDER;
    (void)frame(test, theme, 0);
    cui_id selected_cell = cui_table_cell_id(TABLE, selected, SENDER);
    cui_focus(test->ui, selected_cell);
    cui_rect before = cui_bounds(test->ui, selected_cell);
    assert(before.w > 0 && before.h > 0);

    memmove(rows + 1, rows, count * sizeof(*rows));
    rows[0] = (cui_table_row){.key = count + 1, .label = cui_string("New message")};
    assert(cui_table_bind(test->model, rows, count + 1, columns, 3) == CUI_TABLE_OK);
    test->state.scroll_y = cui_table_restore_row_anchor(test->model, anchor, table_options);
    move(test, selected + 1);
    move(test, selected);
    sample updated = frame(test, theme, selected);
    assert(updated.cells <= 33 && test->state.row == selected);
    assert(test->state.visible_begin == anchor.fallback_index + 1);
    assert(cui_focused(test->ui) == selected_cell);
    assert(cui_bounds(test->ui, selected_cell).y == before.y);

    assert(cui_queue_event(test->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    (void)frame(test, theme, selected);
    assert(test->state.row == selected + 1 && test->state.cursor_row == selected + 1);
    assert(test->state.visible_begin == anchor.fallback_index + 1);
}

static size_t exercise(cui_sdl3 *host, SDL_WindowID window, cui_font *font, size_t row_count) {
    size_t middle = row_count / 2;
    inbox test = {
        .host = host,
        .window = window,
        .model = cui_table_create(row_count + 1),
        .state = {.scroll_y = (float)middle * ROW_HEIGHT},
        .ui = cui_create((cui_config){.measure = cui_font_measure,
                                      .measure_user = font,
                                      .max_elements = 256,
                                      .max_commands = 512}),
    };
    cui_table_row *rows = calloc(row_count + 1, sizeof(*rows));
    assert(rows && test.model && test.ui);
    for (size_t r = 0; r < row_count; ++r)
        rows[r] = (cui_table_row){.key = r + 1, .label = cui_string("Inbox message")};
    cui_table_column columns[] = {{.key = SENDER, .width = 136, .label = cui_string("Sender")},
                                  {.key = SUBJECT, .width = 300, .label = cui_string("Subject")},
                                  {.key = RECEIVED, .width = 84, .label = cui_string("Received")}};
    assert(cui_table_bind(test.model, rows, row_count, columns, 3) == CUI_TABLE_OK);
    cui_theme theme = cui_theme_light();
    (void)frame(&test, &theme, 0);
    for (size_t f = 0; f < 8; ++f) {
        cui_id row = middle + 1 + f % 6;
        move(&test, row);
        (void)frame(&test, &theme, row);
    }
    double poll[SAMPLES], compose[SAMPLES], present[SAMPLES], total[SAMPLES];
    sample last = {0};
    for (size_t f = 0; f < SAMPLES; ++f) {
        cui_id row = middle + 1 + f % 6;
        move(&test, middle + 7);
        move(&test, row);
        sample current = frame(&test, &theme, row);
        assert(current.cells == 30);
        assert(!f || current.commands == last.commands);
        poll[f] = current.poll_us;
        compose[f] = current.compose_us;
        present[f] = current.present_us;
        total[f] = poll[f] + compose[f] + present[f];
        last = current;
    }
    qsort(poll, SAMPLES, sizeof(*poll), compare_time);
    qsort(compose, SAMPLES, sizeof(*compose), compare_time);
    qsort(present, SAMPLES, sizeof(*present), compare_time);
    qsort(total, SAMPLES, sizeof(*total), compare_time);
    const size_t median = SAMPLES / 2, p95 = (SAMPLES * 95 + 99) / 100 - 1;
    printf("Inbox %zu rows, %zu cells, %zu draw commands, %d measured frames\n"
           "  wall microseconds p50/p95: poll %.1f/%.1f, compose %.1f/%.1f,"
           " software present %.1f/%.1f, total %.1f/%.1f\n",
           row_count, last.cells, last.commands, SAMPLES, poll[median], poll[p95], compose[median],
           compose[p95], present[median], present[p95], total[median], total[p95]);

    float previous_offset = test.state.scroll_y;
    SDL_Event wheel = {.type = SDL_EVENT_MOUSE_WHEEL};
    wheel.wheel.windowID = window;
    wheel.wheel.mouse_x = 40;
    wheel.wheel.mouse_y = 100;
    wheel.wheel.y = -1;
    assert(SDL_PushEvent(&wheel));
    (void)frame(&test, &theme, 0);
    assert(test.state.scroll_y == previous_offset + 48 && test.state.visible_begin == middle + 1);
    update_inbox(&test, &theme, rows, row_count, columns);

    cui_destroy(test.ui);
    cui_table_destroy(test.model);
    free(rows);
    return last.commands;
}

int main(int argc, char **argv) {
    assert(argc == 2);
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE));
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    cui_sdl3 *host =
        cui_sdl3_create_with_options("Inbox benchmark", WIDTH, HEIGHT, font,
                                     (cui_sdl3_window_options){.initially_hidden = true});
    assert(host);
    int window_count;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    assert(windows && window_count == 1);
    SDL_WindowID window = SDL_GetWindowID(windows[0]);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    assert(renderer && SDL_SetRenderVSync(renderer, 0));
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    size_t small = exercise(host, window, font, 1000);
    size_t large = exercise(host, window, font, 10000);
    assert(small == large);
    cui_sdl3_destroy(host);
    cui_font_close(font);
    return 0;
}
