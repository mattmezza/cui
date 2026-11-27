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
typedef struct {
    cui_str raw, trimmed;
    unsigned match_count, query_count;
} filter_observation;
static bool match_original(const cui_command_item *item, cui_str text, void *user) {
    filter_observation *capture = user;
    assert(text.data == capture->trimmed.data && text.length == capture->trimmed.length);
    ++capture->match_count;
    return item->keyword_count != 0;
}
static void query_original(cui_str text, void *user) {
    filter_observation *capture = user;
    assert(text.data == capture->raw.data && text.length == capture->raw.length);
    ++capture->query_count;
}
static void test_unicode_filter(void) {
    const struct {
        const char *label, *query;
        bool matches;
    } cases[] = {
        {u8"\u00c9diter le profil", u8"\u00a0\u00c9DITER\u3000", true},
        {u8"\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c", u8"\u0425\u0420\u0410\u041d",
         true},
        {u8"\U00010400\U00010401", u8"\U00010429", true},
        {u8"temperature \u212a", "k", true},
        {u8"\u0130stanbul", u8"i\u0307s", true},
        {u8"\u0130stanbul", u8"\u0307s", true},
        {u8"\u0130stanbul", "is", false},
        {u8"\u039f\u03a3", u8"\u03bf\u03c2", true},
        {u8"\u039f\u03a3", u8"\u03bf\u03c3", false},
        {u8"\u039f\u03a3.\u0391", u8"\u03bf\u03c3.\u03b1", true},
        {u8"\u039f.\u03a3", u8"\u03bf.\u03c2", true},
        {u8"\u039f\u03a3 \u0391", u8"\u03bf\u03c2 \u03b1", true},
        {u8"\u03a3", u8"\u03c3", true},
        {u8"\u0130\u03a3", u8"i\u0307\u03c2", true},
        {u8"Stra\u00dfe", "STRASSE", false},
        {u8"caf\u00e9", u8"cafe\u0301", false},
        {"label", u8"\t\u202f\xc2\x85\n", true},
        {"label", u8"\u200blabel", false},
        {"\xf0\x9f", "\x9f", true},
        {"\xc3\xa9", "\xa9", false},
        {"\xf4\x90\x80\x80", "\xf4\x90\x80\x80", true},
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; ++i) {
        cui_command_item item = {.label = cui_string(cases[i].label)};
        assert(cui_command_matches(&item, cui_string(cases[i].query)) == cases[i].matches);
    }
    const char bytes[] = {'X', '\0', 'Y'};
    cui_command_item binary = {.label = (cui_str){bytes, sizeof bytes}};
    assert(cui_command_matches(&binary, (cui_str){bytes + 1, 2}));
    assert(!cui_command_matches(&binary, (cui_str){NULL, 1}));

    cui_str keywords[] = {cui_string(u8"PR\u00c9F\u00c9RENCES")};
    cui_command_entry entries[] = {
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Profile")}},
        {.kind = CUI_COMMAND_ITEM,
         .item = {.label = cui_string("Settings"), .keywords = keywords, .keyword_count = 1}},
    };
    cui_command_row rows[2];
    cui_command_state state;
    cui_command_index index;
    assert(cui_command_init(&state, rows, 2, NULL));
    assert(cui_command_rebuild(&state, entries, 2, cui_string(u8"\u3000pr\u00e9f\u00e9\u00a0"),
                               true, cui_command_default()));
    assert(state.matched_count == 1 && cui_command_selected(&state, &index));
    assert(index.section == 0 && index.row == 1);

    filter_observation capture = {.raw = cui_string(u8" \t\u3000Pr\u00e9f\u00e9\u00a0 \n")};
    capture.trimmed = (cui_str){capture.raw.data + 2, capture.raw.length - 4};
    cui_command_options opt = cui_command_default();
    opt.matches = match_original;
    opt.match_user = &capture;
    opt.on_query = query_original;
    opt.user = &capture;
    assert(cui_command_rebuild(&state, entries, 2, capture.raw, true, opt));
    assert(capture.match_count == 2 && capture.query_count == 1 && state.matched_count == 1);
    opt.matches = NULL;
    assert(cui_command_rebuild(&state, entries, 2, capture.raw, true, opt));
    assert(capture.match_count == 2 && capture.query_count == 2 && state.matched_count == 1);
}
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
static void test_separator_before_heading(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    assert(ui);
    char buffer[64] = "Save";
    cui_edit *edit = editor(buffer, sizeof buffer);
    assert(edit);
    cui_command_state s;
    cui_command_row rows[16];
    assert(cui_command_init(&s, rows, 16, edit));
    cui_command_item group[] = {{.label = cui_string("Settings")}};
    cui_command_entry entries[] = {
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Save")}},
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Open")}},
        {.kind = CUI_COMMAND_SEPARATOR},
        {.kind = CUI_COMMAND_GROUP,
         .heading = cui_string("Preferences"),
         .items = group,
         .item_count = 1}};
    cui_command_options o = cui_command_default();
    draw(ui, &s, entries, 4, o);
    assert(s.matched_count == 1);
    cui_command_focus(ui, 42, o);
    draw(ui, &s, entries, 4, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    draw(ui, &s, entries, 4, o);
    assert(cui_edit_get_text(edit).length == 0 && s.matched_count == 3);
    assert(cui_event_consumed(ui, 0));
    draw(ui, &s, entries, 4, o);
    cui_edit_destroy(edit);
    cui_destroy(ui);
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
static void test_panel_width(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[8];
    cui_command_entry entry = {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Open")}};
    cui_command_options options = cui_command_default();
    assert(ui && edit && cui_command_init(&state, rows, 8, edit));
    const float widths[] = {280, 420};
    for (unsigned i = 0; i < 2; i++) {
        options.width = widths[i];
        draw(ui, &state, &entry, 1, options);
        cui_rect panel = cui_bounds(ui, 42), list = cui_bounds(ui, cui_command_list_id(42));
        cui_rect row = cui_bounds(ui, cui_command_row_id(42, (cui_command_index){0, 0}));
        assert(fabsf(list.w - panel.w + 2) < .01f);
        assert(fabsf(row.w - list.w + 18) < .01f);
        assert(row.x >= list.x && row.x + row.w <= list.x + list.w - 10);
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static cui_draw_list grow_panel(cui_context *ui, cui_command_state *state,
                                const cui_command_entry *entries, size_t count, float width,
                                cui_command_options options, bool hidden) {
    cui_theme theme = cui_theme_light();
    if (hidden)
        theme.scrollbar.visibility = CUI_SCROLLBAR_HIDDEN;
    assert(cui_begin_frame(ui, 700, 400, .016f));
    cui_begin_box(ui, 41, (cui_box){.width = width, .padding = 8});
    assert(cui_command(ui, 42, &theme, state, entries, count, options));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(list.count && cui_last_error(ui) == CUI_OK);
    return list;
}

static void test_grow_panel_width(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[8];
    cui_command_entry entries[] = {
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Open file")}},
        {.kind = CUI_COMMAND_SEPARATOR},
        {.kind = CUI_COMMAND_ITEM, .item = {.label = cui_string("Save file")}},
    };
    cui_command_options options = cui_command_default();
    options.width = -1;
    options.on_confirm = confirm;
    options.user = order;
    assert(ui && edit && cui_command_init(&state, rows, 8, edit));
    float widths[] = {260, 520, 320};
    for (size_t i = 0; i < sizeof widths / sizeof widths[0]; ++i) {
        cui_draw_list draw = grow_panel(ui, &state, entries, 3, widths[i], options, false);
        cui_rect panel = cui_bounds(ui, 42);
        cui_rect list = cui_bounds(ui, cui_command_list_id(42));
        cui_id row_id = cui_command_row_id(42, (cui_command_index){0, 0});
        cui_rect row = cui_bounds(ui, row_id);
        assert(fabsf(panel.w - widths[i] + 16) < .01f);
        assert(fabsf(list.w - panel.w + 2) < .01f);
        assert(fabsf(row.w - list.w + 18) < .01f);
        assert(fabsf(row.x - list.x - 4) < .01f);
        const cui_semantic *target = semantic(ui, row_id);
        assert(target && fabsf(target->bounds.w - row.w) < .01f);
        bool fill = false;
        cui_color accent = cui_theme_light().accent;
        for (size_t j = 0; j < draw.count; ++j) {
            const cui_draw_command *cmd = &draw.commands[j];
            if (cmd->type == CUI_DRAW_RECT && fabsf(cmd->bounds.x - row.x) < .01f &&
                fabsf(cmd->bounds.y - row.y) < .01f && fabsf(cmd->bounds.w - row.w) < .01f &&
                !memcmp(&accent, &cmd->color, sizeof accent))
                fill = true;
        }
        assert(fill);
    }

    cui_rect row = cui_bounds(ui, cui_command_row_id(42, (cui_command_index){0, 1}));
    cui_event tap = {.type = CUI_EVENT_POINTER_DOWN,
                     .pointer_type = CUI_POINTER_TOUCH,
                     .pointer_id = 2,
                     .x = row.x + row.w - 2,
                     .y = row.y + row.h / 2};
    order_count = 0;
    assert(cui_queue_event(ui, tap));
    grow_panel(ui, &state, entries, 3, 320, options, false);
    assert(order_count == 0);
    tap.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, tap));
    grow_panel(ui, &state, entries, 3, 320, options, false);
    assert(order_count == 1 && selected_index.row == 1);
    assert(cui_focused(ui) == cui_command_input_id(42));
    options.width = 304;
    grow_panel(ui, &state, entries, 3, 320, options, true);
    cui_rect list = cui_bounds(ui, cui_command_list_id(42));
    row = cui_bounds(ui, cui_command_row_id(42, (cui_command_index){0, 1}));
    assert(fabsf(row.w - list.w + 8) < .01f);
    assert(cui_bounds(ui, cui_scrollbar_id(cui_command_list_id(42), true)).w == 0);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static float measure_width(float width, float font, void *user) {
    (void)font;
    *(float *)user = width;
    return width < 300 ? 60 : 32;
}

static bool custom_header_prepaint(const cui_prepaint_info *info, const void *payload,
                                   cui_prepaint_writer *writer) {
    (void)info;
    (void)payload;
    (void)writer;
    return true;
}

static void custom_header(cui_context *ui, const cui_theme *theme, const cui_command_state *state,
                          void *user) {
    (void)theme;
    (void)state;
    (void)user;
    assert(cui_set_prepaint(
        ui, (cui_prepaint_options){.callback = custom_header_prepaint, .command_budget = 1}));
    /* A clipped header can be wider than the palette. Keep the list at the
     * resolved width while retaining the header's own layout. */
    cui_begin_box(ui, 40, (cui_box){.width = 620, .height = 10});
    cui_end_box(ui);
}

static void test_grow_custom_measure(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[8];
    float measured_width = 0;
    cui_command_entry entry = {.kind = CUI_COMMAND_ITEM,
                               .item = {.label = cui_string("Wrapped entry"),
                                        .measure = measure_width,
                                        .content_user = &measured_width}};
    cui_command_options options = cui_command_default();
    options.width = -1;
    options.searchable = false;
    options.header = custom_header;
    assert(ui && edit && cui_command_init(&state, rows, 8, edit));
    cui_id row_id = cui_command_row_id(42, (cui_command_index){0, 0});
    grow_panel(ui, &state, &entry, 1, 260, options, false);
    cui_rect row = cui_bounds(ui, row_id);
    assert(fabsf(row.w - measured_width) < .01f && row.h == 60 && !cui_needs_redraw(ui));
    grow_panel(ui, &state, &entry, 1, 520, options, false);
    row = cui_bounds(ui, row_id);
    assert(fabsf(row.w - measured_width) < .01f && row.h == 32 && !cui_needs_redraw(ui));
    grow_panel(ui, &state, &entry, 1, 260, options, true);
    row = cui_bounds(ui, row_id);
    assert(fabsf(row.w - measured_width) < .01f && row.h == 60 && !cui_needs_redraw(ui));
    assert(row.w == 234);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static float measure_short_row(float width, float font, void *user) {
    (void)font;
    *(float *)user = width;
    return width >= 225 ? 5 : 7;
}

static void test_short_custom_row_matches_measurement(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[2];
    float measured_width = 0;
    cui_command_entry entry = {.kind = CUI_COMMAND_ITEM,
                               .item = {.label = cui_string("Short entry"),
                                        .measure = measure_short_row,
                                        .content_user = &measured_width}};
    cui_command_options options = cui_command_default();
    options.searchable = false;
    options.on_confirm = confirm;
    options.user = order;
    assert(ui && edit && cui_command_init(&state, rows, 2, edit));
    cui_id list_id = cui_command_list_id(42);
    cui_id track_id = cui_scrollbar_id(list_id, true);
    cui_id row_id = cui_command_row_id(42, (cui_command_index){0, 0});
    const float widths[] = {244, -1};
    for (size_t i = 0; i < sizeof widths / sizeof *widths; ++i) {
        options.width = widths[i];
        grow_panel(ui, &state, &entry, 1, 260, options, false);
        cui_rect row = cui_bounds(ui, row_id), list = cui_bounds(ui, list_id);
        cui_rect track = cui_bounds(ui, track_id);
        const cui_semantic *option = semantic(ui, row_id);
        assert(list.h == 15 && track.w == 7.5f && track.x + track.w == list.x + list.w);
        assert(measured_width == 224 && row.w == measured_width && row.h == 7);
        assert(option && option->bounds.w == row.w && !cui_needs_redraw(ui));

        order_count = 0;
        for (unsigned inside = 0; inside < 2; ++inside) {
            cui_event tap = {.type = CUI_EVENT_POINTER_DOWN,
                             .pointer_type = CUI_POINTER_TOUCH,
                             .pointer_id = 2,
                             .x = row.x + row.w + (inside ? -1 : 1),
                             .y = row.y + row.h / 2};
            assert(cui_queue_event(ui, tap));
            grow_panel(ui, &state, &entry, 1, 260, options, false);
            tap.type = CUI_EVENT_POINTER_UP;
            assert(cui_queue_event(ui, tap));
            grow_panel(ui, &state, &entry, 1, 260, options, false);
            assert(order_count == inside);
        }

        grow_panel(ui, &state, &entry, 1, 260, options, true);
        row = cui_bounds(ui, row_id);
        assert(measured_width == 234 && row.w == measured_width && row.h == 5);
        assert(cui_bounds(ui, list_id).h == 13 && cui_bounds(ui, track_id).w == 0);
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static void test_custom_height_reflow_keeps_visible_selection(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[12];
    cui_command_entry entries[10];
    float measured_width = 0;
    for (size_t i = 0; i < 10; ++i)
        entries[i] = (cui_command_entry){.kind = CUI_COMMAND_ITEM,
                                         .item = {.label = cui_string("Wrapped entry"),
                                                  .measure = measure_width,
                                                  .content_user = &measured_width}};
    cui_command_options options = cui_command_default();
    options.width = -1;
    options.searchable = false;
    options.max_height = 100;
    options.on_select = selection;
    options.user = order;
    order_count = 0;
    assert(ui && edit && cui_command_init(&state, rows, 12, edit));

    cui_command_index target = {0, 7}, selected;
    cui_id target_id = cui_command_row_id(42, target);
    grow_panel(ui, &state, entries, 10, 520, options, false);
    assert(cui_command_select(&state, target, true, options));
    grow_panel(ui, &state, entries, 10, 520, options, false);
    assert(state.scroll.y == 160 && cui_bounds(ui, target_id).h == 32);

    grow_panel(ui, &state, entries, 10, 260, options, false);
    cui_rect viewport = cui_bounds(ui, cui_command_list_id(42));
    const cui_semantic *item = semantic(ui, target_id);
    assert(item && item->checked && item->bounds.h == 60);
    assert(item->bounds.y >= viewport.y &&
           item->bounds.y + item->bounds.h <= viewport.y + viewport.h);
    assert(state.scroll.y == 384 && !cui_needs_redraw(ui));
    assert(order_count == 1);
    assert(cui_command_selected(&state, &selected) && cui_command_index_equal(selected, target));

    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = viewport.x + 20,
                                           .y = viewport.y + 20,
                                           .dy = 344,
                                           .scroll_precise = true}));
    grow_panel(ui, &state, entries, 10, 260, options, false);
    assert(cui_event_consumed(ui, 0) && state.scroll.y == 40 && !semantic(ui, target_id));
    grow_panel(ui, &state, entries, 10, 520, options, false);
    assert(state.scroll.y == 40 && !semantic(ui, target_id) && order_count == 1);
    assert(cui_command_selected(&state, &selected) && cui_command_index_equal(selected, target));

    cui_edit_destroy(edit);
    cui_destroy(ui);
}

static void test_custom_height_with_later_sibling(bool short_row) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    char buffer[32] = {0};
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_command_state state;
    cui_command_row rows[8];
    float measured_width = 0;
    cui_command_entry entry = {.kind = CUI_COMMAND_ITEM,
                               .item = {.label = cui_string("Wrapped entry"),
                                        .measure = short_row ? measure_short_row : measure_width,
                                        .content_user = &measured_width}};
    cui_command_options options = cui_command_default();
    options.width = -1;
    options.searchable = false;
    cui_theme theme = cui_theme_light();
    assert(ui && edit && cui_command_init(&state, rows, 8, edit));
    for (unsigned frame = 0; frame < 2; ++frame) {
        assert(cui_begin_frame(ui, 700, 400, .016f));
        cui_begin_box(ui, 41, (cui_box){.width = 520, .padding = 8, .gap = 12, .row = true});
        assert(cui_command(ui, 42, &theme, &state, &entry, 1, options));
        cui_begin_box(ui, 43, (cui_box){.width = 250, .height = 20});
        cui_end_box(ui);
        cui_end_box(ui);
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_rect row = cui_bounds(ui, cui_command_row_id(42, (cui_command_index){0, 0}));
        float expected_height = short_row ? (frame ? 7 : 5) : (frame ? 60 : 32);
        assert(row.w == 222 && row.h == expected_height);
        assert(cui_needs_redraw(ui) == !frame);
        if (frame)
            assert(row.w == measured_width);
    }
    cui_edit_destroy(edit);
    cui_destroy(ui);
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
                         cui_command_entry *entry, cui_command_options o, float width) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 400, .016f));
    (void)cui_base_button(ui, 1, cui_string("Open"), false);
    cui_begin_box(ui, 1, (cui_box){.width = 60, .height = 30});
    cui_text(ui, cui_string("Open"), 14, t.text);
    cui_end_box(ui);
    if (cui_surface_begin(ui, &t, surface, cui_string("Commands"), (cui_str){0},
                          (cui_surface_options){.viewport = {0, 0, 600, 400},
                                                .width = width,
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
    float measured_width = 0;
    cui_command_entry e = {.kind = CUI_COMMAND_ITEM,
                           .item = {.label = cui_string("Profile"),
                                    .measure = measure_width,
                                    .content_user = &measured_width}};
    cui_surface_state surface;
    cui_surface_init(&surface, 100, CUI_SURFACE_DIALOG);
    host h = {ui, &surface, 0};
    cui_command_options o = cui_command_default();
    o.on_cancel = cancel;
    o.user = &h;
    o.bordered = false;
    o.width = -1;
    dialog_frame(ui, &surface, &s, &e, o, 270);
    cui_focus(ui, 1);
    cui_surface_open(ui, &surface, 1);
    dialog_frame(ui, &surface, &s, &e, o, 270);
    cui_id row_id = cui_command_row_id(42, (cui_command_index){0, 0});
    cui_rect row = cui_bounds(ui, row_id);
    assert(row.h == 60 && row.w == measured_width);
    cui_command_focus(ui, 42, o);
    dialog_frame(ui, &surface, &s, &e, o, 420);
    row = cui_bounds(ui, row_id);
    assert(row.h == 32 && row.w == measured_width);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "pro"}));
    dialog_frame(ui, &surface, &s, &e, o, 420);
    assert(cui_edit_get_text(edit).length == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    dialog_frame(ui, &surface, &s, &e, o, 420);
    assert(surface.overlay.open && h.cancelled == 0 && cui_edit_get_text(edit).length == 0 &&
           cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    dialog_frame(ui, &surface, &s, &e, o, 420);
    assert(surface.overlay.open && h.cancelled == 1 &&
           surface.pending_close == CUI_SURFACE_CANCELLED);
    dialog_frame(ui, &surface, &s, &e, o, 420);
    assert(!surface.overlay.open && cui_focused(ui) == 1 &&
           cui_surface_take_result(&surface) == CUI_SURFACE_CANCELLED);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}

#endif
int main(void) {
    test_unicode_filter();
    test_model();
#ifndef CUI_COMMAND_BASE_ONLY
    test_separator_before_heading();
    test_render();
    test_virtual_geometry();
    test_panel_width();
    test_grow_panel_width();
    test_grow_custom_measure();
    test_short_custom_row_matches_measurement();
    test_custom_height_reflow_keeps_visible_selection();
    test_custom_height_with_later_sibling(false);
    test_custom_height_with_later_sibling(true);
    test_dialog();
    test_slots_and_loading();
#endif
    return 0;
}
