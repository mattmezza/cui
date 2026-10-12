/* SPDX-License-Identifier: MIT */
#include "cui/cui_command.h"
#ifndef CUI_COMMAND_BASE_ONLY
#include "cui/cui_components.h"
#include "cui/cui_surface.h"
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned order[32], order_count;
static cui_command_index selected_index;
static void selection(cui_command_index i, void *u) {
    assert(u == order);
    selected_index = i;
    order[order_count++] = 1;
}
static void query(cui_str q, void *u) {
    assert(u == order);
    (void)q;
    order[order_count++] = 2;
}
static void action(void *u) {
    assert(u == order);
    order[order_count++] = 3;
}
static void confirm(cui_command_index i, void *u) {
    assert(u == order);
    selected_index = i;
    order[order_count++] = 4;
}
#ifndef CUI_COMMAND_BASE_ONLY
static cui_edit *editor(char *buffer, size_t capacity) {
    return cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = capacity});
}
#endif
static cui_str command_string(const char *text) {
    return (cui_str){text, strlen(text)};
}
#define cui_string command_string
static void test_model(void) {
    cui_edit *edit = NULL;
    cui_command_row rows[16];
    cui_command_state s;
    assert(!cui_command_init(&s, rows, 0, edit));
    assert(cui_command_init(&s, rows, 16, edit));
    cui_str keywords[] = {cui_string("account")};
    cui_command_item group[] = {{.label = cui_string("Disabled"), .disabled = true},
                                {.label = cui_string("Profile"),
                                 .keywords = keywords,
                                 .keyword_count = 1,
                                 .action = action,
                                 .action_user = order},
                                {.label = cui_string("Billing")}};
    cui_command_entry entries[] = {
        {.kind = CUI_COMMAND_SEPARATOR},
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Calendar")}},
        {.kind = CUI_COMMAND_SEPARATOR},
        {.kind = CUI_COMMAND_SEPARATOR},
        {.kind = CUI_COMMAND_GROUP,
         .heading = cui_string("Settings"),
         .items = group,
         .item_count = 3},
        {.kind = CUI_COMMAND_SEPARATOR}};
    cui_command_options o = cui_command_default();
    o.on_select = selection;
    o.on_query = query;
    o.on_confirm = confirm;
    o.user = order;
    assert(cui_command_rebuild(&s, entries, 6, cui_string(""), false, o));
    assert(s.row_count == 6 && s.matched_count == 4);
    assert(s.selected == 0);
    cui_command_index i;
    assert(cui_command_selected(&s, &i) && i.section == 0 && i.row == 0);
    assert(cui_command_move(&s, true, o));
    assert(selected_index.section == 1 && selected_index.row == 1);
    assert(cui_command_confirm(&s, o));
    assert(order[0] == 1 && order[1] == 3 && order[2] == 4);
    assert(cui_command_move(&s, true, o));
    assert(cui_command_move(&s, true, o));
    assert(cui_command_selected(&s, &i) && i.section == 0);
    assert(cui_command_move(&s, false, o));
    assert(cui_command_selected(&s, &i) && i.row == 2 && i.section == 1);
    order_count = 0;
    assert(cui_command_rebuild(&s, entries, 6, cui_string("  ACCOUNT  "), true, o));
    assert(s.row_count == 2 && s.matched_count == 1 && s.selected == 1);
    assert(order_count == 2 && order[0] == 1 && order[1] == 2);
    assert(cui_command_selected(&s, &i) && i.section == 1 && i.row == 1);
    s.scroll.y = 45;
    s.pending_scroll = false;
    assert(cui_command_rebuild(&s, entries, 6, cui_string("account"), false, o));
    assert(!s.pending_scroll && s.scroll.y == 45);
    cui_command_clear_selection(&s);
    assert(!cui_command_selected(&s, NULL));
    assert(cui_command_rebuild(&s, entries, 6, cui_string("account"), false, o));
    assert(!cui_command_selected(&s, NULL));
    assert(cui_command_move(&s, true, o));
    assert(cui_command_selected(&s, &i));
    assert(!cui_command_select(&s, (cui_command_index){1, 0}, true, o));
    assert(cui_command_rebuild(&s, entries, 6, cui_string("absent"), true, o));
    assert(s.row_count == 0 && s.matched_count == 0);
    assert(!cui_command_move(&s, true, o) && !cui_command_confirm(&s, o));
    o.filterable = false;
    assert(cui_command_rebuild(&s, entries, 6, cui_string("absent"), true, o));
    assert(s.matched_count == 4);
    o.filterable = true;
    o.searchable = false;
    assert(cui_command_rebuild(&s, entries, 6, cui_string("absent"), true, o));
    assert(s.matched_count == 4);
    size_t old = s.row_count;
    s.capacity = 2;
    assert(!cui_command_rebuild(&s, entries, 6, cui_string(""), false, o));
    assert(s.row_count == old);
    s.capacity = 16;
    cui_command_entry invalid = {.kind = CUI_COMMAND_GROUP, .item_count = 1};
    assert(!cui_command_rebuild(&s, &invalid, 1, cui_string(""), false, o));
    assert(s.row_count == old);
    group[1].disabled = true;
    group[2].disabled = true;
    entries[1].item.disabled = true;
    assert(cui_command_rebuild(&s, entries, 6, cui_string(""), false, o));
    assert(!cui_command_selected(&s, NULL));
    assert(!cui_command_confirm(&s, o));
}
#ifndef CUI_COMMAND_BASE_ONLY
static cui_draw_list draw(cui_context *ui, cui_command_state *s, const cui_command_entry *e,
                          size_t n, cui_command_options o) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 500, 400, .016f));
    assert(cui_command(ui, 42, &t, s, e, n, o));
    cui_draw_list result = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_render(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    char buffer[64] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    assert(edit);
    cui_command_state s;
    cui_command_row rows[8];
    assert(cui_command_init(&s, rows, 8, edit));
    cui_command_entry e[3] = {
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("First"), .checked = true}},
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Disabled"), .disabled = true}},
        {.kind = CUI_COMMAND_ITEM,
         .item = {.label = cui_string("Last"), .action = action, .action_user = order}}};
    cui_command_options o = cui_command_default();
    o.on_confirm = confirm;
    o.user = order;
    draw(ui, &s, e, 3, o);
    cui_rect input_bounds = cui_bounds(ui, cui_command_input_id(42));
    cui_rect input_view = cui_bounds(ui, cui_input_viewport_id(cui_command_input_id(42)));
    assert(fabsf(input_bounds.x - 37) < .01f && fabsf(input_view.x - input_bounds.x) < .01f);
    assert(fabsf(input_bounds.h - 32) < .01f);
    cui_command_focus(ui, 42, o);
    draw(ui, &s, e, 3, o);
    cui_event event = {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN};
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    cui_command_index index;
    assert(cui_command_selected(&s, &index) && index.row == 2);
    order_count = 0;
    event.key = CUI_KEY_ENTER;
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    assert(order_count == 2 && order[0] == 3 && order[1] == 4);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Fir"}));
    draw(ui, &s, e, 3, o);
    assert(s.matched_count == 1);
    assert(cui_edit_get_text(edit).length == 3);
    event.key = CUI_KEY_ESCAPE;
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    assert(cui_edit_get_text(edit).length == 0 && s.matched_count == 3 &&
           cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    assert(!cui_event_consumed(ui, 0));
    cui_rect last = cui_bounds(ui, cui_command_row_id(42, (cui_command_index){0, 2}));
    assert(last.w > 0 && last.h >= 32);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = last.x + 10, .y = last.y + 10}));
    draw(ui, &s, e, 3, o);
    assert(cui_command_selected(&s, &index) && index.row == 2);
    event.key = CUI_KEY_UP;
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    draw(ui, &s, e, 3, o);
    assert(cui_command_selected(&s, &index) && index.row == 0); /* stationary hover */
    order_count = 0;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = last.x + 10, .y = last.y + 10}));
    draw(ui, &s, e, 3, o);
    assert(order_count == 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = last.x + 12, .y = last.y + 10}));
    draw(ui, &s, e, 3, o);
    assert(order_count == 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = last.x + 12, .y = last.y + 10}));
    draw(ui, &s, e, 3, o);
    assert(order_count == 2);
    assert(cui_focused(ui) == cui_command_input_id(42));
    o.searchable = false;
    draw(ui, &s, e, 3, o);
    cui_command_focus(ui, 42, o);
    draw(ui, &s, e, 3, o);
    assert(cui_focused(ui) == 42);
    assert(cui_edit_insert(edit, cui_string("hidden")) == CUI_EDIT_OK);
    event.key = CUI_KEY_ESCAPE;
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    assert(cui_edit_get_text(edit).length == 6 && !cui_event_consumed(ui, 0));
    o.disabled = true;
    event.key = CUI_KEY_DOWN;
    assert(cui_queue_event(ui, event));
    draw(ui, &s, e, 3, o);
    assert(!cui_event_consumed(ui, 0));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static unsigned content_calls;
