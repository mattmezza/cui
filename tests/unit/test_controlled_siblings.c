/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include "cui/cui_numeric.h"
#include "cui/cui_tree.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* This audit deliberately retains failing externally observable regressions.
 * Run all cases so one defect does not conceal the remaining coverage. */
static unsigned failures;
static void check(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}
static const cui_semantic *node(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == id)
            return &nodes[i];
    assert(!"missing semantic fixture");
    return NULL;
}
static void click(cui_context *ui, cui_id id) {
    cui_rect b = cui_bounds(ui, id);
    assert(b.w > 0 && b.h > 0);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                       .pointer_button = CUI_POINTER_PRIMARY,
                       .x = b.x + b.w / 2,
                       .y = b.y + b.h / 2};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
}
static cui_context *context(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 500});
    assert(ui);
    return ui;
}
static void begin(cui_context *ui) {
    assert(cui_begin_frame(ui, 600, 500, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 500, .gap = 8});
}
static cui_draw_list end(cui_context *ui) {
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && draw.count);
    return draw;
}
static const cui_choice_item items[] = {
    {11, {"Earlier", 7}, false}, {12, {"Later", 5}, false}, {13, {"Disabled", 8}, true}};
typedef struct {
    cui_id selected;
    bool values[3], multi, accept;
    unsigned activations, confirms, cancels, painted;
} owner;
static owner *expected_owner;
static void activate(cui_id id, void *user) {
    assert(user == expected_owner && id == 12);
    owner *o = user;
    ++o->activations;
    /* The owner can replace the complete selection, or restore/reject it. */
    o->selected = o->accept ? 12 : 11;
    o->values[0] = !o->accept;
    o->values[1] = o->accept;
    o->values[2] = false;
}
static void confirm(cui_id id, bool secondary, void *user) {
    assert(id == 12 && !secondary);
    ++expected_owner->confirms;
    check(user == expected_owner, "ListMulti on_confirm receives caller activation_user");
    /* Do not dereference the wrong user pointer: real owner callbacks could corrupt memory. */
}
static void cancel(void *user) {
    ++expected_owner->cancels;
    check(user == expected_owner, "ListMulti on_cancel receives caller activation_user");
}
static void render(cui_context *ui, const cui_choice_item *item, size_t index, void *user) {
    owner *o = user;
    assert(index < 3);
    ++o->painted;
    if (o->activations) {
        assert(o->selected == (o->accept ? 12 : 11));
        assert(o->values[0] == !o->accept && o->values[1] == o->accept);
    }
    cui_text(ui, item->label, 14, (cui_color){23, 45, 67, 255});
}
static bool list_frame(cui_context *ui, owner *o, cui_list_state *state, bool disabled) {
    begin(ui);
    cui_theme theme = cui_theme_light();
    cui_list_options options = {.disabled = disabled,
                                .on_activate = activate,
                                .activation_user = o,
                                .on_confirm = confirm,
                                .on_cancel = cancel,
                                .render = render,
                                .render_user = o};
    bool activated = o->multi ? cui_list_multi(ui, 10, &theme, items, 3, o->values, state, options)
                              : cui_list(ui, 10, &theme, items, 3, &o->selected, state, options);
    cui_draw_list draw = end(ui);
    if (o->multi) {
        unsigned marks = 0;
        for (size_t d = 0; d < draw.count; ++d)
            if (draw.commands[d].type == CUI_DRAW_IMAGE) {
                ++marks;
                cui_rect selected_bounds = cui_bounds(ui, o->values[0] ? 11 : 12);
                assert(draw.commands[d].bounds.y >= selected_bounds.y &&
                       draw.commands[d].bounds.y < selected_bounds.y + selected_bounds.h);
            }
        assert(marks == 1);
    }
    return activated;
}
static void lists(void) {
    for (unsigned multi = 0; multi < 2; ++multi)
        for (unsigned accept = 0; accept < 2; ++accept) {
            cui_context *ui = context();
            owner o = {.selected = 11,
                       .values = {true, false, false},
                       .multi = multi != 0,
                       .accept = accept != 0};
            expected_owner = &o;
            cui_list_state state = {.active = 11};
            list_frame(ui, &o, &state, false);
            list_frame(ui, &o, &state, false);
            click(ui, 12);
            assert(list_frame(ui, &o, &state, false));
            assert(o.activations == 1 && o.confirms == 1 && !o.cancels);
            assert(node(ui, 11)->checked == !o.accept && node(ui, 12)->checked == o.accept);
            assert(node(ui, 11)->details.selected == !o.accept);
            printf("%s %s: final same-frame state, corrective redraw=%s\n",
                   multi ? "ListMulti" : "List", accept ? "accept" : "restore",
                   cui_needs_redraw(ui) ? "yes" : "no");
            unsigned painted = o.painted;
            list_frame(ui, &o, &state, false);
            assert(o.activations == 1 && o.confirms == 1 && o.painted == painted + 3);
            assert(node(ui, 11)->checked == !o.accept && node(ui, 12)->checked == o.accept);
            click(ui, 13);
            assert(!list_frame(ui, &o, &state, false));
            assert(o.activations == 1 && o.confirms == 1);
            click(ui, 12);
            assert(!list_frame(ui, &o, &state, true));
            assert(o.activations == 1 && o.confirms == 1);
            list_frame(ui, &o, &state, false);
            cui_focus(ui, 12);
            assert(cui_queue_event(ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
            assert(!list_frame(ui, &o, &state, false));
            assert(o.cancels == 1);
            list_frame(ui, &o, &state, false);
            assert(o.cancels == 1);
            cui_destroy(ui);
        }
}
static void radios(void) {
    cui_context *ui = context();
    cui_theme theme = cui_theme_light();
    int selected = 0;
    for (unsigned frame = 0; frame < 4; ++frame) {
        if (frame == 2)
            click(ui, 12);
        begin(ui);
        cui_radio(ui, 11, &theme, cui_string("Earlier"), &selected, 0, false);
        bool changed = cui_radio(ui, 12, &theme, cui_string("Later"), &selected, 1, false);
        cui_radio(ui, 13, &theme, cui_string("Disabled"), &selected, 2, true);
        cui_draw_list draw = end(ui);
        unsigned painted_selection = 0;
        for (size_t d = 0; d < draw.count; ++d)
            painted_selection += draw.commands[d].type == CUI_DRAW_IMAGE;
        assert(painted_selection == (frame == 2 ? 2u : 1u));
        assert(changed == (frame == 2));
        if (frame == 2) {
            assert(selected == 1 && cui_needs_redraw(ui));
            check(!node(ui, 11)->checked && node(ui, 12)->checked,
                  "Radio earlier sibling semantic reflects final shared selection in activation "
                  "frame");
        }
        if (frame == 3)
            assert(!node(ui, 11)->checked && node(ui, 12)->checked);
    }
    click(ui, 13);
    begin(ui);
    assert(!cui_radio(ui, 13, &theme, cui_string("Disabled"), &selected, 2, true));
    end(ui);
    assert(selected == 1);
    cui_destroy(ui);
}
static void ordered_widgets(void) {
    for (unsigned kind = 0; kind < 3; ++kind) {
        cui_context *ui = context();
        cui_theme theme = cui_theme_light();
        cui_id selected = 11;
        size_t current = 0;
        cui_tree_state state = {0};
        cui_tree_model *tree = cui_tree_create(3);
        cui_tree_node nodes[] = {{11, {"Earlier", 7}, 0, false},
                                 {12, {"Later", 5}, 0, false},
                                 {13, {"Disabled", 8}, 0, true}};
        bool expanded[3] = {false};
        assert(cui_tree_rebuild(tree, nodes, expanded, 3) == CUI_TREE_OK);
        cui_step steps[] = {{.title = {"Earlier", 7}},
                            {.title = {"Later", 5}},
                            {.title = {"Disabled", 8}, .disabled = true}};
        cui_id earlier = kind == 1   ? cui_tree_row_id(10, 11)
                         : kind == 2 ? cui_numeric_part_id(10, 1)
                                     : 11;
        cui_id later = kind == 1   ? cui_tree_row_id(10, 12)
                       : kind == 2 ? cui_numeric_part_id(10, 2)
                                   : 12;
        for (unsigned frame = 0; frame < 4; ++frame) {
            if (frame == 2)
                click(ui, later);
            begin(ui);
            if (!kind)
                cui_tabs(ui, 10, &theme, items, 3, &selected, (cui_tabs_options){0}, NULL, NULL);
            else if (kind == 1)
                cui_tree(ui, 10, &theme, tree, &state, &selected, (cui_tree_options){0});
            else
                cui_stepper(ui, 10, &theme, steps, 3, &current, false, false);
            cui_draw_list draw = end(ui);
            if (frame >= 2) {
                assert(kind == 2 ? current == 1 : selected == 12);
                assert(!node(ui, earlier)->details.selected && node(ui, later)->details.selected);
                if (frame == 2)
                    printf("%s: final same-frame state, corrective redraw=%s\n",
                           !kind       ? "Tabs"
                           : kind == 1 ? "Tree"
                                       : "Stepper",
                           cui_needs_redraw(ui) ? "yes" : "no");
                if (kind == 2) {
                    unsigned labels = 0;
                    for (size_t d = 0; d < draw.count; ++d)
                        labels += draw.commands[d].type == CUI_DRAW_TEXT &&
                                  draw.commands[d].text.length == 7 &&
                                  !memcmp(draw.commands[d].text.data, "Current", 7);
                    assert(labels == 1);
                }
            }
        }
        cui_id disabled_id = kind == 1   ? cui_tree_row_id(10, 13)
                             : kind == 2 ? cui_numeric_part_id(10, 3)
                                         : 13;
        click(ui, disabled_id);
        begin(ui);
        if (!kind)
            assert(
                !cui_tabs(ui, 10, &theme, items, 3, &selected, (cui_tabs_options){0}, NULL, NULL));
        else if (kind == 1)
            assert(!cui_tree(ui, 10, &theme, tree, &state, &selected, (cui_tree_options){0})
                        .selection_changed);
        else
            assert(!cui_stepper(ui, 10, &theme, steps, 3, &current, false, false));
        end(ui);
        assert(kind == 2 ? current == 1 : selected == 12);
        cui_tree_destroy(tree);
        cui_destroy(ui);
    }
}
static void checkboxes(void) {
    cui_context *ui = context();
    cui_theme theme = cui_theme_light();
    bool values[3] = {true, false, false};
    for (unsigned frame = 0; frame < 4; ++frame) {
        if (frame == 2)
            click(ui, 12);
        begin(ui);
        cui_checkbox(ui, 11, &theme, cui_string("Earlier"), &values[0], false);
        bool changed = cui_checkbox(ui, 12, &theme, cui_string("Later"), &values[1], false);
        cui_checkbox(ui, 13, &theme, cui_string("Disabled"), &values[2], true);
        end(ui);
        assert(changed == (frame == 2));
        assert(node(ui, 11)->checked == values[0] && node(ui, 12)->checked == values[1]);
    }
    click(ui, 13);
    begin(ui);
    assert(!cui_checkbox(ui, 13, &theme, cui_string("Disabled"), &values[2], true));
    end(ui);
    assert(!values[2]);
    cui_destroy(ui);
}

static bool menu_frame(cui_context *ui, cui_menu_tree_state *state, int *value, cui_id *action,
                       bool disabled) {
    begin(ui);
    cui_theme theme = cui_theme_light();
    cui_menu_entry entries[] = {{.id = 11,
                                 .label = {"Earlier", 7},
                                 .kind = CUI_MENU_RADIO,
                                 .radio_value = value,
                                 .radio_option = 0},
                                {.id = 12,
                                 .label = {"Later", 5},
                                 .kind = CUI_MENU_RADIO,
                                 .radio_value = value,
                                 .radio_option = 1},
                                {.id = 13,
                                 .label = {"Disabled", 8},
                                 .kind = CUI_MENU_RADIO,
                                 .radio_value = value,
                                 .radio_option = 2,
                                 .disabled = true}};
    bool result = cui_menu_tree(
        ui, 20, &theme, cui_string("Menu"), entries, 3, action, state,
        (cui_menu_tree_options){
            .popup = {.disabled = disabled, .viewport = {0, 0, 600, 500}, .width = 220},
            .keep_open_checks = true});
    end(ui);
    return result;
}
static void menus(void) {
    cui_context *ui = context();
    cui_menu_tree_state state;
    cui_menu_tree_init(&state, 10);
    int selected = 0;
    cui_id action = 0;
    menu_frame(ui, &state, &selected, &action, false);
    click(ui, 20);
    menu_frame(ui, &state, &selected, &action, false);
    assert(state.overlay.open);
    click(ui, 12);
    assert(menu_frame(ui, &state, &selected, &action, false));
    assert(selected == 1 && action == 12 && state.overlay.open);
    assert(!node(ui, 11)->checked && node(ui, 12)->checked);
    printf("MenuTree: final same-frame state, corrective redraw=%s\n",
           cui_needs_redraw(ui) ? "yes" : "no");
    assert(!menu_frame(ui, &state, &selected, &action, false));
    assert(!node(ui, 11)->checked && node(ui, 12)->checked);
    click(ui, 13);
    assert(!menu_frame(ui, &state, &selected, &action, false));
    assert(selected == 1);
    menu_frame(ui, &state, &selected, &action, true);
    assert(selected == 1 && !state.overlay.open);
    cui_destroy(ui);
}

int main(int argc, char **argv) {
    bool radio_only = argc == 2 && !strcmp(argv[1], "--radio");
    if (argc > 1 && !radio_only) {
        fprintf(stderr, "usage: %s [--radio]\n", argv[0]);
        return 2;
    }
    if (radio_only) {
        radios();
        fprintf(stderr, "known Radio gap: %u regression(s)\n", failures);
        return failures ? 1 : 0;
    }
    lists();
    ordered_widgets();
    menus();
    checkboxes();
    fprintf(stderr, "controlled sibling audit: %u confirmed regression(s)\n", failures);
    return failures ? 1 : 0;
}
