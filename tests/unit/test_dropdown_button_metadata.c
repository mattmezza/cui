/* SPDX-License-Identifier: MIT */
#include "../../src/components/cui_choice.c"
#include "../fixtures/dropdown_button_styles/legacy_box.h"
#include "cui/cui_software.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
static cui_context *ui;
static cui_menu_tree_state menu;
static cui_id command;
static cui_menu_entry entries[] = {{.id = 12, .label = {"Export", 6}, .kind = CUI_MENU_ACTION}};
static bool sentinels, blur_after;
static unsigned calls, commands, bodies;
static cui_dropdown_button_metadata *mutate;
static cui_semantic node(cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (s[i].id == id)
            return s[i];
    return (cui_semantic){0};
}
static bool name_is(cui_id id, const char *text) {
    cui_str s = node(id).name;
    return s.length == strlen(text) && (!s.length || !memcmp(s.data, text, s.length));
}
static void action(void *user, const cui_component_event_metadata *event) {
    CHECK(user == &calls && event->target == 2);
    calls++;
    CHECK(name_is(2, "Enregistrer") && node(2).checked && node(2).value == 1);
    CHECK(node(2).tab_ordered && node(2).tab_index == -10);
    if (blur_after)
        cui_focus(ui, 0);
    if (mutate) {
        mutate->action.name = cui_string("Changed");
        mutate->trigger.name = cui_string("Changed");
        mutate->toggled = false;
        mutate->trigger.tab_index = 999;
    }
}
static void menu_command(void *user, const cui_menu_command *event) {
    CHECK(user == &commands && event->command == 12);
    commands++;
    CHECK(name_is(3, "Autres") && node(3).tab_ordered && node(3).tab_index == 20);
    CHECK(node(3).checked && node(3).value == 0); /* Existing open-menu callback state. */
}
static void content(cui_context *context, void *user) {
    CHECK(context == ui && user == &bodies);
    bodies++;
    cui_text(ui, cui_string("Visible custom"), 12, (cui_color){240, 240, 240, 255});
}
static void sentinel(cui_id id, int index) {
    cui_base_button(ui, id, cui_string("Sibling"), false);
    cui_set_tab_index(ui, id, index);
    cui_begin_box(ui, id, (cui_box){.width = 20, .height = 10});
    cui_end_box(ui);
}
static cui_draw_list frame(const cui_dropdown_button_metadata *m, cui_dropdown_button_options o,
                           bool old, cui_dropdown_event_handlers handlers,
                           cui_dropdown_event_result *out) {
    cui_theme theme = cui_theme_light();
    CHECK(cui_begin_frame(ui, 360, 180, .016f));
    if (sentinels)
        sentinel(4, -20);
    o.menu.popup.viewport = (cui_rect){0, 0, 360, 180};
    cui_dropdown_event_result r = {0};
    if (old)
        r.legacy = legacy_render(ui, 1, 2, 3, &theme, cui_string("Save"), cui_string("More"),
                                 entries, 1, &command, &menu, o, handlers, NULL);
    else
        r = cui_dropdown_button_with_metadata_events(ui, 1, 2, 3, &theme, cui_string("Save"),
                                                     cui_string("More"), entries, 1, &command,
                                                     &menu, o, NULL, m, handlers);
    if (sentinels)
        sentinel(5, 30);
    cui_draw_list draw = cui_end_frame(ui);
    if (cui_last_error(ui) != CUI_OK)
        fprintf(stderr, "frame error %d hide %d/%d loading %d name %d\n", cui_last_error(ui),
                o.hide_action, o.hide_menu, o.action_loading, m ? m->action.custom_name : 0);
    CHECK(cui_last_error(ui) == CUI_OK);
    if (out)
        *out = r;
    return draw;
}
static void setup(void) {
    ui = cui_create((cui_config){.width = 360, .height = 180});
    CHECK(ui);
    cui_set_reduced_motion(ui, true);
    cui_menu_tree_init(&menu, 10);
    command = 0;
    sentinels = false;
    blur_after = false;
    calls = commands = bodies = 0;
    mutate = NULL;
}
static void click(cui_id id, bool touch) {
    cui_rect b = cui_bounds(ui, id);
    CHECK(b.w > 0 && b.h > 0);
    for (unsigned up = 0; up < 2; up++)
        CHECK(cui_queue_event(
            ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                            .pointer_id = touch ? 42 : 0,
                            .x = b.x + 3,
                            .y = b.y + 3}));
}
static cui_dropdown_button_metadata metadata(void) {
    return (cui_dropdown_button_metadata){.action = {.custom_name = true,
                                                     .name = {"Enregistrer", 11},
                                                     .custom_tab_index = true,
                                                     .tab_index = -10},
                                          .trigger = {.custom_name = true,
                                                      .name = {"Autres", 6},
                                                      .custom_tab_index = true,
                                                      .tab_index = 20},
                                          .toggle_semantics = true,
                                          .toggled = true};
}
static void routed(void) {
    setup();
    cui_dropdown_button_metadata m = metadata();
    cui_dropdown_button_options o = {.width = 130, .content = content, .content_user = &bodies};
    cui_dropdown_event_handlers h = {{action, &calls}, {menu_command, &commands}};
    cui_dropdown_event_result r;
    frame(&m, o, false, h, &r);
    CHECK(r.valid && name_is(2, "Enregistrer") && name_is(3, "Autres") && node(2).checked &&
          !node(3).checked);
    for (unsigned mode = 0; mode < 5; mode++) {
        m = metadata();
        mutate = &m;
        if (mode < 2) {
            click(2, mode == 1);
            if (!mode)
                click(2, false);
        } else if (mode == 2) {
            cui_focus(ui, 2);
            CHECK(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        } else if (mode == 3) {
            cui_focus(ui, 2);
            CHECK(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
            CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
        } else
            CHECK(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
        unsigned before = calls, paint = bodies;
        frame(&m, o, false, h, &r);
        CHECK(r.action_count == (mode ? 1u : 2u) && calls - before == r.action_count &&
              r.legacy.action_activated && bodies == paint + 1);
        CHECK(name_is(3, "Autres") && node(3).tab_index == 20 && node(2).checked);
    }
    mutate = NULL;
    m = metadata();
    blur_after = true;
    cui_focus(ui, 2);
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    unsigned before = calls;
    frame(&m, o, false, h, &r);
    /* Legacy Button reduction accepts the ordered batch before handlers. */
    CHECK(r.action_count == 2 && calls == before + 2);
    blur_after = false;
    frame(&m, o, false, h, &r);
    click(3, false);
    frame(&m, o, false, h, &r);
    CHECK(menu.overlay.open && node(3).details.expanded && node(3).details.controls == 10);
    CHECK(cui_queue_accessibility_action(ui, 12, CUI_ACCESSIBILITY_ACTIVATE));
    frame(&m, o, false, h, &r);
    CHECK(commands == 1 && r.command_count == 1 && !menu.overlay.open && cui_focused(ui) == 3);
    CHECK(name_is(3, "Autres") && node(3).tab_index == 20 && !node(3).checked &&
          node(3).details.expanded && node(3).details.controls == 10);
    frame(&m, o, false, h, &r);
    CHECK(!node(3).details.expanded && name_is(3, "Autres"));
    o.action_loading = true;
    frame(&m, o, false, h, &r);
    CHECK(node(2).disabled && node(2).details.busy && node(2).checked);
    CHECK(!cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(ui);
    o.action_loading = false;
    o.selected = true;
    m.toggle_semantics = false;
    frame(&m, o, false, h, &r);
    CHECK(!node(2).checked && node(2).value == 0);
    m.toggle_semantics = true;
    m.toggled = false;
    frame(&m, o, false, h, &r);
    CHECK(!node(2).checked && node(2).value == 0);
    cui_destroy(ui);
}
static void names_and_invalid(void) {
    setup();
    cui_dropdown_button_metadata m = metadata();
    cui_dropdown_button_options o = {.width = 130};
    cui_dropdown_event_result r;
    char label[] = "R\xc3\xa9glages";
    m.action.name = cui_string(label);
    CHECK(cui_begin_frame(ui, 360, 180, .016f));
    cui_theme t = cui_theme_light();
    cui_dropdown_button_result direct = cui_dropdown_button_with_metadata_ex(
        ui, 1, 2, 3, &t, cui_string("Save"), cui_string("More"), entries, 1, &command, &menu, o,
        NULL, &m);
    CHECK(!direct.action_activated);
    label[0] = 'X';
    CHECK(name_is(2, "R\xc3\xa9glages"));
    cui_draw_list draw = cui_end_frame(ui);
    bool visible = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].text.length == 4 && !memcmp(draw.commands[i].text.data, "Save", 4))
            visible = true;
    CHECK(visible);
    m.action.name = (cui_str){0};
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
    CHECK(name_is(2, ""));
    const cui_str bad[] = {{NULL, 1}, {"\xc0\xaf", 2}, {"a\0b", 3}};
    for (size_t i = 0; i < 3; i++) {
        frame(NULL, o, false, (cui_dropdown_event_handlers){0}, &r);
        click(2, false);
        m.action.name = bad[i];
        menu.drag_scroll = false;
        o.menu.drag_scroll = true;
        command = 77;
        frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
        CHECK(!r.valid && !r.legacy.action_activated && !menu.drag_scroll && command == 77 &&
              !node(2).id);
        o.menu.drag_scroll = false;
    }
    o.hide_action = true;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
    CHECK(r.valid && !node(2).id && node(3).id);
    o.hide_action = false;
    m.action.custom_name = false;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
    CHECK(r.valid && name_is(2, "Save"));
    cui_destroy(ui);
}
static void tab(void) {
    CHECK(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
}
static void ordering(void) {
    setup();
    sentinels = true;
    cui_dropdown_button_metadata m = metadata();
    cui_dropdown_button_options o = {.width = 130};
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    cui_focus(ui, 4);
    tab();
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    CHECK(cui_focused(ui) == 2);
    tab();
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    CHECK(cui_focused(ui) == 3);
    m.action.skip_tab = true;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    cui_focus(ui, 4);
    tab();
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    CHECK(cui_focused(ui) == 3);
    m.action.skip_tab = false;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    cui_focus(ui, 3);
    CHECK(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB, .modifiers = CUI_MOD_SHIFT}));
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    CHECK(cui_focused(ui) == 2);
    m.action.skip_tab = true;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, NULL);
    click(2, false);
    cui_dropdown_event_result r;
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
    CHECK(r.legacy.action_activated);
    CHECK(cui_queue_accessibility_action(ui, 2, CUI_ACCESSIBILITY_ACTIVATE));
    frame(&m, o, false, (cui_dropdown_event_handlers){0}, &r);
    CHECK(r.legacy.action_activated);
    cui_destroy(ui);
}
static bool eq(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void pixels(cui_draw_list draw, unsigned char out[360 * 180 * 4]) {
    cui_draw_command commands[512];
    cui_draw_list copy = {.commands = commands, .width = 360, .height = 180};
    CHECK(draw.count < 512);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type != CUI_DRAW_TEXT)
            commands[copy.count++] = draw.commands[i];
    cui_software *sw = cui_software_create(360, 180, NULL);
    CHECK(sw);
    CHECK(cui_software_render(sw, &copy, (cui_color){255, 255, 255, 255}));
    memcpy(out, cui_software_pixels(sw, NULL, NULL), 360 * 180 * 4);
    cui_software_destroy(sw);
}
static void compare(cui_context *a, cui_context *b, cui_draw_list da, cui_draw_list db) {
    CHECK(da.count == db.count);
    for (size_t i = 0; i < da.count; i++) {
        const cui_draw_command *x = &da.commands[i], *y = &db.commands[i];
        CHECK(x->type == y->type && eq(x->color, y->color));
        CHECK(x->bounds.x == y->bounds.x && x->bounds.y == y->bounds.y &&
              x->bounds.w == y->bounds.w && x->bounds.h == y->bounds.h);
        CHECK(x->text.length == y->text.length);
        CHECK(!x->text.length || !memcmp(x->text.data, y->text.data, x->text.length));
    }
    unsigned char pa[360 * 180 * 4], pb[360 * 180 * 4];
    pixels(da, pa);
    pixels(db, pb);
    CHECK(!memcmp(pa, pb, sizeof pa));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
    CHECK(na == nb);
    for (size_t i = 0; i < na; i++) {
        CHECK(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
              sa[i].disabled == sb[i].disabled && sa[i].checked == sb[i].checked &&
              sa[i].focused == sb[i].focused && sa[i].value == sb[i].value &&
              sa[i].tab_ordered == sb[i].tab_ordered && sa[i].tab_index == sb[i].tab_index &&
              sa[i].tab_stop_disabled == sb[i].tab_stop_disabled);
        CHECK(sa[i].name.length == sb[i].name.length &&
              !memcmp(sa[i].name.data, sb[i].name.data, sa[i].name.length));
        CHECK(sa[i].details.expanded == sb[i].details.expanded &&
              sa[i].details.busy == sb[i].details.busy &&
              sa[i].details.controls == sb[i].details.controls);
        CHECK(sa[i].bounds.x == sb[i].bounds.x && sa[i].bounds.y == sb[i].bounds.y &&
              sa[i].bounds.w == sb[i].bounds.w && sa[i].bounds.h == sb[i].bounds.h);
    }
}
static void legacy_zero(void) {
    setup();
    cui_context *a = ui;
    cui_menu_tree_state ma = menu;
    setup();
    cui_context *b = ui;
    cui_menu_tree_state mb = menu;
    cui_dropdown_button_metadata zero = {0};
    for (unsigned size = 0; size < 4; size++)
        for (unsigned state = 0; state < 7; state++) {
            cui_dropdown_button_options o = {.width = 130,
                                             .size = (cui_dropdown_button_size)size,
                                             .selected = state == 1,
                                             .disabled = state == 2,
                                             .action_loading = state == 3,
                                             .outline = state == 4,
                                             .hide_action = state == 5,
                                             .hide_menu = state == 6};
            ui = a;
            menu = ma;
            cui_draw_list da = frame(NULL, o, true, (cui_dropdown_event_handlers){0}, NULL);
            ma = menu;
            ui = b;
            menu = mb;
            cui_draw_list db =
                frame(state & 1 ? &zero : NULL, o, false, (cui_dropdown_event_handlers){0}, NULL);
            mb = menu;
            compare(a, b, da, db);
        }
    cui_destroy(a);
    cui_destroy(b);
}
int main(void) {
    legacy_zero();
    routed();
    names_and_invalid();
    ordering();
    return 0;
}
