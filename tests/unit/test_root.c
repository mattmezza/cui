/* SPDX-License-Identifier: MIT */
#include "cui/cui_root.h"
#include <assert.h>
#include <string.h>
typedef struct {
    char log[128];
    size_t length;
    char tag;
} plugin_state;
typedef struct {
    plugin_state states[2];
    size_t count;
    char log[128];
    size_t length;
} window_state;
static void mark(window_state *window, char stage, char tag) {
    assert(window->length + 2 < sizeof window->log);
    window->log[window->length++] = stage;
    window->log[window->length++] = tag;
    window->log[window->length] = 0;
}
typedef struct {
    window_state *window;
    char tag;
} root_test_plugin;
static root_test_plugin plugin_states[8];
static size_t plugin_state_count;
static unsigned destroyed;
static void *build_plugin(void *user, void *window_user) {
    char tag = *(char *)user;
    window_state *window = window_user;
    assert(window->count < 2 && plugin_state_count < 8);
    plugin_state *state = &window->states[window->count++];
    state->tag = tag;
    root_test_plugin *instance = &plugin_states[plugin_state_count++];
    *instance = (root_test_plugin){window, tag};
    (void)state;
    return instance;
}
static void prepare(cui_context *ui, void *user) {
    (void)ui;
    root_test_plugin *plugin = user;
    mark(plugin->window, 'P', plugin->tag);
}
static void style(cui_context *ui, void *user, cui_root_style *value) {
    (void)ui;
    root_test_plugin *plugin = user;
    mark(plugin->window, 'S', plugin->tag);
    value->background = (cui_color){(uint8_t)plugin->tag, 0, 0, 255};
    value->foreground = (cui_color){55, 0, 0, 255};
    value->padding = 6;
}
static void begin(cui_context *ui, void *user, cui_root_style value) {
    root_test_plugin *plugin = user;
    mark(plugin->window, 'B', plugin->tag);
    assert(value.background.r == 77 && value.foreground.r == 55 && value.padding == 6);
    cui_begin_box(ui, 700 + (unsigned char)plugin->tag,
                  (cui_box){.width = 400, .height = 300, .padding = 10});
}
static void end(cui_context *ui, void *user) {
    root_test_plugin *plugin = user;
    mark(plugin->window, 'E', plugin->tag);
    cui_end_box(ui);
}
static void overlay(cui_context *ui, void *user, cui_rect viewport) {
    root_test_plugin *plugin = user;
    mark(plugin->window, 'O', plugin->tag);
    assert(viewport.w == 400 && viewport.h == 300);
    cui_begin_portal(ui, 900 + (unsigned char)plugin->tag, (cui_box){.width = 5, .height = 5},
                     (cui_rect){0, 0, 5, 5}, 5);
    cui_end_box(ui);
}
static void app(cui_context *ui, void *user) {
    window_state *window = user;
    mark(window, 'C', '_');
    cui_begin_box(ui, 12, (cui_box){.width = 30, .height = 20});
    cui_end_box(ui);
}
static void facility(cui_context *ui, void *user) {
    (void)ui;
    mark(user, 'F', '_');
}
static bool copy_selection(cui_context *ui, void *user) {
    (void)ui;
    unsigned *count = user;
    (*count)++;
    return true;
}
static void destroy_plugin(void *registration_user, void *state) {
    (void)registration_user;
    assert(state);
    destroyed++;
}
static void *fail_plugin(void *registration_user, void *window_user) {
    (void)registration_user;
    (void)window_user;
    return NULL;
}
static cui_root_plugin make_plugin(uint64_t key, char *tag) {
    return (cui_root_plugin){.key = key,
                             .build = build_plugin,
                             .destroy = destroy_plugin,
                             .prepare = prepare,
                             .style = style,
                             .decorate_begin = begin,
                             .decorate_end = end,
                             .overlay = overlay,
                             .registration_user = tag};
}
static void test_registry_and_base_root(void) {
    cui_root_plugin registrations[2];
    cui_root_registry registry;
    assert(cui_root_registry_init(&registry, registrations, 2));
    char a = 'a', b = 'b', x = 'x';
    assert(cui_root_register(&registry, make_plugin(1, &a)));
    assert(cui_root_register(&registry, make_plugin(2, &b)));
    assert(registry.count == 2);
    cui_root_registry corrupt = registry;
    corrupt.count = 3;
    assert(!cui_root_register(&corrupt, make_plugin(3, &x)));
    corrupt = registry;
    corrupt.entries = NULL;
    assert(!cui_root_register(&corrupt, make_plugin(3, &x)));
    cui_root first = {0}, second = {0};
    cui_root_plugin_instance first_entries[2], second_entries[2];
    window_state first_window = {0}, second_window = {0};
    assert(!cui_root_init(&first, first_entries, 1, &registry, &first_window));
    assert(!first.initialized && first_window.count == 0);
    cui_root_registry failing = registry;
    cui_root_plugin failure_entries[2] = {registrations[0], registrations[1]};
    failure_entries[1].build = fail_plugin;
    failing.entries = failure_entries;
    assert(!cui_root_init(&first, first_entries, 2, &failing, &first_window));
    assert(!first.initialized && first_window.count == 1 && destroyed == 1);
    assert(!first_entries[0].state);
    first_window.count = 0;
    assert(cui_root_init(&first, first_entries, 2, &registry, &first_window));
    assert(cui_root_register(&registry, make_plugin(1, &x)));
    assert(registry.count == 2);
    assert(cui_root_init(&second, second_entries, 2, &registry, &second_window));
    assert(cui_root_plugin_state(&first, 1) != cui_root_plugin_state(&second, 1));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_root_style_override override = {.value = {.background = {77, 0, 0, 255}},
                                        .background = true};
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_base_root_render(ui, 100, &first, (cui_rect){0, 0, 400, 300}, (cui_root_style){0},
                                &override, app, &first_window, facility, &first_window));
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(strcmp(first_window.log, "PaPbSaSbBbBaC_OaObF_EaEb") == 0);
    assert(cui_bounds(ui, 100).w == 400 && cui_bounds(ui, 100).h == 300);
    assert(cui_bounds(ui, 700 + 'b').x < cui_bounds(ui, 700 + 'a').x &&
           cui_bounds(ui, 700 + 'a').x < cui_bounds(ui, 100).x);
    bool override_painted = false;
    for (size_t i = 0; i < draw.count; i++)
        override_painted |=
            draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 77;
    assert(override_painted);
    unsigned copied = 0;
    assert(cui_root_set_copy_hook(&first, copy_selection, &copied));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_C, .modifiers = CUI_MOD_CTRL}));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_base_root_render(ui, 100, &first, (cui_rect){0, 0, 400, 300}, (cui_root_style){0},
                                &override, app, &first_window, facility, &first_window));
    cui_end_frame(ui);
    assert(copied == 1 && cui_last_error(ui) == CUI_OK);
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_base_root_render(ui, 101, &second, (cui_rect){0, 0, 400, 300}, (cui_root_style){0},
                                &override, app, &second_window, facility, &second_window));
    cui_end_frame(ui);
    assert(strcmp(second_window.log, "PxPbSxSbBbBxC_OxObF_ExEb") == 0);
    cui_root_dispose(&first);
    cui_root_dispose(&second);
    assert(destroyed == 5 && !first.initialized && !second.initialized);
    cui_destroy(ui);
}
#ifndef CUI_ROOT_BASE_ONLY
static void dialog_content(cui_context *ui, void *user) {
    cui_id id = *(cui_id *)user;
    cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Inside"), false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = 80, .height = 24});
    cui_end_box(ui);
}
static void plain_content(cui_context *ui, void *user) {
    (void)user;
    cui_interact(ui, 1, CUI_ROLE_BUTTON, cui_string("Application"), false, false, 0);
    cui_begin_box(ui, 1, (cui_box){.width = 100, .height = 24});
    cui_end_box(ui);
}
static void styled_frame(cui_context *ui, cui_root *root, const cui_theme *theme, double now) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_root_render(ui, 500, root, theme, (cui_rect){0, 0, 400, 300}, NULL, plain_content,
                           NULL, now));
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_styled_facilities(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_theme theme = cui_theme_light();
    cui_root root = {0};
    assert(cui_root_init(&root, NULL, 0, NULL, NULL));
    cui_root_surface dialogs[2];
    cui_notification_entry note_entries[2];
    cui_notification_state notes;
    cui_notification_settings settings = cui_notification_settings_default();
    settings.reduced_motion = true;
    assert(cui_notification_init(&notes, note_entries, 2, settings, (cui_notification_host){0}));
    assert(cui_root_bind_facilities(&root, dialogs, 2, &notes));
    styled_frame(ui, &root, &theme, 0);
    cui_surface_state parent, child, sheet;
    cui_surface_init(&parent, 1001, CUI_SURFACE_DIALOG);
    cui_surface_init(&child, 1002, CUI_SURFACE_DIALOG);
    cui_surface_init(&sheet, 1003, CUI_SURFACE_SHEET);
    cui_id parent_control = 201, child_control = 202, sheet_control = 203;
    cui_root_surface first = {.surface = &parent,
                              .title = cui_string("Parent"),
                              .content = dialog_content,
                              .user = &parent_control};
    cui_root_surface second = {.surface = &child,
                               .title = cui_string("Child"),
                               .content = dialog_content,
                               .user = &child_control};
    cui_root_surface side = {.surface = &sheet,
                             .title = cui_string("Sheet"),
                             .content = dialog_content,
                             .user = &sheet_control};
    assert(cui_root_sheet_open(&root, ui, side, 1));
    assert(cui_root_dialog_open(&root, ui, first, 1));
    styled_frame(ui, &root, &theme, .1);
    assert(cui_bounds(ui, 1001).w > 0 && cui_bounds(ui, 1003).w > 0);
    cui_focus(ui, 201);
    assert(cui_root_dialog_open(&root, ui, second, 201));
    assert(!cui_root_dialog_open(&root, ui, second, 201));
    styled_frame(ui, &root, &theme, .2);
    assert(cui_bounds(ui, 1002).w > 0);
    cui_focus(ui, 202);
    for (unsigned i = 0; i < 5; i++)
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    styled_frame(ui, &root, &theme, .3);
    assert(cui_focused(ui) == 202 || cui_focused(ui) == cui_surface_part_id(1002, 1));
    cui_notification note = cui_notification_default();
    note.id = (cui_notification_id){42, 1};
    note.title = cui_string("Saved");
    note.message = cui_string("Changes saved");
    note.autohide = false;
    note.height = 70;
    assert(cui_root_notification_push(&root, note, .3));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    styled_frame(ui, &root, &theme, .4);
    assert(!child.overlay.open && parent.overlay.open && cui_focused(ui) == 201);
    assert(notes.count == 1 && note_entries[0].bounds.w > 0);
    assert(note_entries[0].bounds.x >= 0 &&
           note_entries[0].bounds.x + note_entries[0].bounds.w <= 400);
    for (unsigned edge = CUI_SHEET_LEFT; edge <= CUI_SHEET_BOTTOM; edge++) {
        root.sheet.options.edge = (cui_sheet_edge)edge;
        root.sheet.options.width = 120;
        root.sheet.options.height = 100;
        styled_frame(ui, &root, &theme, .5 + edge * .1);
        cui_rect panel = cui_bounds(ui, sheet.overlay.id);
        cui_rect notification = note_entries[0].bounds;
        assert(notification.w > 0 && notification.h > 0);
        if (edge == CUI_SHEET_LEFT)
            assert(notification.x >= panel.x + panel.w);
        else if (edge == CUI_SHEET_RIGHT)
            assert(notification.x + notification.w <= panel.x);
        else if (edge == CUI_SHEET_TOP)
            assert(notification.y >= panel.y + panel.h);
        else
            assert(notification.y + notification.h <= panel.y);
    }
    cui_destroy(ui);
}
#endif
int main(void) {
    test_registry_and_base_root();
#ifndef CUI_ROOT_BASE_ONLY
    test_styled_facilities();
#endif
    return 0;
}