static void custom(cui_context *ui, const cui_theme *t, const cui_command_state *s, void *u) {
    (void)s;
    (void)u;
    ++content_calls;
    cui_text(ui, cui_string("Custom row"), 14, t->text);
}
static float measure(float w, float f, void *u) {
    (void)w;
    (void)f;
    return *(float *)u;
}
static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t n;
    const cui_semantic *all = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (all[i].id == id)
            return &all[i];
    return NULL;
}
static void test_virtual_geometry(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    char buffer[64] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    assert(edit);
    cui_command_state s;
    cui_command_row rows[64];
    assert(cui_command_init(&s, rows, 64, edit));
    float height = 40;
    cui_command_item items[50];
    for (size_t i = 0; i < 50; ++i)
        items[i] = (cui_command_item){.label = cui_string("Row"),
                                      .content = custom,
                                      .content_user = &height,
                                      .measure = measure};
    cui_command_entry e = {.kind = CUI_COMMAND_GROUP,
                           .heading = cui_string("Group"),
                           .items = items,
                           .item_count = 50};
    cui_command_options o = cui_command_default();
    o.max_height = 120;
    o.searchable = false;
    content_calls = 0;
    draw(ui, &s, &e, 1, o);
    assert(content_calls < 5 && content_calls > 0);
    assert(fabsf(s.rows[1].height - 40) < .01f);
    assert(fabsf(cui_bounds(ui, cui_command_list_id(42)).h - 120) < .01f);
    const cui_semantic *list = semantic(ui, cui_command_list_id(42));
    assert(list && list->role == CUI_ROLE_LISTBOX);
    const cui_semantic *option = semantic(ui, cui_command_row_id(42, (cui_command_index){0, 0}));
    assert(option && option->role == CUI_ROLE_OPTION && option->checked);
    assert(!semantic(ui, cui_command_row_id(42, (cui_command_index){0, 49})));
    cui_rect viewport = cui_bounds(ui, cui_command_list_id(42));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = viewport.x + 20,
                                           .y = viewport.y + 20,
                                           .dy = -45,
                                           .scroll_precise = true}));
    draw(ui, &s, &e, 1, o);
    assert(fabsf(s.scroll.y - 45) < .01f && cui_event_consumed(ui, 0));
    assert(cui_command_select(&s, (cui_command_index){0, 49}, true, o));
    draw(ui, &s, &e, 1, o);
    assert(s.scroll.y > 1800);
    assert(semantic(ui, cui_command_row_id(42, (cui_command_index){0, 49})));
    assert(!semantic(ui, cui_command_row_id(42, (cui_command_index){0, 0})));
    height = NAN;
    assert(cui_begin_frame(ui, 500, 400, .016f));
    cui_theme t = cui_theme_light();
    assert(!cui_command(ui, 42, &t, &s, &e, 1, o));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    height = 60;
    draw(ui, &s, &e, 1, o);
    assert(fabsf(s.rows[1].height - 60) < .01f);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; ++i) {
        assert(cui_begin_frame(ui, 500, 400, .016f));
        assert(cui_command(ui, 42, &themes[i], &s, &e, 1, o));
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    o.max_height = NAN;
    assert(cui_begin_frame(ui, 500, 400, .016f));
    assert(!cui_command(ui, 42, &t, &s, &e, 1, o));
    (void)cui_end_frame(ui);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static unsigned headers, footers, empties, bindings;
static void header(cui_context *ui, const cui_theme *t, const cui_command_state *s, void *u) {
    (void)s;
    (void)u;
    ++headers;
    cui_text(ui, cui_string("Header"), 14, t->text);
}
static void footer(cui_context *ui, const cui_theme *t, const cui_command_state *s, void *u) {
    (void)s;
    (void)u;
    ++footers;
    cui_text(ui, cui_string("Footer"), 14, t->text);
}
static void empty(cui_context *ui, const cui_theme *t, const cui_command_state *s, void *u) {
    (void)u;
    assert(s->matched_count == 0);
    ++empties;
    cui_text(ui, cui_string("Nothing"), 14, t->muted);
}
static cui_str binding(void *u) {
    (void)u;
    ++bindings;
    return cui_string("Ctrl+P");
}
static void test_slots_and_loading(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    char buffer[64] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    assert(edit);
    cui_command_state s;
    cui_command_row rows[8];
    assert(cui_command_init(&s, rows, 8, edit));
    cui_command_options o = cui_command_default();
    o.header = header;
    o.footer = footer;
    o.empty = empty;
    headers = footers = empties = 0;
    draw(ui, &s, NULL, 0, o);
    assert(headers == 1 && footers == 1 && empties == 1);
    s.loading = true;
    draw(ui, &s, NULL, 0, o);
    assert(headers == 2 && footers == 2 && empties == 1);
    assert(semantic(ui, cui_command_list_id(42))->details.busy);
    s.loading = false;
    cui_command_entry e = {.kind = CUI_COMMAND_ITEM,
                           .item = {.label = cui_string("Profile"),
                                    .checked = true,
                                    .action = action,
                                    .binding = binding,
                                    .action_user = order}};
    bindings = 0;
    order_count = 0;
    cui_draw_list commands = draw(ui, &s, &e, 1, o);
    assert(bindings == 1 && order_count == 0);
    bool hint = false;
    for (size_t i = 0; i < commands.count; ++i)
        if (commands.commands[i].type == CUI_DRAW_TEXT && commands.commands[i].text.length == 6 &&
            !memcmp(commands.commands[i].text.data, "Ctrl+P", 6))
            hint = true;
    assert(hint);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

typedef struct {
    cui_context *ui;
    cui_surface_state *surface;
    unsigned cancelled;
} host;
static void cancel(void *u) {
    host *h = u;
    ++h->cancelled;
    cui_surface_request_close(h->ui, h->surface, CUI_SURFACE_CANCELLED);
}
static void dialog_frame(cui_context *ui, cui_surface_state *surface, cui_command_state *s,
                         cui_command_entry *entry, cui_command_options o) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 400, .016f));
    (void)cui_base_button(ui, 1, cui_string("Open"), false);
    cui_begin_box(ui, 1, (cui_box){.width = 60, .height = 30});
    cui_text(ui, cui_string("Open"), 14, t.text);
    cui_end_box(ui);
    if (cui_surface_begin(ui, &t, surface, cui_string("Commands"), (cui_str){0},
                          (cui_surface_options){.viewport = {0, 0, 600, 400},
                                                .width = 420,
                                                .height = 300,
                                                .no_escape_dismiss = true,
                                                .hide_close = true})) {
        assert(cui_command(ui, 42, &t, s, entry, 1, o));
        cui_surface_end(ui);
    }
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_dialog(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    char buffer[64] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    assert(edit);
    cui_command_state s;
    cui_command_row rows[8];
    assert(cui_command_init(&s, rows, 8, edit));
    cui_command_entry e = {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Profile")}};
    cui_surface_state surface;
    cui_surface_init(&surface, 100, CUI_SURFACE_DIALOG);
    host h = {ui, &surface, 0};
    cui_command_options o = cui_command_default();
    o.on_cancel = cancel;
    o.user = &h;
    o.bordered = false;
    dialog_frame(ui, &surface, &s, &e, o);
    cui_focus(ui, 1);
    cui_surface_open(ui, &surface, 1);
    dialog_frame(ui, &surface, &s, &e, o);
    cui_command_focus(ui, 42, o);
    dialog_frame(ui, &surface, &s, &e, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "pro"}));
    dialog_frame(ui, &surface, &s, &e, o);
    assert(cui_edit_get_text(edit).length == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    dialog_frame(ui, &surface, &s, &e, o);
    assert(surface.overlay.open && h.cancelled == 0 && cui_edit_get_text(edit).length == 0 &&
           cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    dialog_frame(ui, &surface, &s, &e, o);
    assert(surface.overlay.open && h.cancelled == 1 &&
           surface.pending_close == CUI_SURFACE_CANCELLED);
    dialog_frame(ui, &surface, &s, &e, o);
    assert(!surface.overlay.open && cui_focused(ui) == 1 &&
           cui_surface_take_result(&surface) == CUI_SURFACE_CANCELLED);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

#endif
int main(void) {
    test_model();
#ifndef CUI_COMMAND_BASE_ONLY
    test_render();
    test_virtual_geometry();
    test_dialog();
    test_slots_and_loading();
#endif
    return 0;
}
